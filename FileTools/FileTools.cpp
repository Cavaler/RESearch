#include "StdAfx.h"
#include "..\RESearch.h"

CParameterSet g_RnParamSet(RenameFilesExecutor,
	"Mask", &MaskText, "Text", &SearchText, "Replace", &ReplaceText, "Script", &EREvaluateScript,
	"@Mask", &FMask, "@Text", &FText, "@Replace", &FRReplace, NULL,
	"SearchIn", &FSearchIn, "AdvancedID", &FAdvancedID, NULL,
	"MaskAsRegExp", &FMaskAsRegExp, "TextAsRegExp", &FSearchAs, "CaseSensitive", &FCaseSensitive, "Repeating", &FRepeating,
	"AsScript", &FREvaluate, NULL
	);
CParameterSet g_QRParamSet(QuickRenameFilesExecutor,
	"Text", &SearchText, "Replace", &ReplaceText,
	"@Text", &FText, "@Replace", &FRReplace, NULL, NULL,
	"TextAsRegExp", &FSearchAs, "CaseSensitive", &FCaseSensitive, "Repeating", &FRepeating, NULL
	);

bool FTAskOverwrite;
bool FTAskCreatePath;
int g_nStartWithNow;

rename_vector m_arrPendingRename;

void FTReadRegistry(CFarSettingsKey Key)
{
	#define DECLARE_PERSIST_LOAD Key
	#include "PersistVars.h"

	RnPresets = new CRnPresetCollection(g_RnParamSet);
	QRPresets = new CQRPresetCollection(g_QRParamSet);
}

void FTWriteRegistry(CFarSettingsKey Key)
{
	#define DECLARE_PERSIST_SAVE Key
	#include "PersistVars.h"
}

void FTCleanup(bool PatternOnly)
{
	if (!PatternOnly) {
		delete RnPresets;
		delete QRPresets;
	}
}

