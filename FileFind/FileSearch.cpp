#include "FileFind.h"

void XLatBuffer(BYTE *Buffer,int Length,int Table) {
	for (register int I=0;I<Length;I++) Buffer[I]=XLatTables[Table][Buffer[I]];
}

BOOL FindTextInBufferWithTable(char *Buffer,int Size,string &Text,char *Table) {
	string TextUpcase=Text;

	int Position=BMHSearch(Buffer,Size,FTextUpcase.data(),FTextUpcase.size(),Table);
	return Position>=0;
}

BOOL FindTextInBuffer(char *Buffer,int Size,string &Text) {
	char *Table=(FCaseSensitive)?NULL:UpCaseTable;
	if (FindTextInBufferWithTable(Buffer,Size,Text,Table)) return TRUE;

	if (FAllCharTables) {
		for (int I=0;I<XLatTableCount;I++) {
			Table=(FCaseSensitive)?XLatTables[I]:UpCaseXLatTables[I];
			if (FindTextInBufferWithTable(Buffer,Size,Text,Table)) return TRUE;
		}
	}

	return FALSE;
}

BOOL FindPlainText(char *Buffer,int Size) {
	return FindTextInBuffer(Buffer,Size,FText);
}

BOOL FindPattern(pcre *Pattern,pcre_extra *PatternExtra,char *Buffer,int Length) {
	if (do_pcre_exec(Pattern,PatternExtra,Buffer,Length,0,0,NULL,0)>=0) return TRUE;
	if (!FAllCharTables||!XLatTableCount) return FALSE;

	char *SaveBuf=(char *)malloc(Length);
	for (int I=0;I<XLatTableCount;I++) {
		memmove(SaveBuf,Buffer,Length);
		XLatBuffer((BYTE *)SaveBuf,Length,I);
		if (do_pcre_exec(Pattern,PatternExtra,SaveBuf,Length,0,0,NULL,0)>=0) {
			free(SaveBuf);return TRUE;
		}
	}
	return FALSE;
}

BOOL FindRegExp(char *Buffer,int Size) {
	char *BufEnd=Buffer;
	do {
		Buffer=BufEnd;
		SkipNoCRLF(BufEnd,&Size);
		if (FindPattern(FPattern,FPatternExtra,Buffer,BufEnd-Buffer)) return TRUE;
		SkipCRLF(BufEnd,&Size);
	} while (Size);
	return FALSE;
}

BOOL FindSeveralLineRegExp(char *Buffer,int Size) {
	char *BufEnd=Buffer;
	int LinesIn=0,Len;
	do {
		SkipNoCRLF(BufEnd,&Size);
		SkipCRLF(BufEnd,&Size);
		LinesIn++;
		if (LinesIn==SeveralLines) {
			Len=BufEnd-Buffer;
			if (FindPattern(FPattern,FPatternExtra,Buffer,Len)) return TRUE;
			SkipWholeLine(Buffer,&Len);
			LinesIn--;
		}
	} while (Size);

	Len=BufEnd-Buffer;
	while (Len) {
		if (FindPattern(FPattern,FPatternExtra,Buffer,Len)) return TRUE;
		SkipWholeLine(Buffer,&Len);
	}
	return FALSE;
}

BOOL FindMultiLineRegExp(char *Buffer,int Size) {
	return (do_pcre_exec(FPattern,FPatternExtra,Buffer,Size,0,0,NULL,0)>=0);
}

BOOL FindRegExpInBuffer(char *Buffer,int Size,string &Text) {
	if (!Text[0]) return TRUE;
	pcre *Pattern;
	pcre_extra *PatternExtra;
	if (PreparePattern(&Pattern,&PatternExtra,Text,FCaseSensitive)) {
		BOOL Return=FindPattern(Pattern,PatternExtra,Buffer,Size);
		pcre_free(Pattern);pcre_free(PatternExtra);
		return Return;
	} else {Interrupt=TRUE;return FALSE;}
}

BOOL FindMulti(char *Buffer,int Size,BOOL (*Searcher)(char *,int,string &)) {
	string What=FText;
	string Word;
	BOOL Return=TRUE,WereAnyMaybes=FALSE,AnyMaybesFound=FALSE;

	do {
		GetStripWord(What,Word);
		if (Word.size()==0) break;
		if (Word[0]=='+') {
			Word.erase(0,1);
			if (!Searcher(Buffer,Size,Word)) {Return=FALSE;break;} else continue;
		}
		if (Word[0]=='-') {
			Word.erase(0,1);
			if (Searcher(Buffer,Size,Word)) {Return=FALSE;break;} else continue;
		}
		WereAnyMaybes=TRUE;
		if (AnyMaybesFound) continue;
		if (Searcher(Buffer,Size,Word)) AnyMaybesFound=TRUE;
	} while (Word.size()&&(!Interrupt));

	// All OK with Require end Exclude
	if (Interrupt) Return=FALSE;
	if (Return) Return=(!WereAnyMaybes)||AnyMaybesFound;
	return Return;
}

