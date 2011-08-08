#include "StdAfx.h"
#include "..\RESearch.h"

BOOL FindTextInBufferWithTable(const TCHAR *Buffer, int Size, tstring &TextUpcase, TCHAR *Table) {
	return BMHSearch(Buffer, Size, TextUpcase.data(), TextUpcase.size(), Table) >= 0;
}

BOOL FindTextInBuffer(const char *Buffer, int Size, tstring &Text) {
	if (Size == 0) return FALSE;

	TCHAR *Table=(FCaseSensitive) ? NULL : UpCaseTable;

//	Re-preparing BMH for the multi-text search
	tstring TextUpcase = (FCaseSensitive) ? Text : UpCaseString(Text);
	PrepareBMHSearch(TextUpcase.data(), TextUpcase.length());

	vector<TCHAR> arrData;
	eLikeUnicode nDetect = UNI_NONE;

	nDetect = LikeUnicode(Buffer, Size);
	if (nDetect != UNI_NONE) {
		if (FromUnicodeSkipDetect(Buffer, Size, arrData, nDetect)) {
			if ((arrData.size() > 0) && FindTextInBufferWithTable(&arrData[0], arrData.size(), TextUpcase, Table)) return TRUE;
		}
#ifdef TRY_ENCODINGS_WITH_BOM
		if (!FAllCharTables) return FALSE;
#else
		return FALSE;
#endif
	}

#ifdef UNICODE
	if (FCanUseDefCP) {
		string OEMTextUpcase = DefFromUnicode(TextUpcase);
		char *OEMTable=(FCaseSensitive) ? NULL : GetUpCaseTable(-1);
		PrepareBMHSearchA(OEMTextUpcase.data(), OEMTextUpcase.size());
		if (BMHSearchA(Buffer, Size, OEMTextUpcase.data(), OEMTextUpcase.size(), OEMTable) >= 0) return TRUE;
	}
#else
	if (FindTextInBufferWithTable(Buffer, Size, TextUpcase, Table)) return TRUE;
#endif

	if (!FAllCharTables) return FALSE;

#ifdef UNICODE
	for (cp_set::iterator it = g_setAllCPs.begin(); it != g_setAllCPs.end(); it++) {
		UINT nCP = *it;
		if (nCP == (g_bDefaultOEM ? GetOEMCP() : GetACP())) continue;
		if ((nCP == CP_UNICODE) || (nCP == CP_REVERSEBOM) || (nCP == CP_UTF8)) continue;

		string strTextUpcase = StrFromUnicode(TextUpcase, nCP);
		if (StrToUnicode(strTextUpcase, nCP) != TextUpcase) continue;

		char *szTable=(FCaseSensitive) ? NULL : GetUpCaseTable(nCP);
		PrepareBMHSearchA(strTextUpcase.data(), strTextUpcase.size());
		if (BMHSearchA(Buffer, Size, strTextUpcase.data(), strTextUpcase.size(), szTable) >= 0) return TRUE;
	}
#else
	for (size_t I=0; I<XLatTables.size(); I++) {
		Table = (char *)((FCaseSensitive) ? XLatTables[I].DecodeTable : XLatTables[I].UpperDecodeTable);
		if (FindTextInBufferWithTable(Buffer, Size, TextUpcase, Table)) return TRUE;
	}
#endif

	// Restore initial
	Table = (FCaseSensitive) ? NULL : UpCaseTable;

	// No signature, but anyway Unicode?
	if (
#ifdef UNICODE
		(g_setAllCPs.find(CP_UNICODE) != g_setAllCPs.end()) &&
#endif
		FromUnicodeLE(Buffer, Size, arrData)) {
		if ((arrData.size() > 0) && FindTextInBufferWithTable(&arrData[0], arrData.size(), TextUpcase, Table)) return TRUE;
	}
	if (
#ifdef UNICODE
		(g_setAllCPs.find(CP_REVERSEBOM) != g_setAllCPs.end()) &&
#endif
		FromUnicodeBE(Buffer, Size, arrData)) {
		if ((arrData.size() > 0) && FindTextInBufferWithTable(&arrData[0], arrData.size(), TextUpcase, Table)) return TRUE;
	}
	if (
#ifdef UNICODE
		(g_setAllCPs.find(CP_UTF8) != g_setAllCPs.end()) &&
#endif
		FromUTF8(Buffer, Size, arrData)) {
		if ((arrData.size() > 0) && FindTextInBufferWithTable(&arrData[0], arrData.size(), TextUpcase, Table)) return TRUE;
	}

	return FALSE;
}