LONG_PTR WINAPI FileSelectDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2)
{
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
	} while (!FPrepareMaskPattern(FACaseSensitive));

	CPanelInfo PInfo;
	PInfo.GetInfo(false);

	for (size_t I=0; I<PInfo.ItemsNumber; I++) {
		if (MultipleMasksApply(FarPanelFileName(PInfo.PanelItems[I]), FarPanelShortFileName(PInfo.PanelItems[I]))) {
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

bool ConfirmWholeFileRename(const TCHAR *From, const TCHAR *To)
{
	if (g_bInterrupted) return false;
	if (FileConfirmed) return true;

	const TCHAR *Lines[]={
		GetMsg(MMenuRename),GetMsg(MAskRename),From,GetMsg(MAskTo),To,
		GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(0,_T("FRAskRename"),Lines,9,4)) {
	case 1:FRConfirmFileThisRun=false;
	case 0:return (FileConfirmed=true);
	case -1:
	case 3:g_bInterrupted=true;
	}
	return false;
}

bool ConfirmRename(const TCHAR *From,const TCHAR *To)
{
	if (g_bInterrupted) return false;
	if (!FRConfirmLineThisFile) return true;
	if (_tcscmp(From, To) == 0) return true;

	const TCHAR *Lines[]={
		GetMsg(MMenuRename),GetMsg(MAskRename),From,GetMsg(MAskTo),To,
		GetMsg(MOk),GetMsg(MAll),GetMsg(MAllFiles),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(0,_T("FRAskRename"),Lines,10,5)) {
	case 2:FRConfirmLineThisRun=false;
	case 1:FRConfirmLineThisFile=false;
	case 0:return true;
	case -1:
	case 3:return false;
	}
	g_bInterrupted=true;
	return false;
}

bool FindRename(const TCHAR *FileName, int &MatchStart, int &MatchLength)
{
	REParam.Clear();
	REParam.AddSource(FileName, _tcslen(FileName));

	if (FSearchAs==SA_REGEXP) {
		REParam.AddRE(FPattern);

		if (do_pcre_exec(FPattern,FPatternExtra,FileName,_tcslen(FileName),MatchStart,0,REParam.Match(),REParam.Count())>=0) {
			MatchDone();
			REParam.FillStartLength(&MatchStart, &MatchLength);
			return true;
		}
	} else {
		int NewMatchStart;
		TCHAR *Table = FCaseSensitive ? NULL : UpCaseTable;
		NewMatchStart=BMHSearch(FileName+MatchStart,_tcslen(FileName)-MatchStart,FTextUpcase.data(),FTextUpcase.size(),Table);

		if (NewMatchStart>=0) {
			MatchStart+=NewMatchStart;
			MatchLength=FText.size();
			return true;
		}
	}

	return false;
}

bool WaitingForRename(const tstring &name)
{
	for (size_t nItem = 0; nItem < m_arrPendingRename.size(); nItem++)
		if (m_arrPendingRename[nItem].first == name)
			return true;

	return false;
}

void SortPendingRename()
{
	rename_vector PendingRename;

	while (true) {
		bool any = false;
		for (size_t nItem = 0; nItem < m_arrPendingRename.size(); )
		{
			if (!WaitingForRename(m_arrPendingRename[nItem].second)) {
				PendingRename.push_back(m_arrPendingRename[nItem]);
				m_arrPendingRename.erase(m_arrPendingRename.begin() + nItem);
				any = true;
			}
			else
				nItem++;
		}
		if (!any) break;
	}

	m_arrPendingRename.insert(m_arrPendingRename.begin(), PendingRename.begin(), PendingRename.end());
}

void PostPerformRename(rename_pair &Item)
{
	for (size_t nItem = 0; nItem < m_arrPendingRename.size(); nItem++) {
		rename_pair &Pair = m_arrPendingRename[nItem];

		//	Not ">=" since we don't want to replace Item itself if it is still in there
		if ((Pair.first.length() > Item.first.length()) &&
			(Pair.first.substr(0, Item.first.length()+1) == Item.first + _T("\\")))
		{
			Pair.first = Item.second + Pair.first.substr(Item.first.length());
		}
		if ((Pair.second.length() > Item.first.length()) &&
			(Pair.second.substr(0, Item.first.length()+1) == Item.first + _T("\\")))
		{
			Pair.second = Item.second + Pair.second.substr(Item.first.length());
		}
	}
}

bool PerformSingleRename(rename_pair &Item)
{
	bool bOverwrite = !FTAskOverwrite;
	do {
		DWORD dwFlags = MOVEFILE_COPY_ALLOWED;
		if (bOverwrite) dwFlags |= MOVEFILE_REPLACE_EXISTING;

		if (MoveFileEx(FullExtendedFileName(Item.first).c_str(), FullExtendedFileName(Item.second).c_str(), dwFlags)) {
			PostPerformRename(Item);
			return true;
		}

		DWORD dwError = GetLastError();

		switch (dwError) {
		case ERROR_ALREADY_EXISTS:
			if (FTAskOverwrite) {
				const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MFile),Item.second.c_str(),
					GetMsg(MAskOverwrite),Item.first.c_str(),GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)};
				switch (StartupInfo.Message(FMSG_WARNING,_T("FRenameOverwrite"),Lines,9,4)) {
				case 1:
					FTAskOverwrite = false;
				case 0:
					bOverwrite = true;
					continue;
				case 3:
					g_bInterrupted = true;
				case -1:
				case 2:
					return false;
				}
			}
			break;

		case ERROR_PATH_NOT_FOUND:
			if (FTAskCreatePath) {
				const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MFile),Item.second.c_str(),
					GetMsg(MAskCreatePath),GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)};
				switch (StartupInfo.Message(FMSG_WARNING,_T("FRenameCreatePath"),Lines,8,4)) {
				case 1:
					FTAskCreatePath=false;
				case 0:
					break;
				case 3:
					g_bInterrupted=true;
				case -1:
				case 2:
					return false;
				}
			}

			if (!CreateDirectoriesForFile(Item.second.c_str())) {
				tstring strPath = GetFullFileName(GetPath(Item.second));
				const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MPathCreateError),strPath.c_str(),
					GetMsg(MSkip),GetMsg(MCancel)};
				switch (StartupInfo.Message(FMSG_WARNING|FMSG_ERRORTYPE,_T("FRenameCreatePathError"),Lines,5,2)) {
				case 1:
					g_bInterrupted=true;
				case -1:
				case 0:
					return false;
				}
			}

			continue;
		}

		const TCHAR *Lines[]={GetMsg(MMenuRename),GetMsg(MRenameError),Item.first.c_str(),
			GetMsg(MAskTo),Item.second.c_str(),GetMsg(MBtnRetry),GetMsg(MSkip),GetMsg(MCancel)};
		switch (StartupInfo.Message(FMSG_WARNING|FMSG_ERRORTYPE, _T("FRenameError"), Lines, 8, 3)) {
		case 0:
			continue;
		case 2:
			g_bInterrupted=true;
		case -1:
		case 1:
			return false;
		}

	} while (true);

	return false;
}

