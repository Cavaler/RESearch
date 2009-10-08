#include "StdAfx.h"
#include "..\RESearch.h"

CParameterSet g_RnParamSet(RenameFilesExecutor, 6, 3,
	"Mask", &MaskText, "Text", &SearchText, "Replace", &ReplaceText,
	"@Mask", &FMask, "@Text", &FText, "@Replace", &FRReplace,
	"MaskAsRegExp", &FMaskAsRegExp, "TextAsRegExp", &FSearchAs, "Repeating", &FRepeating
	);
CParameterSet g_QRParamSet(QuickRenameFilesExecutor, 4, 2,
	"Text", &SearchText, "Replace", &ReplaceText,
	"@Text", &FText, "@Replace", &FRReplace,
	"TextAsRegExp", &FSearchAs, "Repeating", &FRepeating
	);

int FileNumber;
bool FTAskOverwrite;
bool FTAskCreatePath;
int g_nStartWithNow;
bool g_bFRStripCommon;

void FTReadRegistry(HKEY Key) {
	#define DECLARE_PERSIST_LOAD Key
	#include "PersistVars.h"

	RnPresets = new CRnPresetCollection(g_RnParamSet);
	QRPresets = new CQRPresetCollection(g_QRParamSet);
}

void FTWriteRegistry(HKEY Key) {
	#define DECLARE_PERSIST_SAVE Key
	#include "PersistVars.h"
}

void FTCleanup(BOOL PatternOnly) {
	if (!PatternOnly) {
		delete RnPresets;
		delete QRPresets;
	}
}

void ChangeSelection(int How) {
	tstring MaskText;

	CFarDialog Dialog(44,10,_T("FileSelectDlg"));
	Dialog.AddFrame(How);
	Dialog.Add(new CFarCheckBoxItem(5,3,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarCheckBoxItem(5,4,0,MCaseSensitive,&FACaseSensitive));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,38,DIF_HISTORY,_T("Masks"), MaskText));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.SetFocus(4);
	FACaseSensitive=MaskCaseHere();

	do {
		MaskText=FMask;
		if (Dialog.Display(-1)==-1) return;
		FMask=MaskText;
	} while (!FPrepareMaskPattern());

	CPanelInfo PInfo;
	PInfo.GetInfo(false);

	CFarMaskSet Mask(FMask.c_str());

	for (int I=0;I<PInfo.ItemsNumber;I++) {
		if (Mask(FarFileName(PInfo.PanelItems[I].FindData))) {
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

#ifdef UNICODE
	SetPanelSelection(false, PInfo.PanelItems);
	StartupInfo.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, NULL);
#else
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_SETSELECTION,&PInfo);
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_REDRAWPANEL,NULL);
#endif
}

BOOL ConfirmRename(const TCHAR *From,const TCHAR *To) {
	if (!ConfirmFile(MMenuRename,From)) return FALSE;
	if (g_bInterrupted) return FALSE;
	if (!FRConfirmLineThisFile) return TRUE;

	const TCHAR *Lines[]={
		GetMsg(MMenuRename),GetMsg(MAskRename),From,GetMsg(MAskTo),To,
		GetMsg(MOk),GetMsg(MAll),GetMsg(MAllFiles),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(StartupInfo.ModuleNumber,0,_T("FRAskRename"),Lines,10,5)) {
	case 2:FRConfirmLineThisRun=FALSE;
	case 1:FRConfirmLineThisFile=FALSE;
	case 0:return TRUE;
	case -1:
	case 3:return FALSE;
	}
	g_bInterrupted=TRUE;
	return FALSE;
}

