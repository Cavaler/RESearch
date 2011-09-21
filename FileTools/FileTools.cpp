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

bool FTAskOverwrite;
bool FTAskCreatePath;
int g_nStartWithNow;

rename_vector m_arrPendingRename;

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

LONG_PTR WINAPI FileSelectDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2) {
	switch (nMsg) {
	case DN_INITDIALOG:
		HighlightREError(pDlg);
		break;
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

void ChangeSelection(int How) {
	tstring MaskText;

	CFarDialog Dialog(44,10,_T("FileSelectDlg"));
	Dialog.SetWindowProc(FileSelectDialogProc, 0);

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

	for (int I=0;I<PInfo.ItemsNumber;I++) {
		if (MultipleMasksApply(FarFileName(PInfo.PanelItems[I].FindData))) {
			switch (How) {
			case MSelect:
				PInfo.PanelItems[I].Flags|=PPIF_SELECTED;
				break;
			case MUnselect:
				PInfo.PanelItems[I].Flags&=~PPIF_SELECTED;
				break;
			case MFlipSelection:
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

BOOL ConfirmWholeFileRename(const TCHAR *From, const TCHAR *To) {
	if (g_bInterrupted) return FALSE;
	if (FileConfirmed) return TRUE;

	const TCHAR *Lines[]={
		GetMsg(MMenuRename),GetMsg(MAskRename),From,GetMsg(MAskTo),To,
		GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(StartupInfo.ModuleNumber,0,_T("FRAskRename"),Lines,9,4)) {
	case 1:FRConfirmFileThisRun=FALSE;
	case 0:return (FileConfirmed=TRUE);
	case -1:
	case 3:g_bInterrupted=TRUE;
	}
	return FALSE;
}

BOOL ConfirmRename(const TCHAR *From,const TCHAR *To) {
	if (g_bInterrupted) return FALSE;
	if (!FRConfirmLineThisFile) return TRUE;
	if (_tcscmp(From, To) == 0) return TRUE;

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

BOOL FindRename(const TCHAR *FileName, int &MatchStart, int &MatchLength)
{
	REParam.Clear();
	REParam.AddSource(FileName, _tcslen(FileName));

	if (FSearchAs==SA_REGEXP) {
		REParam.AddRE(FPattern);

		if (do_pcre_exec(FPattern,FPatternExtra,FileName,_tcslen(FileName),MatchStart,0,REParam.Match(),REParam.Count())>=0) {
			MatchDone();
			REParam.FillStartLength(&MatchStart, &MatchLength);
			return TRUE;
		}
	} else {
		int NewMatchStart;
		TCHAR *Table = FCaseSensitive ? NULL : UpCaseTable;
		NewMatchStart=BMHSearch(FileName+MatchStart,_tcslen(FileName)-MatchStart,FTextUpcase.data(),FTextUpcase.size(),Table);

		if (NewMatchStart>=0) {
			MatchStart+=NewMatchStart;
			MatchLength=FText.size();
			return TRUE;
		}
	}
	return FALSE;
}

bool PerformSingleRename(rename_pair &Item) {
	if (MoveFile(Item.first.c_str(), Item.second.c_str())) return true;

	DWORD Error = GetLastError();
	switch (Error) {
	case ERROR_ALREADY_EXISTS:
		if (FTAskOverwrite) {
			const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MFile),Item.second.c_str(),
				GetMsg(MAskOverwrite),Item.first.c_str(),GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)};
			switch (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FRenameOverwrite"),Lines,9,4)) {
			case 1:
				FTAskOverwrite=false;
			case 0:
				break;
			case 3:
				g_bInterrupted=TRUE;
			case 2:
				return false;
			}
		}

		if (MoveFileEx(Item.first.c_str(), Item.second.c_str(), MOVEFILE_REPLACE_EXISTING))
			Error = ERROR_SUCCESS;
		else
			Error = GetLastError();
		break;

	case ERROR_PATH_NOT_FOUND:
		if (FTAskCreatePath) {
			const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MFile),Item.second.c_str(),
				GetMsg(MAskCreatePath),GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)};
			switch (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FRenameCreatePath"),Lines,8,4)) {
			case 1:
				FTAskCreatePath=false;
			case 0:
				break;
			case 3:
				g_bInterrupted=TRUE;
			case 2:
				return false;
			}
		}

		if (CreateDirectoriesForFile(Item.second.c_str()) && MoveFile(Item.first.c_str(), Item.second.c_str()))
			Error = ERROR_SUCCESS;
		else
			Error = GetLastError();
	}

	if (Error != ERROR_SUCCESS) {
		const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MRenameError),Item.first.c_str(),
			GetMsg(MAskTo),Item.second.c_str(),GetMsg(MOk),GetMsg(MCancel)};
		if (StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FRenameError"),Lines,7,2)==1) g_bInterrupted=TRUE;
		return false;
	}

	return true;
}

