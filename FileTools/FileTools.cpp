#include "FileTools.h"

char *FRStrings[]={"Mask","Text","Replace"};
char *FRInts[]={"MaskAsRegExp","TextAsRegExp","Repeating"};
char *FQRStrings[]={"Text","Replace"};
char *FQRInts[]={"TextAsRegExp","Repeating"};

CParameterBatch g_RBatch(3, 3,
	"Mask", &MaskText, "Text", &SearchText, "Replace", &ReplaceText,
	"MaskAsRegExp", &FMaskAsRegExp, "TextAsRegExp", &FSearchAs, "Repeating", &FRepeating
	);
CParameterBatch g_QRBatch(2, 2,
	"Text", &SearchText, "Replace", &ReplaceText,
	"TextAsRegExp", &FSearchAs, "Repeating", &FRepeating
	);

CRPresetCollection *RPresets;
CQRPresetCollection *QRPresets;
CPresetBatchCollection *RBatch;
CPresetBatchCollection *QRBatch;

int FileNumber;
bool FTAskOverwrite;

void FTReadRegistry(HKEY Key) {
	RPresets=new CRPresetCollection();
	QRPresets=new CQRPresetCollection();
	RBatch=new CPresetBatchCollection(RPresets);
	QRBatch=new CPresetBatchCollection(QRPresets);
}

void FTWriteRegistry(HKEY Key) {
}

void FTCleanup(BOOL PatternOnly) {
	if (!PatternOnly) {
		delete RBatch;
		delete QRBatch;
		delete RPresets;
		delete QRPresets;
	}
}

void ChangeSelection(int How) {
	string MaskText;

	CFarDialog Dialog(44,10,"FileSelectDlg");
	Dialog.AddFrame(How);
	Dialog.Add(new CFarCheckBoxItem(5,3,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarCheckBoxItem(5,4,0,MCaseSensitive,&FACaseSensitive));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,38,DIF_HISTORY,"Masks", MaskText));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.SetFocus(4);
	FACaseSensitive=MaskCaseHere();

	do {
		MaskText=FMask;
		if (Dialog.Display(-1)==-1) return;
		FMask=MaskText;
	} while (!FPreparePattern());

	PanelInfo PInfo;
	int I;

	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
	for (I=0;I<PInfo.ItemsNumber;I++) {
		if (MultipleMasksApply(FMask,PInfo.PanelItems[I].FindData.cFileName)) {
			switch (How) {
			case MMenuSelect:
				PInfo.PanelItems[I].Flags|=PPIF_SELECTED;
				break;
			case MMenuUnselect:
				PInfo.PanelItems[I].Flags&=~PPIF_SELECTED;
				break;
			case MMenuFlipSelection:
				PInfo.PanelItems[I].Flags^=PPIF_SELECTED;
				break;
			}
		}
	}
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_SETSELECTION,&PInfo);
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_REDRAWPANEL,NULL);
}

BOOL ConfirmRename(const char *From,const char *To) {
	if (!ConfirmFile(MMenuRename,From)) return FALSE;
	if (Interrupt) return FALSE;
	if (!FRConfirmLineThisFile) return TRUE;
	const char *Lines[]={
		GetMsg(MMenuRename),GetMsg(MAskRename),From,GetMsg(MAskTo),To,
		GetMsg(MOk),GetMsg(MAll),GetMsg(MAllFiles),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(StartupInfo.ModuleNumber,0,"FRAskRename",Lines,10,5)) {
	case 2:FRConfirmLineThisRun=FALSE;
	case 1:FRConfirmLineThisFile=FALSE;
	case 0:return TRUE;
	case 3:return FALSE;
	}
	Interrupt=TRUE;
	return FALSE;
}