BOOL FindRename(TCHAR *FileName,int *&Match,int &MatchCount,int &MatchStart,int &MatchLength) {
	if (FSearchAs==SA_REGEXP) {
		MatchCount=pcre_info(FPattern,NULL,NULL)+1;
		Match=new int[MatchCount*3];
		if (do_pcre_exec(FPattern,FPatternExtra,FileName,_tcslen(FileName),MatchStart,0,Match,MatchCount*3)>=0) {
			MatchStart=Match[0];
			MatchLength=Match[1]-Match[0];
			return TRUE;
		}
		delete[] Match;
	} else {
		int NewMatchStart;
		TCHAR *Table = FCaseSensitive ? NULL : UpCaseTable;
		NewMatchStart=BMHSearch(FileName+MatchStart,_tcslen(FileName)-MatchStart,FTextUpcase.data(),FTextUpcase.size(),Table);

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
	TCHAR NewName[MAX_PATH];
	TCHAR *FileName=_tcsrchr(FindData->cFileName,'\\');
	if (FileName) FileName++; else FileName=FindData->cFileName;

	FRConfirmLineThisFile=FRConfirmLineThisRun;
	FileConfirmed=!FRConfirmFileThisRun;

	FileNumber++;
	while (FindRename(FileName,Match,MatchCount,MatchStart,MatchLength)) {
		int Numbers[3]={FileNumber,FileNumber,ReplaceNumber};
		tstring NewSubName=CreateReplaceString(FileName,Match,MatchCount,FRReplace.c_str(),_T(""),Numbers,-1);

		_tcsncpy(NewName,FileName,MatchStart);
		_tcscpy(NewName+MatchStart,NewSubName.c_str());
		_tcscat(NewName,FileName+MatchStart+MatchLength);
		MatchStart += NewSubName.length();
		if (Match) delete[] Match;

		if (ConfirmRename(FileName,NewName)) {
			memmove(NewName+(FileName-FindData->cFileName),NewName,(_tcslen(NewName)+1)*sizeof(TCHAR));
			_tcsncpy(NewName,FindData->cFileName,FileName-FindData->cFileName);
			if (MoveFile(FindData->cFileName,NewName)) {
				_tcscpy(FileName,NewName+(FileName-FindData->cFileName));
				Modified=TRUE;ReplaceNumber++;
				if (!FRepeating) break;
			} else {
				DWORD Error = GetLastError();
				switch (Error) {
				case ERROR_ALREADY_EXISTS:
					if (FTAskOverwrite) {
						const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MFile),NewName,
							GetMsg(MAskOverwrite),FindData->cFileName,GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)};
						switch (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FRenameOverwrite"),Lines,9,4)) {
						case 1:
							FTAskOverwrite=false;
						case 0:
							break;
						case 3:
							g_bInterrupted=TRUE;
						case 2:
							return;
						}
					}

					if (DeleteFile(NewName)) {
						if (MoveFile(FindData->cFileName,NewName)) {
							_tcscpy(FileName,NewName+(FileName-FindData->cFileName));
							Modified=TRUE;ReplaceNumber++;
							Error = ERROR_SUCCESS;
							break;
						}
					}
					Error=GetLastError();
					break;

				case ERROR_PATH_NOT_FOUND:
					if (FTAskCreatePath) {
						const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MFile),NewName,
							GetMsg(MAskCreatePath),GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)};
						switch (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FRenameCreatePath"),Lines,8,4)) {
						case 1:
							FTAskCreatePath=false;
						case 0:
							break;
						case 3:
							g_bInterrupted=TRUE;
						case 2:
							return;
						}
					}

					if (CreateDirectoriesForFile(NewName)) {
						if (MoveFile(FindData->cFileName,NewName)) {
							_tcscpy(FileName,NewName+(FileName-FindData->cFileName));
							Modified=TRUE;ReplaceNumber++;
							Error = ERROR_SUCCESS;
							break;
						}
					}
					Error=GetLastError();
				}

				if (Error != ERROR_SUCCESS) {
					const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MRenameError),FindData->cFileName,
						GetMsg(MAskTo),NewName,GetMsg(MOk),GetMsg(MCancel)};
					if (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FRenameError"),Lines,7,2)==1) g_bInterrupted=TRUE;
					return;
				} else {
					if (!FRepeating) break;
					continue;
				}
			}
		}
		if (g_bInterrupted) return;
	}
	if (Modified) AddFile(FindData,PanelItems,ItemsNumber);
}