BOOL FindPlainText(const char *Buffer,int Size) {
	return FindTextInBuffer(Buffer, Size, FText);
}

//////////////////////////////////////////////////////////////////////////

BOOL FindPattern(const char *Buffer, int Size, eLikeUnicode nUni, bool bNeedColumn) {
	if (Size == 0) return FALSE;

//	NOTE
//	We don't do MatchDone() here, since it'll require potentially heavy SetSource()
//	And not really needed - we'll get named parameters only from last file

	if (nUni != UNI_NONE) {
		vector<TCHAR> arrData;
		if (FromUnicodeDetect(Buffer, Size, arrData, nUni)) {
			if ((arrData.size() > 0) && (do_pcre_exec(FPattern, FPatternExtra, &arrData[0], arrData.size(), 0, 0, NULL, 0) >= 0)) return TRUE;
		}
		return FALSE;
	}

#ifdef UNICODE
	int nResult = do_pcre_execA(FPatternA, FPatternExtraA, Buffer, Size, 0, 0, REParamA.Match(), REParamA.Count());
#else
	int nResult = do_pcre_exec(FPattern, FPatternExtra, Buffer, Size, 0, 0, REParamA.Match(), REParamA.Count());
#endif
	if (nResult >= 0) {
//		OEMMatchDone();
		if (bNeedColumn) g_nFoundColumn = REParamA.m_arrMatch[0]+1;
		return TRUE;
	}

	if (!FAllCharTables) return FALSE;

#ifdef UNICODE
	hack_string strData(Buffer, Size);
	for (cp_set::iterator it = g_setAllCPs.begin(); it != g_setAllCPs.end(); it++) {
		UINT nCP = *it;
		if (nCP == (g_bDefaultOEM ? GetOEMCP() : GetACP())) continue;
		if ((nCP == CP_UNICODE) || (nCP == CP_REVERSEBOM)) continue;

		wstring wstrData = StrToUnicode(strData, nCP);
		if (do_pcre_exec(FPattern, FPatternExtra, wstrData.data(), wstrData.size(), 0, 0, NULL, 0) >= 0) return TRUE;
	}
#else
	vector<char> SaveBuf(Size);
	for (size_t I=0; I < XLatTables.size(); I++) {
		memmove(&SaveBuf[0], Buffer, Size);
		XLatBuffer((BYTE *)&SaveBuf[0], Size, I);
		if (do_pcre_exec(FPattern, FPatternExtra, &SaveBuf[0], Size, 0, 0, NULL, 0) >= 0) return TRUE;
	}
#endif

	vector<TCHAR> arrData;

	if ((nUni != UNI_LE) && FromUnicodeLE(Buffer, Size, arrData)
#ifdef UNICODE
		&& (g_setAllCPs.find(CP_UNICODE) != g_setAllCPs.end())
#endif
		) {
		if ((arrData.size() > 0) && (do_pcre_exec(FPattern, FPatternExtra, &arrData[0], arrData.size(), 0, 0, NULL, 0) >= 0)) return TRUE;
	}
	if ((nUni != UNI_BE) && FromUnicodeBE(Buffer, Size, arrData)
#ifdef UNICODE
		&& (g_setAllCPs.find(CP_REVERSEBOM) != g_setAllCPs.end())
#endif
		) {
			if ((arrData.size() > 0) && (do_pcre_exec(FPattern, FPatternExtra, &arrData[0], arrData.size(), 0, 0, NULL, 0) >= 0)) return TRUE;
	}
#ifndef UNICODE
	if ((nUni != UNI_UTF8) && FromUTF8(Buffer, Size, arrData)) {
		if ((arrData.size() > 0) && (do_pcre_exec(FPattern, FPatternExtra, &arrData[0], arrData.size(), 0, 0, NULL, 0) >= 0)) return TRUE;
	}
#endif

	return FALSE;
}

BOOL FindRegExpWithEncoding(const char *Buffer, int Size, eLikeUnicode nUni) {
	const char *BufEnd = Buffer;
	int nRest = (nUni == UNI_NONE) || (nUni == UNI_UTF8) ? 0 : 1;
	do {
		Buffer=BufEnd;
		SkipNoCRLF(BufEnd, &Size, nUni);
		if (FindPattern(Buffer, BufEnd-Buffer, nUni, true)) return TRUE;
		SkipCRLF(BufEnd, &Size, nUni);
		g_nFoundLine++;
	} while (Size > nRest);
	return FALSE;
}

