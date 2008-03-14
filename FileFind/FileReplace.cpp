#include "StdAfx.h"
#include "..\RESearch.h"

BOOL ConfirmFileReadonly(char *FileName) {
	if (!FRConfirmReadonlyThisRun) return TRUE;
	if (FRReplaceReadonly == RR_NEVER) return FALSE;
	const char *Lines[]={
		GetMsg(MREReplace),GetMsg(MTheFile),FileName,GetMsg(MModifyReadonlyRequest),
		GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(StartupInfo.ModuleNumber,0,"FRConfirmReadonly",Lines,8,4)) {
	case 1:FRConfirmReadonlyThisRun=FALSE;
	case 0:return TRUE;
	case -1:
	case 3:g_bInterrupted=TRUE;
	}
	return FALSE;
}

BOOL ConfirmReplacement(const char *Found, const char *Replaced, const char *FileName) {
	if (!FRConfirmLineThisFile) return TRUE;
	if (g_bInterrupted) return FALSE;
	const char *Lines[]={
		GetMsg(MREReplace),GetMsg(MAskReplace),Found,GetMsg(MAskWith),Replaced,
		GetMsg(MInFile),FileName,GetMsg(MReplace),GetMsg(MAll),GetMsg(MAllFiles),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(StartupInfo.ModuleNumber,0,"FRAskReplace",Lines,12,5)) {
	case 2:FRConfirmLineThisRun=FALSE;
	case 1:FRConfirmLineThisFile=FALSE;
	case 0:return TRUE;
	case -1:
	case 4:g_bInterrupted=TRUE;
	}
	return FALSE;
}

BOOL WriteBuffer(HANDLE hFile,const void *Buffer,DWORD BufLen,const char *FileName) {
	DWORD WrittenBytes;
	if (!WriteFile(hFile,Buffer,BufLen,&WrittenBytes,NULL)||
		(WrittenBytes!=BufLen)) {
		const char *Lines[]={GetMsg(MREReplace),GetMsg(MFileWriteError),FileName,GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FRWriteError",Lines,4,1);
		return FALSE;
	} else return TRUE;
}

BOOL DoReplace(HANDLE &hFile,const char *&Found,int FoundLen,const char *Replace,int ReplaceLength,const char *&Skip,int SkipLen,WIN32_FIND_DATA *FindData) {
	if (hFile==INVALID_HANDLE_VALUE) {
		if (!ConfirmFile(MREReplace,FindData->cFileName)) return FALSE;
		if (FindData->dwFileAttributes&FILE_ATTRIBUTE_READONLY) {
			if (!ConfirmFileReadonly(FindData->cFileName)) return FALSE;
			SetFileAttributes(FindData->cFileName,FindData->dwFileAttributes&~FILE_ATTRIBUTE_READONLY);
		}
		hFile=CreateFile(FindData->cFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
			FindData->dwFileAttributes,INVALID_HANDLE_VALUE);
		if (hFile==INVALID_HANDLE_VALUE) {
			const char *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),FindData->cFileName,GetMsg(MOk)};
			StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FRCreateError",Lines,4,1);
			return FALSE;
		}
	}

	if (!WriteBuffer(hFile,Skip,SkipLen,FindData->cFileName)) return FALSE;

	char *szFound=(char *)malloc(FoundLen+1);
	strncpy(szFound,Found,FoundLen);szFound[FoundLen]=0;

	if (ConfirmReplacement(szFound,Replace,FindData->cFileName)) {
		if (!WriteBuffer(hFile,Replace,ReplaceLength,FindData->cFileName)) return FALSE;
	} else {
		if (!WriteBuffer(hFile,Found,FoundLen,FindData->cFileName)) return FALSE;
	}
	Skip=Found+FoundLen;Found+=(FoundLen)?FoundLen:1;
	return TRUE;
}

BOOL FinishReplace(HANDLE hFile,const char *&Skip,int SkipLen,WIN32_FIND_DATA *FindData) {
	if (hFile!=INVALID_HANDLE_VALUE) {
		WriteBuffer(hFile,Skip,SkipLen,FindData->cFileName);
		CloseHandle(hFile);
		if (FindData->dwFileAttributes&FILE_ATTRIBUTE_READONLY)
			SetFileAttributes(FindData->cFileName,FindData->dwFileAttributes);
		return TRUE;
	} else return FALSE;
}

