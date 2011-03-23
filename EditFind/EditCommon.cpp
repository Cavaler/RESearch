#include "StdAfx.h"
#include "..\RESearch.h"

CParameterSet g_ESParamSet(EditorSearchExecutor, 2, 4,
	"Text", &SearchText, "@Text", &EText,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine,
	"ListAll", &EListAllFromPreset
					 );
CParameterSet g_ERParamSet(EditorReplaceExecutor, 4, 7,
	"Text", &SearchText, "Replace", &ReplaceText,
	 "@Text", &EText,  "@Replace", &ERReplace,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine,
	"RemoveEmpty", &ERRemoveEmpty, "RemoveNoMatch", &ERRemoveNoMatch,
	"AsScript", &EREvaluate, "Script", &EREvaluateScript
					 );
CParameterSet g_EFParamSet(EditorFilterExecutor, 2, 3,
	"Text", &SearchText, "@Text", &EText,
	"LeaveFilter", &EFLeaveFilter, "IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive
					 );
CParameterSet g_ETParamSet(EditorTransliterateExecutor, 2, 0,
	"Text", &SearchText, "Replace", &ReplaceText,
	 "@Text", &EText,  "@Replace", &ERReplace
					 );

void EReadRegistry(HKEY Key) {
	#define DECLARE_PERSIST_LOAD Key
	#include "PersistVars.h"

	SelType=BTYPE_NONE;

	ESPresets = new CESPresetCollection(g_ESParamSet);
	ERPresets = new CERPresetCollection(g_ERParamSet);
	EFPresets = new CEFPresetCollection(g_EFParamSet);
	ETPresets = new CETPresetCollection(g_ETParamSet);
}

void EWriteRegistry(HKEY Key) {
	#define DECLARE_PERSIST_SAVE Key
	#include "PersistVars.h"
}

BOOL SearchIn(const TCHAR *Line,int Start,int Length,int *MatchStart,int *MatchLength) {
	REParam.Clear(FileMaskNamedParameters);
	REParam.AddSource(Line, Length);

	if (ERegExp) {
		REParam.AddRE(EPattern);

		if (pcre_exec(EPattern,EPatternExtra,Line,Start+Length,Start,0,REParam.Match(),REParam.Count())>=0) {
			REParam.FillStartLength(MatchStart, MatchLength);
			return TRUE;
		}
	} else {
		TCHAR *Table = ECaseSensitive ? NULL : UpCaseTable;
		int Position=(EReverse)?ReverseBMHSearch(Line+Start,Length,ETextUpcase.data(),ETextUpcase.length(),Table)
									  :BMHSearch(Line+Start,Length,ETextUpcase.data(),ETextUpcase.length(),Table);
		if (Position>=0) {
			if (MatchStart) *MatchStart=Start+Position;
			if (MatchLength) *MatchLength=EText.length();
			return TRUE;
		}
	}
	return FALSE;
}

BOOL SearchInLine(const TCHAR *Line,int Length,int Start,int End,int *MatchStart,int *MatchLength) {
	int Len;

	if (Start>Length) return FALSE;
	if (Start==-1) Start=0;
	if ((End==-1)||(End>Length)) Len=Length-Start; else Len=End-Start;
	if (Len<0) return FALSE;

#ifdef UNICODE
	return SearchIn(Line,Start,Len,MatchStart,MatchLength);
#else
	if (ERegExp) {
		return SearchIn(Line,Start,Len,MatchStart,MatchLength);
	} else {
		if (EdInfo.AnsiMode || (EdInfo.TableNum != -1)) {
			//	static - to allow REParam somewhere inside to store just a pointer, not a copy
			//	NOT using a constructor!
			static string OEMLine;
			OEMLine = string(Line, Length);
			EditorToOEM(OEMLine);
			int Result=SearchIn(OEMLine.c_str(),Start,Len,MatchStart,MatchLength);
			return Result;
		} else {
			return SearchIn(Line, Start, Len, MatchStart, MatchLength);
		}
	}
#endif
}