BOOL FindMultiPlainText(char *Buffer,int Size) {return FindMulti(Buffer,Size,FindTextInBuffer);}
BOOL FindMultiRegExp(char *Buffer,int Size) {return FindMulti(Buffer,Size,FindRegExpInBuffer);}

BOOL FindMemoryMapped(char *FileName,BOOL (*Searcher)(char *,int)) {
	BOOL IsFound;

	__try {
		HANDLE hFile=CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile==INVALID_HANDLE_VALUE) throw;
		__try {
			HANDLE hMap=CreateFileMapping(hFile,NULL,PAGE_READONLY|SEC_COMMIT,0,0,NULL);
			if (hMap==NULL) throw;
			__try {
				char *FileData=(char *)MapViewOfFile(hMap,FILE_MAP_READ,0,0,0);
				if (FileData==NULL) throw;

				DWORD FileSize=GetFileSize(hFile,NULL);
				if (FAdvanced && FASearchHead && (FileSize>FASearchHeadLimit)) FileSize=FASearchHeadLimit;
				IsFound=Searcher(FileData,FileSize);
				UnmapViewOfFile(FileData);
			} __finally {CloseHandle(hMap);}
		} __finally {CloseHandle(hFile);}
	} __except (1) {
		const char *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),FileName,GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FSOpenError",Lines,4,1);
		return FALSE;
	}

	return IsFound;
}

void SearchFile(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber) {
	BOOL IsFound;
	if (FindData->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
		if (FText[0]==0) AddFile(FindData,PanelItems,ItemsNumber);
		return;
	}
	if (FText[0]==0) IsFound=TRUE; else 
	if (FindData->nFileSizeLow==0) IsFound=FALSE; else 
	switch (FSearchAs) {
	case SA_PLAINTEXT		:IsFound=FindMemoryMapped(FindData->cFileName,FindPlainText);break;
	case SA_REGEXP			:IsFound=FindMemoryMapped(FindData->cFileName,FindRegExp);break;
	case SA_SEVERALLINE		:IsFound=FindMemoryMapped(FindData->cFileName,FindSeveralLineRegExp);break;
	case SA_MULTILINE		:IsFound=FindMemoryMapped(FindData->cFileName,FindMultiLineRegExp);break;
	case SA_MULTITEXT		:IsFound=FindMemoryMapped(FindData->cFileName,FindMultiPlainText);break;
	case SA_MULTIREGEXP		:IsFound=FindMemoryMapped(FindData->cFileName,FindMultiRegExp);break;
	default:IsFound=FALSE;
	}
	// This is exclusive 'OR'...
	if ((IsFound)?!FSInverse:FSInverse) AddFile(FindData,PanelItems,ItemsNumber);
}