BOOL ProcessPlainTextBuffer(const char *Buffer,int BufLen,WIN32_FIND_DATA *FindData) {
	const char *Current=Buffer;
	const char *Skip=Buffer;
	HANDLE hFile=INVALID_HANDLE_VALUE;

	char *Table=(FCaseSensitive) ? NULL : UpCaseTable;

	while (Current+FText.size()<=Buffer+BufLen) {
		int nPosition = BMHSearch(Current, Buffer+BufLen-Current, FTextUpcase.data(), FTextUpcase.size(), Table);
		if (nPosition < 0) break;
		Current += nPosition;

		string Replace=CreateReplaceString(Buffer,NULL,0,FRReplace.c_str(),"\n",NULL,-1);
		if (!DoReplace(hFile,Current,FText.size(),Replace.c_str(),Replace.length(),Skip,Current-Skip,FindData)) break;
	}

	return FinishReplace(hFile,Skip,Buffer+BufLen-Skip,FindData);
}

BOOL ProcessRegExpBuffer(const char *Buffer,int BufLen,WIN32_FIND_DATA *FindData) {
	const char *BufEnd=Buffer;
	const char *Skip=Buffer;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	int MatchCount=pcre_info(FPattern,NULL,NULL)+1;
	int *Match=new int[MatchCount*3];
	BOOL Error=FALSE;

	do {
		int Start=0;
		Buffer=BufEnd;
		SkipNoCRLF(BufEnd,&BufLen);
		while ((BufEnd!=Buffer)&&do_pcre_exec(FPattern,FPatternExtra,Buffer,BufEnd-Buffer,Start,0,Match,MatchCount*3)>=0) {
			string Replace=CreateReplaceString(Buffer,Match,MatchCount,FRReplace.c_str(),"\n",NULL,-1);
			const char *NewBuffer=Buffer+Match[0];
			if (!DoReplace(hFile,NewBuffer,Match[1]-Match[0],Replace.c_str(),Replace.length(),Skip,NewBuffer-Skip,FindData)) {
				Error=TRUE;break;
			}
			Start=NewBuffer-Buffer;
		}
		SkipCRLF(BufEnd,&BufLen);

		if (hFile == INVALID_HANDLE_VALUE) g_nFoundLine++;	// Yet looking for first match
	} while (BufLen&&(!Error));

	delete[] Match;
	return FinishReplace(hFile,Skip,BufEnd-Skip,FindData);
}

int CountLinesIn(const char *Buffer,int Len) {
	int LinesIn=0;
	while (Len) {
		SkipWholeLine(Buffer,&Len);LinesIn++;
	}
	return LinesIn;
}