bool PerformSingleRename(rename_pair &Item, panelitem_vector &PanelItems)
{
	if (PerformSingleRename(Item)) {
		m_arrLastRename.push_back(Item);
		AddFile(Item.second.c_str(), PanelItems);
		return true;
	} else
		return false;
}

void PerformFinalRename(panelitem_vector &PanelItems)
{
	SortPendingRename();

	for (size_t nItem = 0; nItem < m_arrPendingRename.size(); nItem++) {
		PerformSingleRename(m_arrPendingRename[nItem], PanelItems);
		if (Interrupted()) break;
	}
}

void GenerateRenameStrings(vector<tstring> &arrFrom, vector<tstring> &arrTo, int nMaxX)
{
	arrFrom.clear();
	arrTo.clear();

	for each (const rename_pair &Item in m_arrPendingRename) {
		QuoteString(Item.first. c_str(), Item.first. size(), arrFrom, nMaxX);
		QuoteString(Item.second.c_str(), Item.second.size(), arrTo,   nMaxX);
	}

	MakeSameWidth(arrFrom);
	MakeSameWidth(arrTo  );
}

void RenamePreview(panelitem_vector &PanelItems)
{
	vector<tstring> arrItems;
	int nBreakKey, nResult = 0;
	int BreakKeys[] = {VK_INSERT, VK_DELETE, VK_F2, VK_F3, VK_F10, 0};

	if (m_arrPendingRename.empty()) return;

	bool bWrap = true;
	RefreshConsoleInfo();

	do {
		vector<tstring> arrFrom;
		vector<tstring> arrTo;

		size_t nMaxX = bWrap ? ConInfo.dwSize.X/2-6 : 65535;
		GenerateRenameStrings(arrFrom, arrTo, nMaxX);

		arrItems.clear();
		for (size_t nItem = 0; nItem < arrFrom.size(); nItem++) {
			arrItems.push_back(arrFrom[nItem] + _T(" => ") + arrTo[nItem]);
		}

		nResult = ChooseMenu(arrItems, GetMsg(MRenamePreview), _T("Ins/Enter, Del, F2, F3, F10, Esc"), _T("RenamePreview"), nResult, FMENU_WRAPMODE,
			BreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			if (nResult >= 0) {
				if (PerformSingleRename(m_arrPendingRename[nResult], PanelItems)) {
					m_arrPendingRename.erase(m_arrPendingRename.begin() + nResult);
				}
			} else {
				g_bInterrupted = true;
				return;
			}
			break;
		case 0:
			if (PerformSingleRename(m_arrPendingRename[nResult], PanelItems)) {
				m_arrPendingRename.erase(m_arrPendingRename.begin() + nResult);
			}
			break;
		case 1:
			m_arrPendingRename.erase(m_arrPendingRename.begin() + nResult);
			break;
		case 2:
			bWrap = !bWrap;
			break;
		case 3:{
			tstring strContent;
			GenerateRenameStrings(arrFrom, arrTo, 65535);
			for (size_t nItem = 0; nItem < arrFrom.size(); nItem++) {
				strContent += arrFrom[nItem] + _T(" => ") + arrTo[nItem] + _T("\n");
			}
			RunExternalViewer(strContent);
			break;
			   }
		case 4:
			PerformFinalRename(PanelItems);
			return;
		}

		if (m_arrPendingRename.empty()) {
			g_bInterrupted = true;
			return;
		}
	} while (true);
}

