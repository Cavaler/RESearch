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

int FileNumber;
bool FTAskOverwrite;
bool FTAskCreatePath;
int g_nStartWithNow;

void FTReadRegistry(HKEY Key) {
	QueryRegStringValue(Key, "FTRStrip",   g_strStrip,   "^\\d+\\s*([-.]\\s*)?");
	QueryRegStringValue(Key, "FTRPrefix",  g_strPrefix,  "");
	QueryRegStringValue(Key, "FTRPostfix", g_strPostfix,  " - ");
	QueryRegIntValue   (Key, "FTRStart",  &g_nStartWith, 1, 0);
	QueryRegIntValue   (Key, "FTRWidth",  &g_nWidth,     2, 0);

	RPresets  = new CRPresetCollection();
	QRPresets = new CQRPresetCollection();
	RBatch    = new CPresetBatchCollection(RPresets);
	QRBatch   = new CPresetBatchCollection(QRPresets);
}

void FTWriteRegistry(HKEY Key) {
	SetRegStringValue(Key, "FTRStrip",   g_strStrip);
	SetRegStringValue(Key, "FTRPrefix",  g_strPrefix);
	SetRegStringValue(Key, "FTRPostfix", g_strPostfix);
	SetRegIntValue   (Key, "FTRStart",   g_nStartWith);
	SetRegIntValue   (Key, "FTRWidth",   g_nWidth);
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
				DWORD Error = GetLastError();
				switch (Error) {
				case ERROR_ALREADY_EXISTS:
					if (FTAskOverwrite) {
						const char *Lines[]={GetMsg(MMenuRename),GetMsg(MFile),NewName,
							GetMsg(MAskOverwrite),FindData->cFileName,GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)};
						switch (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FRenameOverwrite",Lines,9,4)) {
						case 1:
							FTAskOverwrite=false;
						case 0:
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
							Error = ERROR_SUCCESS;
							break;
						}
					}
					Error=GetLastError();
					break;

				case ERROR_PATH_NOT_FOUND:
					if (FTAskCreatePath) {
						const char *Lines[]={GetMsg(MMenuRename),GetMsg(MFile),NewName,
							GetMsg(MAskCreatePath),GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)};
						switch (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FRenameCreatePath",Lines,9,4)) {
						case 1:
							FTAskCreatePath=false;
						case 0:
							break;
						case 3:
							Interrupt=TRUE;
						case 2:
							return;
						}
					}

					if (CreateDirectoriesForFile(NewName)) {
						if (MoveFile(FindData->cFileName,NewName)) {
							strcpy(FileName,NewName+(FileName-FindData->cFileName));
							Modified=TRUE;ReplaceNumber++;
							Error = ERROR_SUCCESS;
							break;
						}
					}
					Error=GetLastError();
				}

				if (Error != ERROR_SUCCESS) {
					const char *Lines[]={GetMsg(MMenuRename),GetMsg(MRenameError),FindData->cFileName,
						GetMsg(MAskTo),NewName,GetMsg(MOk),GetMsg(MCancel)};
					if (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FRenameError",Lines,7,2)==1) Interrupt=TRUE;
					return;
				} else {
					if (!FRepeating) break;
					continue;
				}
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
	FTAskOverwrite = FTAskCreatePath = true;
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
	if (!g_bFromCmdLine) FCaseSensitive=MaskCaseHere();
	FACaseSensitive=FCaseSensitive;

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
	FTAskOverwrite = FTAskCreatePath = true;
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
	FTAskOverwrite = FTAskCreatePath = true;

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
	if (!g_bFromCmdLine) FCaseSensitive=MaskCaseHere();

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
	FTAskOverwrite = FTAskCreatePath = true;

	PerformRenameSelectedFiles(PInfo,PanelItems,ItemsNumber);
	return (*ItemsNumber==0)?NoFilesFound():OR_OK;
}

void ProcessNames(vector<string> &arrFileNames, vector<string> &arrProcessedNames) {
	arrProcessedNames.resize(0);
	CRegExp reStrip(g_strStrip, PCRE_CASELESS);

	for (size_t nItem = 0; nItem < arrFileNames.size(); nItem++) {
		string strName = arrFileNames[nItem];

		if (!strName.empty()) {
			vector<string> arrMatches;
			if (reStrip.Match(strName, PCRE_ANCHORED, &arrMatches)) {
				strName.erase(0, arrMatches[0].length());
			}
			char szNumber[16];
			sprintf(szNumber, "%0*d", g_nWidth, nItem+g_nStartWithNow);
			strName = g_strPrefix + szNumber + g_strPostfix + strName;

			arrProcessedNames.push_back(strName);
		} else {
			arrProcessedNames.push_back("---");
		}
	}
}