void AdjustPosition(TCHAR *Lines, int &FirstLine,int &StartPos)
{
	do {
		TCHAR *NewLine=(TCHAR *)_tmemchr(Lines,'\n',StartPos);
		if (NewLine) {
			int Pos = NewLine-Lines;
			Lines += Pos+1;
			StartPos -= Pos+1;
			FirstLine++;
		} else break;
	} while (TRUE);
}

void Relative2Absolute(int Line,TCHAR *Lines,int MatchStart,int MatchLength,int &FirstLine,int &StartPos,int &LastLine,int &EndPos)
{
	FirstLine = Line;
	StartPos = MatchStart;
	AdjustPosition(Lines, FirstLine, StartPos);

	LastLine = FirstLine;
	EndPos = StartPos+MatchLength;
	AdjustPosition(Lines, LastLine, EndPos);
}

void ClearLineBuffer() {
	g_LineBuffer.clear();
	g_LineOffsets.clear();
	g_FirstLine = 0;
}

void FillLineBuffer(size_t FirstLine, size_t LastLine) {
	if (g_FirstLine < FirstLine) {
		if (g_FirstLine+g_LineOffsets.size() > FirstLine) {
			size_t nCut = g_LineOffsets[FirstLine-g_FirstLine];
			g_LineBuffer.erase(g_LineBuffer.begin(), g_LineBuffer.begin()+nCut);
			g_LineOffsets.erase(g_LineOffsets.begin(), g_LineOffsets.begin()+(FirstLine-g_FirstLine));
			for (size_t nOff = 0; nOff < g_LineOffsets.size(); nOff++) {
				g_LineOffsets[nOff] -= nCut;
			}
			g_FirstLine = FirstLine;
		} else {
			g_LineBuffer.clear();
			g_LineOffsets.clear();
		}
	}

	if (LastLine+1 < g_FirstLine+g_LineOffsets.size()) {
		if (LastLine >= g_FirstLine) {
			size_t nCut = g_LineOffsets[LastLine-g_FirstLine+1];
			g_LineBuffer.resize(nCut);
			g_LineOffsets.resize(LastLine-g_FirstLine+1);
		} else {
			g_LineBuffer.clear();
			g_LineOffsets.clear();
		}
	}

	if (g_LineOffsets.empty()) {
		g_FirstLine = (EReverse) ? LastLine : FirstLine;
	}

	EditorGetString String;
	EditorSetPosition Position={0,-1,-1,-1,-1,-1};

	String.StringNumber=-1;

	if (FirstLine < g_FirstLine) {
		vector<TCHAR> NewBuffer;

		for (Position.CurLine = g_FirstLine-1; Position.CurLine >= (int)FirstLine; Position.CurLine--) {
			EctlSetPosition(&Position);
			EctlGetString(&String);

			g_LineOffsets.insert(g_LineOffsets.begin(), 0);
			for (size_t nOff = 1; nOff < g_LineOffsets.size(); nOff++) g_LineOffsets[nOff] += String.StringLength+1;

			NewBuffer.insert(NewBuffer.begin(), String.StringText, String.StringText+String.StringLength);
			if (g_bUseRealEOL)
				NewBuffer.insert(NewBuffer.begin()+String.StringLength, String.StringEOL, _tcschr(String.StringEOL, 0));
			else
				NewBuffer.insert(NewBuffer.begin()+String.StringLength, '\n');

			g_FirstLine = Position.CurLine;
			if (g_LineBuffer.size()+NewBuffer.size() >= SeveralLinesKB*1024u) break;
		}

		g_LineBuffer.insert(g_LineBuffer.begin(), NewBuffer.begin(), NewBuffer.end());
	}

	if (LastLine >= g_FirstLine+g_LineOffsets.size()) {
		for (Position.CurLine = (int)(g_FirstLine+g_LineOffsets.size()); Position.CurLine <= (int)LastLine; Position.CurLine++) {
			EctlSetPosition(&Position);
			EctlGetString(&String);

			g_LineOffsets.push_back(g_LineBuffer.size());
			g_LineBuffer.insert(g_LineBuffer.end(), String.StringText, String.StringText+String.StringLength);
			if (String.StringEOL != NULL && (String.StringEOL[0] != 0)) {
				if (g_bUseRealEOL)
					g_LineBuffer.insert(g_LineBuffer.end(), String.StringEOL, _tcschr(String.StringEOL, 0));
				else
					g_LineBuffer.insert(g_LineBuffer.end(), '\n');
			}

			if (g_LineBuffer.size() >= SeveralLinesKB*1024u) break;
		}
	}

	if (FirstLine == LastLine) {
		while (!g_LineBuffer.empty() && ((g_LineBuffer.back() == '\r') || (g_LineBuffer.back() == '\n')))
			g_LineBuffer.erase(g_LineBuffer.end()-1);
	}
}