void RenameFile(const FIND_DATA *FindData, panelitem_vector &PanelItems)
{
	int MatchStart = 0, MatchLength;
	
	FindNumber = ReplaceNumber = 0;

	tstring strPath, strOriginalName, strCurrentName;

	const TCHAR *szSlash=_tcsrchr(FindData->cFileName,'\\');
	if (szSlash) {
		strPath = tstring(FindData->cFileName, szSlash - FindData->cFileName + 1);
		strOriginalName = szSlash+1;
	} else {
		strOriginalName = FindData->cFileName;
	}
	strCurrentName = strOriginalName;

	FRConfirmLineThisFile = FRConfirmLineThisRun;
	FileConfirmed = !FRConfirmFileThisRun;

	bool bOnlyGlobalConfirm = !FRConfirmLineThisFile;

	while (FindRename(strCurrentName.c_str(), MatchStart, MatchLength))
	{
		REParam.AddFNumbers(FilesScanned, FileNumber, FindNumber, ReplaceNumber);

		tstring strNewSubName = CSO::CreateReplaceString(FRReplace.c_str(), _T(""), ScriptEngine(FREvaluate), REParam);

		tstring strNewName = strCurrentName.substr(0, MatchStart) + strNewSubName + strCurrentName.substr(MatchStart + MatchLength);

		MatchStart += (strNewSubName.empty()) ? 1 : strNewSubName.length();

		if (!g_bSkipReplace)
		{
			bool bConfirm = true;

			if (!bOnlyGlobalConfirm) {
				if (!ConfirmFile(MMenuRename,strOriginalName.c_str())) break;

				bConfirm = ConfirmRename(strCurrentName.c_str(), strNewName.c_str());
			}

			if (bConfirm) {
				strCurrentName = strNewName;
				ReplaceNumber++;
			}
		}

		FindNumber++;

		if (!FRepeating) break;
	}

	if (strOriginalName == strCurrentName) return;
	if (bOnlyGlobalConfirm && !ConfirmWholeFileRename(strOriginalName.c_str(), strCurrentName.c_str())) return;

	strCurrentName = strPath + strCurrentName;

	rename_pair Item(FindData->cFileName, strCurrentName);
	m_arrPendingRename.push_back(Item);
}

bool RenameFilesPrompt()
{
	CFarDialog Dialog(76, 21, _T("FileRenameDlg"));
	Dialog.SetWindowProc(FileSearchDialogProc, 0);
	Dialog.SetUseID(true);

	Dialog.AddFrame(MRename);

	Dialog.Add(new CFarCheckBoxItem(25,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("Masks"), MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("SearchText"), SearchText));
	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,_T("ReplaceText"), ReplaceText));

	Dialog.Add(new CFarCheckBoxItem(25,4,0,MRegExp,(bool *)&FSearchAs));
	Dialog.Add(new CFarCheckBoxItem(52,4,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(25,6,0,MRepeating,&FRepeating));
	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));

	Dialog.Add(new CFarTextItem(5,9,0,MSearchIn));
	Dialog.Add(new CFarComboBoxItem(15,9,55,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));

	Dialog.Add(new CFarCheckBoxItem(5, 10, 0, MEvaluateAsScript, &FREvaluate));
	Dialog.Add(new CFarComboBoxItem(30, 10, 55, 0, new CFarListData(m_lstEngines, false), new CFarEngineStorage(EREvaluateScript)));
	Dialog.Add(new CFarButtonItem(60, 10, 0, false, MRunEditor));

	Dialog.Add(new CFarCheckBoxItem(56,11,0,MLeftBracket,&FAdvanced));
	Dialog.Add(new CFarButtonItem  (62,11,DIF_NOBRACKETS,0,MBtnAdvanced));
	Dialog.Add(new CFarButtonItem(58,12,0,0,MBtnREBuilder));

	Dialog.Add(new CFarCheckBoxItem(5,12,0,MViewModified, &FROpenModified));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MConfirmFile,  &FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(5,14,0,MConfirmLine,  &FRConfirmLine));
	Dialog.Add(new CFarCheckBoxItem(5,15,0,MPreviewRename,&FRPreviewRename));
	Dialog.AddButtons(MOk,MCancel,MBtnClose);
	Dialog.Add(new CFarButtonItem(60,17,0,0,MBtnPresets));
	Dialog.SetFocus(MMask, 1);

	if (FSearchAs>=SA_SEVERALLINE) FSearchAs=SA_PLAINTEXT;
	if (!g_bFromCmdLine) FCaseSensitive=MaskCaseHere();
	FACaseSensitive=FCaseSensitive;

	MaskText=FMask;
	SearchText=FText;
	ReplaceText=FRReplace;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display()) {
		case MOk:
		case MBtnClose:
			FMask=MaskText;
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case MBtnPresets:
			RnPresets->ShowMenu(true);
			break;
		case MBtnREBuilder:
			if (RunREBuilder(SearchText, ReplaceText)) {
				FSearchAs = SA_REGEXP;
			}
			break;
		case MRunEditor:
			RunExternalEditor(ReplaceText);
			break;
		case MBtnAdvanced:
			if (AdvancedSettings()) FAdvanced=true;
			break;
		default:
			return false;
		}
	} while (!IsOKClose(ExitCode) || !FPreparePattern(false) || !CompileLUAString(ReplaceText, ScriptEngine(FREvaluate)));

	return (ExitCode == MOk);
}