BOOL FindSeveralLineRegExpWithEncoding(const char *Buffer, int Size, eLikeUnicode nUni) {
	const char *BufEnd=Buffer;
	int nRest = (nUni == UNI_NONE) || (nUni == UNI_UTF8) ? 0 : 1;
	int LinesIn=0,Len;
	do {
		SkipNoCRLF(BufEnd, &Size, nUni);
		SkipCRLF(BufEnd, &Size, nUni);
		LinesIn++;
		if (LinesIn == SeveralLines) {
			Len=BufEnd-Buffer;
			if (FindPattern(Buffer, Len, nUni, false)) return TRUE;
			SkipWholeLine(Buffer, &Len, nUni);
			g_nFoundLine++;
			LinesIn--;
		}
	} while (Size > nRest);

	Len=BufEnd-Buffer;
	while (Len) {
		if (FindPattern(Buffer, Len, nUni, false)) return TRUE;
		SkipWholeLine(Buffer, &Len, nUni);
		g_nFoundLine++;
	}
	return FALSE;
}

BOOL FindMultiLineRegExpWithEncoding(const char *Buffer, int Size, eLikeUnicode nUni) {
	return FindPattern(Buffer, Size, nUni, false);
}

BOOL FindExamineEncoding(const char *Buffer, int Size, BOOL (*Searcher)(const char *, int, eLikeUnicode)) {
	eLikeUnicode nDetect = UNI_NONE;

	nDetect = LikeUnicode(Buffer, Size);
	if (nDetect != UNI_NONE) {
		int nSkip = (nDetect == UNI_UTF8) ? 3 : 2;
		if (Searcher(Buffer+nSkip, Size-nSkip, nDetect)) return TRUE;
#ifdef TRY_ENCODINGS_WITH_BOM
		if (!FAllCharTables) return FALSE;
#else
		return FALSE;
#endif
	}

	// Checking existing chartables is per-line
	if (Searcher(Buffer, Size, UNI_NONE)) return TRUE;

	if (FAllCharTables) {		// No signature, but anyway Unicode?
		if ((nDetect != UNI_LE)   && Searcher(Buffer, Size, UNI_LE))   return TRUE;
		if ((nDetect != UNI_BE)   && Searcher(Buffer, Size, UNI_BE))   return TRUE;
		if ((nDetect != UNI_UTF8) && Searcher(Buffer, Size, UNI_UTF8)) return TRUE;
	}

	return FALSE;
}

BOOL FindRegExp(const char *Buffer, int Size) {
	return FindExamineEncoding(Buffer, Size, FindRegExpWithEncoding);
}

BOOL FindSeveralLineRegExp(const char *Buffer, int Size) {
	return FindExamineEncoding(Buffer, Size, FindSeveralLineRegExpWithEncoding);
}

BOOL FindMultiLineRegExp(const char *Buffer,int Size) {
	return FindExamineEncoding(Buffer, Size, FindMultiLineRegExpWithEncoding);
}

//////////////////////////////////////////////////////////////////////////

BOOL FindRegExpInBuffer(const char *Buffer,int Size,tstring &Text) {
	if (!Text[0]) return TRUE;
	pcre *Pattern;
	pcre_extra *PatternExtra;
	if (PreparePattern(&Pattern, &PatternExtra, Text, FCaseSensitive, OEMCharTables)) {
		BOOL Return = FindExamineEncoding(Buffer, Size, FindRegExpWithEncoding);
		pcre_free(Pattern);
		pcre_free(PatternExtra);
		return Return;
	} else {g_bInterrupted=TRUE;return FALSE;}
}

BOOL FindMulti(const char *Buffer,int Size,BOOL (*Searcher)(const char *,int,tstring &)) {
	bool Return = true;
	bool WereAnyMaybes = false;
	bool AnyMaybesFound = false;

	for (size_t nWord = 0; nWord < FSWords.size(); nWord++) {
		if (Interrupted()) {
			Return = false;
			break;
		}

		tstring Word = FSWords[nWord];
		if (Word.empty()) continue;

		if (Word[0]=='+') {
			Word.erase(0,1);
			if (!Searcher(Buffer,Size,Word)) {Return=false;break;} else continue;
		}
		if (Word[0]=='-') {
			Word.erase(0,1);
			if (Searcher(Buffer,Size,Word)) {Return=false;break;} else continue;
		}

		WereAnyMaybes=true;
		if (AnyMaybesFound) continue;
		if (Searcher(Buffer,Size,Word)) AnyMaybesFound=true;
	}

	// All OK with Require end Exclude
	Return &= !WereAnyMaybes || AnyMaybesFound;

	return Return;
}