int SearchPrompt(BOOL Plugin) {
	CFarDialog Dialog(76,25,"FileSearchDlg");
	Dialog.AddFrame(MRESearch);

	Dialog.Add(new CFarCheckBoxItem(35,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"Masks", MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY,"SearchText", SearchText));
	Dialog.Add(new CFarButtonItem(67,5,0,0,"&\\"));

	Dialog.Add(new CFarTextItem(5,6,DIF_BOXCOLOR|DIF_SEPARATOR,(char *)NULL));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarRadioButtonItem(5,8,DIF_GROUP,MPlainText,(int *)&FSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,9,0,MRegExp,			(int *)&FSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,MSeveralLineRegExp,(int *)&FSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MMultiLineRegExp,	(int *)&FSearchAs,SA_MULTILINE));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,MMultiPlainText,	(int *)&FSearchAs,SA_MULTITEXT));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,MMultiRegExp,		(int *)&FSearchAs,SA_MULTIREGEXP));
	Dialog.Add(new CFarCheckBoxItem(5,14,0,MInverseSearch,&FSInverse));
	Dialog.Add(new CFarCheckBoxItem(35,14,0,MAllCharTables,&FAllCharTables));

	Dialog.Add(new CFarTextItem(5,15,DIF_BOXCOLOR|DIF_SEPARATOR,""));
	if (Plugin) {
		if (FSearchIn<SI_FROMCURRENT) FSearchIn=SI_FROMCURRENT;
		Dialog.Add(new CFarRadioButtonItem(5,16,DIF_GROUP,MFromCurrent,		(int *)&FSearchIn,SI_FROMCURRENT));
		Dialog.Add(new CFarRadioButtonItem(5,17,0,MCurrentOnly,		(int *)&FSearchIn,SI_CURRENTONLY));
		Dialog.Add(new CFarRadioButtonItem(5,18,0,MSelected,		(int *)&FSearchIn,SI_SELECTED));
	} else {
		Dialog.Add(new CFarRadioButtonItem(5,16,DIF_GROUP,MAllDrives,(int *)&FSearchIn,SI_ALLDRIVES));
		Dialog.Add(new CFarRadioButtonItem(5,17,0,MAllLocalDrives,	(int *)&FSearchIn,SI_ALLLOCAL));
		Dialog.Add(new CFarRadioButtonItem(5,18,0,MFromRoot,		(int *)&FSearchIn,SI_FROMROOT));
		Dialog.Add(new CFarRadioButtonItem(5,19,0,MFromCurrent,		(int *)&FSearchIn,SI_FROMCURRENT));
		Dialog.Add(new CFarRadioButtonItem(5,20,0,MCurrentOnly,		(int *)&FSearchIn,SI_CURRENTONLY));
		Dialog.Add(new CFarRadioButtonItem(5,21,0,MSelected,		(int *)&FSearchIn,SI_SELECTED));
	}

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,7,0,0,MPresets));
	Dialog.Add(new CFarCheckBoxItem(56,9,0,"",&FAdvanced));
	Dialog.Add(new CFarButtonItem(60,9,0,0,MAdvanced));
	Dialog.Add(new CFarCheckBoxItem(56,11,0,"",&FUTF8));
	Dialog.Add(new CFarButtonItem(60,11,0,0,MUTF8));
	Dialog.SetFocus(3);
	FACaseSensitive=FADirectoryCaseSensitive=MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(5,-7,6,-5,-3,-1)) {
		case 0:
			FMask=MaskText;
			FText=SearchText;
			break;
		case 1:
			if ((FSearchAs!=SA_PLAINTEXT) && (FSearchAs!=SA_MULTITEXT)) QuoteRegExpString(SearchText);
			break;
		case 2:
			FSPresets->ShowMenu(g_FSBatch);
			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
			break;
		case 3:
			if (AdvancedSettings()) FAdvanced=TRUE;
			break;
		case 4:
			UTF8Converter(SearchText);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!FPreparePattern());
	if (FUTF8) FAllCharTables=FALSE;
	return TRUE;
}

OperationResult FileFind(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog) {
	PanelInfo PInfo;
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
	if (PInfo.PanelType!=PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin&&((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;

	if (ShowDialog) {
		if (!SearchPrompt(PInfo.Plugin)) return OR_CANCEL;
	} else {
		if (!FPreparePattern()) return OR_CANCEL;
	}

	if (ScanDirectories(PanelItems,ItemsNumber,SearchFile)) {
		if (*ItemsNumber==0) return NoFilesFound(); else return OR_PANEL;
	} else return OR_FAILED;
}

BOOL CFSPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,22,"FSPresetDlg");
	Dialog.AddFrame(MFSPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"RESearch.PresetName", pPreset->m_strName));

	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));
	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"Masks", pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,"SearchText", pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,9,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	int *pSearchAs = &pPreset->m_mapInts["SearchAs"];
	Dialog.Add(new CFarRadioButtonItem(5,10,DIF_GROUP,MPlainText,pSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,		MRegExp,	 pSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,		MSeveralLineRegExp,	pSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,		MMultiLineRegExp,	pSearchAs,SA_MULTILINE));
	Dialog.Add(new CFarRadioButtonItem(5,14,0,		MMultiPlainText,	pSearchAs,SA_MULTITEXT));
	Dialog.Add(new CFarRadioButtonItem(5,15,0,		MMultiRegExp,		pSearchAs,SA_MULTIREGEXP));
	Dialog.Add(new CFarCheckBoxItem(5,16,0,MInverseSearch,&pPreset->m_mapInts["Inverse"]));
	Dialog.Add(new CFarCheckBoxItem(35,9,0,"",&pPreset->m_mapInts["UTF8"]));
	Dialog.Add(new CFarButtonItem(39,9,0,0,MUTF8));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(2, -2, -3)) {
		case 0:
			return TRUE;
		case 1:
			UTF8Converter(pPreset->m_mapStrings["Text"]);
			break;
		default:
			return FALSE;
		}
	} while (true);
}