BOOL RenameFilesPrompt() {
	CFarDialog Dialog(76,19,_T("FileRenameDlg"));
	Dialog.AddFrame(MMenuRename);

	Dialog.Add(new CFarCheckBoxItem(25,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarCheckBoxItem(52,2,0,MCaseSensitive,&FACaseSensitive));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("Masks"), MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("SearchText"), SearchText));
	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,_T("ReplaceText"), ReplaceText));

	Dialog.Add(new CFarCheckBoxItem(25,4,0,MRegExp,(BOOL *)&FSearchAs));
	Dialog.Add(new CFarCheckBoxItem(52,4,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(25,6,0,MRepeating,&FRepeating));
	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));

	Dialog.Add(new CFarTextItem(5,9,0,MSearchIn));
	Dialog.Add(new CFarComboBoxItem(15,9,60,0,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));

	Dialog.Add(new CFarCheckBoxItem(5,11,0,MViewModified,&FROpenModified));
	Dialog.Add(new CFarCheckBoxItem(5,12,0,MConfirmFile,&FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MConfirmLine,&FRConfirmLine));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,9,0,0,MBtnPresets));
	Dialog.SetFocus(4);

	if (FSearchAs>=SA_SEVERALLINE) FSearchAs=SA_PLAINTEXT;
	if (!g_bFromCmdLine) FCaseSensitive=MaskCaseHere();
	FACaseSensitive=FCaseSensitive;

	MaskText=FMask;
	SearchText=FText;
	ReplaceText=FRReplace;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(2, -3, -1)) {
		case 0:
			FMask=MaskText;
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case 1:
			RnPresets->ShowMenu(true);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!FPreparePattern(false));
	return TRUE;
}

OperationResult RenameFiles(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog) {
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.Plugin||(PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	FUTF8=FALSE;
	if (ShowDialog) {
		if (!RenameFilesPrompt()) return OR_CANCEL;
	} else {
		if (!FPreparePattern(false)) return OR_CANCEL;
	}

	FRConfirmFileThisRun=FRConfirmFile;
	FRConfirmLineThisRun=FRConfirmLine;
	FTAskOverwrite = FTAskCreatePath = true;
	FileNumber=-1;g_bInterrupted=FALSE;

	if (ScanDirectories(PanelItems,ItemsNumber,RenameFile)) {
		if (!FROpenModified) return OR_OK; else
		return (*ItemsNumber==0) ? NoFilesFound() : OR_PANEL;
	} else return OR_FAILED;
}

OperationResult RenameFilesExecutor() {
	FMask=MaskText;
	FText=SearchText;
	FRReplace=ReplaceText;
	if (!FPreparePattern(false)) return OR_FAILED;
	FTAskOverwrite = FTAskCreatePath = true;
	FileNumber=-1;g_bInterrupted=FALSE;

	FRConfirmFileThisRun = FALSE;	//FRConfirmFile;
	FRConfirmLineThisRun = FALSE;	//FRConfirmLine;

	if (ScanDirectories(&PanelItems, &ItemsNumber, RenameFile)) {
//		if (!FROpenModified) return OR_OK; else
//			return (ItemsNumber == 0) ? OR_OK : OR_PANEL;
		return OR_OK;
	} else return OR_FAILED;
}

BOOL PerformRenameSelectedFiles(CPanelInfo &PInfo,PluginPanelItem **PanelItems,int *ItemsNumber) {
	FileNumber=-1;g_bInterrupted=FALSE;

	if ((PInfo.SelectedItemsNumber==0)&&(PInfo.ItemsNumber>0)&&
		(_tcscmp(FarFileName(PInfo.PanelItems[PInfo.CurrentItem].FindData),_T(".."))==0)) {

		for (int I=0;I<PInfo.ItemsNumber;I++) {
			if (I==PInfo.CurrentItem) continue;
			if (g_bInterrupted) break;
			RenameFile(&FFDtoWFD(PInfo.PanelItems[I].FindData),PanelItems,ItemsNumber);
		}
	} else {
		for (int I=0;I<PInfo.SelectedItemsNumber;I++) {
			if (g_bInterrupted) break;
			RenameFile(&FFDtoWFD(PInfo.SelectedItems[I].FindData),PanelItems,ItemsNumber);
		}
	}

#ifdef UNICODE
	StartupInfo.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
#else
	StartupInfo.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, NULL);
#endif
	return TRUE;
}