BOOL ReplaceSeveralLineBuffer(HANDLE &hFile,const char *&Buffer,const char *BufEnd,int *Match,int MatchCount,
							  const char *&Skip, int &LinesIn,WIN32_FIND_DATA *FindData) {
	int Start=0;
	const char *LineEnd=Buffer;
	int LineLen=BufEnd-Buffer;
	SkipWholeLine(LineEnd,&LineLen);

	while ((Buffer < BufEnd) && do_pcre_exec(FPattern,FPatternExtra,Buffer,BufEnd-Buffer,Start,0,Match,MatchCount*3)>=0) {
		const char *NewBuffer=Buffer+Match[0];
		if (NewBuffer>=LineEnd) break;
		string Replace=CreateReplaceString(Buffer,Match,MatchCount,FRReplace.c_str(),"\n",NULL,-1);
		if (!DoReplace(hFile,NewBuffer,Match[1]-Match[0],Replace.c_str(),Replace.length(),Skip,NewBuffer-Skip,FindData)) {
			return FALSE;
		}
//		Start=NewBuffer-Buffer;
		Buffer = Skip;
		LineEnd=Buffer;
		int LineLen=BufEnd-Buffer;
		SkipWholeLine(LineEnd,&LineLen);

		Start = 0;
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
	int MatchCount=pcre_info(FPattern,NULL,NULL)+1;
	int *Match=new int[MatchCount*3];
	BOOL Error=FALSE;

	do {
		SkipWholeLine(BufEnd,&BufLen);
		LinesIn++;
		if ((LinesIn==SeveralLines) || ((BufEnd-Buffer) >= SeveralLinesKB*1024)) {
			if (!ReplaceSeveralLineBuffer(hFile,Buffer,BufEnd,Match,MatchCount,Skip,LinesIn,FindData)) {Error=TRUE;break;};
		}
	} while (BufLen);

	while (Buffer<BufEnd) {
		if (!ReplaceSeveralLineBuffer(hFile,Buffer,BufEnd,Match,MatchCount,Skip,LinesIn,FindData)) {Error=TRUE;break;};
	}
	delete[] Match;
	return FinishReplace(hFile,Skip,BufEnd-Skip,FindData);
}

BOOL ProcessBuffer(const char *Buffer,int BufLen,WIN32_FIND_DATA *FindData) {
	FRConfirmLineThisFile = FRConfirmLineThisRun;
	FileConfirmed = !FRConfirmFileThisRun;
	m_pReplaceTable = NULL;
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

void ReplaceFile(WIN32_FIND_DATA *FindData, PluginPanelItem **PanelItems, int *ItemsNumber) {
	string strBackupFileName;
	BOOL ReturnValue=FALSE;

	InitFoundPosition();

	if (FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return;
	if ((FRReplaceReadonly == RR_NEVER) && (FindData->dwFileAttributes&FILE_ATTRIBUTE_READONLY)) return;

	int nTry = 0;
	do {
		strBackupFileName = FindData->cFileName;
		if (nTry > 0) {
			char szNum[8];
			sprintf(szNum, ".%02d", nTry);
			strBackupFileName += szNum;
		}
		strBackupFileName += ".bak";

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

		const char *Lines[]={GetMsg(MREReplace),GetMsg(MFileCreateError),strBackupFileName.c_str(),GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FRBackupError",Lines,4,1);
		return;
	} while (true);

	CFileMapping mapFile;
	if (mapFile.Open(strBackupFileName.c_str())) {
		if (ProcessBuffer(mapFile, FindData->nFileSizeLow, FindData)) {
			mapFile.Close();
			if (!FRSaveOriginal) DeleteFile(strBackupFileName.c_str());
			AddFile(FindData,PanelItems,ItemsNumber);
		} else {
			mapFile.Close();
			MoveFile(strBackupFileName.c_str(),FindData->cFileName);
		}
	} else {
		const char *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),FindData->cFileName,GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FSOpenError",Lines,4,1);
		return;
	}
}

BOOL FileReplaceExecutor(CParameterBatch &Batch) {
	FMask=MaskText;
	FText=SearchText;
	FRReplace=ReplaceText;
	if (!FPreparePattern(false)) return FALSE;
	if (FUTF8) FAllCharTables=FALSE;

	FRConfirmFileThisRun = FALSE;		// FRConfirmFile;
	FRConfirmReadonlyThisRun = FALSE;	// (FRReplaceReadonly == RR_ALWAYS);
	FRConfirmLineThisRun = FALSE;		// FRConfirmLine;
	ScanDirectories(&PanelItems,&ItemsNumber,ReplaceFile);

	return TRUE;
}

int ReplacePrompt(BOOL Plugin) {
	CFarDialog Dialog(76,24,"FileReplaceDlg");
	Dialog.AddFrame(MREReplace);

	Dialog.Add(new CFarCheckBoxItem(35,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"Masks", MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY,"SearchText", SearchText));

	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,7,65,DIF_HISTORY,"ReplaceText", ReplaceText));

	Dialog.Add(new CFarButtonItem(67,5,0,0,"&\\"));
	Dialog.Add(new CFarButtonItem(67,7,0,0,"&/"));

	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR,""));
	Dialog.Add(new CFarRadioButtonItem(5,9,DIF_GROUP,MPlainText,(int *)&FSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,MRegExp,			 (int *)&FSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MSeveralLineRegExp,(int *)&FSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarCheckBoxItem(5,12,0,MCaseSensitive,&FCaseSensitive));

	Dialog.Add(new CFarTextItem(5,14,0,MSearchIn));
	if (Plugin) {
		if (FSearchIn<SI_FROMCURRENT) FSearchIn=SI_FROMCURRENT;
		Dialog.Add(new CFarComboBoxItem(15,14,60,0,new CFarListData(g_WhereToSearchPlugin, false),(int *)&FSearchIn,NULL,3));
	} else {
		Dialog.Add(new CFarComboBoxItem(15,14,60,0,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));
	}

	Dialog.Add(new CFarCheckBoxItem(5,16,0,MViewModified,&FROpenModified));
	Dialog.Add(new CFarCheckBoxItem(5,17,0,MConfirmFile,&FRConfirmFile));
	Dialog.Add(new CFarCheckBoxItem(5,18,0,MConfirmLine,&FRConfirmLine));
	Dialog.Add(new CFarCheckBoxItem(40,16,0,MSaveOriginal,&FRSaveOriginal));
	Dialog.Add(new CFarCheckBoxItem(42,17,0,MOverwriteBackup,&FROverwriteBackup));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,20,0,0,MBatch));
	Dialog.Add(new CFarButtonItem(60,9,0,0,MPresets));
	Dialog.Add(new CFarCheckBoxItem(56,10,0,"",&FAdvanced));
	Dialog.Add(new CFarButtonItem(60,10,0,0,MAdvanced));
	Dialog.Add(new CFarCheckBoxItem(56,11,0,"",&FUTF8));
	Dialog.Add(new CFarButtonItem(60,11,0,0,MUTF8));
	Dialog.SetFocus(3);
	if (FSearchAs>=SA_MULTILINE) FSearchAs=SA_PLAINTEXT;
	FACaseSensitive=FADirectoryCaseSensitive=MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;
	ReplaceText=FRReplace;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(7,-8,8,9,-6,-5,-3,-1)) {
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
			if (FRBatch->ShowMenu(FileReplaceExecutor, g_FRBatch) >= 0)
				return FALSE;
			break;
		case 4:
			FRPresets->ShowMenu(g_FRBatch);
			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
			break;
		case 5:
			if (AdvancedSettings()) FAdvanced=TRUE;
			break;
		case 6:
			UTF8Converter(SearchText);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!FPreparePattern(false));

	if (FUTF8) FAllCharTables=FALSE;
	return TRUE;
}

