#include "StdAfx.h"
#include "..\RESearch.h"

tstring g_strNewFileName;
tstring g_strBackupFileName;

LONG_PTR WINAPI ConfirmReplacementDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2)
{
	switch (nMsg) {
	case DN_INITDIALOG:
		pDlg->SendDlgMessage(DM_SETDLGDATA, 0, lParam2);
		break;
#ifdef FAR3
	case DN_CTLCOLORDLGITEM:{
		int nCounts = pDlg->SendDlgMessage(DM_GETDLGDATA, 0, 0);
		int nFoundCount = nCounts >> 16;
		int nReplacedCount = nCounts & 0xFFFF;
		FarDialogItemColors *pColors = (FarDialogItemColors *)lParam2;
		if ((nParam1 >= 2) && (nParam1 < 2+nFoundCount)) {
			pColors->Colors[0].Flags = FCF_4BITMASK;
			pColors->Colors[0].BackgroundColor = 0x0A;
			pColors->Colors[0].ForegroundColor = 0x00;
			return true;
		}
		if ((nParam1 >= 3+nFoundCount) && (nParam1 < 3+nFoundCount+nReplacedCount)) {
			pColors->Colors[0].Flags = FCF_4BITMASK;
			pColors->Colors[0].BackgroundColor = 0x0B;
			pColors->Colors[0].ForegroundColor = 0x00;
			return true;
		}
		if (nParam1 == 4+nFoundCount+nReplacedCount) {
			pColors->Colors[0].Flags = FCF_4BITMASK;
			pColors->Colors[0].BackgroundColor = 0x02;
			pColors->Colors[0].ForegroundColor = 0x00;
			return true;
		}
		break;
							}
#endif
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

bool ConfirmReplacement()
{
	return !g_bSkipReplace && !FRConfirmLineThisFile;
}

bool ConfirmReplacement(const TCHAR *Found, const TCHAR *Replaced, const TCHAR *FileName)
{
	if (g_bInterrupted) return false;
	if (g_bSkipReplace) return false;
	if (!FRConfirmLineThisFile) return true;

	if (g_bIgnoreIdentReplace) {
		if (_tcscmp(Found, Replaced) == 0) return false;
	}

	RefreshConsoleInfo();

	vector<tstring> arrFound;
	QuoteStrings(Found,    arrFound,    ConInfo.dwSize.X - 12, ConInfo.dwSize.Y/2-7);
	size_t nFoundLen   = MakeSameWidth(arrFound);

	vector<tstring> arrReplaced;
	QuoteStrings(Replaced, arrReplaced, ConInfo.dwSize.X - 12, ConInfo.dwSize.Y/2-7);
	size_t nReplaceLen = MakeSameWidth(arrReplaced);

	size_t nWidth = max(nFoundLen, nReplaceLen);
	if (nWidth < 60) nWidth = 60;

	tstring strFileName = QuoteString(FileName, -1, nWidth);

	size_t nCount = arrFound.size()+arrReplaced.size();

	CFarDialog Dialog(nWidth+12, nCount+10, _T("FRAskReplace"));
	Dialog.SetWindowProc(ConfirmReplacementDialogProc, (arrFound.size() << 16) + arrReplaced.size());
	Dialog.AddFrame(MREReplace);

	Dialog.Add(new CFarTextItem(-1, 2, DIF_NOAUTOHOTKEY, MAskReplace));
	for (size_t I = 0; I<arrFound.size(); I++)
		Dialog.Add(new CFarTextItem(-1, 3 + I, DIF_SHOWAMPERSAND|DLG_SETCOLOR(0xA0), arrFound[I]));

	Dialog.Add(new CFarTextItem(-1, 3 + arrFound.size(), DIF_NOAUTOHOTKEY, MAskWith));
	for (size_t I = 0; I<arrReplaced.size();I++)
		Dialog.Add(new CFarTextItem(-1, 4 + arrFound.size() + I, DIF_SHOWAMPERSAND|DLG_SETCOLOR(0xB0), arrReplaced[I]));

	Dialog.Add(new CFarTextItem(-1, 4 + nCount, DIF_NOAUTOHOTKEY, MInFile));
	Dialog.Add(new CFarTextItem(-1, 5 + nCount,  DIF_SHOWAMPERSAND|DLG_SETCOLOR(0x20), strFileName.c_str()));

	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, true,  MReplace));
	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, false, MAll));
	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, false, MAllFiles));
	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, false, MSkip));
	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, false, MSkipFile));
	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, false, MCancel));
	int Result = Dialog.Display(5, -6, -5, -4, -3, -2);

	switch (Result) {
	case 2:
		FRConfirmLineThisRun=false;
	case 1:
		FRConfirmLineThisFile=false;
	case 0:
		return true;
	case 3:
		break;
	case 4:
		FRSkipThisFile = true;
	case -1:
		g_bInterrupted = true;
		break;
	}
	return false;
}