bool PerformSingleRename(rename_pair &Item, panelitem_vector &PanelItems) {
	if (PerformSingleRename(Item)) {
		m_arrLastRename.push_back(Item);
		AddFile(Item.second.c_str(), PanelItems);
		return true;
	} else
		return false;
}

void PerformFinalRename(panelitem_vector &PanelItems) {
	for (size_t nItem = 0; nItem < m_arrPendingRename.size(); nItem++) {
		PerformSingleRename(m_arrPendingRename[nItem], PanelItems);
	}
}

void RenamePreview(panelitem_vector &PanelItems) {
	vector<tstring> arrItems;
	int nBreakKey, nResult = 0;
	int BreakKeys[] = {VK_INSERT, VK_DELETE, 0};

	if (m_arrPendingRename.empty()) return;

	RefreshConsoleInfo();
	int nMaxX = ConInfo.dwSize.X/2 - 8;

	do {
		vector<tstring> arrFrom;
		vector<tstring> arrTo;

		for each (const rename_pair &Item in m_arrPendingRename) {
			QuoteString(Item.first. data(), Item.first. size(), arrFrom, nMaxX);
			QuoteString(Item.second.data(), Item.second.size(), arrTo,   nMaxX);
		}

		MakeSameWidth(arrFrom);
		MakeSameWidth(arrTo  );

		arrItems.clear();
		for (size_t nItem = 0; nItem < arrFrom.size(); nItem++) {
			arrItems.push_back(arrFrom[nItem] + _T(" => ") + arrTo[nItem]);
		}

		nResult = ChooseMenu(arrItems, GetMsg(MRenamePreview), _T("Ins, Del, Enter, Esc"), _T("RenamePreview"), nResult, FMENU_WRAPMODE,
			BreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			if (nResult >= 0) {
				PerformFinalRename(PanelItems);
			} else {
				g_bInterrupted = TRUE;
			}
			return;
		case 0:
			if (PerformSingleRename(m_arrPendingRename[nResult], PanelItems)) {
				m_arrPendingRename.erase(m_arrPendingRename.begin() + nResult);
			}
			break;
		case 1:
			m_arrPendingRename.erase(m_arrPendingRename.begin() + nResult);
			break;
		}

		if (m_arrPendingRename.empty()) {
			g_bInterrupted = TRUE;
			return;
		}
	} while (true);
}

void RenameFile(WIN32_FIND_DATA *FindData, panelitem_vector &PanelItems) {
	int MatchStart = 0, MatchLength;
	int FindNumber = 0, ReplaceNumber = 0;

	tstring strPath, strOriginalName, strCurrentName;

	TCHAR *szSlash=_tcsrchr(FindData->cFileName,'\\');
	if (szSlash) {
		strPath = tstring(FindData->cFileName, szSlash - FindData->cFileName + 1);
		strOriginalName = szSlash+1;
	} else {
		strOriginalName = FindData->cFileName;
	}
	strCurrentName = strOriginalName;

	FRConfirmLineThisFile=FRConfirmLineThisRun;
	FileConfirmed=!FRConfirmFileThisRun;

	FileNumber++;

	bool bOnlyGlobalConfirm = !FRConfirmLineThisFile;

	while (FindRename(strCurrentName.c_str(), MatchStart, MatchLength)) {
		REParam.AddFNumbers(FileNumber, FindNumber, ReplaceNumber);

		tstring strNewSubName = CSO::CreateReplaceString(FRReplace.c_str(), _T(""), -1, REParam);

		tstring strNewName = strCurrentName.substr(0, MatchStart) + strNewSubName + strCurrentName.substr(MatchStart + MatchLength);

		MatchStart += (strNewSubName.empty()) ? 1 : strNewSubName.length();

		BOOL bConfirm = TRUE;

		if (!bOnlyGlobalConfirm) {
			if (!ConfirmFile(MMenuRename,strOriginalName.c_str())) break;

			bConfirm = ConfirmRename(strCurrentName.c_str(), strNewName.c_str());
		}

		if (bConfirm) {
			strCurrentName = strNewName;
			ReplaceNumber++;
		}
		FindNumber++;

		if (!FRepeating) break;
	}

	if (strOriginalName == strCurrentName) return;
	if (bOnlyGlobalConfirm && !ConfirmWholeFileRename(strOriginalName.c_str(), strCurrentName.c_str())) return;

	strCurrentName = strPath + strCurrentName;

	rename_pair Item(FindData->cFileName, strCurrentName);
	if (FRPreviewRename) {
		m_arrPendingRename.push_back(Item);
	} else {
		PerformSingleRename(Item, PanelItems);
	}
}

