#include "StdAfx.h"
#include "..\RESearch.h"

CREParameters<char> REParamA;
PSECURITY_DESCRIPTOR g_pSD = NULL;

BOOL ConfirmFileReadonly(TCHAR *FileName) {
	if (!FRConfirmReadonlyThisRun) return TRUE;
	if (FRReplaceReadonly == RR_NEVER) return FALSE;
	const TCHAR *Lines[]={
		GetMsg(MREReplace),GetMsg(MTheFile),FileName,GetMsg(MModifyReadonlyRequest),
		GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(StartupInfo.ModuleNumber,0,_T("FRConfirmReadonly"),Lines,8,4)) {
	case 1:FRConfirmReadonlyThisRun=FALSE;
	case 0:return TRUE;
	case -1:
	case 3:g_bInterrupted=TRUE;
	}
	return FALSE;
}

BOOL ConfirmReplacement(const char *Found, const char *Replaced, const TCHAR *FileName) {
	if (!FRConfirmLineThisFile) return TRUE;
	if (g_bInterrupted) return FALSE;

#ifdef UNICODE
	wstring strFound = DefToUnicode(Found);
	wstring strReplaced = DefToUnicode(Replaced);

	const TCHAR *Lines[]={
		GetMsg(MREReplace),GetMsg(MAskReplace),strFound.c_str(),GetMsg(MAskWith),strReplaced.c_str(),
		GetMsg(MInFile),FileName,GetMsg(MReplace),GetMsg(MAll),GetMsg(MAllFiles),GetMsg(MSkip),GetMsg(MCancel)
	};
#else
	const TCHAR *Lines[]={
		GetMsg(MREReplace),GetMsg(MAskReplace),Found,GetMsg(MAskWith),Replaced,
		GetMsg(MInFile),FileName,GetMsg(MReplace),GetMsg(MAll),GetMsg(MAllFiles),GetMsg(MSkip),GetMsg(MCancel)
	};
#endif

	switch (StartupInfo.Message(StartupInfo.ModuleNumber,0,_T("FRAskReplace"),Lines,12,5)) {
	case 2:FRConfirmLineThisRun=FALSE;
	case 1:FRConfirmLineThisFile=FALSE;
	case 0:return TRUE;
	case -1:
	case 4:g_bInterrupted=TRUE;
	}
	return FALSE;
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

BOOL DoFileReplace(HANDLE &hFile,const char *&Found,int FoundLen,const char *Replace,int ReplaceLength,const char *&Skip,int SkipLen,WIN32_FIND_DATA *FindData) {
	if (hFile==INVALID_HANDLE_VALUE) {
		if (!ConfirmFile(MREReplace,FindData->cFileName)) return FALSE;
		if (FindData->dwFileAttributes&FILE_ATTRIBUTE_READONLY) {
			if (!ConfirmFileReadonly(FindData->cFileName)) return FALSE;
			SetFileAttributes(FindData->cFileName,FindData->dwFileAttributes&~FILE_ATTRIBUTE_READONLY);
		}

		SECURITY_ATTRIBUTES Sec;
		Sec.nLength = sizeof(Sec);
		Sec.bInheritHandle = false;
		Sec.lpSecurityDescriptor = g_pSD;

		hFile=CreateFile(FindData->cFileName, GENERIC_READ|GENERIC_WRITE, 0, &Sec, CREATE_ALWAYS,
			FindData->dwFileAttributes, INVALID_HANDLE_VALUE);

		//	Cannot set via attributes
		USHORT fmt = (FindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ? COMPRESSION_FORMAT_DEFAULT : COMPRESSION_FORMAT_NONE;
		DWORD dwReturned;
		DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &fmt, sizeof(fmt), NULL, 0, &dwReturned, NULL);

		if (hFile==INVALID_HANDLE_VALUE) {
			const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),FindData->cFileName,GetMsg(MOk)};
			StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FRCreateError"),Lines,4,1);
			return FALSE;
		}
	}

	if (!WriteBuffer(hFile,Skip,SkipLen,FindData->cFileName)) return FALSE;

	bool bIgnore = g_bIgnoreIdentReplace &&
		(FoundLen == ReplaceLength) && (memcmp(Found, Replace, FoundLen) == 0);

	if (!bIgnore && ConfirmReplacement(string(Found, FoundLen).c_str(),Replace,FindData->cFileName)) {
		if (!WriteBuffer(hFile,Replace,ReplaceLength,FindData->cFileName)) return FALSE;
		ReplaceNumber++;
	} else {
		if (!WriteBuffer(hFile,Found,FoundLen,FindData->cFileName)) return FALSE;
		if (bIgnore) ReplaceNumber++;
	}

	Skip = Found+FoundLen;
	Found += (FoundLen)?FoundLen:1;
	return TRUE;
}