bool RunReplace(LPCTSTR szFileName, __int64 dwFileSize)
{
	switch (FSearchAs) {
	case SA_PLAINTEXT:{
		CReplacePlainTextFrontend Frontend;
		return RunReplace(szFileName, g_strNewFileName.c_str(), &Frontend);
					  }
	case SA_REGEXP:{
		CReplaceRegExpFrontend Frontend;
		return RunReplace(szFileName, g_strNewFileName.c_str(), &Frontend);
				   }
	case SA_SEVERALLINE:{
		CReplaceSeveralLineRegExpFrontend Frontend;
		return RunReplace(szFileName, g_strNewFileName.c_str(), &Frontend);
						}
	case SA_MULTILINE:
#ifdef _WIN64
		if (dwFileSize < 1i64*1024*1024*1024*1024)
#else
		if (dwFileSize < 256*1024*1024)
#endif
		{
			CReplaceMultiLineRegExpFrontend Frontend;
			return RunReplace(szFileName, g_strNewFileName.c_str(), &Frontend);
		}
		else
		{
			CReplaceSeveralLineRegExpFrontend Frontend;
			return RunReplace(szFileName, g_strNewFileName.c_str(), &Frontend);
		}
	}

	return false;
}

tstring GetUniqueFileName(const TCHAR *szCurrent, const TCHAR *szExt)
{
	int nTry = 0;
	do {
		tstring strName = szCurrent;
		if (nTry > 0) strName += FormatStr(_T(".%d"), nTry);
		strName += szExt;
		if (GetFileAttributes(ExtendedFileName(strName).c_str()) == INVALID_FILE_ATTRIBUTES) {
			if (GetLastError() == ERROR_FILE_NOT_FOUND) return strName;
			return tstring();
		}
		nTry++;
	} while (true);
}

class ROBackup
{
public:
	ROBackup(LPCTSTR szFileName)
	{
		m_dwAttr = GetFileAttributes(ExtendedFileName(szFileName).c_str());
		if ((m_dwAttr != INVALID_FILE_ATTRIBUTES) && (m_dwAttr & FILE_ATTRIBUTE_READONLY)) {
			SetFileAttributes(ExtendedFileName(szFileName).c_str(), m_dwAttr & ~FILE_ATTRIBUTE_READONLY);
			m_strFileName = szFileName;
		}
	}
	void Done()
	{
		if (!m_strFileName.empty()) {
			SetFileAttributes(ExtendedFileName(m_strFileName).c_str(), m_dwAttr);
			m_strFileName.clear();
		}
	}
	~ROBackup()
	{
		Done();
	}
protected:
	tstring m_strFileName;
	DWORD m_dwAttr;
};