OperationResult FileReplace(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog) {
	PanelInfo PInfo;
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
	if (PInfo.PanelType!=PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin&&((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;

	if (ShowDialog) {
		if (!ReplacePrompt(PInfo.Plugin)) return OR_CANCEL;
	} else {
		if (!FPreparePattern(false)) return OR_CANCEL;
	}

	FRConfirmFileThisRun=FRConfirmFile;
	FRConfirmReadonlyThisRun = (FRReplaceReadonly != RR_ALWAYS);
	FRConfirmLineThisRun=FRConfirmLine;
	if (ScanDirectories(PanelItems,ItemsNumber,ReplaceFile)) {
		if (!FROpenModified) return OR_OK; else
		return (*ItemsNumber==0)?NoFilesFound():OR_PANEL;
	} else return OR_FAILED;
}

BOOL CFRPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,20,"FRPresetDlg");
	Dialog.AddFrame(MFRPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"RESearch.PresetName", pPreset->Name()));

	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));
	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"Masks", pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,"SearchText", pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,8,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,9,70,DIF_HISTORY,"ReplaceText", pPreset->m_mapStrings["Replace"]));

	Dialog.Add(new CFarTextItem(5,10,DIF_BOXCOLOR|DIF_SEPARATOR,(char *)NULL));

	int *pSearchAs = &pPreset->m_mapInts["SearchAs"];
	Dialog.Add(new CFarRadioButtonItem(5,11,DIF_GROUP,MPlainText,pSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,MRegExp,		pSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,MSeveralLineRegExp,pSearchAs,SA_SEVERALLINE));

	Dialog.Add(new CFarCheckBoxItem(5,14,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(56,12,0,"",&pPreset->m_mapInts["UTF8"]));
	Dialog.Add(new CFarButtonItem(60,12,0,0,MUTF8));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(2, -2, -3)) {
		case 0:
			return TRUE;
		case 1:{		// avoid Internal Error for icl
			string str = pPreset->m_mapStrings["Text"];
			UTF8Converter(str);
			break;
			  }
		default:
			return FALSE;
		}
	} while (true);
}