OperationResult RenameFiles(panelitem_vector &PanelItems, bool ShowDialog)
{
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.Plugin||(PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	if (ShowDialog) {
		if (!RenameFilesPrompt()) return OR_CANCEL;
	} else {
		if (!FPreparePattern(false)) return OR_CANCEL;
	}

	FRConfirmFileThisRun = FRConfirmFile;
	FRConfirmLineThisRun = FRConfirmLine;
	FTAskOverwrite = FTAskCreatePath = true;
	g_bInterrupted = false;

	m_arrPendingRename.clear();
	m_arrLastRename.clear();

	CDebugTimer tm(_T("FileRename() took %d ms"));
	bool bResult = ScanDirectories(PanelItems, RenameFile);
	tm.Stop();

	if (bResult) {
		if (FRPreviewRename)
			RenamePreview(PanelItems);
		else
			PerformFinalRename(PanelItems);

		if (!FROpenModified)
			return OR_OK;
		else
			return (PanelItems.empty()) ? NoFilesFound() : OR_PANEL;
	} else return OR_FAILED;
}

OperationResult RenameFilesExecutor()
{
	FMask=MaskText;
	FText=SearchText;
	FRReplace=ReplaceText;
	
	if (!FPreparePattern(false)) return OR_FAILED;
	if (!CompileLUAString(FRReplace, ScriptEngine(FREvaluate))) return OR_FAILED;

	FTAskOverwrite = FTAskCreatePath = true;
	g_bInterrupted = false;

	m_arrPendingRename.clear();
	m_arrLastRename.clear();

	FRConfirmFileThisRun = false;	//FRConfirmFile;
	FRConfirmLineThisRun = false;	//FRConfirmLine;
	FRPreviewRename = false;
	SanitateEngine();

	bool bResult = ScanDirectories(g_PanelItems, RenameFile);

	if (bResult) {
		PerformFinalRename(g_PanelItems);
		return OR_OK;
	} else return OR_FAILED;
}

void CallRenameFile(const FIND_DATA &FindData, panelitem_vector &PanelItems)
{
	int PrevFindNumber = FindNumber;

	RenameFile(&FindData, PanelItems);

	FilesScanned++;
	if (FindNumber > PrevFindNumber) FileNumber++;
}

bool PerformRenameSelectedFiles(CPanelInfo &PInfo, panelitem_vector &PanelItems)
{
	FilesScanned = FileNumber = 0;
	FindNumber = ReplaceNumber = 0;

	g_bInterrupted = false;

	m_arrPendingRename.clear();
	m_arrLastRename.clear();

	//	To allow 'Restore selection' of _unmodified_ files
	vector<tstring> arrOrigNames;

	bool bRestoreSelection = FRLeaveSelection;
	if ((PInfo.SelectedItemsNumber==0) && (PInfo.ItemsNumber>0) &&
		(_tcscmp(FarPanelFileName(PInfo.PanelItems[PInfo.CurrentItem]),_T(".."))==0)) {

		bRestoreSelection = false;
		for (size_t I=0; I<PInfo.ItemsNumber; I++) {
			if (I==PInfo.CurrentItem) continue;
			if (g_bInterrupted) break;

			LPCTSTR szFileName = FarPanelFileName(PInfo.PanelItems[I]);
			FileFillNamedParameters(CatFile(PInfo.CurDir, szFileName).c_str());
			arrOrigNames.push_back(szFileName);

			CallRenameFile(PanelToFD(PInfo.PanelItems[I]), PanelItems);
		}
	} else {
		if ((PInfo.SelectedItemsNumber == 1) && ((PInfo.SelectedItems[0].Flags & PPIF_SELECTED) == 0))
			bRestoreSelection = false;
		for (size_t I=0; I<PInfo.SelectedItemsNumber ;I++) {
			if (g_bInterrupted) break;

			LPCTSTR szFileName = FarPanelFileName(PInfo.SelectedItems[I]);
			FileFillNamedParameters(CatFile(PInfo.CurDir, szFileName).c_str());
			arrOrigNames.push_back(szFileName);

			CallRenameFile(PanelToFD(PInfo.SelectedItems[I]), PanelItems);
		}
	}

	if (FRPreviewRename)
		RenamePreview(PanelItems);
	else
		PerformFinalRename(PanelItems);

#ifdef UNICODE
	StartupInfo.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
#else
	StartupInfo.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, NULL);
#endif
	if (bRestoreSelection) {
		CPanelInfo PNewInfo;
		PNewInfo.GetInfo(false);

		for (size_t nFile = 0; nFile < PanelItems.size(); nFile++) {
			int nItem = PNewInfo.Find(FarPanelFileName(PanelItems[nFile]));
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

	return true;
}

bool RenameSelectedFilesPrompt()
{
	CFarDialog Dialog(76, 15, _T("SelectedFileRenameDlg"));
	Dialog.SetWindowProc(FileSearchDialogProc, 0);

	Dialog.AddFrame(MRenameSelected);
	Dialog.SetUseID(true);

	Dialog.Add(new CFarCheckBoxItem(25,2,0,MRegExp,(bool *)&FSearchAs));
	Dialog.Add(new CFarCheckBoxItem(50,2,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarTextItem(5,2,0,MText));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("SearchText"), SearchText));

	Dialog.Add(new CFarTextItem(5,4,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("ReplaceText"), ReplaceText));
	Dialog.Add(new CFarCheckBoxItem(25,4,0,MRepeating,&FRepeating));

	Dialog.Add(new CFarButtonItem(58,7,0,0,MBtnREBuilder));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MConfirmLine,&FRConfirmLine));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MLeaveSelection,&FRLeaveSelection));
	Dialog.Add(new CFarCheckBoxItem(5,9,0,MPreviewRename,&FRPreviewRename));
	Dialog.AddButtons(MOk,MCancel,MBtnClose);
	Dialog.Add(new CFarButtonItem(60,11,0,0,MBtnPresets));
	Dialog.SetFocus(MText, 1);

	if (FSearchAs>=SA_SEVERALLINE) FSearchAs=SA_PLAINTEXT;
	if (!g_bFromCmdLine) FCaseSensitive=MaskCaseHere();

	int ExitCode;
	SearchText=FText;
	ReplaceText=FRReplace;
	FREvaluate = false;
	do {
		switch (ExitCode=Dialog.Display()) {
		case MOk:
		case MBtnClose:
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case MBtnPresets:
			QRPresets->ShowMenu(true);
			break;
		case MBtnREBuilder:
			if (RunREBuilder(SearchText, ReplaceText)) {
				FSearchAs = SA_REGEXP;
			}
			break;
		default:
			return false;
		}
	} while (!IsOKClose(ExitCode) || !FPreparePattern(false));

	return (ExitCode == MOk);
}