BOOL RenameFilesPrompt() {
	CFarDialog Dialog(76,20,_T("FileRenameDlg"));
	Dialog.SetWindowProc(FileSearchDialogProc, 0);

	Dialog.AddFrame(MRename);

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
	Dialog.Add(new CFarComboBoxItem(15,9,55,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));

	Dialog.Add(new CFarCheckBoxItem(5,11,0,MViewModified,&FROpenModified));
	Dialog.Add(new CFarCheckBoxItem(5,12,0,MConfirmFile,&FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MConfirmLine,&FRConfirmLine));
	Dialog.Add(new CFarCheckBoxItem(5,14,0,MPreviewRename,&FRPreviewRename));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,9,0,0,MBtnPresets));
	Dialog.Add(new CFarButtonItem(58,10,0,0,MBtnREBuilder));
	Dialog.SetFocus(4);

	if (FSearchAs>=SA_SEVERALLINE) FSearchAs=SA_PLAINTEXT;
	if (!g_bFromCmdLine) FCaseSensitive=MaskCaseHere();
	FACaseSensitive=FCaseSensitive;

	MaskText=FMask;
	SearchText=FText;
	ReplaceText=FRReplace;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(3, -4, -2, -1)) {
		case 0:
			FMask=MaskText;
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case 1:
			RnPresets->ShowMenu(true);
			break;
		case 2:
			if (RunREBuilder(SearchText, ReplaceText)) {
				FSearchAs = SA_REGEXP;
			}
			break;
		default:
			return FALSE;
		}
	} while ((ExitCode>=1)||!FPreparePattern(false));
	return TRUE;
}

OperationResult RenameFiles(panelitem_vector &PanelItems, BOOL ShowDialog) {
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.Plugin||(PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	if (ShowDialog) {
		if (!RenameFilesPrompt()) return OR_CANCEL;
	} else {
		if (!FPreparePattern(false)) return OR_CANCEL;
	}

	FRConfirmFileThisRun=FRConfirmFile;
	FRConfirmLineThisRun=FRConfirmLine;
	FTAskOverwrite = FTAskCreatePath = true;
	FileNumber=-1;
	g_bInterrupted=FALSE;

	m_arrPendingRename.clear();
	m_arrLastRename.clear();

	if (ScanDirectories(PanelItems, RenameFile)) {
		if (FRPreviewRename) RenamePreview(PanelItems);
		if (!FROpenModified) return OR_OK; else
		return (PanelItems.empty()) ? NoFilesFound() : OR_PANEL;
	} else return OR_FAILED;
}

OperationResult RenameFilesExecutor() {
	FMask=MaskText;
	FText=SearchText;
	FRReplace=ReplaceText;
	if (!FPreparePattern(false)) return OR_FAILED;
	FTAskOverwrite = FTAskCreatePath = true;
	FileNumber=-1;
	g_bInterrupted=FALSE;
	
	m_arrPendingRename.clear();
	m_arrLastRename.clear();

	FRConfirmFileThisRun = FALSE;	//FRConfirmFile;
	FRConfirmLineThisRun = FALSE;	//FRConfirmLine;
	FRPreviewRename = FALSE;

	if (ScanDirectories(g_PanelItems, RenameFile)) {
//		if (!FROpenModified) return OR_OK; else
//			return (ItemsNumber == 0) ? OR_OK : OR_PANEL;
		return OR_OK;
	} else return OR_FAILED;
}

BOOL PerformRenameSelectedFiles(CPanelInfo &PInfo, panelitem_vector &PanelItems)
{
	FileNumber=-1;
	g_bInterrupted=FALSE;

	m_arrPendingRename.clear();
	m_arrLastRename.clear();

	//	To allow 'Restore selection' of _unmodified_ files
	vector<tstring> arrOrigNames;

	BOOL bRestoreSelection = FRLeaveSelection;
	if ((PInfo.SelectedItemsNumber==0)&&(PInfo.ItemsNumber>0)&&
		(_tcscmp(FarFileName(PInfo.PanelItems[PInfo.CurrentItem].FindData),_T(".."))==0)) {

		bRestoreSelection = FALSE;
		for (int I=0;I<PInfo.ItemsNumber;I++) {
			if (I==PInfo.CurrentItem) continue;
			if (g_bInterrupted) break;

			LPCTSTR szFileName = FarFileName(PInfo.PanelItems[I].FindData);
			FileFillNamedParameters(CatFile(PInfo.CurDir, szFileName).c_str());
			arrOrigNames.push_back(szFileName);

			RenameFile(&FFDtoWFD(PInfo.PanelItems[I].FindData),PanelItems);
		}
	} else {
		if ((PInfo.SelectedItemsNumber == 1) && ((PInfo.SelectedItems[0].Flags & PPIF_SELECTED) == 0))
			bRestoreSelection = FALSE;
		for (int I=0;I<PInfo.SelectedItemsNumber;I++) {
			if (g_bInterrupted) break;

			LPCTSTR szFileName = FarFileName(PInfo.SelectedItems[I].FindData);
			FileFillNamedParameters(CatFile(PInfo.CurDir, szFileName).c_str());
			arrOrigNames.push_back(szFileName);

			RenameFile(&FFDtoWFD(PInfo.SelectedItems[I].FindData),PanelItems);
		}
	}

	if (FRPreviewRename) RenamePreview(PanelItems);

#ifdef UNICODE
	StartupInfo.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
#else
	StartupInfo.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, NULL);
#endif
	if (bRestoreSelection) {
		CPanelInfo PNewInfo;
		PNewInfo.GetInfo(false);

		for (size_t nFile = 0; nFile < PanelItems.size(); nFile++) {
			int nItem = PNewInfo.Find(FarFileName(PanelItems[nFile].FindData));
			if (nItem >= 0)
				PNewInfo.PanelItems[nItem].Flags |= PPIF_SELECTED;
		}
		for (size_t nFile = 0; nFile < arrOrigNames.size(); nFile++) {
			int nItem = PNewInfo.Find(arrOrigNames[nFile].c_str());
			if (nItem >= 0)
				PNewInfo.PanelItems[nItem].Flags |= PPIF_SELECTED;
		}
		SetPanelSelection(PNewInfo, false, true);
	}
	return TRUE;
}

