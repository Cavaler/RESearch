#include "StdAfx.h"
#include "..\RESearch.h"

CHandle g_hOutput;
int     g_nLines;

struct sBufferedLine {
	sBufferedLine(const char *_Buffer, const char *_BufEnd) : szBuffer(_Buffer), szBufEnd(_BufEnd) {}
	inline int Length() const {return szBufEnd - szBuffer;}
	const char *szBuffer;
	const char *szBufEnd;
};

void AddGrepLine(const char *szLine, bool bEOL = true) {
	DWORD dwWritten;
	WriteFile(g_hOutput, szLine, strlen(szLine), &dwWritten, NULL);
	if (bEOL) WriteFile(g_hOutput, "\r\n", 2, &dwWritten, NULL);
}

void AddGrepResultLine(const sBufferedLine &Line, int nLineNumber) {
	if (FGAddLineNumbers) {
		AddGrepLine(FormatStr("%d:", nLineNumber).c_str(), false);
	}
	AddGrepLine(string(Line.szBuffer, Line.szBufEnd).c_str());
}

bool GrepLineFound(const sBufferedLine &strBuf) {
	BOOL bResult;
	if (FSearchAs == SA_REGEXP) {
		bResult = FindPattern(FPattern, FPatternExtra, strBuf.szBuffer, strBuf.Length());
	} else {
		bResult = FindTextInBuffer(strBuf.szBuffer, strBuf.Length(), FText);
	}

	return (bResult != 0) != (FSInverse != 0);
}

void GrepFile(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber) {
	if (FText.empty()) {
		AddGrepLine(FindData->cFileName);
		return;
	}

	CFileMapping mapFile;
	if (!mapFile.Open(FindData->cFileName)) {
//		const char *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),FindData->cFileName,GetMsg(MOk)};
//		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"FSOpenError",Lines,4,1);
		return;
	}

	int FileSize = mapFile.Size();
	if (FAdvanced && FASearchHead && (FileSize > (int)FASearchHeadLimit)) FileSize=FASearchHeadLimit;

	deque<sBufferedLine> arrStringBuffer;
	const char *szBuffer, *szBufEnd = mapFile;

	int nFoundCount = 0;

	int nFirstBufferLine = 0;
	int nLastMatched = -1;

	int nContextLines = FGAddContext ? FGContextLines : 0;
	while (FileSize) {
		szBuffer = szBufEnd;
		SkipNoCRLF(szBufEnd, &FileSize);

		arrStringBuffer.push_back(sBufferedLine(szBuffer, szBufEnd));

		if (GrepLineFound(arrStringBuffer.back())) {
			nFoundCount++;
			nLastMatched = arrStringBuffer.size()-1;

			switch (FGrepWhat) {
			case GREP_NAMES:
				AddGrepLine(FindData->cFileName);
				return;
			case GREP_NAMES_LINES:
				if (nFoundCount == 1) AddGrepLine(FindData->cFileName);
				break;
			}
		} else {
			if (nLastMatched >= 0) {
				if (nLastMatched < (int)arrStringBuffer.size()-nContextLines*2-1) {
					switch (FGrepWhat) {
					case GREP_NAMES_LINES:
					case GREP_LINES:
						if (nContextLines > 0) AddGrepLine(">>>");
						for (int nLine = 0; nLine <= nLastMatched+nContextLines; nLine++) {
							AddGrepResultLine(arrStringBuffer[nLine], nLine + nFirstBufferLine + 1);
						}
						if (nContextLines > 0) AddGrepLine("<<<");
						break;
					}
					while ((int)arrStringBuffer.size() > nContextLines) {
						arrStringBuffer.pop_front();
						nFirstBufferLine++;
					}
					nLastMatched = -1;
				} // else not yet time for output
			} else {
				while ((int)arrStringBuffer.size() > nContextLines) {
					arrStringBuffer.pop_front();
					nFirstBufferLine++;
				}
			}
		}

		SkipCRLF(szBufEnd, &FileSize);
	}

	switch (FGrepWhat) {
	case GREP_NAMES_COUNT:
		if (nFoundCount > 0) AddGrepLine(FormatStr("%s:%d", FindData->cFileName, nFoundCount).c_str());
		break;
	case GREP_NAMES_LINES:
	case GREP_LINES:
		if (nLastMatched >= 0) {
			if (nContextLines > 0) AddGrepLine(">>>");
			for (int nLine = 0; (nLine < (int)arrStringBuffer.size()) && (nLine <= nLastMatched+nContextLines); nLine++) {
				AddGrepResultLine(arrStringBuffer[nLine], nLine + nFirstBufferLine + 1);
			}
			if (nContextLines > 0) AddGrepLine("<<<");
		}
		break;
	}
}