void PerformRename(vector<string> &arrFileNames, vector<string> &arrProcessedNames) {
	for (size_t nItem = 0; nItem < arrFileNames.size(); nItem++) {
		if (!arrFileNames[nItem].empty())
			MoveFile(arrFileNames[nItem].c_str(), arrProcessedNames[nItem].c_str());
	}
}

OperationResult RenumberFiles() {
	PanelInfo PInfo;
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
	if (PInfo.PanelType != PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin && ((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;
	g_nStartWithNow = g_nStartWith;

	vector<string> arrFileNames;
	for (int nItem = 0; nItem < PInfo.SelectedItemsNumber; nItem++)
		arrFileNames.push_back(PInfo.SelectedItems[nItem].FindData.cFileName);

	int BreakKeys[] = {
		VK_F2, VK_F7, (PKF_CONTROL<<16)|VK_UP, (PKF_CONTROL<<16)|VK_DOWN,
		VK_ADD, (PKF_CONTROL<<16)|VK_ADD, VK_SUBTRACT, (PKF_CONTROL<<16)|VK_SUBTRACT, VK_F10,
		VK_INSERT, VK_DELETE, 0
	};

	bool bOriginal = false;
	int nPosition = 0;
	int nOK = 0;
	do {
		vector<string> arrProcessedNames;
		ProcessNames(arrFileNames, arrProcessedNames);
		vector<string> &arrNames = bOriginal ? arrFileNames : arrProcessedNames;

		char szBreak[32];
		sprintf(szBreak, "--- %0*d ----------", g_nWidth, nOK);
		arrNames.insert(arrNames.begin()+nOK, szBreak);
		if (nPosition >= nOK) nPosition++;

		int nBreakKey = 0;
		nPosition = ChooseMenu(arrNames, GetMsg(MRenumber), "F2, F7, Ctrl-\x18\x19, F10=Go", "Renumber",
			nPosition, FMENU_WRAPMODE, BreakKeys, &nBreakKey);
		if (nPosition >= nOK) nPosition--; else
			if (nPosition < 0) nPosition = -2;		// -1 is not Esc

		arrNames.erase(arrNames.begin()+nOK);

		switch (nBreakKey) {
		case -1:
			if (nPosition < -1) return OR_CANCEL;
			// Send to end of OK
			if (nPosition >= nOK) {
				string strPrev = arrFileNames[nPosition];
				for (int nPos = nPosition; nPos > nOK; nPos--)
					arrFileNames[nPos] = arrFileNames[nPos-1];
				arrFileNames[nOK] = strPrev;
				nPosition = ++nOK;
			}
			break;
		case 0:
			bOriginal = !bOriginal;
			break;
		case 1:{
			nOK = 0;
			for (size_t nItem = 0; nItem < arrFileNames.size(); ) {
				if (arrFileNames[nItem].empty()) arrFileNames.erase(arrFileNames.begin()+nItem); else nItem++;
			}
			break;
			  }
		case 2:
			if (nPosition > nOK) {
				string strPrev = arrFileNames[nPosition-1];
				arrFileNames[nPosition-1] = arrFileNames[nPosition];
				arrFileNames[nPosition] = strPrev;
				nPosition--;
			}
			break;
		case 3:
			if ((nPosition >= nOK) && (nPosition < (int)arrFileNames.size()-1)) {
				string strNext = arrFileNames[nPosition+1];
				arrFileNames[nPosition+1] = arrFileNames[nPosition];
				arrFileNames[nPosition] = strNext;
				nPosition++;
			}
			break;
		case 4:
			g_nStartWithNow++;
			break;
		case 5:
			g_nStartWithNow+=10;
			break;
		case 6:
			if (g_nStartWithNow > 0) g_nStartWithNow--;
			break;
		case 7:
			if (g_nStartWithNow > 10) g_nStartWithNow -= 10; else g_nStartWithNow = 0;
			break;
		case 8:
			PerformRename(arrFileNames, arrProcessedNames);
			return OR_OK;
		case 9:
			arrFileNames.insert(arrFileNames.begin()+nOK, "");
			nPosition = ++nOK;
			break;
		case 10:
			if (nOK > 0) {
				if (arrFileNames[nOK-1].empty()) arrFileNames.erase(arrFileNames.begin()+nOK-1);
				nOK--;
			}
			nPosition = nOK;
			break;
		}
	} while (true);
	return OR_CANCEL;
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