BOOL FindRename(char *FileName,int *&Match,int &MatchCount,int &MatchStart,int &MatchLength) {
	if (FSearchAs==SA_REGEXP) {
		MatchCount=pcre_info(FPattern,NULL,NULL)+1;
		Match=new int[MatchCount*3];
		if (do_pcre_exec(FPattern,FPatternExtra,FileName,strlen(FileName),MatchStart,0,Match,MatchCount*3)>=0) {
			MatchStart=Match[0];
			MatchLength=Match[1]-Match[0];
			return TRUE;
		}
		delete[] Match;
	} else {
		int NewMatchStart;
		char *Table = FCaseSensitive ? NULL : UpCaseTable;
		NewMatchStart=BMHSearch(FileName+MatchStart,strlen(FileName)-MatchStart,FTextUpcase.data(),FTextUpcase.size(),Table);

		Match=NULL;MatchCount=0;
		if (NewMatchStart>=0) {
			MatchStart+=NewMatchStart;
			MatchLength=FText.size();
			return TRUE;
		}
	}
	return FALSE;
}

void RenameFile(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber) {
	int MatchCount,MatchStart=0,MatchLength,ReplaceNumber=0;
	int *Match;
	BOOL Modified=FALSE;
	char NewName[MAX_PATH];
	char *FileName=strrchr(FindData->cFileName,'\\');
	if (FileName) FileName++; else FileName=FindData->cFileName;

	FRConfirmLineThisFile=FRConfirmLineThisRun;
	FileConfirmed=!FRConfirmFileThisRun;

	FileNumber++;
	while (FindRename(FileName,Match,MatchCount,MatchStart,MatchLength)) {
		int Numbers[3]={FileNumber,FileNumber,ReplaceNumber};
		int ReplaceLength;
		char *NewSubName=CreateReplaceString(FileName,Match,MatchCount,FRReplace.c_str(),"",Numbers,ReplaceLength);
		strncpy(NewName,FileName,MatchStart);
		strcpy(NewName+MatchStart,NewSubName);
		strcat(NewName,FileName+MatchStart+MatchLength);
		MatchStart+=strlen(NewSubName);
		free(NewSubName);
		if (Match) delete[] Match;

		if (ConfirmRename(FileName,NewName)) {
			memmove(NewName+(FileName-FindData->cFileName),NewName,strlen(NewName)+1);
			strncpy(NewName,FindData->cFileName,FileName-FindData->cFileName);
			if (MoveFile(FindData->cFileName,NewName)) {
				strcpy(FileName,NewName+(FileName-FindData->cFileName));
				Modified=TRUE;ReplaceNumber++;
				if (!FRepeating) break;
			} else {
				DWORD Error=GetLastError();
				if (Error==ERROR_ALREADY_EXISTS) {

					if (FTAskOverwrite) {
						bool bOverwrite;
						const char *Lines[]={GetMsg(MMenuRename),GetMsg(MFile),NewName,
							GetMsg(MAskOverwrite),FindData->cFileName,GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)};
						switch (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FRenameOverwrite",Lines,9,4)) {
						case 1:
							FTAskOverwrite=false;
						case 0:
							bOverwrite = true;
							break;
						case 3:
							Interrupt=TRUE;
						case 2:
							return;
						}
					}

					if (DeleteFile(NewName)) {
						if (MoveFile(FindData->cFileName,NewName)) {
							strcpy(FileName,NewName+(FileName-FindData->cFileName));
							Modified=TRUE;ReplaceNumber++;
							if (!FRepeating) break;
							continue;
						}
					}
					Error=GetLastError();
				}
				const char *Lines[]={GetMsg(MMenuRename),GetMsg(MRenameError),FindData->cFileName,
					GetMsg(MAskTo),NewName,GetMsg(MOk),GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FRenameError",Lines,7,2)==1) Interrupt=TRUE;
				return;
			}
		}
		if (Interrupt) return;
	}
	if (Modified) AddFile(FindData,PanelItems,ItemsNumber);
}

BOOL RenameFilesExecutor(CParameterBatch &Batch) {
	FMask=MaskText;
	FText=SearchText;
	FRReplace=ReplaceText;
	if (!FPreparePattern()) return FALSE;
	FTAskOverwrite=true;
	FileNumber=-1;Interrupt=FALSE;

	FRConfirmFileThisRun = FALSE;//FRConfirmFile;
	FRConfirmLineThisRun = FALSE;//FRConfirmLine;
	ScanDirectories(&PanelItems,&ItemsNumber,RenameFile);

	return TRUE;
}