bool GrepPrompt(BOOL bPlugin) {
	BOOL AsRegExp = (FSearchAs == SA_REGEXP) || (FSearchAs == SA_SEVERALLINE) || (FSearchAs == SA_MULTILINE) || (FSearchAs == SA_MULTIREGEXP);

	CFarDialog Dialog(76,25,"FileGrepDlg");
	Dialog.AddFrame(MREGrep);

	Dialog.Add(new CFarCheckBoxItem(35,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"Masks", MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY,"SearchText", SearchText));
	Dialog.Add(new CFarButtonItem(67,5,0,0,"&\\"));

	Dialog.Add(new CFarTextItem(5,6,DIF_BOXCOLOR|DIF_SEPARATOR,(char *)NULL));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&AsRegExp));
	Dialog.Add(new CFarCheckBoxItem(35,7,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MInverseSearch,&FSInverse));
	Dialog.Add(new CFarCheckBoxItem(35,8,0,MAllCharTables,&FAllCharTables));
	Dialog.Add(new CFarTextItem(5,9,DIF_BOXCOLOR|DIF_SEPARATOR,""));

	Dialog.Add(new CFarRadioButtonItem(5,10,DIF_GROUP,MGrepNames,		(int *)&FGrepWhat,GREP_NAMES));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MGrepNamesCount,	(int *)&FGrepWhat,GREP_NAMES_COUNT));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,MGrepLines,		(int *)&FGrepWhat,GREP_LINES));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,MGrepNamesLines,	(int *)&FGrepWhat,GREP_NAMES_LINES));

	Dialog.Add(new CFarCheckBoxItem(5,14,0,MGrepAdd,&FGAddContext));
	Dialog.Add(new CFarEditItem(15,14,20,0,NULL,(int &)FGContextLines,new CFarIntegerRangeValidator(0,1024)));
	Dialog.Add(new CFarTextItem(22,14,0,MGrepContext));
	Dialog.Add(new CFarCheckBoxItem(5,15,0,MGrepAddLineNumbers,&FGAddLineNumbers));

	Dialog.Add(new CFarCheckBoxItem(5,16,0,MGrepOutput,&FGOutputToFile));
	Dialog.Add(new CFarEditItem(20,16,45,DIF_HISTORY,"RESearch.GrepOutput", FGOutputFile));
	Dialog.Add(new CFarCheckBoxItem(5,17,0,MGrepEditor,&FGOpenInEditor));

	Dialog.Add(new CFarTextItem(5,19,0,MSearchIn));
	Dialog.Add(new CFarComboBoxItem(15,19,60,0,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,10,0,0,MBtnPresets));
	Dialog.Add(new CFarCheckBoxItem(56,11,0,"",&FAdvanced));
	Dialog.Add(new CFarButtonItem(60,11,0,0,MBtnAdvanced));
	Dialog.Add(new CFarCheckBoxItem(56,12,0,"",&FUTF8));
	Dialog.Add(new CFarButtonItem(60,12,0,0,MUTF8));
	Dialog.SetFocus(3);
	FACaseSensitive=FADirectoryCaseSensitive=MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(5,-7,6,-5,-3,-1)) {
		case 0:
			FSearchAs = AsRegExp ? SA_REGEXP : SA_PLAINTEXT;
			FMask=MaskText;
			FText=SearchText;
			break;
		case 1:
			if (AsRegExp) QuoteRegExpString(SearchText);
			break;
		case 2:
			FGPresets->ShowMenu(true);
			break;
		case 3:
			if (AdvancedSettings()) FAdvanced=TRUE;
			break;
		case 4:
			UTF8Converter(SearchText);
			break;
		case -1:
			return false;
		}
	} while ((ExitCode>=1) || !FPreparePattern(true) || (!FGOutputToFile && !FGOpenInEditor));

	if (FUTF8) FAllCharTables=FALSE;
	return true;
}