bool ReplaceSingleFile_Normal(const FIND_DATA &FindData)
{
	bool bProcess = RunReplace(FindData.cFileName, FindData.nFileSize);

	if (bProcess) {
		if (!FRReplaceToNew) {
			ROBackup _ro(FindData.cFileName);
			while (!ReplaceFile(ExtendedFileName(FindData.strFileName).c_str(), ExtendedFileName(g_strNewFileName).c_str(),
				(FRSaveOriginal) ? ExtendedFileName(g_strBackupFileName).c_str() : NULL,
				REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL))
			{
				const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileBackupError),FindData.cFileName,GetMsg(MOk),GetMsg(MBtnRetry)};
				if (StartupInfo.Message(FMSG_WARNING|FMSG_ERRORTYPE,_T("FRBackupError"),Lines,5,2) == 0) break;
			}
		}
	} else {
		DeleteFile(ExtendedFileName(g_strNewFileName).c_str());
	}

	return bProcess;
}

bool CopyFileBack(LPCTSTR szFrom, LPCTSTR szTo)
{
	CFileMapping mapFrom;
	if (!mapFrom.Open(szFrom)) return false;

	CFileMapping mapTo;
	if (!mapTo.Open(szTo, true, mapFrom.Size())) return false;

	CopyMemory(mapTo, mapFrom, mapFrom.Size());

	return true;
}

//	Using slow but reliable mechanism for files with hardlinks
bool ReplaceSingleFile_CopyFirst(const FIND_DATA &FindData)
{
	if (FRReplaceToNew || !CopyFile(ExtendedFileName(FindData.cFileName).c_str(), ExtendedFileName(g_strBackupFileName).c_str(), false)) {
		return ReplaceSingleFile_Normal(FindData);
	}

	ROBackup _ro(FindData.cFileName);
	g_strNewFileName = FindData.cFileName;
	bool bProcess = RunReplace(g_strBackupFileName.c_str(), FindData.nFileSize);

	if (bProcess) {
		if (!FRSaveOriginal) {
			ROBackup _ro2(g_strBackupFileName.c_str());
			DeleteFile(ExtendedFileName(g_strBackupFileName).c_str());
		}
	} else {
		CopyFileBack(g_strBackupFileName.c_str(), FindData.cFileName);
		DeleteFile(ExtendedFileName(g_strBackupFileName).c_str());
	}

	return bProcess;
}

void ReplaceSingleFile(const FIND_DATA *FindData, panelitem_vector &PanelItems)
{
	FRConfirmLineThisFile = FRConfirmLineThisRun;
	FRSkipThisFile = false;
	FileConfirmed = !FRConfirmFileThisRun;
#ifndef UNICODE
	m_pReplaceTable = NULL;
#endif

	InitFoundPosition();

	if (FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return;

	if (FindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
		if (FRReplaceReadonly == RR_NEVER) return;
	}

	g_strBackupFileName = (FROverwriteBackup)
		? tstring(FindData->cFileName) + _T(".bak")
		: GetUniqueFileName(FindData->cFileName, _T(".bak"));
	g_strNewFileName = GetUniqueFileName(FindData->cFileName, _T(".new"));

	HANDLE hFile = CreateFile(ExtendedFileName(FindData->cFileName).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION hfInfo;
		GetFileInformationByHandle(hFile, &hfInfo);
		CloseHandle(hFile);

		bool bProcess;
		if (hfInfo.nNumberOfLinks > 1) {
			bProcess = ReplaceSingleFile_CopyFirst(*FindData);
		} else {
			bProcess = ReplaceSingleFile_Normal(*FindData);
		}

		if (bProcess) {
			AddFile(FindData->GetFileName(), PanelItems);
		} else {
			if (FRSkipThisFile)
				g_bInterrupted = false;
		}
	} else {
		const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),FindData->cFileName,GetMsg(MOk)};
		StartupInfo.Message(FMSG_WARNING,_T("FSOpenError"),Lines,4,1);
	}
}