BOOL RenameSelectedFilesPrompt() {
	CFarDialog Dialog(76, 13, _T("SelectedFileRenameDlg"));
	Dialog.AddFrame(MMenuRename);

	Dialog.Add(new CFarCheckBoxItem(25,2,0,MRegExp,(BOOL *)&FSearchAs));
	Dialog.Add(new CFarCheckBoxItem(50,2,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarTextItem(5,2,0,MText));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("SearchText"), SearchText));

	Dialog.Add(new CFarTextItem(5,4,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("ReplaceText"), ReplaceText));
	Dialog.Add(new CFarCheckBoxItem(25,4,0,MRepeating,&FRepeating));

//	Dialog.Add(new CFarCheckBoxItem(5,7,0,MConfirmFile,&FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MConfirmLine,&FRConfirmLine));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,6,0,0,MBtnPresets));
	Dialog.SetFocus(4);

	if (FSearchAs>=SA_SEVERALLINE) FSearchAs=SA_PLAINTEXT;
	if (!g_bFromCmdLine) FCaseSensitive=MaskCaseHere();

	int ExitCode;
	SearchText=FText;
	ReplaceText=FRReplace;
	FRConfirmFile = FALSE;
	do {
		switch (ExitCode=Dialog.Display(2, -3, -1)) {
		case 0:
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case 1:
			QRPresets->ShowMenu(true);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!FPreparePattern(false));
	return TRUE;
}

