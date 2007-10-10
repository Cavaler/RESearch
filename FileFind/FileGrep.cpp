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

bool GrepLineFound(const sBufferedLine &strBuf) {
	if (FSearchAs == SA_REGEXP) {
		return FindPattern(FPattern, FPatternExtra, strBuf.szBuffer, strBuf.Length()) != 0;
	} else {
		return FindTextInBuffer(strBuf.szBuffer, strBuf.Length(), FText) != 0;
	}
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
	const char *szBuffer = mapFile;
	const char *szBufEnd = szBuffer;
	g_nFoundLine = 0;

	int nFoundCount = 0;
	int nLastMatched = -1;

	int nContextLines = FGAddContext ? FGContextLines : 0;
	while (FileSize) {
		szBuffer = szBufEnd;
		SkipNoCRLF(szBufEnd, &FileSize);

		arrStringBuffer.push_back(sBufferedLine(szBuffer, szBufEnd));
		if (arrStringBuffer.size() > nContextLines*2+1) arrStringBuffer.pop_front();

		if (arrStringBuffer.size() == nContextLines*2+1) {
			const sBufferedLine &strTest = arrStringBuffer[nContextLines];

			if (GrepLineFound(strTest)) {
				nFoundCount++;

				switch (FGrepWhat) {
				case GREP_NAMES:
					AddGrepLine(FindData->cFileName);
					return;
				case GREP_NAMES_LINES:
					if (nFoundCount == 1) AddGrepLine(FindData->cFileName);
					// break;
				case GREP_LINES:
					if (FGAddLineNumbers) {
						AddGrepLine(FormatStr("%d:", g_nFoundLine-nContextLines).c_str(), false);
					}
					AddGrepLine(string(strTest.szBuffer, strTest.szBufEnd).c_str());
					break;
				}
			}
		}

		SkipCRLF(szBufEnd, &FileSize);
		g_nFoundLine++;
	}

	switch (FGrepWhat) {
	case GREP_NAMES_COUNT:
		if (nFoundCount > 0) AddGrepLine(FormatStr("%s:%d", FindData->cFileName, nFoundCount).c_str());
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
	Dialog.Add(new CFarRadioButtonItem(5,11,DIF_GROUP,MGrepNamesCount,	(int *)&FGrepWhat,GREP_NAMES_COUNT));
	Dialog.Add(new CFarRadioButtonItem(5,12,DIF_GROUP,MGrepLines,		(int *)&FGrepWhat,GREP_LINES));
	Dialog.Add(new CFarRadioButtonItem(5,13,DIF_GROUP,MGrepNamesLines,	(int *)&FGrepWhat,GREP_NAMES_LINES));

	Dialog.Add(new CFarCheckBoxItem(5,14,0,MGrepAdd,&FGAddContext));
	Dialog.Add(new CFarEditItem(15,14,20,0,NULL,(int &)FGContextLines,new CFarIntegerRangeValidator(0,1024)));
	Dialog.Add(new CFarTextItem(22,14,0,MGrepContext));
	Dialog.Add(new CFarCheckBoxItem(5,15,0,MGrepAddLineNumbers,&FGAddLineNumbers));

	Dialog.Add(new CFarCheckBoxItem(5,16,0,MGrepOutput,&FGOutputToFile));
	Dialog.Add(new CFarEditItem(20,16,45,DIF_HISTORY,"RESearch.GrepOutput", FGOutputFile));
	Dialog.Add(new CFarCheckBoxItem(5,17,0,MGrepEditor,&FGOpenInEditor));

	Dialog.Add(new CFarTextItem(5,19,0,MSearchIn));
	Dialog.Add(new CFarComboBoxItem(15,19,45,0,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,10,0,0,MPresets));
	Dialog.Add(new CFarCheckBoxItem(56,11,0,"",&FAdvanced));
	Dialog.Add(new CFarButtonItem(60,11,0,0,MAdvanced));
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
//			FSPresets->ShowMenu(g_FSBatch);
//			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
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
				EF_NONMODAL|EF_IMMEDIATERETURN|EF_ENABLE_F6|EF_DELETEONLYFILEONCLOSE, 0, 1);
		}
		return OR_OK;
	} else return OR_FAILED;
}