bool PrepareFileReplacePattern()
{
	if (!FPreparePattern(false)) return false;

	if (FAdvanced) {
		FAUseStreams = false;
		if (!CompileAdvancedSettings()) return false;
	}

	if (!CompileLUAString(FRReplace, ScriptEngine(FREvaluate))) return OR_FAILED;

	return true;
}

//////////////////////////////////////////////////////////////////////////

void UpdateFRDialog(CFarDialog *pDlg)
{
	bool bAsScript = pDlg->IsDlgItemChecked(MEvaluateAsScript);
	pDlg->EnableDlgItem(MEvaluateAsScript, bAsScript, 1);

	bool bReplaceToNew = pDlg->IsDlgItemChecked(MReplaceToNew);
	pDlg->EnableCheckDlgItem(MSaveOriginal, !bReplaceToNew);

	bool bSaveOriginal = pDlg->IsDlgItemChecked(MSaveOriginal);
	pDlg->EnableCheckDlgItem(MOverwriteBackup, bSaveOriginal);
}

LONG_PTR WINAPI FileReplaceDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2)
{
	ReplaceEOLDialogProc(nMsg, lParam2);

	int nCtlID = pDlg->GetID(nParam1);

	switch (nMsg) {
	case DN_INITDIALOG:
		UpdateFRDialog(pDlg);
		break;

	case DN_BTNCLICK:
		switch (nCtlID)
		{
		case MQuoteSearch:
			QuoteRegExpString(pDlg, MSearchFor);
			return TRUE;
		case MQuoteReplace:
			QuoteReplaceString(pDlg, MReplaceWith);
			return TRUE;
		}

		UpdateFRDialog(pDlg);
		break;
	}

	return FileSearchDialogProc(pDlg, nMsg, nParam1, lParam2);
}

bool ReplacePrompt(bool Plugin)
{
	CFarDialog Dialog(76, 28, _T("FileReplaceDlg"));
	Dialog.SetWindowProc(FileReplaceDialogProc, 0);
	Dialog.SetUseID(true);

	Dialog.AddFrame(MREReplace);

	Dialog.Add(new CFarCheckBoxItem(35,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("Masks"), MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), SearchText));

	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,7,65,DIF_HISTORY|DIF_VAREDIT,_T("ReplaceText"), ReplaceText));

	Dialog.Add(new CFarButtonItem(67,5,0,0,MQuoteSearch));
	Dialog.Add(new CFarButtonItem(67,7,0,0,MQuoteReplace));

	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR, _T("")));
	Dialog.Add(new CFarRadioButtonItem(5,9,DIF_GROUP,MPlainText, (int *)&FSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,MRegExp,			 (int *)&FSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MSeveralLineRegExp,(int *)&FSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,MMultiLineRegExp,	 (int *)&FSearchAs,SA_MULTILINE));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,14,0,MAllCharTables,&FAllCharTables));

	Dialog.Add(new CFarCheckBoxItem( 5, 15, 0, MEvaluateAsScript, &FREvaluate));
	Dialog.Add(new CFarComboBoxItem(30, 15, 55, 0, new CFarListData(m_lstEngines, false), new CFarEngineStorage(EREvaluateScript)));
	Dialog.Add(new CFarButtonItem  (60, 15, 0, false, MRunEditor));

	Dialog.Add(new CFarTextItem(5,17,0,MSearchIn));
	if (Plugin) {
		if (FSearchIn<SI_FROMCURRENT) FSearchIn=SI_FROMCURRENT;
		Dialog.Add(new CFarComboBoxItem(15,17,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearchPlugin, false),(int *)&FSearchIn,NULL,3));
	} else {
		Dialog.Add(new CFarComboBoxItem(15,17,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));
	}

	Dialog.Add(new CFarCheckBoxItem(56,10,0,MLeftBracket,&FAdvanced));
	Dialog.Add(new CFarButtonItem  (62,10,DIF_NOBRACKETS,0,MBtnAdvanced));
	Dialog.Add(new CFarButtonItem(_tcslen(GetMsg(MSeveralLineRegExp))+10,11,0,0,MEllipsis));
	Dialog.Add(new CFarButtonItem(57, 12, 0, false, MBtnREBuilder));

	Dialog.Add(new CFarCheckBoxItem(5,19,0,MConfirmFile,&FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(5,20,0,MConfirmLine,&FRConfirmLine));
	Dialog.Add(new CFarCheckBoxItem(5,21,0,MViewModified,&FROpenModified));
	Dialog.Add(new CFarCheckBoxItem(5,22,0,MShowStatistics,&FShowStatistics));

	Dialog.Add(new CFarCheckBoxItem(40,19,0,MReplaceToNew,&FRReplaceToNew));
	Dialog.Add(new CFarCheckBoxItem(40,20,0,MSaveOriginal,&FRSaveOriginal));
	Dialog.Add(new CFarCheckBoxItem(42,21,0,MOverwriteBackup,&FROverwriteBackup));

	Dialog.AddButtons(MOk,MCancel,MBtnClose);
	Dialog.Add(new CFarButtonItem(60,24,0,0,MBtnPresets));

	Dialog.SetFocus(MMask, 1);
	if (FSearchAs>=SA_MULTITEXT) FSearchAs=SA_PLAINTEXT;
	FACaseSensitive=FADirectoryCaseSensitive=MaskCaseHere();

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
			FRPresets->ShowMenu(true);
			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
			break;
		case MBtnAdvanced:
			if (AdvancedSettings()) FAdvanced=true;
			break;
		case MBtnREBuilder:
			if (RunREBuilder(SearchText, ReplaceText)) {
				if (FSearchAs == SA_PLAINTEXT) FSearchAs = SA_REGEXP;
			}
			break;
		case MRunEditor:
			RunExternalEditor(ReplaceText);
			break;
		case MEllipsis:
			ConfigureSeveralLines();
			break;
		default:
			return false;
		}
	} while (!IsOKClose(ExitCode) || !PrepareFileReplacePattern());

	return (ExitCode == MOk);
}