BOOL FindMultiPlainText(const char *Buffer,int Size) {return FindMulti(Buffer,Size,FindTextInBuffer);}

BOOL FindMemoryMapped(TCHAR *FileName,BOOL (*Searcher)(const char *,int)) {
	CFileMapping mapFile;
	if (!mapFile.Open(FileName)) {
		const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),FileName,GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FSOpenError"),Lines,4,1);
		return FALSE;
	}

	DWORD FileSize = mapFile.Size();
	if (FAdvanced && FASearchHead && (FileSize>FASearchHeadLimit)) FileSize=FASearchHeadLimit;
	if ((FSearchAs == SA_PLAINTEXT) && (FileSize < FText.length())) return FALSE;

//	Pre-cache file
//	for (size_t n = 0; n < FileSize/4; n++) __asm mov eax, dword ptr [n];
//	for (size_t n = 0; n < FileSize;   n++) __asm mov al, byte ptr [n];

	return Searcher(mapFile, FileSize);
}

void SearchFile(WIN32_FIND_DATA *FindData, panelitem_vector &PanelItems) {
	BOOL IsFound;

	InitFoundPosition();

	if (FindData->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
		if (FText.empty()) AddFile(FindData, PanelItems);
		return;
	}

	REParam.Clear();
	if (FPattern) REParam.AddRE(FPattern);
#ifdef UNICODE
	REParamA.Clear();
	if (FPattern) REParamA.AddRE(FPattern);
#endif

	if (FText.empty()) IsFound=TRUE; else 
	if (FindData->nFileSizeLow==0) IsFound=FALSE; else 
	switch (FSearchAs) {
	case SA_PLAINTEXT		:IsFound=FindMemoryMapped(FindData->cFileName,FindPlainText);break;
	case SA_REGEXP			:IsFound=FindMemoryMapped(FindData->cFileName,FindRegExp);break;
	case SA_SEVERALLINE		:IsFound=FindMemoryMapped(FindData->cFileName,FindSeveralLineRegExp);break;
	case SA_MULTILINE		:IsFound=FindMemoryMapped(FindData->cFileName,FindMultiLineRegExp);break;
	case SA_MULTITEXT		:IsFound=FindMemoryMapped(FindData->cFileName,FindMultiPlainText);break;
	default:IsFound=FALSE;
	}

	// This is exclusive 'OR'...
	if ((IsFound) ? !FSInverse : FSInverse) AddFile(FindData, PanelItems, true);
}

bool PrepareFileSearchPattern() {
	if (!FPreparePattern(true)) return false;
	if (FAdvanced) {
		if (!CompileAdvancedSettings()) return false;
	}
	return true;
}

void UpdateFSDialog(CFarDialog *pDlg) {
	bool bRegExp = !pDlg->IsDlgItemChecked(MPlainText);

	if (pDlg->HasItem(MMultiPlainText))
		bRegExp = bRegExp && !pDlg->IsDlgItemChecked(MMultiPlainText);

	if (pDlg->HasItem(MQuoteSearch))
		pDlg->EnableDlgItem(MQuoteSearch, bRegExp);

	if (pDlg->HasItem(MQuoteReplace))
		pDlg->EnableDlgItem(MQuoteReplace, bRegExp || g_bEscapesInPlainText);
}