BOOL FinishReplace(HANDLE hFile,const char *&Skip,int SkipLen,WIN32_FIND_DATA *FindData) {
	if (hFile!=INVALID_HANDLE_VALUE) {
		if (ReplaceNumber == 0) {
			CloseHandle(hFile);
			return FALSE;
		}

		WriteBuffer(hFile,Skip,SkipLen,FindData->cFileName);

		SetFileTime(hFile, &FindData->ftCreationTime, NULL, NULL);
		CloseHandle(hFile);

		return TRUE;
	} else {
		return FALSE;
	}
}

BOOL ProcessPlainTextBuffer(const char *Buffer,int BufLen,WIN32_FIND_DATA *FindData) {
	const char *Current=Buffer;
	const char *Skip=Buffer;
	HANDLE hFile=INVALID_HANDLE_VALUE;

#ifdef UNICODE
	char *Table=(FCaseSensitive) ? NULL : GetUpCaseTable(-1);
#else
	char *Table=(FCaseSensitive) ? NULL : UpCaseTable;
#endif

	REParamA.Clear();

	while (Current+FText.size()<=Buffer+BufLen) {
#ifdef UNICODE
		int nPosition = BMHSearchA(Current, Buffer+BufLen-Current, FOEMTextUpcase.data(), FOEMTextUpcase.size(), Table);
#else
		int nPosition = BMHSearch(Current, Buffer+BufLen-Current, FTextUpcase.data(), FTextUpcase.size(), Table);
#endif
		if (nPosition < 0) break;
		Current += nPosition;

		REParamA.AddSource(Current, Buffer+BufLen-Current);
		REParamA.AddFNumbers(FileNumber, FindNumber, ReplaceNumber);
#ifdef UNICODE
		string Replace = CSOA::CreateReplaceString(FOEMReplace.c_str(), "\n", -1, REParamA);
#else
		string Replace = CSOA::CreateReplaceString(FRReplace.c_str(), "\n", -1, REParamA);
#endif
		if (!DoFileReplace(hFile,Current,FText.size(),Replace.c_str(),Replace.length(),Skip,Current-Skip,FindData)) break;
		FindNumber++;
	}

	return FinishReplace(hFile,Skip,Buffer+BufLen-Skip,FindData);
}

BOOL ProcessRegExpBuffer(const char *Buffer,int BufLen,WIN32_FIND_DATA *FindData) {
	const char *BufEnd=Buffer;
	const char *Skip=Buffer;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	BOOL Error=FALSE;

	REParamA.Clear();
	REParamA.AddRE(FPatternA);

	do {
		int Start=0;
		Buffer=BufEnd;
		SkipNoCRLF(BufEnd,&BufLen);
		while ((BufEnd!=Buffer)&&do_pcre_execA(FPatternA,FPatternExtraA,Buffer,BufEnd-Buffer,Start,0,REParamA.Match(),REParamA.Count())>=0) {

			REParamA.AddSource(Buffer,BufEnd-Buffer);
			REParamA.AddFNumbers(FileNumber, FindNumber, ReplaceNumber);
#ifdef UNICODE
			string Replace = CSOA::CreateReplaceString(FOEMReplace.c_str(), "\n", -1, REParamA);
#else
			string Replace = CSOA::CreateReplaceString(FRReplace.c_str(), "\n", -1, REParamA);
#endif
			const char *NewBuffer = Buffer + REParamA.m_arrMatch[0];
			if (!DoFileReplace(hFile,NewBuffer,REParamA.m_arrMatch[1]-REParamA.m_arrMatch[0],Replace.c_str(),Replace.length(),Skip,NewBuffer-Skip,FindData)) {
				Error=TRUE;break;
			}
			Start=NewBuffer-Buffer;
			FindNumber++;
		}
		SkipCRLF(BufEnd,&BufLen);

		if (hFile == INVALID_HANDLE_VALUE) g_nFoundLine++;	// Yet looking for first match
	} while (BufLen&&(!Error));

	return FinishReplace(hFile,Skip,BufEnd-Skip,FindData);
}

int CountLinesIn(const char *Buffer,int Len) {
	int LinesIn=0;
	while (Len) {
		SkipWholeLine(Buffer,&Len);LinesIn++;
	}
	return LinesIn;
}