BOOL RenameSelectedFilesPrompt() {
	CFarDialog Dialog(76, 15, _T("SelectedFileRenameDlg"));
	Dialog.SetWindowProc(FileSearchDialogProc, 0);

	Dialog.AddFrame(MRenameSelected);

	Dialog.Add(new CFarCheckBoxItem(25,2,0,MRegExp,(BOOL *)&FSearchAs));
	Dialog.Add(new CFarCheckBoxItem(50,2,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarTextItem(5,2,0,MText));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("SearchText"), SearchText));

	Dialog.Add(new CFarTextItem(5,4,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("ReplaceText"), ReplaceText));
	Dialog.Add(new CFarCheckBoxItem(25,4,0,MRepeating,&FRepeating));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MConfirmLine,&FRConfirmLine));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MLeaveSelection,&FRLeaveSelection));
	Dialog.Add(new CFarCheckBoxItem(5,9,0,MPreviewRename,&FRPreviewRename));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,6,0,0,MBtnPresets));
	Dialog.Add(new CFarButtonItem(58,7,0,0,MBtnREBuilder));
	Dialog.SetFocus(4);

	if (FSearchAs>=SA_SEVERALLINE) FSearchAs=SA_PLAINTEXT;
	if (!g_bFromCmdLine) FCaseSensitive=MaskCaseHere();

	int ExitCode;
	SearchText=FText;
	ReplaceText=FRReplace;
	do {
		switch (ExitCode=Dialog.Display(3, -4, -2, -1)) {
		case 0:
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case 1:
			QRPresets->ShowMenu(true);
			break;
		case 2:
			if (RunREBuilder(SearchText, ReplaceText)) {
				FSearchAs = SA_REGEXP;
			}
			break;
		default:
			return FALSE;
		}
	} while ((ExitCode>=1)||!FPreparePattern(false));
	return TRUE;
}