LONG_PTR WINAPI FileSearchDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2) {
	int nCtlID = pDlg->GetID(nParam1);

	switch (nMsg) {
	case DN_INITDIALOG:
		UpdateFSDialog(pDlg);
		HighlightREError(pDlg);
		break;
	case DN_BTNCLICK:
		switch (nCtlID) {
		case MPlainText:
		case MRegExp:
		case MSeveralLineRegExp:
		case MMultiLineRegExp:
		case MMultiPlainText:
		case MMultiRegExp:
			UpdateFSDialog(pDlg);
			break;
		}
		break;
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

int SearchPrompt(BOOL Plugin) {
	CFarDialog Dialog(76,24,_T("FileSearchDlg"));
	Dialog.SetWindowProc(FileSearchDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

	Dialog.AddFrame(MRESearch);

	Dialog.Add(new CFarCheckBoxItem(35,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("Masks"), MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), SearchText));
	Dialog.Add(new CFarButtonItem(67,5,0,0,MQuoteSearch));

	Dialog.Add(new CFarTextItem(5,6,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarRadioButtonItem(5,7,DIF_GROUP,MPlainText,(int *)&FSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,8,0,MRegExp,			(int *)&FSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,9,0,MSeveralLineRegExp,(int *)&FSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,MMultiLineRegExp,	(int *)&FSearchAs,SA_MULTILINE));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MMultiPlainText,	(int *)&FSearchAs,SA_MULTITEXT));
	Dialog.Add(new CFarRadioButtonItem(5,12,DIF_DISABLE,MMultiRegExp,		(int *)&FSearchAs,SA_MULTIREGEXP));

	Dialog.Add(new CFarCheckBoxItem(5,14,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,15,0,MInverseSearch,&FSInverse));
	Dialog.Add(new CFarCheckBoxItem(5,16,0,MAllCharTables,&FAllCharTables));

	Dialog.Add(new CFarTextItem(5,18,0,MSearchIn));
	if (Plugin) {
		if (FSearchIn<SI_FROMCURRENT) FSearchIn=SI_FROMCURRENT;
		Dialog.Add(new CFarComboBoxItem(15,18,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearchPlugin, false),(int *)&FSearchIn,NULL,3));
	} else {
		Dialog.Add(new CFarComboBoxItem(15,18,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));
	}

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,7,0,0,MBtnPresets));
	Dialog.Add(new CFarCheckBoxItem(56,9,0,_T(""),&FAdvanced));
	Dialog.Add(new CFarButtonItem(60,9,0,0,MBtnAdvanced));
	Dialog.SetFocus(MMask, 1);
	FACaseSensitive=FADirectoryCaseSensitive=MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(-1)) {
		case MOk:
			FMask=MaskText;
			FText=SearchText;
			break;
		case MQuoteSearch:
			CSO::QuoteRegExpString(SearchText);
			break;
		case MBtnPresets:
			FSPresets->ShowMenu(true);
			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
			break;
		case MBtnAdvanced:
			if (AdvancedSettings()) FAdvanced=TRUE;
			break;
		default:
			return FALSE;
		}
	} while ((ExitCode != MOk) || !PrepareFileSearchPattern());

	return TRUE;
}

OperationResult FileFind(panelitem_vector &PanelItems, BOOL ShowDialog, BOOL bSilent) {
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.PanelType!=PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin&&((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;

	if (ShowDialog) {
		if (!SearchPrompt(PInfo.Plugin)) return OR_CANCEL;
	} else {
		if (!PrepareFileSearchPattern()) return OR_CANCEL;
	}

	CDebugTimer tm(_T("FileFind() took %d ms"));
	int nResult = ScanDirectories(PanelItems, SearchFile);
	tm.Stop();

	if (nResult) {
		return (PanelItems.empty()) ? (bSilent ? OR_OK : NoFilesFound()) : OR_PANEL;
	} else return OR_FAILED;
}

OperationResult FileSearchExecutor() {
	FMask = MaskText;
	FText = SearchText;
	return FileFind(g_PanelItems, FALSE, TRUE);
}

BOOL CFSPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,25,_T("FSPresetDlg"));
	Dialog.AddFrame(MFSPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"), pPreset->Name()));

	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));
	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("Masks"), pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));

	int *pSearchAs = &pPreset->m_mapInts["SearchAs"];
	int  nAdvancedID = pPreset->m_mapInts["AdvancedID"];
	bool bFAdvanced = nAdvancedID > 0;

	Dialog.Add(new CFarRadioButtonItem(5, 9,DIF_GROUP,MPlainText,pSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,		MRegExp,	 pSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,		MSeveralLineRegExp,	pSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,		MMultiLineRegExp,	pSearchAs,SA_MULTILINE));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,		MMultiPlainText,	pSearchAs,SA_MULTITEXT));
	Dialog.Add(new CFarRadioButtonItem(5,14,DIF_DISABLE,MMultiRegExp,	pSearchAs,SA_MULTIREGEXP));

	Dialog.Add(new CFarCheckBoxItem(5,16,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(5,17,0,MInverseSearch,&pPreset->m_mapInts["Inverse"]));
	Dialog.Add(new CFarCheckBoxItem(56,9,0,_T(""),&bFAdvanced));
	Dialog.Add(new CFarButtonItem(60,9,0,0,MBtnAdvanced));

	Dialog.Add(new CFarCheckBoxItem(5,19,0,MAddToMenu,&pPreset->m_bAddToMenu));
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