OperationResult FileReplace(panelitem_vector &PanelItems, bool ShowDialog, bool bSilent)
{
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.PanelType!=PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin&&((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;

	if (ShowDialog) {
		if (!ReplacePrompt(PInfo.Plugin)) return OR_CANCEL;
	} else {
		if (!PrepareFileReplacePattern()) return OR_CANCEL;
	}

	FRConfirmFileThisRun = FRConfirmFile;
	FRConfirmReadonlyThisRun = (FRReplaceReadonly != RR_ALWAYS);
	FRConfirmLineThisRun = FRConfirmLine;

	CDebugTimer tm(_T("FileReplace() took %d ms"));
	bool bResult = ScanDirectories(PanelItems, ReplaceSingleFile);
	tm.Stop();

	if (!bResult) return OR_FAILED;

	if (FShowStatistics)
		ShowStatistics(true, PanelItems);

	if (!FROpenModified)
		return OR_OK;
	else
		return !PanelItems.empty() ? OR_PANEL :
			(bSilent || FShowStatistics) ? OR_OK :
			(FindNumber > 0) ? NoFilesModified() :  NoFilesFound();
}

OperationResult FileReplaceExecutor()
{
	FMask = MaskText;
	FText = SearchText;
	FRReplace = ReplaceText;
	FROpenModified = false;
	SanitateEngine();

	return FileReplace(g_PanelItems, false, true);
}

bool CFRPresetCollection::EditPreset(CPreset *pPreset)
{
	CFarDialog Dialog(76, 32, _T("FRPresetDlg"));
	Dialog.SetWindowProc(FileReplaceDialogProc, 0);
	Dialog.SetUseID(true);

	Dialog.AddFrame(MFRPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"), pPreset->Name()));

	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));
	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("Masks"), pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,8,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,9,70,DIF_HISTORY|DIF_VAREDIT,_T("ReplaceText"), pPreset->m_mapStrings["Replace"]));

	Dialog.Add(new CFarTextItem(5,10,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));

	int *pSearchAs = &pPreset->m_mapInts["SearchAs"];
	int  nAdvancedID = pPreset->m_mapInts["AdvancedID"];
	bool bFAdvanced = nAdvancedID > 0;

	Dialog.Add(new CFarRadioButtonItem(5,11,DIF_GROUP,MPlainText,pSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,MRegExp,           pSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,MSeveralLineRegExp,pSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarRadioButtonItem(5,14,0,MMultiLineRegExp,	 pSearchAs,SA_MULTILINE));

	Dialog.Add(new CFarCheckBoxItem(5,15,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(5,16,0,MAllCharTables,&pPreset->m_mapInts["AllCharTables"]));

	Dialog.Add(new CFarCheckBoxItem(5, 17, 0, MEvaluateAsScript, &pPreset->m_mapInts["AsScript"]));
	Dialog.Add(new CFarComboBoxItem(30, 17, 55, 0, new CFarListData(m_lstEngines, false), new CFarEngineStorage(pPreset->m_mapStrings["Script"])));
	Dialog.Add(new CFarButtonItem(60, 17, 0, FALSE, MRunEditor));

	Dialog.Add(new CFarTextItem(5,19,0,MSearchIn));
	Dialog.Add(new CFarComboBoxItem(15,19,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));

	Dialog.Add(new CFarCheckBoxItem(56,12,0,MLeftBracket,&bFAdvanced));
	Dialog.Add(new CFarButtonItem  (62,12,DIF_NOBRACKETS,0,MBtnAdvanced));
	Dialog.Add(new CFarButtonItem(_tcslen(GetMsg(MSeveralLineRegExp))+10,13,0,0,MEllipsis));
	Dialog.Add(new CFarButtonItem(57, 14, 0, false, MBtnREBuilder));

	Dialog.Add(new CFarCheckBoxItem(5,21,0,MConfirmFile,&pPreset->m_mapInts["ConfirmFile"]));
	Dialog.Add(new CFarCheckBoxItem(5,22,0,MConfirmLine,&pPreset->m_mapInts["ConfirmLine"]));
	Dialog.Add(new CFarCheckBoxItem(5,23,0,MViewModified,&pPreset->m_mapInts["OpenModified"]));
	Dialog.Add(new CFarCheckBoxItem(5,24,0,MShowStatistics,&pPreset->m_mapInts["ShowStatistics"]));

	Dialog.Add(new CFarCheckBoxItem(40,21,0,MReplaceToNew,&pPreset->m_mapInts["ReplaceToNew"]));
	Dialog.Add(new CFarCheckBoxItem(40,22,0,MSaveOriginal,&pPreset->m_mapInts["SaveOriginal"]));
	Dialog.Add(new CFarCheckBoxItem(42,23,0,MOverwriteBackup,&pPreset->m_mapInts["OverwriteBackup"]));

	Dialog.Add(new CFarCheckBoxItem(5,26,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display()) {
		case MOk:
			pPreset->m_mapInts["AdvancedID"] = bFAdvanced ? nAdvancedID : 0;
			return true;
		case MBtnAdvanced:
			SelectAdvancedPreset(nAdvancedID, bFAdvanced);
			break;
		case MBtnREBuilder:
			if (RunREBuilder(pPreset->m_mapStrings["Text"], pPreset->m_mapStrings["Replace"])) {
				if (*pSearchAs == SA_PLAINTEXT) *pSearchAs = SA_REGEXP;
			}
			break;
		case MRunEditor:
			RunExternalEditor(pPreset->m_mapStrings["Replace"]);
			break;
		case MEllipsis:
			ConfigureSeveralLines();
			break;
		default:
			return false;
		}
	} while (true);
}