OperationResult RenameSelectedFiles(panelitem_vector &PanelItems, BOOL ShowDialog) {
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.Plugin||(PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	if (ShowDialog) {
		if (!RenameSelectedFilesPrompt()) return OR_CANCEL;
	} else {
		if (!FPreparePattern(false)) return OR_CANCEL;
	}

	PanelItems.clear();
	FRConfirmFileThisRun=FALSE;
	FRConfirmLineThisRun=FRConfirmLine;
	FTAskOverwrite = FTAskCreatePath = true;

	PerformRenameSelectedFiles(PInfo, PanelItems);
	return (PanelItems.empty() && !g_bInterrupted) ? NoFilesFound() : OR_OK;
}

OperationResult QuickRenameFilesExecutor() {
	FText=SearchText;
	FRReplace=ReplaceText;
	if (!FPreparePattern(false)) return OR_FAILED;
	FTAskOverwrite = FTAskCreatePath = true;

	FRConfirmFileThisRun = FALSE;
	FRConfirmLineThisRun = FALSE;
	FRPreviewRename = FALSE;

	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.Plugin || (PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	return PerformRenameSelectedFiles(PInfo, g_PanelItems) ? OR_OK : OR_CANCEL;
}

//////////////////////////////////////////////////////////////////////////

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

	if (g_bStripCommon) StripCommonPart(arrStripped);

	if (g_bStripRegExp) {
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
		if (g_bStripCommon) StripCommonPart(arrStripped);
	}

	for (size_t nItem = 0; nItem < arrStripped.size(); nItem++) {
		tstring strName = arrStripped[nItem];

		if (!strName.empty()) {
			TCHAR szNumber[16];
			_stprintf_s(szNumber, 16, _T("%0*d"), g_nWidth, nItem+g_nStartWithNow);
			strName = g_strPrefix + szNumber + g_strPostfix + strName;

			arrProcessedNames.push_back(strName);
		} else {
			arrProcessedNames.push_back(_T("---"));
		}
	}
}

void PerformRenumber(vector<tstring> &arrFileNames, vector<tstring> &arrProcessedNames, LPCTSTR szCurDir) {
	m_arrLastRename.clear();

	for (size_t nItem = 0; nItem < arrFileNames.size(); nItem++) {
		if (!arrFileNames[nItem].empty() && (arrFileNames[nItem] != arrProcessedNames[nItem])) {
			tstring strSrc = szCurDir + arrFileNames[nItem];
			tstring strTgt = szCurDir + arrProcessedNames[nItem];

			if (MoveFile(strSrc.c_str(), strTgt.c_str())) {
				m_arrLastRename.push_back(make_pair(strSrc, strTgt));
			}
		}
	}

#ifdef UNICODE
	StartupInfo.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
#else
	StartupInfo.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, NULL);
#endif

	if (FRLeaveSelection) {
		CPanelInfo PInfo;
		PInfo.GetInfo(false);

		for (size_t nFile = 0; nFile < arrProcessedNames.size(); nFile++) {
			int nItem = PInfo.Find(arrProcessedNames[nFile].c_str());
			if (nItem >= 0)
				PInfo.PanelItems[nItem].Flags |= PPIF_SELECTED;
		}
		SetPanelSelection(PInfo, false, true);
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
		VK_F10, VK_INSERT, VK_DELETE, VK_F8, VK_F4, 0
	};

	bool bOriginal = false;
	int nPosition = 0;
	int nOK = 0;
	do {
		vector<tstring> arrProcessedNames;
		ProcessNames(arrFileNames, arrProcessedNames);
		vector<tstring> &arrNames = bOriginal ? arrFileNames : arrProcessedNames;

		TCHAR szBreak[32];
		_stprintf_s(szBreak, 32, _T("--- %0*d ----------"), g_nWidth, nOK);
		arrNames.insert(arrNames.begin()+nOK, szBreak);
		if (nPosition >= nOK) nPosition++;

		int nBreakKey = 0;
		nPosition = ChooseMenu(arrNames, GetMsg(MRenumber), _T("F2, F4, F7, Ctrl-\x18\x19, F10=Go"), _T("Renumber"),
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
			PerformRenumber(arrFileNames, arrProcessedNames, AddSlash(tstring(PInfo.CurDir)).c_str());
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
			g_bStripCommon = !g_bStripCommon;
			break;
		case 16:
			ConfigureRenumbering(true);
			WriteRegistry();
			break;
		}
	} while (true);

	return OR_CANCEL;
}

OperationResult UndoRenameFiles() {
	FRConfirmFileThisRun = TRUE;

	for (int nFile = m_arrLastRename.size()-1; nFile >= 0; nFile--) {
		FileConfirmed = !FRConfirmFileThisRun;

		if (ConfirmWholeFileRename(m_arrLastRename[nFile].second.c_str(), m_arrLastRename[nFile].first.c_str()))
			MoveFile(m_arrLastRename[nFile].second.c_str(), m_arrLastRename[nFile].first.c_str());

		if (g_bInterrupted) break;
	}
	m_arrLastRename.clear();

#ifdef UNICODE
	StartupInfo.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
#else
	StartupInfo.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, NULL);
#endif

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
	Dialog.AddFrame(MQRPreset);

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