BOOL SearchInText(int &FirstLine,int &StartPos,int &LastLine,int &EndPos) {
	int Line,MatchStart,MatchLength;
	TCHAR *Lines;
	int LinesLength;

	ClearLineBuffer();
	RefreshEditorInfo();

	if (EReverse) {
		for (Line=LastLine;Line>=FirstLine;Line--) {
			ShowCurrentLine(Line,EdInfo.TotalLines,EdInfo.WindowSizeX);
			if (Interrupted256(Line)) return FALSE;

			int CurLastLine = min(LastLine+SeveralLines-1, LastLine);
			FillLineBuffer(Line, CurLastLine);
			Lines = (!g_LineBuffer.empty()) ? &g_LineBuffer[0] : _T("");
			LinesLength = g_LineBuffer.size();

			if (CurLastLine == LastLine) {
				int nLastLength = g_LineBuffer.size()-g_LineOffsets[g_LineOffsets.size()-1];
				if ((EndPos >= 0) && (nLastLength >= EndPos)) {
					LinesLength -= nLastLength-EndPos;
				}
			}

			if (SearchInLine(Lines, LinesLength, (Line==FirstLine) ? StartPos : 0, -1, &MatchStart, &MatchLength)) {
				Relative2Absolute(Line, Lines, MatchStart, MatchLength, FirstLine, StartPos, LastLine, EndPos);
				return TRUE;
			}
		}
	} else {
		int FirstLineLength;
		for (Line=FirstLine;Line<=LastLine;Line++) {
			ShowCurrentLine(Line,EdInfo.TotalLines,EdInfo.WindowSizeX);
			if (Interrupted256(Line)) return FALSE;

			FillLineBuffer(Line, min(LastLine, Line+SeveralLines-1));
			Lines = (!g_LineBuffer.empty()) ? &g_LineBuffer[0] : _T("");
			LinesLength = g_LineBuffer.size();
			FirstLineLength = (g_LineOffsets.size() <= 1) ? LinesLength : g_LineOffsets[1]-1;

			if ((Line + g_LineOffsets.size() - 1 == LastLine) && (EndPos!=-1)) {
				int nLastLength = g_LineBuffer.size()-g_LineOffsets[g_LineOffsets.size()-1];
				if (nLastLength >= EndPos) {
					LinesLength -= nLastLength-EndPos;
					if (g_LineOffsets.size() <= 1) FirstLineLength -= nLastLength-EndPos;
				}
			}

			if (SearchInLine(Lines,LinesLength,(Line==FirstLine)?StartPos:0,-1,&MatchStart,&MatchLength)) {
				if (MatchStart<=FirstLineLength) {
					Relative2Absolute(Line,Lines,MatchStart,MatchLength,FirstLine,StartPos,LastLine,EndPos);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

int TopLine(int NeededLine) {
	if (EKeepLineIfVisible) {
		//	We're in the visible range right now
		if ((NeededLine >= StartEdInfo.TopScreenLine) && (NeededLine < StartEdInfo.TopScreenLine+EdInfo.WindowSizeY))
			return StartEdInfo.TopScreenLine;
	}

	int Top;
	switch (EShowPosition) {
	case SP_TOP:
		Top=NeededLine-EShowPositionOffset;break;
	case SP_CENTER:
		Top=NeededLine-EdInfo.WindowSizeY/2-EShowPositionOffset;break;
	case SP_BOTTOM:
		Top=NeededLine-EdInfo.WindowSizeY+EShowPositionOffset+1;break;
	}
	if (Top<0) Top=0;
	if (Top>=EdInfo.TotalLines) Top=EdInfo.TotalLines-1;

	return Top;
}

int  TopLine(int FirstLine, int NeededLine, int LastLine) {
	int nLastLine = EdInfo.WindowSizeY - ETDSideOffset - 1;
	int nLine = TopLine(NeededLine);

	if (LastLine   - nLine > nLastLine) nLine = LastLine-nLastLine;
	if (FirstLine  - nLine < ETDSideOffset) nLine = FirstLine-ETDSideOffset;
	if (nLine < 0) nLine = 0;
	if (NeededLine - nLine > nLastLine) nLine = NeededLine-nLastLine;

	return nLine;
}

int RealToTab(int AtPosition) {
	EditorConvertPos ConvertPos={-1, AtPosition, 0};
	StartupInfo.EditorControl(ECTL_REALTOTAB, &ConvertPos);
	return ConvertPos.DestPos;
}

int LeftColumn(int AtPosition) {
	AtPosition = RealToTab(AtPosition);

	if (AtPosition < EdInfo.WindowSizeX-ELRSideOffset) return 0;
	return (AtPosition-(EdInfo.WindowSizeX-ELRSideOffset));
}

int LeftColumn(int LeftPosition, int AtPosition, int RightPosition) {
	LeftPosition  = RealToTab(LeftPosition);
	AtPosition    = RealToTab(AtPosition);
	RightPosition = RealToTab(RightPosition);

	if (LeftPosition > RightPosition) swap(LeftPosition, RightPosition);

	int nLastPosition = EdInfo.WindowSizeX - ELRSideOffset - 1;
	int nPosition = 0;

	if (RightPosition - nPosition > nLastPosition) nPosition = RightPosition-nLastPosition;
	if (LeftPosition  - nPosition < ELRSideOffset) nPosition = LeftPosition-ELRSideOffset;
	if (nPosition < 0) nPosition = 0;
	if (AtPosition    - nPosition > nLastPosition) nPosition = AtPosition-nLastPosition;

	return nPosition;
}

void GetHighlightPosition(EditorSetPosition &Position, int FirstLine,int StartPos,int LastLine,int EndPos)
{
	bool bAtStart = (EPositionAt == EP_BEGIN) || ((EPositionAt == EP_DIR) && EReverse);

	Position.CurLine = (bAtStart) ? FirstLine : LastLine;
	Position.CurPos = (bAtStart) ? StartPos : EndPos;
	Position.CurTabPos = -1;
	Position.TopScreenLine = TopLine(FirstLine, Position.CurLine, LastLine);
	Position.LeftPos = LeftColumn(StartPos, ((bAtStart) ? StartPos : EndPos), EndPos);
	Position.Overtype = -1;

	if (EPositionAtSub) {
		int nSub = REParam.FindParam(EPositionSubName);
		if (nSub >= 0) {
			Position.CurLine = FirstLine;
			Position.CurPos  = StartPos + REParam.m_arrMatch[nSub*2] - REParam.m_arrMatch[0];
			AdjustPosition(&g_LineBuffer[0], Position.CurLine, Position.CurPos);
			Position.TopScreenLine = TopLine(FirstLine, Position.CurLine, LastLine);
			Position.LeftPos = LeftColumn(StartPos, Position.CurPos, EndPos);
		}
	}
}

void SaveSelection() {
	int I;

	RefreshEditorInfo();
	if ((SelType=EdInfo.BlockType)!=BTYPE_NONE) {
		for (I=SelStartLine=EdInfo.BlockStartLine;I<EdInfo.TotalLines;I++) {
			EditorGetString String;
			String.StringNumber=I;
			EctlGetString(&String);
			if (String.SelStart==-1) break;
			if (I==SelStartLine) SelStartPos=String.SelStart;
			SelEndPos=String.SelEnd;
		}
		SelEndLine=I-1;
	}
}

void RestoreSelection() {
	if (SelType!=BTYPE_NONE) {
		EditorSelect Select={SelType,SelStartLine,SelStartPos,SelEndPos-SelStartPos,SelEndLine-SelStartLine+1};
		StartupInfo.EditorControl(ECTL_SELECT,&Select);
		SelType=BTYPE_NONE;
	}
}

void RestorePosition(const EditorInfo &StartEdInfo) {
	EditorSetPosition Position;
	Position.CurLine = StartEdInfo.CurLine;
	Position.CurPos = StartEdInfo.CurPos;
	Position.CurTabPos = StartEdInfo.CurTabPos;
	Position.TopScreenLine = StartEdInfo.TopScreenLine;
	Position.LeftPos = StartEdInfo.LeftPos;
	Position.Overtype = StartEdInfo.Overtype;
	EctlForceSetPosition(&Position);
}

BOOL EPreparePattern(tstring &SearchText) {
	ECleanup(TRUE);

	if (!CheckUsage(SearchText, ERegExp!=0, ESeveralLine!=0)) return FALSE;

	REErrorField  = MSearchFor;
	if (ERegExp) {
		RefreshEditorInfo();

#ifdef UNICODE
		BOOL Result = PreparePattern(&EPattern,&EPatternExtra,SearchText,ECaseSensitive,NULL);
#else
		if (ECharacterTables && (ECharacterTables != ANSICharTables) && (ECharacterTables != OEMCharTables))
			pcre_free((void *)ECharacterTables);

		if (EdInfo.TableNum != -1) {
			CharTableSet TableSet;
			StartupInfo.CharTable(EdInfo.TableNum, (char *)&TableSet, sizeof(TableSet));
			setlocale(LC_ALL, FormatStr(_T(".%d"), GetOEMCP()).c_str());	// We use DecodeTable for isspace() etc
			ECharacterTables = far_maketables(&TableSet);
		} else if (EdInfo.AnsiMode) {
			setlocale(LC_ALL, FormatStr(_T(".%d"), GetACP()).c_str());
			ECharacterTables = ANSICharTables;
		} else {
			setlocale(LC_ALL, FormatStr(_T(".%d"), GetOEMCP()).c_str());
			ECharacterTables = OEMCharTables;
		}

		char *OEMLine = _strdup(SearchText.c_str());
		OEMToEditor(OEMLine, SearchText.size());
		BOOL Result = PreparePattern(&EPattern,&EPatternExtra,OEMLine,ECaseSensitive,ECharacterTables);
		free(OEMLine);
#endif
		return Result;
	} else {
		ETextUpcase = (ECaseSensitive) ? SearchText : UpCaseString(SearchText);
		PrepareBMHSearch(ETextUpcase.data(), ETextUpcase.length());
		REErrorOffset = -1;
		return TRUE;
	}
}

void ECleanup(BOOL PatternOnly) {
	if (EPattern) {pcre_free(EPattern);EPattern=NULL;}
	if (EPatternExtra) {pcre_free(EPatternExtra);EPatternExtra=NULL;}

	REErrorOffset = -1;

	if (!PatternOnly) {
		delete ESPresets;
		delete ERPresets;
		delete EFPresets;
		delete ETPresets;
	}
}

void SynchronizeWithFile(bool bReplace) {
	ECaseSensitive = FCaseSensitive;
	EReverse = FALSE;

	if (FSearchAs <= SA_MULTILINE) {
		ERegExp = (FSearchAs != SA_PLAINTEXT);
		ESeveralLine = (FSearchAs == SA_SEVERALLINE) || (FSearchAs == SA_MULTILINE);
		EText = FText;
	} else {
		ERegExp = (FSearchAs == SA_MULTIREGEXP);
		ESeveralLine = FALSE;
		EText = FSWords.empty() ? tstring() : FSWords[0];
		if (!EText.empty() && ((EText[0] == '-') || (EText[0] == '+'))) EText.erase(0,1);
	}

	EPreparePattern(EText);
	if (bReplace) {
		ERReplace = FRReplace;
		LastAction = 1;
	} else
		LastAction = 0;
}

int LastLine=0;
DWORD LastTickCount=0;
BOOL ClockPresent=FALSE;

void FindIfClockPresent() {
	HKEY Key;
	RegCreateKeyEx(HKEY_CURRENT_USER,_T("Software\\Far\\Screen"),0,NULL,0,KEY_ALL_ACCESS,NULL,&Key,NULL);
	if (Key==INVALID_HANDLE_VALUE) return;
	QueryRegIntValue(Key,_T("ViewerEditorClock"),&ClockPresent,0,0,1);
	RegCloseKey(Key);
}

void ShowCurrentLine(int CurLine,int TotalLines,int TotalColumns) {
	if ((CurLine<LastLine) || (CurLine>=LastLine+1000) || (GetTickCount() > LastTickCount+1000)) {
		int Position = (ClockPresent) ? 19 : 25;
		if (TotalColumns > 80) Position += (TotalColumns-81);
#ifdef UNICODE
		Position+=17;
#else
		Position+=22;
#endif

		tstring strTotal = FormatStr(_T("%d"), TotalLines);
		int nWidth  = (strTotal.length() < 6) ? 13 : strTotal.length()*2+2;
		tstring strText = FormatStr(_T("%*s"), nWidth, FormatStr(_T("%d/%d"), CurLine, TotalLines).c_str());

		StartupInfo.Text(Position, 0, 0x30, strText.c_str());
		StartupInfo.Text(0,0,0x30,NULL);
		LastLine=CurLine;
		LastTickCount=GetTickCount();
	}
}

tstring PickupSelection() {
	EditorGetString String;

	RefreshEditorInfo();
	if (EdInfo.BlockType==BTYPE_NONE) return _T("");
	String.StringNumber=EdInfo.BlockStartLine+1;
	EctlGetString(&String);
	if (String.SelStart>=0) return _T("");

	String.StringNumber=EdInfo.BlockStartLine;
	EctlGetString(&String);
	if (String.SelEnd==-1) String.SelEnd=String.StringLength;

#ifdef UNICODE
	return tstring(String.StringText+String.SelStart, String.SelEnd-String.SelStart);
#else
	char *Word=(char *)malloc(String.SelEnd-String.SelStart+1);
	strncpy(Word,String.StringText+String.SelStart,String.SelEnd-String.SelStart);
	Word[String.SelEnd-String.SelStart]=0;
	EditorToOEM(Word,String.SelEnd-String.SelStart);
	string RetWord = Word;
	free(Word);
	return RetWord;
#endif
}

#ifdef UNICODE
BOOL IsWordChar(TCHAR C) {
	return iswalnum(C)||(C=='_');
}
#else
BOOL IsWordChar(char C) {
	WCHAR WChar;
	MultiByteToWideChar(CP_OEMCP,0,&C,1,&WChar,1);
	return iswalnum(WChar)||(C=='_');
}
#endif

tstring PickupWord() {
	if (EFindTextAtCursor==FT_NONE) return _T("");
	RefreshEditorInfo();

	EditorGetString String;
	String.StringNumber=-1;
	EctlGetString(&String);

	if (EdInfo.CurPos>=String.StringLength) return _T("");

	TCHAR *WordStart=(TCHAR *)String.StringText+EdInfo.CurPos,*WordEnd=WordStart;

	if (IsWordChar(*WordStart)) {
		while (IsWordChar(*(WordStart-1))&&(WordStart>String.StringText)) WordStart--;
		while (IsWordChar(*WordEnd)&&(WordEnd<String.StringText+String.StringLength)) WordEnd++;
	} else {
		if (EFindTextAtCursor==FT_WORD) return _T("");
	}

	if (WordEnd==WordStart) WordEnd++;

	int WordLen=WordEnd-WordStart;
#ifdef UNICODE
	return tstring(WordStart,WordLen);
#else
	char *Word=(char *)malloc(WordLen+1);
	strncpy(Word,WordStart,WordLen);Word[WordLen]=0;
	EditorToOEM(Word,WordLen);
	string RetWord = Word;
	free(Word);
	return RetWord;
#endif
}

tstring PickupText() {
	tstring PickUp;
	if (EFindSelection) {
		PickUp=PickupSelection();
		if (!PickUp.empty()) return PickUp;
	}
	if (EFindTextAtCursor!=FT_NONE) {
		PickUp=PickupWord();
		if (!PickUp.empty()) return PickUp;
	}
	return _T("");
}

#ifndef UNICODE

void EditorToOEM(char *Buffer,int Length) {
	EditorConvertText Convert={Buffer,Length};
	StartupInfo.EditorControl(ECTL_EDITORTOOEM,&Convert);
}

void EditorToOEM(EditorGetString &String) {
	EditorToOEM((char *)String.StringText,String.StringLength);
}

void EditorToOEM(string &String) {
	char *szString = _strdup(String.c_str());
	EditorToOEM(szString, String.length());
	String = szString;
	free(szString);
}

void OEMToEditor(char *Buffer,int Length) {
	EditorConvertText Convert={Buffer,Length};
	StartupInfo.EditorControl(ECTL_OEMTOEDITOR,&Convert);
}

void OEMToEditor(string &String) {
	char *szString = _strdup(String.c_str());
	OEMToEditor(szString, String.length());
	String = szString;
	free(szString);
}

#endif

void EctlGetString(EditorGetString *String) {
	StartupInfo.EditorControl(ECTL_GETSTRING, String);
}

tstring EctlGetString(int nLine) {
	if (nLine >= 0) {
		EditorSetPosition Position = {nLine,-1,-1,-1,-1,-1};
		EctlSetPosition(&Position);
	}
	EditorGetString String = {-1};
	StartupInfo.EditorControl(ECTL_GETSTRING, &String);
	return ToString(String);
}

tstring ToString(EditorGetString &String) {
	return tstring(String.StringText, String.StringLength);
}

void EctlSetString(EditorSetString *String) {
	StartupInfo.EditorControl(ECTL_SETSTRING, String);
}

int _nOldLine = -2;

void EctlSetPosition(EditorSetPosition *Position) {
	if (Position->CurLine == _nOldLine) return;
	StartupInfo.EditorControl(ECTL_SETPOSITION, Position);
//	OutputDebugString(FormatStr("ESP %d\r\n", Position->CurLine).c_str());
	_nOldLine = Position->CurLine;
}

void EctlForceSetPosition(EditorSetPosition *Position) {
	if (Position) {
		StartupInfo.EditorControl(ECTL_SETPOSITION, Position);
		_nOldLine = Position->CurLine;
	} else {
		_nOldLine = -2;
	}
}

void RefreshEditorInfo() {
	StartupInfo.EditorControl(ECTL_GETINFO, &EdInfo);
#ifdef UNICODE
	size_t FileNameSize=StartupInfo.EditorControl(ECTL_GETFILENAME, NULL);
	if (FileNameSize) {
		vector<TCHAR> arrFileName(FileNameSize);
		StartupInfo.EditorControl(ECTL_GETFILENAME, &arrFileName[0]);
		EditorFileName = &arrFileName[0];
	} else {
		EditorFileName = _T("");
	}
#endif
}

void EditorSeekToBeginEnd() {
	if (EReverse) {
		RefreshEditorInfo();

		EditorSetPosition Position = {EdInfo.TotalLines, 0, -1, -1, -1, -1};
		EctlSetPosition(&Position);

		EditorSetString String = {-1};
		EctlSetString(&String);

		Position.CurPos = String.StringLength;
		EctlSetPosition(&Position);
	} else {
		EditorSetPosition Position = {0, 0, 0, -1, -1, -1};
		EctlSetPosition(&Position);
	}
}