BOOL RenameFilesPrompt() {
	CFarDialog Dialog(76,20,"FileRenameDlg");
	Dialog.AddFrame(MMenuRename);

	Dialog.Add(new CFarCheckBoxItem(25,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarCheckBoxItem(52,2,0,MCaseSensitive,&FACaseSensitive));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"Masks", MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"SearchText", SearchText));
	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,"ReplaceText", ReplaceText));

	Dialog.Add(new CFarCheckBoxItem(25,4,0,MRegExp,(BOOL *)&FSearchAs));
	Dialog.Add(new CFarCheckBoxItem(52,4,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(25,6,0,MRepeating,&FRepeating));
	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR,""));

	Dialog.Add(new CFarRadioButtonItem(5,9,DIF_GROUP,MAllDrives,(int *)&FSearchIn,SI_ALLDRIVES));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,MAllLocalDrives,	(int *)&FSearchIn,SI_ALLLOCAL));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MFromRoot,		(int *)&FSearchIn,SI_FROMROOT));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,MFromCurrent,		(int *)&FSearchIn,SI_FROMCURRENT));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,MCurrentOnly,		(int *)&FSearchIn,SI_CURRENTONLY));
	Dialog.Add(new CFarRadioButtonItem(5,14,0,MSelected,		(int *)&FSearchIn,SI_SELECTED));

	Dialog.Add(new CFarCheckBoxItem(44,11,0,MViewModified,&FROpenModified));
	Dialog.Add(new CFarCheckBoxItem(44,12,0,MConfirmFile,&FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(44,13,0,MConfirmLine,&FRConfirmLine));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,16,0,0,MBatch));
	Dialog.Add(new CFarButtonItem(60,9,0,0,MPresets));
	Dialog.SetFocus(4);

	if (FSearchAs>=SA_SEVERALLINE) FSearchAs=SA_PLAINTEXT;
	FACaseSensitive=FCaseSensitive=MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;
	ReplaceText=FRReplace;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(3,-4,-2,-1)) {
		case 0:
			FMask=MaskText;
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case 1:
			if (RBatch->ShowMenu(RenameFilesExecutor, g_RBatch) >= 0)
				return FALSE;
			break;
		case 2:
			RPresets->ShowMenu(g_RBatch);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!FPreparePattern());
	return TRUE;
}