OperationResult RenameSelectedFiles(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog) {
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.Plugin||(PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	FUTF8=FALSE;
	if (ShowDialog) {
		if (!RenameSelectedFilesPrompt()) return OR_CANCEL;
	} else {
		if (!FPreparePattern(false)) return OR_CANCEL;
	}

	*ItemsNumber=0;*PanelItems=NULL;
	FRConfirmFileThisRun=FRConfirmFile;
	FRConfirmLineThisRun=FRConfirmLine;
	FTAskOverwrite = FTAskCreatePath = true;

	PerformRenameSelectedFiles(PInfo,PanelItems,ItemsNumber);
	return (*ItemsNumber==0) ? NoFilesFound() : OR_OK;
}

OperationResult QuickRenameFilesExecutor() {
	FText=SearchText;
	FRReplace=ReplaceText;
	if (!FPreparePattern(false)) return OR_FAILED;
	FTAskOverwrite = FTAskCreatePath = true;

	FRConfirmFileThisRun = FALSE;	//FRConfirmFile;
	FRConfirmLineThisRun = FALSE;	//FRConfirmLine;

	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.Plugin || (PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	return PerformRenameSelectedFiles(PInfo,&PanelItems,&ItemsNumber) ? OR_OK : OR_CANCEL;
}

void StripCommonPart(vector<tstring> &arrFileNames) {
	int nCommon = -1;
	for (size_t nStr = 0; nStr < arrFileNames.size(); nStr++) {
		if (arrFileNames[nStr].empty()) continue;
		if (nCommon < 0) {
			nCommon = arrFileNames[nStr].rfind('.');
			if (nCommon == string::npos)
				nCommon = arrFileNames[nStr].length();
			continue;
		}
		while ((nCommon > 0) && (_tcsnicmp(arrFileNames[0].c_str(), arrFileNames[nStr].c_str(), nCommon) != 0)) nCommon--;
		if (nCommon == 0) return;
	}

	for (size_t nStr = 0; nStr < arrFileNames.size(); nStr++)
		arrFileNames[nStr].erase(0, nCommon);
}

void ProcessNames(vector<tstring> &arrFileNames, vector<tstring> &arrProcessedNames) {
	arrProcessedNames.resize(0);
	CRegExp reStrip(g_strStrip, PCRE_CASELESS);

	vector<tstring> arrStripped = arrFileNames;

	if (g_bFRStripCommon) StripCommonPart(arrStripped);
	for (size_t nItem = 0; nItem < arrStripped.size(); nItem++) {
		if (!arrStripped[nItem].empty()) {
			// Preserving extensions!
			size_t nExt = arrStripped[nItem].rfind('.');
			if (nExt != tstring::npos) {
				tstring strReplacing = arrStripped[nItem].substr(0, nExt);

				vector<tstring> arrMatches;
				if (reStrip.Match(strReplacing, PCRE_ANCHORED, &arrMatches)) {
					arrStripped[nItem] = strReplacing.substr(arrMatches[0].length()) + arrStripped[nItem].substr(nExt);
				}
			} else {
				vector<tstring> arrMatches;
				if (reStrip.Match(arrStripped[nItem], PCRE_ANCHORED, &arrMatches)) {
					arrStripped[nItem] = arrStripped[nItem].substr(arrMatches[0].length());
				}
			}
		}
	}
	if (g_bFRStripCommon) StripCommonPart(arrStripped);

	for (size_t nItem = 0; nItem < arrStripped.size(); nItem++) {
		tstring strName = arrStripped[nItem];

		if (!strName.empty()) {
			TCHAR szNumber[16];
			_tprintf(szNumber, _T("%0*d"), g_nWidth, nItem+g_nStartWithNow);
			strName = g_strPrefix + szNumber + g_strPostfix + strName;

			arrProcessedNames.push_back(strName);
		} else {
			arrProcessedNames.push_back(_T("---"));
		}
	}
}

void PerformRename(vector<tstring> &arrFileNames, vector<tstring> &arrProcessedNames) {
	for (size_t nItem = 0; nItem < arrFileNames.size(); nItem++) {
		if (!arrFileNames[nItem].empty())
			MoveFile(arrFileNames[nItem].c_str(), arrProcessedNames[nItem].c_str());
	}
}

OperationResult RenumberFiles() {
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.PanelType != PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin && ((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;
	g_nStartWithNow = g_nStartWith;

	vector<tstring> arrFileNames;
	for (int nItem = 0; nItem < PInfo.SelectedItemsNumber; nItem++)
		arrFileNames.push_back(FarFileName(PInfo.SelectedItems[nItem].FindData));

	int BreakKeys[] = {
		VK_F2, VK_F7, (PKF_CONTROL<<16)|VK_UP, (PKF_CONTROL<<16)|VK_DOWN,
		VK_ADD, (PKF_CONTROL<<16)|VK_ADD, (PKF_SHIFT<<16)|VK_ADD, (PKF_ALT<<16)|VK_ADD,
		VK_SUBTRACT, (PKF_CONTROL<<16)|VK_SUBTRACT, (PKF_SHIFT<<16)|VK_SUBTRACT, (PKF_ALT<<16)|VK_SUBTRACT,
		VK_F10, VK_INSERT, VK_DELETE, VK_F8, 0
	};

	bool bOriginal = false;
	int nPosition = 0;
	int nOK = 0;
	g_bFRStripCommon = true;
	do {
		vector<tstring> arrProcessedNames;
		ProcessNames(arrFileNames, arrProcessedNames);
		vector<tstring> &arrNames = bOriginal ? arrFileNames : arrProcessedNames;

		TCHAR szBreak[32];
		_tprintf(szBreak, _T("--- %0*d ----------"), g_nWidth, nOK);
		arrNames.insert(arrNames.begin()+nOK, szBreak);
		if (nPosition >= nOK) nPosition++;

		int nBreakKey = 0;
		nPosition = ChooseMenu(arrNames, GetMsg(MRenumber), _T("F2, F7, Ctrl-\x18\x19, F10=Go"), _T("Renumber"),
			nPosition, FMENU_WRAPMODE, BreakKeys, &nBreakKey);
		if (nPosition >= nOK) nPosition--; else
			if (nPosition < 0) nPosition = -2;		// -1 is not Esc

		arrNames.erase(arrNames.begin()+nOK);

		switch (nBreakKey) {
		case -1:
			if (nPosition < -1) return OR_CANCEL;
			// Send to end of OK
			if (nPosition >= nOK) {
				tstring strPrev = arrFileNames[nPosition];
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
				tstring strPrev = arrFileNames[nPosition-1];
				arrFileNames[nPosition-1] = arrFileNames[nPosition];
				arrFileNames[nPosition] = strPrev;
				nPosition--;
			}
			break;
		case 3:
			if ((nPosition >= nOK) && (nPosition < (int)arrFileNames.size()-1)) {
				tstring strNext = arrFileNames[nPosition+1];
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
			g_nStartWithNow+=100;
			break;
		case 7:
			g_nStartWithNow+=1000;
			break;
		case 8:
			if (g_nStartWithNow > 0) g_nStartWithNow--;
			break;
		case 9:
			if (g_nStartWithNow > 10) g_nStartWithNow -= 10; else g_nStartWithNow = 0;
			break;
		case 10:
			if (g_nStartWithNow > 100) g_nStartWithNow -= 100; else g_nStartWithNow = 0;
			break;
		case 11:
			if (g_nStartWithNow > 1000) g_nStartWithNow -= 1000; else g_nStartWithNow = 0;
			break;
		case 12:
			PerformRename(arrFileNames, arrProcessedNames);
			return OR_OK;
		case 13:
			arrFileNames.insert(arrFileNames.begin()+nOK, _T(""));
			nPosition = ++nOK;
			break;
		case 14:
			if (nOK > 0) {
				if (arrFileNames[nOK-1].empty()) arrFileNames.erase(arrFileNames.begin()+nOK-1);
				nOK--;
			}
			nPosition = nOK;
			break;
		case 15:
			g_bFRStripCommon = !g_bFRStripCommon;
			break;
		}
	} while (true);
	return OR_CANCEL;
}

BOOL CRnPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,17,_T("RPresetDlg"));
	Dialog.AddFrame(MFRPreset);

	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"), pPreset->Name()));

	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));
	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("Masks"), pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MSearchFor));
	Dialog.Add(new CFarCheckBoxItem(35,6,0,MAsRegExp,&pPreset->m_mapInts["TextAsRegExp"]));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,8,0,MReplaceWith));
	Dialog.Add(new CFarCheckBoxItem(35,8,0,MRepeating,&pPreset->m_mapInts["Repeating"]));
	Dialog.Add(new CFarEditItem(5,9,70,DIF_HISTORY,_T("ReplaceText"), pPreset->m_mapStrings["Replace"]));

	Dialog.Add(new CFarCheckBoxItem(5,11,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	return Dialog.Display(-1)==0;
}

BOOL CQRPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,15,_T("QRPresetDlg"));
	Dialog.AddFrame(MFRPreset);

	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"), pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["TextAsRegExp"]));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarCheckBoxItem(35,6,0,MRepeating,&pPreset->m_mapInts["Repeating"]));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,_T("ReplaceText"), pPreset->m_mapStrings["Replace"]));

	Dialog.Add(new CFarCheckBoxItem(5,9,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	return Dialog.Display(-1)==0;
}
