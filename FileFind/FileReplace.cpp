#include "StdAfx.h"
#include "..\RESearch.h"

tstring g_strNewFileName;
tstring g_strBackupFileName;

bool ConfirmFileReadonly(const TCHAR *FileName)
{
	if (!FRConfirmReadonlyThisRun) return true;
	if (FRReplaceReadonly == RR_NEVER) return false;

	const TCHAR *Lines[]={
		GetMsg(MREReplace),GetMsg(MTheFile),FileName,GetMsg(MModifyReadonlyRequest),
		GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(StartupInfo.ModuleNumber,0,_T("FRConfirmReadonly"),Lines,8,4)) {
	case 1:
		FRConfirmReadonlyThisRun = FALSE;
	case 0:
		return true;
	case 2:
		break;
	case 3:
	case -1:
		g_bInterrupted = true;
		break;
	}
	return false;
}

bool ConfirmReplacement(const TCHAR *Found, const TCHAR *Replaced, const TCHAR *FileName)
{
	if (!FRConfirmLineThisFile) return true;
	if (g_bInterrupted) return false;

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
	if (nWidth < 50) nWidth = 50;

	size_t nCount = arrFound.size()+arrReplaced.size();

	CFarDialog Dialog(nWidth+12, nCount+10, _T("FRAskReplace"));
	Dialog.AddFrame(MREReplace);
	
	Dialog.Add(new CFarTextItem(-1, 2, 0, MAskReplace));
	for (size_t I = 0; I<arrFound.size(); I++)
		Dialog.Add(new CFarTextItem(-1, 3 + I, DIF_SETCOLOR|0x30, arrFound[I]));

	Dialog.Add(new CFarTextItem(-1, 3 + arrFound.size(), 0, MAskWith));
	for (size_t I = 0; I<arrReplaced.size();I++)
		Dialog.Add(new CFarTextItem(-1, 4 + arrFound.size() + I, DIF_SETCOLOR|0xB0, arrReplaced[I]));

	Dialog.Add(new CFarTextItem(-1, 4 + nCount, 0, MInFile));
	Dialog.Add(new CFarTextItem(-1, 5 + nCount,  DIF_SETCOLOR|0x20, FileName));

	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, TRUE,  MReplace));
	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MAll));
	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MAllFiles));
	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MSkip));
	Dialog.Add(new CFarButtonItem(0, 7 + nCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MCancel));
	int Result = Dialog.Display(5, -5, -4, -3, -2, -1);

//	const TCHAR *Lines[]={
//		GetMsg(MREReplace),GetMsg(MAskReplace),Found,GetMsg(MAskWith),Replaced,
//		GetMsg(MInFile),FileName,GetMsg(MReplace),GetMsg(MAll),GetMsg(MAllFiles),GetMsg(MSkip),GetMsg(MCancel)
//	};
//	Result = StartupInfo.Message(StartupInfo.ModuleNumber,0,_T("FRAskReplace"),Lines,12,5)

	switch (Result) {
	case 2:
		FRConfirmLineThisRun=FALSE;
	case 1:
		FRConfirmLineThisFile=FALSE;
	case 0:
		return true;
	case 3:
		break;
	case 4:
	case -1:
		g_bInterrupted = TRUE;
		break;
	}
	return false;
}

BOOL WriteBuffer(HANDLE hFile,const void *Buffer,DWORD BufLen,const TCHAR *FileName) {
	DWORD WrittenBytes;
	if (!WriteFile(hFile,Buffer,BufLen,&WrittenBytes,NULL)||
		(WrittenBytes!=BufLen)) {
		const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileWriteError),FileName,GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FRWriteError"),Lines,4,1);
		return FALSE;
	} else return TRUE;
}

bool RunReplace(LPCTSTR szFileName)
{
	switch (FSearchAs) {
	case SA_PLAINTEXT:{
		CReplacePlainTextFrontend Frontend;
		return RunReplace(szFileName, g_strNewFileName.c_str(), &Frontend, false);
					  }
	case SA_REGEXP:{
		CReplaceRegExpFrontend Frontend;
		return RunReplace(szFileName, g_strNewFileName.c_str(), &Frontend, true);
				   }
	case SA_SEVERALLINE:{
		CReplaceSeveralLineRegExpFrontend Frontend;
		return RunReplace(szFileName, g_strNewFileName.c_str(), &Frontend, true);
						}
	}

	return false;
}