OperationResult RenameSelectedFiles(panelitem_vector &PanelItems, bool ShowDialog)
{
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.Plugin||(PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	if (ShowDialog) {
		if (!RenameSelectedFilesPrompt()) return OR_CANCEL;
	} else {
		if (!FPreparePattern(false)) return OR_CANCEL;
	}

	PanelItems.clear();
	FRConfirmFileThisRun=false;
	FRConfirmLineThisRun=FRConfirmLine;
	FTAskOverwrite = FTAskCreatePath = true;

	PerformRenameSelectedFiles(PInfo, PanelItems);
	return (PanelItems.empty() && !g_bInterrupted) ? NoFilesFound() : OR_OK;
}

OperationResult QuickRenameFilesExecutor()
{
	FText=SearchText;
	FRReplace=ReplaceText;
	if (!FPreparePattern(false)) return OR_FAILED;
	FTAskOverwrite = FTAskCreatePath = true;

	FRConfirmFileThisRun = false;
	FRConfirmLineThisRun = false;
	FRPreviewRename = false;

	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.Plugin || (PInfo.PanelType!=PTYPE_FILEPANEL)) return OR_FAILED;

	return PerformRenameSelectedFiles(PInfo, g_PanelItems) ? OR_OK : OR_CANCEL;
}

//////////////////////////////////////////////////////////////////////////