OperationResult RenameFiles(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog) {
	PanelInfo PInfo;
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
	if (PInfo.Plugin||(PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	FUTF8=FALSE;
	if (ShowDialog) {
		if (!RenameFilesPrompt()) return OR_CANCEL;
	} else {
		if (!FPreparePattern()) return OR_CANCEL;
	}

	FRConfirmFileThisRun=FRConfirmFile;
	FRConfirmLineThisRun=FRConfirmLine;
	FTAskOverwrite=true;
	FileNumber=-1;Interrupt=FALSE;

	if (ScanDirectories(PanelItems,ItemsNumber,RenameFile)) {
		if (!FROpenModified) return OR_OK; else
		return (*ItemsNumber==0)?NoFilesFound():OR_PANEL;
	} else return OR_FAILED;
}

BOOL PerformRenameSelectedFiles(PanelInfo &PInfo,PluginPanelItem **PanelItems,int *ItemsNumber) {
	FileNumber=-1;Interrupt=FALSE;

	if ((PInfo.SelectedItemsNumber==0)&&(PInfo.ItemsNumber>0)&&
		(strcmp(PInfo.PanelItems[PInfo.CurrentItem].FindData.cFileName,"..")==0)) {

		for (int I=0;I<PInfo.ItemsNumber;I++) {
			if (I==PInfo.CurrentItem) continue;
			RenameFile(&PInfo.PanelItems[I].FindData,PanelItems,ItemsNumber);
		}
	} else {
		for (int I=0;I<PInfo.SelectedItemsNumber;I++) {
			RenameFile(&PInfo.SelectedItems[I].FindData,PanelItems,ItemsNumber);
		}
	}

	StartupInfo.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, NULL);
	return TRUE;
}

BOOL RenameSelectedFilesExecutor(CParameterBatch &Batch) {
	FText=SearchText;
	FRReplace=ReplaceText;
	if (!FPreparePattern()) return FALSE;
	FTAskOverwrite=true;

	FRConfirmFileThisRun = FALSE;//FRConfirmFile;
	FRConfirmLineThisRun = FALSE;//FRConfirmLine;

	PanelInfo PInfo;
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
	if (PInfo.Plugin||(PInfo.PanelType!=PTYPE_FILEPANEL)) return FALSE;

	PerformRenameSelectedFiles(PInfo,&PanelItems,&ItemsNumber);

	return TRUE;
}

BOOL RenameSelectedFilesPrompt() {
	CFarDialog Dialog(76,14,"SelectedFileRenameDlg");
	Dialog.AddFrame(MMenuRename);

	Dialog.Add(new CFarCheckBoxItem(25,2,0,MRegExp,(BOOL *)&FSearchAs));
	Dialog.Add(new CFarCheckBoxItem(50,2,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarTextItem(5,2,0,MText));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"SearchText", SearchText));

	Dialog.Add(new CFarTextItem(5,4,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"ReplaceText", ReplaceText));
	Dialog.Add(new CFarCheckBoxItem(25,4,0,MRepeating,&FRepeating));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MConfirmFile,&FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MConfirmLine,&FRConfirmLine));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,10,0,0,MBatch));
	Dialog.Add(new CFarButtonItem(60,7,0,0,MPresets));
	Dialog.SetFocus(4);

	if (FSearchAs>=SA_SEVERALLINE) FSearchAs=SA_PLAINTEXT;
	FCaseSensitive=MaskCaseHere();

	int ExitCode;
	SearchText=FText;
	ReplaceText=FRReplace;
	do {
		switch (ExitCode=Dialog.Display(3,-4,-2,-1)) {
		case 0:
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case 1:
			if (QRBatch->ShowMenu(RenameSelectedFilesExecutor, g_QRBatch) >= 0)
				return FALSE;
			break;
		case 2:
			QRPresets->ShowMenu(g_QRBatch);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!FPreparePattern());
	return TRUE;
}

OperationResult RenameSelectedFiles(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog) {
	PanelInfo PInfo;
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
	if (PInfo.Plugin||(PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	FUTF8=FALSE;
	if (ShowDialog) {
		if (!RenameSelectedFilesPrompt()) return OR_CANCEL;
	} else {
		if (!FPreparePattern()) return OR_CANCEL;
	}

	*ItemsNumber=0;*PanelItems=NULL;
	FRConfirmFileThisRun=FRConfirmFile;
	FRConfirmLineThisRun=FRConfirmLine;
	FTAskOverwrite=true;

	PerformRenameSelectedFiles(PInfo,PanelItems,ItemsNumber);
	return OR_OK;
}

BOOL CRPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,16,"RPresetDlg");
	Dialog.AddFrame(MFRPreset);

	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"RESearch.PresetName", pPreset->m_strName));

	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));
	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"Masks", pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MSearchFor));
	Dialog.Add(new CFarCheckBoxItem(35,6,0,MAsRegExp,&pPreset->m_mapInts["TextAsRegExp"]));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,"SearchText", pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,8,0,MReplaceWith));
	Dialog.Add(new CFarCheckBoxItem(35,8,0,MRepeating,&pPreset->m_mapInts["Repeating"]));
	Dialog.Add(new CFarEditItem(5,9,70,DIF_HISTORY,"ReplaceText", pPreset->m_mapStrings["Replace"]));

	Dialog.AddButtons(MOk,MCancel);

	return Dialog.Display(-1)==0;
}

BOOL CQRPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,14,"QRPresetDlg");
	Dialog.AddFrame(MFRPreset);

	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"RESearch.PresetName", pPreset->m_strName));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["TextAsRegExp"]));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"SearchText", pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarCheckBoxItem(35,6,0,MRepeating,&pPreset->m_mapInts["Repeating"]));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,"ReplaceText", pPreset->m_mapStrings["Replace"]));

	Dialog.AddButtons(MOk,MCancel);

	return Dialog.Display(-1)==0;
}