char *AddExtension(char *FileName,char *Extension) {
	char *New=(char *)malloc(strlen(FileName)+strlen(Extension)+1);
	return strcat(strcpy(New,FileName),Extension);
}

tstring GetUniqueFileName(const TCHAR *szCurrent, const TCHAR *szExt) {
	int nTry = 0;
	do {
		tstring strName = szCurrent;
		if (nTry > 0) strName += FormatStr(_T(".%d"), nTry);
		strName += szExt;
		if (GetFileAttributes(strName.c_str()) == INVALID_FILE_ATTRIBUTES) {
			if (GetLastError() == ERROR_FILE_NOT_FOUND) return strName;
			return tstring();
		}
		nTry++;
	} while (true);
}

bool ReplaceSingleFile_Normal(WIN32_FIND_DATA &FindData)
{
	bool bProcess = RunReplace(FindData.cFileName);

	if (bProcess) {
		if (!FRReplaceToNew) {
			if (!ReplaceFile(FindData.cFileName, g_strNewFileName.c_str(),
				(FRSaveOriginal) ? g_strBackupFileName.c_str() : NULL,
				REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL)) {

					const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileBackupError),FindData.cFileName,GetMsg(MOk)};
					StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING|FMSG_ERRORTYPE,_T("FRBackupError"),Lines,4,1);
			}
		}
	} else {
		DeleteFile(g_strNewFileName.c_str());
	}

	return bProcess;
}

//	Using slow but reliable mechanism for files with hardlinks
bool ReplaceSingleFile_CopyFirst(WIN32_FIND_DATA &FindData)
{
	if (FRReplaceToNew || !CopyFile(FindData.cFileName, g_strBackupFileName.c_str(), FALSE)) {
		return ReplaceSingleFile_Normal(FindData);
	}

	g_strNewFileName = FindData.cFileName;
	bool bProcess = RunReplace(g_strBackupFileName.c_str());

	if (bProcess) {
		if (!FRSaveOriginal) DeleteFile(g_strBackupFileName.c_str());
	} else {
		MoveFileEx(g_strBackupFileName.c_str(), FindData.cFileName, MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED);
	}

	return bProcess;
}

void ReplaceSingleFile(WIN32_FIND_DATA *FindData, panelitem_vector &PanelItems)
{
	FRConfirmLineThisFile = FRConfirmLineThisRun;
	FileConfirmed = !FRConfirmFileThisRun;
	FindNumber = ReplaceNumber = 0;
#ifndef UNICODE
	m_pReplaceTable = NULL;
#endif

	InitFoundPosition();

	if (FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return;
	
	if (FindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
		if (!ConfirmFileReadonly(FindData->cFileName)) return;
		FileConfirmed = true;
	}

	FileNumber++;

	g_strBackupFileName = (FROverwriteBackup)
		? tstring(FindData->cFileName) + _T(".bak")
		: GetUniqueFileName(FindData->cFileName, _T(".bak"));
	g_strNewFileName = GetUniqueFileName(FindData->cFileName, _T(".new"));

	HANDLE hFile = CreateFile(FindData->cFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {

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
			AddFile(FindData, PanelItems);
		}
	} else {
		const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),FindData->cFileName,GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FSOpenError"),Lines,4,1);
		return;
	}
}

bool PrepareFileReplacePattern() {
	if (!FPreparePattern(false)) return false;
	if (FAdvanced) {
		if (!CompileAdvancedSettings()) return false;
	}
	return true;
}