void StripCommonPart(vector<tstring> &arrFileNames)
{
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

void ProcessNames(vector<tstring> &arrFileNames, vector<tstring> &arrProcessedNames)
{
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

bool PerformRenumberOnce(vector<tstring> &arrFileNames, vector<tstring> &arrProcessedNames, LPCTSTR szCurDir)
{
	bool any = false;

	for (size_t nItem = 0; nItem < arrFileNames.size(); ) {
		if (arrFileNames[nItem].empty() || (arrFileNames[nItem] == arrProcessedNames[nItem])) {
			arrFileNames.erase(arrFileNames.begin() + nItem);
			arrProcessedNames.erase(arrProcessedNames.begin() + nItem);
			any = true;
			continue;
		}

		tstring strSrc = szCurDir + arrFileNames[nItem];
		tstring strTgt = szCurDir + arrProcessedNames[nItem];

		if (MoveFile(ExtendedFileName(strSrc).c_str(), ExtendedFileName(strTgt).c_str())) {
			m_arrLastRename.push_back(make_pair(strSrc, strTgt));
			arrFileNames.erase(arrFileNames.begin() + nItem);
			arrProcessedNames.erase(arrProcessedNames.begin() + nItem);
			any = true;
			continue;
		}

		nItem++;
	}

	return any;
}

void PerformRenumber(const vector<tstring> &arrFileNames, const vector<tstring> &arrProcessedNames, LPCTSTR szCurDir)
{
	m_arrLastRename.clear();

	vector<tstring> arrFN = arrFileNames;
	vector<tstring> arrPN = arrProcessedNames;

	while (PerformRenumberOnce(arrFN, arrPN, szCurDir)) {}

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

OperationResult RenumberFiles()
{
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.PanelType != PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin && ((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;
	g_nStartWithNow = g_nStartWith;

	vector<tstring> arrFileNames;
	for (size_t nItem = 0; nItem < PInfo.SelectedItemsNumber; nItem++)
		arrFileNames.push_back(FarPanelFileName(PInfo.SelectedItems[nItem]));

	int BreakKeys[] = {
		VK_F2, VK_F4, VK_F7, VK_F8, VK_F10, VK_CTRL_UP, VK_CTRL_DOWN,
		VK_ADD, VK_CTRL_ADD, VK_SHIFT_ADD, VK_ALT_ADD,
		VK_SUBTRACT, VK_CTRL_SUBTRACT, VK_SHIFT_SUBTRACT, VK_ALT_SUBTRACT,
		VK_INSERT, VK_DELETE, 0
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
		nPosition = ChooseMenu(arrNames, GetMsg(MRenumber), _T("F2, F4, F7, F8, Ctrl-\x18\x19, F10=Go"), _T("Renumber"),
			nPosition, FMENU_WRAPMODE|FMENU_RETURNCODE, BreakKeys, &nBreakKey);
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
		case VK_F2:
			bOriginal = !bOriginal;
			break;
		case VK_F4:
			ConfigureRenumbering(true);
			WriteRegistry();
			g_nStartWithNow = g_nStartWith;
			break;
		case VK_F7:{
			nOK = 0;
			for (size_t nItem = 0; nItem < arrFileNames.size(); ) {
				if (arrFileNames[nItem].empty()) arrFileNames.erase(arrFileNames.begin()+nItem); else nItem++;
			}
			break;
			  }
		case VK_F8:
			g_bStripCommon = !g_bStripCommon;
			break;
		case VK_F10:
			PerformRenumber(arrFileNames, arrProcessedNames, AddSlash(tstring(PInfo.CurDir)).c_str());
			return OR_OK;
		case VK_CTRL_UP:
			if (nPosition > nOK) {
				tstring strPrev = arrFileNames[nPosition-1];
				arrFileNames[nPosition-1] = arrFileNames[nPosition];
				arrFileNames[nPosition] = strPrev;
				nPosition--;
			}
			break;
		case VK_CTRL_DOWN:
			if ((nPosition >= nOK) && (nPosition < (int)arrFileNames.size()-1)) {
				tstring strNext = arrFileNames[nPosition+1];
				arrFileNames[nPosition+1] = arrFileNames[nPosition];
				arrFileNames[nPosition] = strNext;
				nPosition++;
			}
			break;
		case VK_ADD:
			g_nStartWithNow++;
			break;
		case VK_CTRL_ADD:
			g_nStartWithNow+=10;
			break;
		case VK_SHIFT_ADD:
			g_nStartWithNow+=100;
			break;
		case VK_ALT_ADD:
			g_nStartWithNow+=1000;
			break;
		case VK_SUBTRACT:
			if (g_nStartWithNow > 0) g_nStartWithNow--;
			break;
		case VK_CTRL_SUBTRACT:
			if (g_nStartWithNow > 10) g_nStartWithNow -= 10; else g_nStartWithNow = 0;
			break;
		case VK_SHIFT_SUBTRACT:
			if (g_nStartWithNow > 100) g_nStartWithNow -= 100; else g_nStartWithNow = 0;
			break;
		case VK_ALT_SUBTRACT:
			if (g_nStartWithNow > 1000) g_nStartWithNow -= 1000; else g_nStartWithNow = 0;
			break;
		case VK_INSERT:
			arrFileNames.insert(arrFileNames.begin()+nOK, _T(""));
			nPosition = ++nOK;
			break;
		case VK_DELETE:
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

OperationResult UndoRenameFiles()
{
	FRConfirmFileThisRun = true;

	for (int nFile = m_arrLastRename.size()-1; nFile >= 0; nFile--) {
		FileConfirmed = !FRConfirmFileThisRun;

		if (ConfirmWholeFileRename(m_arrLastRename[nFile].second.c_str(), m_arrLastRename[nFile].first.c_str()))
			MoveFile(ExtendedFileName(m_arrLastRename[nFile].second).c_str(), ExtendedFileName(m_arrLastRename[nFile].first).c_str());

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

bool CRnPresetCollection::EditPreset(CPreset *pPreset)
{
	CFarDialog Dialog(76,20,_T("RPresetDlg"));
	Dialog.AddFrame(MRnPreset);
	Dialog.SetUseID(true);

	Dialog.Add(new CFarCheckBoxItem(25,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));

	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"), pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("Masks"), pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MText));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,_T("SearchText"), pPreset->m_mapStrings["Text"]));
	Dialog.Add(new CFarTextItem(5,8,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,9,70,DIF_HISTORY,_T("ReplaceText"), pPreset->m_mapStrings["Replace"]));

	Dialog.Add(new CFarCheckBoxItem(25,6,0,MRegExp,&pPreset->m_mapInts["TextAsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(52,6,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(25,8,0,MRepeating,&pPreset->m_mapInts["Repeating"]));

	Dialog.Add(new CFarTextItem(5,11,0,MSearchIn));
	Dialog.Add(new CFarComboBoxItem(11,11,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),&pPreset->m_mapInts["SearchIn"]));

	Dialog.Add(new CFarCheckBoxItem(5, 12, 0, MEvaluateAsScript, &pPreset->m_mapInts["AsScript"]));
	Dialog.Add(new CFarComboBoxItem(30, 12, 55, 0, new CFarListData(m_lstEngines, false), new CFarEngineStorage(pPreset->m_mapStrings["Script"])));
	Dialog.Add(new CFarButtonItem(60, 12, 0, false, MRunEditor));

	int  nAdvancedID = pPreset->m_mapInts["AdvancedID"];
	bool bFAdvanced = nAdvancedID > 0;

	Dialog.Add(new CFarCheckBoxItem(56,13,0,MLeftBracket,&bFAdvanced));
	Dialog.Add(new CFarButtonItem  (62,13,DIF_NOBRACKETS,0,MBtnAdvanced));

	Dialog.Add(new CFarCheckBoxItem(5,14,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk, MCancel);
	Dialog.SetFocus(MPresetName, 1);

	do {
		switch (Dialog.Display()) {
		case MOk:
			pPreset->m_mapInts["AdvancedID"] = bFAdvanced ? nAdvancedID : 0;
			return true;
		case MRunEditor:
			RunExternalEditor(pPreset->m_mapStrings["Replace"]);
			break;
		case MBtnAdvanced:
			SelectAdvancedPreset(nAdvancedID, bFAdvanced);
			break;
		default:
			return false;
		}
	} while (true);
}

bool CQRPresetCollection::EditPreset(CPreset *pPreset)
{
	CFarDialog Dialog(76,15,_T("QRPresetDlg"));
	Dialog.AddFrame(MQRPreset);

	Dialog.Add(new CFarCheckBoxItem(25,4,0,MRegExp,&pPreset->m_mapInts["TextAsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(52,4,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));

	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"), pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,_T("ReplaceText"), pPreset->m_mapStrings["Replace"]));
	Dialog.Add(new CFarCheckBoxItem(25,6,0,MRepeating,&pPreset->m_mapInts["Repeating"]));

	Dialog.Add(new CFarCheckBoxItem(5,9,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.SetFocus(3);

	return Dialog.Display(-1)==0;
}