OperationResult FileGrep(BOOL ShowDialog) {
	PanelInfo PInfo;
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
	if (PInfo.PanelType!=PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin&&((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;

	if (ShowDialog) {
		if (!GrepPrompt(PInfo.Plugin)) return OR_CANCEL;
	} else {
		if (!FPreparePattern(true)) return OR_CANCEL;
	}

	string strFileName;
	if (FGOutputToFile && !FGOutputFile.empty()) {
		strFileName = FGOutputFile;
	} else {
		char szBuffer[MAX_PATH], szName[MAX_PATH];
		GetTempPath(MAX_PATH, szBuffer);
		GetTempFileName(szBuffer, "re", 0, szName);
		strFileName = szName;
	}

	g_hOutput = CreateFile(strFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
		CREATE_ALWAYS, 0, NULL);
	if (!g_hOutput) {
		ShowLastError(GetMsg(MFileCreateError), strFileName.c_str());
		return OR_FAILED;
	}

	if (ScanDirectories(&PanelItems,&ItemsNumber,GrepFile)) {
		g_hOutput.Close();
		if (FGOpenInEditor) {
			StartupInfo.Editor(strFileName.c_str(), NULL, 0, 0, -1, -1,
				EF_NONMODAL|EF_IMMEDIATERETURN|EF_ENABLE_F6| (FGOutputToFile ? 0 : EF_DELETEONLYFILEONCLOSE), 0, 1);
		}
		return OR_OK;
	} else return OR_FAILED;
}

OperationResult FileGrepExecutor() {
	FMask = MaskText;
	FText = SearchText;

	return FileGrep(FALSE);
}

BOOL CFGPresetCollection::EditPreset(CPreset *pPreset) {
	SearchAs FSA = (SearchAs)pPreset->m_mapInts["SearchAs"];
	BOOL AsRegExp = (FSA == SA_REGEXP) || (FSA == SA_SEVERALLINE) || (FSA == SA_MULTILINE) || (FSA == SA_MULTIREGEXP);

	CFarDialog Dialog(76,25,"FGPresetDlg");
	Dialog.AddFrame(MREGrep);

	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"RESearch.PresetName", pPreset->Name()));
	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));
	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"Masks", pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,"SearchText", pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR,(char *)NULL));
	Dialog.Add(new CFarCheckBoxItem(5,9,0,MRegExp,&AsRegExp));
	Dialog.Add(new CFarCheckBoxItem(35,9,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(5,10,0,MInverseSearch,&pPreset->m_mapInts["Inverse"]));
	Dialog.Add(new CFarCheckBoxItem(35,10,0,"",&pPreset->m_mapInts["UTF8"]));
	Dialog.Add(new CFarButtonItem(39,10,0,0,MUTF8));

	Dialog.Add(new CFarTextItem(5,11,DIF_BOXCOLOR|DIF_SEPARATOR,""));

	Dialog.Add(new CFarRadioButtonItem(5,12,DIF_GROUP,MGrepNames,	&pPreset->m_mapInts["GrepWhat"],GREP_NAMES));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,MGrepNamesCount,		&pPreset->m_mapInts["GrepWhat"],GREP_NAMES_COUNT));
	Dialog.Add(new CFarRadioButtonItem(5,14,0,MGrepLines,			&pPreset->m_mapInts["GrepWhat"],GREP_LINES));
	Dialog.Add(new CFarRadioButtonItem(5,15,0,MGrepNamesLines,		&pPreset->m_mapInts["GrepWhat"],GREP_NAMES_LINES));

	Dialog.Add(new CFarCheckBoxItem(5,16,0,MGrepAdd,&pPreset->m_mapInts["AddContext"]));
	Dialog.Add(new CFarEditItem(15,16,20,0,NULL,&pPreset->m_mapInts["ContextLines"],new CFarIntegerRangeValidator(0,1024)));
	Dialog.Add(new CFarTextItem(22,16,0,MGrepContext));
	Dialog.Add(new CFarCheckBoxItem(5,17,0,MGrepAddLineNumbers,&pPreset->m_mapInts["AddLineNumbers"]));

	Dialog.Add(new CFarCheckBoxItem(5,19,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(2, -2, 13)) {
		case 0:
			pPreset->m_mapInts["SearchAs"] = AsRegExp ? SA_REGEXP : SA_PLAINTEXT;
			return TRUE;
		case 1:
			UTF8Converter(pPreset->m_mapStrings["Text"]);
			break;
		default:
			return FALSE;
		}
	} while (true);
}