BOOL ReplaceSeveralLineBuffer(HANDLE &hFile,const char *&Buffer,const char *BufEnd,
							  const char *&Skip, int &LinesIn,WIN32_FIND_DATA *FindData) {
	int Start=0;
	const char *LineEnd=Buffer;
	int LineLen=BufEnd-Buffer;

	SkipWholeLine(LineEnd,&LineLen);

	while ((Buffer < BufEnd) && do_pcre_execA(FPatternA,FPatternExtraA,Buffer,BufEnd-Buffer,Start,0,REParamA.Match(),REParamA.Count())>=0) {
		const char *NewBuffer=Buffer+REParamA.m_arrMatch[0];
		if (NewBuffer>=LineEnd) break;

		REParamA.AddSource(Buffer,BufEnd-Buffer);
		REParamA.AddFNumbers(FileNumber, FindNumber, ReplaceNumber);
#ifdef UNICODE
		string Replace = CSOA::CreateReplaceString(FOEMReplace.c_str(), "\n", -1, REParamA);
#else
		string Replace = CSOA::CreateReplaceString(FRReplace.c_str(), "\n", -1, REParamA);
#endif
		if (!DoFileReplace(hFile,NewBuffer,REParamA.m_arrMatch[1]-REParamA.m_arrMatch[0],Replace.c_str(),Replace.length(),Skip,NewBuffer-Skip,FindData)) {
			return FALSE;
		}
		Buffer = Skip;
		LineEnd=Buffer;
		int LineLen=BufEnd-Buffer;
		SkipWholeLine(LineEnd,&LineLen);

		Start = 0;
		FindNumber++;
	}
	Buffer=LineEnd;
	if (hFile == INVALID_HANDLE_VALUE) g_nFoundLine++;	// Yet looking for first match
	LinesIn=CountLinesIn(Buffer,BufEnd-Buffer);
	return TRUE;
}

BOOL ProcessSeveralLineBuffer(const char *Buffer,int BufLen,WIN32_FIND_DATA *FindData) {
	const char *BufEnd=Buffer;
	const char *Skip=Buffer;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	int LinesIn=0;
	BOOL Error=FALSE;

	REParamA.Clear();
	REParamA.AddRE(FPatternA);

	do {
		SkipWholeLine(BufEnd,&BufLen);
		LinesIn++;
		if ((LinesIn==SeveralLines) || ((BufEnd-Buffer) >= SeveralLinesKB*1024)) {
			if (!ReplaceSeveralLineBuffer(hFile,Buffer,BufEnd,Skip,LinesIn,FindData)) {Error=TRUE;break;};
		}
	} while (BufLen);

	while (Buffer<BufEnd) {
		if (!ReplaceSeveralLineBuffer(hFile,Buffer,BufEnd,Skip,LinesIn,FindData)) {Error=TRUE;break;};
	}

	return FinishReplace(hFile,Skip,BufEnd-Skip,FindData);
}

BOOL ProcessBuffer(const char *Buffer,int BufLen,WIN32_FIND_DATA *FindData) {
	FRConfirmLineThisFile = FRConfirmLineThisRun;
	FileConfirmed = !FRConfirmFileThisRun;
	FindNumber = ReplaceNumber = 0;
#ifndef UNICODE
	m_pReplaceTable = NULL;
#endif
	switch (FSearchAs) {
	case SA_PLAINTEXT:	return ProcessPlainTextBuffer(Buffer,BufLen,FindData);
	case SA_REGEXP:		return ProcessRegExpBuffer(Buffer,BufLen,FindData);
	case SA_SEVERALLINE:return ProcessSeveralLineBuffer(Buffer,BufLen,FindData);
	}
	return FALSE;
}

char *AddExtension(char *FileName,char *Extension) {
	char *New=(char *)malloc(strlen(FileName)+strlen(Extension)+1);
	return strcat(strcpy(New,FileName),Extension);
}