int ReplacePrompt(BOOL Plugin) {
	CFarDialog Dialog(76, 25, _T("FileReplaceDlg"));
	Dialog.SetWindowProc(FileSearchDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

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
	Dialog.Add(new CFarRadioButtonItem(5,9,DIF_GROUP,MPlainText,(int *)&FSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,MRegExp,			 (int *)&FSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MSeveralLineRegExp,(int *)&FSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarCheckBoxItem(5,12,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MAllCharTables,&FAllCharTables));

	Dialog.Add(new CFarTextItem(5,15,0,MSearchIn));
	if (Plugin) {
		if (FSearchIn<SI_FROMCURRENT) FSearchIn=SI_FROMCURRENT;
		Dialog.Add(new CFarComboBoxItem(15,15,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearchPlugin, false),(int *)&FSearchIn,NULL,3));
	} else {
		Dialog.Add(new CFarComboBoxItem(15,15,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));
	}

	Dialog.Add(new CFarCheckBoxItem(5,17,0,MViewModified,&FROpenModified));
	Dialog.Add(new CFarCheckBoxItem(5,18,0,MConfirmFile,&FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(5,19,0,MConfirmLine,&FRConfirmLine));
	Dialog.Add(new CFarCheckBoxItem(40,17,0,MSaveOriginal,&FRSaveOriginal));
	Dialog.Add(new CFarCheckBoxItem(42,18,0,MOverwriteBackup,&FROverwriteBackup));
	Dialog.Add(new CFarCheckBoxItem(40,19,0,MReplaceToNew,&FRReplaceToNew));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,9,0,0,MBtnPresets));
	Dialog.Add(new CFarCheckBoxItem(56,10,0,_T(""),&FAdvanced));
	Dialog.Add(new CFarButtonItem(60,10,0,0,MBtnAdvanced));
	Dialog.Add(new CFarButtonItem(58, 11, 0, FALSE, MBtnREBuilder));

	Dialog.SetFocus(MMask, 1);
	if (FSearchAs>=SA_MULTILINE) FSearchAs=SA_PLAINTEXT;
	FACaseSensitive=FADirectoryCaseSensitive=MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;
	ReplaceText=FRReplace;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(-1)) {
		case MOk:
			FMask=MaskText;
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case MQuoteSearch:
			if ((FSearchAs!=SA_PLAINTEXT) && (FSearchAs!=SA_MULTITEXT)) CSO::QuoteRegExpString(SearchText);
			break;
		case MQuoteReplace:
			CSO::QuoteReplaceString(ReplaceText);
			break;
		case MBtnPresets:
			FRPresets->ShowMenu(true);
			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
			break;
		case MBtnAdvanced:
			if (AdvancedSettings()) FAdvanced=TRUE;
			break;
		case MBtnREBuilder:
			if (RunREBuilder(SearchText, ReplaceText)) {
				if (FSearchAs == SA_PLAINTEXT) FSearchAs = SA_REGEXP;
			}
			break;
		default:
			return FALSE;
		}
	} while ((ExitCode != MOk) || !PrepareFileReplacePattern());

	return TRUE;
}

OperationResult FileReplace(panelitem_vector &PanelItems, BOOL ShowDialog, BOOL bSilent) {
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.PanelType!=PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin&&((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;

	if (ShowDialog) {
		if (!ReplacePrompt(PInfo.Plugin)) return OR_CANCEL;
	} else {
		if (!PrepareFileReplacePattern()) return OR_CANCEL;
	}

	FRConfirmFileThisRun=FRConfirmFile;
	FRConfirmReadonlyThisRun = (FRReplaceReadonly != RR_ALWAYS);
	FRConfirmLineThisRun=FRConfirmLine;
	FileNumber = -1;	// Easier to increment

	CDebugTimer tm(_T("FileReplace() took %d ms"));
	int nResult = ScanDirectories(PanelItems, ReplaceSingleFile);
	tm.Stop();

	if (nResult) {
		if (!FROpenModified) return OR_OK; else
			return (PanelItems.empty()) ? (bSilent ? OR_OK : NoFilesFound()) : OR_PANEL;
	} else return OR_FAILED;
}

OperationResult FileReplaceExecutor() {
	FMask = MaskText;
	FText = SearchText;
	FRReplace = ReplaceText;
	FROpenModified = FALSE;

	return FileReplace(g_PanelItems, FALSE, TRUE);
}

BOOL CFRPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76, 22, _T("FRPresetDlg"));
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
	Dialog.Add(new CFarRadioButtonItem(5,12,0,MRegExp,		pSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,MSeveralLineRegExp,pSearchAs,SA_SEVERALLINE));

	Dialog.Add(new CFarCheckBoxItem(5,14,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(56,11,0,_T(""),&bFAdvanced));
	Dialog.Add(new CFarButtonItem(60,11,0,0,MBtnAdvanced));
	Dialog.Add(new CFarCheckBoxItem(5,16,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(2, -2, -4)) {
		case 0:
			pPreset->m_mapInts["AdvancedID"] = bFAdvanced ? nAdvancedID : 0;
			return TRUE;
		case 1:
			SelectAdvancedPreset(nAdvancedID, bFAdvanced);
			break;
		default:
			return FALSE;
		}
	} while (true);
}