void ReplaceFile(WIN32_FIND_DATA *FindData, panelitem_vector &PanelItems) {
	tstring strBackupFileName;
	BOOL ReturnValue=FALSE;

	InitFoundPosition();

	if (FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return;
	if ((FRReplaceReadonly == RR_NEVER) && (FindData->dwFileAttributes&FILE_ATTRIBUTE_READONLY)) return;

	FileNumber++;

	int nTry = 0;
	do {
		strBackupFileName = FindData->cFileName;
		if (nTry > 0) {
			TCHAR szNum[8];
			_stprintf_s(szNum, 8, _T(".%02d"), nTry);
			strBackupFileName += szNum;
		}
		strBackupFileName += _T(".bak");

		if (FRSaveOriginal && FROverwriteBackup) {
			if (MoveFileEx(FindData->cFileName, strBackupFileName.c_str(), MOVEFILE_REPLACE_EXISTING)) {
				break;
			}
		} else {
			if (MoveFile(FindData->cFileName, strBackupFileName.c_str())) {
				break;
			} else {
				if (GetLastError() == ERROR_ALREADY_EXISTS) {
					nTry++;
					continue;
				}
			}
		}

		const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileCreateError),strBackupFileName.c_str(),GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FRBackupError"),Lines,4,1);
		return;
	} while (true);

	CFileMapping mapFile;
	if (mapFile.Open(strBackupFileName.c_str())) {

		if (!g_pSD != NULL)
			LocalFree(g_pSD);

		if (GetNamedSecurityInfo(strBackupFileName.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
			NULL, NULL, NULL, NULL, &g_pSD) != ERROR_SUCCESS) {
				g_pSD = NULL;
		}

		if (ProcessBuffer(mapFile, FindData->nFileSizeLow, FindData)) {
			mapFile.Close();
			if (!FRSaveOriginal) DeleteFile(strBackupFileName.c_str());
			AddFile(FindData, PanelItems);
		} else {
			mapFile.Close();
			DeleteFile(FindData->cFileName);
			MoveFile(strBackupFileName.c_str(),FindData->cFileName);
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
	CFarDialog Dialog(76,24,_T("FileReplaceDlg"));
	Dialog.AddFrame(MREReplace);

	Dialog.Add(new CFarCheckBoxItem(35,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("Masks"), MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), SearchText));

	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,7,65,DIF_HISTORY|DIF_VAREDIT,_T("ReplaceText"), ReplaceText));

	Dialog.Add(new CFarButtonItem(67,5,0,0,_T("&\\")));
	Dialog.Add(new CFarButtonItem(67,7,0,0,_T("&/")));

	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR, _T("")));
	Dialog.Add(new CFarRadioButtonItem(5,9,DIF_GROUP,MPlainText,(int *)&FSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,MRegExp,			 (int *)&FSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MSeveralLineRegExp,(int *)&FSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarCheckBoxItem(5,12,0,MCaseSensitive,&FCaseSensitive));

	Dialog.Add(new CFarTextItem(5,14,0,MSearchIn));
	if (Plugin) {
		if (FSearchIn<SI_FROMCURRENT) FSearchIn=SI_FROMCURRENT;
		Dialog.Add(new CFarComboBoxItem(15,14,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearchPlugin, false),(int *)&FSearchIn,NULL,3));
	} else {
		Dialog.Add(new CFarComboBoxItem(15,14,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));
	}

	Dialog.Add(new CFarCheckBoxItem(5,16,0,MViewModified,&FROpenModified));
	Dialog.Add(new CFarCheckBoxItem(5,17,0,MConfirmFile,&FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(5,18,0,MConfirmLine,&FRConfirmLine));
	Dialog.Add(new CFarCheckBoxItem(40,16,0,MSaveOriginal,&FRSaveOriginal));
	Dialog.Add(new CFarCheckBoxItem(42,17,0,MOverwriteBackup,&FROverwriteBackup));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,9,0,0,MBtnPresets));
	Dialog.Add(new CFarCheckBoxItem(56,10,0,_T(""),&FAdvanced));
	Dialog.Add(new CFarButtonItem(60,10,0,0,MBtnAdvanced));
	Dialog.Add(new CFarCheckBoxItem(56,11,0,_T(""),&FUTF8));
	Dialog.Add(new CFarButtonItem(60,11,0,0,MUTF8));
	Dialog.SetFocus(3);
	if (FSearchAs>=SA_MULTILINE) FSearchAs=SA_PLAINTEXT;
	FACaseSensitive=FADirectoryCaseSensitive=MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;
	ReplaceText=FRReplace;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(7, -7, 8, 9, -5, -3, -1)) {
		case 0:
			FMask=MaskText;
			FText=SearchText;
			FRReplace=ReplaceText;
			break;
		case 1:
			if ((FSearchAs!=SA_PLAINTEXT) && (FSearchAs!=SA_MULTITEXT)) QuoteRegExpString(SearchText);
			break;
		case 2:
			QuoteReplaceString(ReplaceText);
			break;
		case 3:
			FRPresets->ShowMenu(true);
			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
			break;
		case 4:
			if (AdvancedSettings()) FAdvanced=TRUE;
			break;
		case 5:
			UTF8Converter(SearchText);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1) || !PrepareFileReplacePattern());

	if (FUTF8) FAllCharTables=FALSE;
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
	if (ScanDirectories(PanelItems, ReplaceFile)) {
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
	Dialog.Add(new CFarCheckBoxItem(56,13,0,_T(""),&pPreset->m_mapInts["UTF8"]));
	Dialog.Add(new CFarButtonItem(60,13,0,0,MUTF8));
	Dialog.Add(new CFarCheckBoxItem(5,16,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(3, -2, -4, -6)) {
		case 0:
			pPreset->m_mapInts["AdvancedID"] = bFAdvanced ? nAdvancedID : 0;
			return TRUE;
		case 1:{		// avoid Internal Error for icl
			tstring str = pPreset->m_mapStrings["Text"];
			UTF8Converter(str);
			break;
			  }
		case 2:
			SelectAdvancedPreset(nAdvancedID, bFAdvanced);
			break;
		default:
			return FALSE;
		}
	} while (true);
}
