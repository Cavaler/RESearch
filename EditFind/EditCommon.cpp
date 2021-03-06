#include "StdAfx.h"
#include "..\RESearch.h"

CParameterSet g_ESParamSet(EditorSearchExecutor,
	"Text", &SearchText, "@Text", &EText, NULL,
	"InSelection", &EInSelectionPreset, NULL,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine,
	"Reverse", &EReverse,
	"ListAll", &EListAllFromPreset, "FromCurrent", &EFromCurrentPosition, "CountAll", &ECountAllFromPreset, NULL
					 );
CParameterSet g_ERParamSet(EditorReplaceExecutor,
	"Text", &SearchText, "Replace", &ReplaceText, "Script", &EREvaluateScript, 
	"@Text", &EText,  "@Replace", &ERReplace, NULL,
	"InSelection", &EInSelectionPreset, NULL,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine,
	"Reverse", &EReverse,
	"RemoveEmpty", &ERRemoveEmpty, "RemoveNoMatch", &ERRemoveNoMatch,
	"AsScript", &EREvaluate, "FromCurrent", &EFromCurrentPosition, NULL
					 );
CParameterSet g_EPParamSet(EditorRepeatExecutor,
	"Replace", &ReplaceText, "Script", &EREvaluateScript, NULL,
	"RepeatCount", &ERRepeatCount, NULL,
	"AsScript", &EREvaluate, NULL
					 );
CParameterSet g_EFParamSet(EditorFilterExecutor,
	"Text", &SearchText, "@Text", &EText, NULL, NULL,
	"LeaveFilter", &EFLeaveFilter, "IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive,
	"Reverse", &EReverse, "FromCurrent", &EFromCurrentPosition, NULL
					 );
CParameterSet g_ETParamSet(EditorTransliterateExecutor,
	"Text", &SearchText, "Replace", &ReplaceText,
	"@Text", &EText,  "@Replace", &ERReplace, NULL, NULL, NULL
					 );

void EReadRegistry(CFarSettingsKey Key)
{
	#define DECLARE_PERSIST_LOAD Key
	#include "PersistVars.h"

	SelType=BTYPE_NONE;

	ESPresets = new CESPresetCollection(g_ESParamSet);
	ERPresets = new CERPresetCollection(g_ERParamSet);
	EPPresets = new CEPPresetCollection(g_EPParamSet);
	EFPresets = new CEFPresetCollection(g_EFParamSet);
	ETPresets = new CETPresetCollection(g_ETParamSet);
}

void EWriteRegistry(CFarSettingsKey Key)
{
	#define DECLARE_PERSIST_SAVE Key
	#include "PersistVars.h"
}

bool SearchIn(const TCHAR *Line,int Start,int Length,int *MatchStart,int *MatchLength,int nExecOptions)
{
	REParam.Clear();
	REParam.AddSource(Line, Start+Length);

	if (ERegExp && (!EReverse || Length == 0)) {
		REParam.AddRE(EPattern);

#ifdef UNICODE
		if (pcre16_exec(EPattern16,EPattern16Extra,(PCRE_SPTR16)Line,Start+Length,Start,nExecOptions,REParam.Match(),REParam.Count())>=0) {
#else
		if (pcre_exec(EPattern,EPatternExtra,Line,Start+Length,Start,nExecOptions,REParam.Match(),REParam.Count())>=0) {
#endif
			MatchDone();
			REParam.FillStartLength(MatchStart, MatchLength);
			return true;
		}
	}
	else if (ERegExp && EReverse)
	{
		REParam.AddRE(EPattern);

		//	Optimize for the case of no match at all
#ifdef UNICODE
		bool bFound = pcre16_exec(EPattern16,EPattern16Extra,(PCRE_SPTR16)Line,Start+Length,Start,nExecOptions,REParam.Match(),REParam.Count()) >= 0;
#else
		bool bFound = pcre_exec(EPattern,EPatternExtra,Line,Start+Length,Start,nExecOptions,REParam.Match(),REParam.Count()) >= 0;
#endif
		if (!bFound) return false;

		//	Look for last match
		for (int Offset = Length-1; Offset >=0; Offset--) {
#ifdef UNICODE
			bFound = pcre16_exec(EPattern16,EPattern16Extra,(PCRE_SPTR16)Line,Start+Length,Start+Offset,nExecOptions,REParam.Match(),REParam.Count()) >= 0;
#else
			bFound = pcre_exec(EPattern,EPatternExtra,Line,Start+Length,Start+Offset,nExecOptions,REParam.Match(),REParam.Count()) >= 0;
#endif
			if (bFound && ((Offset == 0) || (REParam.Match()[0] > Offset))) {
				MatchDone();
				REParam.FillStartLength(MatchStart, MatchLength);
				return true;
			}
		}
	}
	else
	{
		TCHAR *Table = ECaseSensitive ? NULL : UpCaseTable;
		int Position=(EReverse)?ReverseBMHSearch(Line+Start,Length,ETextUpcase.data(),ETextUpcase.length(),Table)
									  :BMHSearch(Line+Start,Length,ETextUpcase.data(),ETextUpcase.length(),Table);
		if (Position>=0) {
			REParam.AddPlainTextMatch(Start+Position, EText.length());
			if (MatchStart) *MatchStart = Start+Position;
			if (MatchLength) *MatchLength = EText.length();
			return true;
		}
	}

	return false;
}

bool SearchInLine(const TCHAR *Line,int Length,int Start,int End,int *MatchStart,int *MatchLength,int nExecOptions)
{
	int Len;

	if (Start>Length) return false;
	if (Start==-1) Start=0;
	if ((End==-1)||(End>Length)) Len=Length-Start; else Len=End-Start;
	if (Len<0) return false;

#ifdef UNICODE
	return SearchIn(Line,Start,Len,MatchStart,MatchLength,nExecOptions);
#else
	if (ERegExp) {
		return SearchIn(Line,Start,Len,MatchStart,MatchLength,nExecOptions);
	} else {
		if (EdInfo.AnsiMode || (EdInfo.TableNum != -1)) {
			//	static - to allow REParam somewhere inside to store just a pointer, not a copy
			//	NOT using a constructor!
			static string OEMLine;
			OEMLine = string(Line, Length);
			EditorToOEM(OEMLine);
			return SearchIn(OEMLine.c_str(),Start,Len,MatchStart,MatchLength,nExecOptions);
		} else {
			return SearchIn(Line, Start, Len, MatchStart, MatchLength,nExecOptions);
		}
	}
#endif
}

void AdjustPosition(TCHAR *Lines, int &FirstLine, int &StartPos)
{
	do {
		TCHAR *NewLine=(TCHAR *)_tmemchr(Lines,'\n',StartPos);
		if (NewLine) {
			int Pos = NewLine-Lines;
			Lines += Pos+1;
			StartPos -= Pos+1;
			FirstLine++;
		} else break;
	} while (true);
}

#if defined(FAR3) && defined(_WIN64)
void AdjustPosition(TCHAR *Lines, intptr_t &FirstLine, intptr_t &StartPos)
{
	do {
		TCHAR *NewLine=(TCHAR *)_tmemchr(Lines,'\n',StartPos);
		if (NewLine) {
			int Pos = NewLine-Lines;
			Lines += Pos+1;
			StartPos -= Pos+1;
			FirstLine++;
		} else break;
	} while (true);
}
#endif

void Relative2Absolute(int Line,TCHAR *Lines,int MatchStart,int MatchLength,int &FirstLine,int &StartPos,int &LastLine,int &EndPos)
{
	FirstLine = Line;
	StartPos = MatchStart;
	AdjustPosition(Lines, FirstLine, StartPos);

	LastLine = FirstLine;
	EndPos = StartPos + MatchLength;
	AdjustPosition(Lines + MatchStart - StartPos, LastLine, EndPos);
}

void ClearLineBuffer()
{
	g_LineBuffer.clear();
	g_LineOffsets.clear();
	g_FirstLine = 0;
}

void SetDefEOL(LPCTSTR szEOL)
{
	g_LastEOL = szEOL ? szEOL : _T("");
	if (CSO::cstrlen(szEOL) > 0) {
		g_DefEOL = szEOL;
	} else if (g_DefEOL.empty()) {
		g_DefEOL = _T("\r\n");
	}
}

void FillSingleLineBuffer(size_t FirstLine)
{
	if ((g_FirstLine != FirstLine) || g_LineOffsets.empty()) {
		EditorGetString String = {ITEM_SS(EditorGetString) -1};
		EditorSetPosition Position={ITEM_SS(EditorSetPosition) FirstLine,-1,-1,-1,-1,-1};

		EctlSetPosition(&Position);
		EctlGetString(&String);
		ToArray(String, g_LineBuffer);

		g_LineOffsets.resize(1);
		g_LineOffsets[0] = 0;

		SetDefEOL(String.StringEOL);
	} else if (g_LineOffsets.size() > 1) {
		size_t nCut = g_LineOffsets[1];
		g_LineBuffer.resize(nCut);
		g_LineOffsets.resize(1);
	}
	g_FirstLine = FirstLine;
}

void FillLineBuffer(size_t FirstLine, size_t LastLine)
{
	if (!ESeveralLine && (FirstLine == LastLine)) {
		FillSingleLineBuffer(FirstLine);
		return;
	}

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

	EditorGetString String INIT_SS(EditorGetString);
	EditorSetPosition Position={ITEM_SS(EditorSetPosition) 0,-1,-1,-1,-1,-1};

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
			SetDefEOL(String.StringEOL);

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
}

bool SearchInText(int &FirstLine,int &StartPos,int &LastLine,int &EndPos,bool bSkipClear,bool bNotBOL)
{
	int Line,MatchStart,MatchLength;
	TCHAR *Lines;
	int LinesLength;

	if (!bSkipClear) ClearLineBuffer();
	RefreshEditorInfo();

	if (EReverse) {
		for (Line=LastLine;Line>=FirstLine;Line--) {
			ShowCurrentLine(Line,EdInfo.TotalLines,EdInfo.WindowSizeX);
			if (Interrupted()) return false;

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

			if (bNotBOL && (StartPos <= 0)) continue;

			int nExecOptions = bNotBOL ? PCRE_NOTEOL : 0;
			if (SearchInLine(Lines, LinesLength, (Line==FirstLine) ? StartPos : 0, -1, &MatchStart, &MatchLength, nExecOptions)) {
				Relative2Absolute(Line, Lines, MatchStart, MatchLength, FirstLine, StartPos, LastLine, EndPos);
				return true;
			}
			bNotBOL = false;
		}
	} else {
		int FirstLineLength;
		for (Line=FirstLine;Line<=LastLine;Line++) {
			ShowCurrentLine(Line,EdInfo.TotalLines,EdInfo.WindowSizeX);
			if (Interrupted()) return false;

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

			if (bNotBOL && (FirstLineLength == StartPos)) continue;

			int nExecOptions = bNotBOL ? PCRE_NOTBOL : 0;
			if (SearchInLine(Lines,LinesLength,(Line==FirstLine)?StartPos:0,-1,&MatchStart,&MatchLength,nExecOptions)) {
				Relative2Absolute(Line,Lines,MatchStart,MatchLength,FirstLine,StartPos,LastLine,EndPos);
				return true;
			}
			bNotBOL = false;
		}
	}
	return false;
}

int TopLine(int NeededLine)
{
	if (EIncremental) {
		return (NeededLine > 2) ? NeededLine - 2 : 0;
	}

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

int RealToTab(int AtPosition)
{
	EditorConvertPos ConvertPos={ITEM_SS(EditorConvertPos) -1, AtPosition, 0};
	StartupInfo.EditorControl(ECTL_REALTOTAB, &ConvertPos);
	return ConvertPos.DestPos;
}

int LeftColumn(int AtPosition)
{
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
	bool bAtStart = (EPositionAt == EP_BEGIN) || ((EPositionAt == EP_DIR) && EReverse) || EIncremental;

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

void SaveSelection()
{
	RefreshEditorInfo();

	if ((SelType=EdInfo.BlockType) != BTYPE_NONE)
	{
		int I;

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

void RestoreSelection()
{
	if (SelType != BTYPE_NONE)
	{
		int nStartPos = (SelType == BTYPE_COLUMN) ? RealToTab(SelStartPos) : SelStartPos;
		int nEndPos   = (SelType == BTYPE_COLUMN) ? RealToTab(SelEndPos)   : SelEndPos;
		EditorSelect Select={ITEM_SS(EditorSelect) SelType, SelStartLine, nStartPos, nEndPos-nStartPos, SelEndLine-SelStartLine+1};
		StartupInfo.EditorControl(ECTL_SELECT, &Select);
		SelType=BTYPE_NONE;
	}
	else
	{
		EditorSelect Select={ITEM_SS(EditorSelect) SelType};
		StartupInfo.EditorControl(ECTL_SELECT, &Select);
	}
}

void RestorePosition(const EditorInfo &StartEdInfo)
{
	EditorSetPosition Position INIT_SS(EditorSetPosition);
	Position.CurLine = StartEdInfo.CurLine;
	Position.CurPos = StartEdInfo.CurPos;
	Position.CurTabPos = StartEdInfo.CurTabPos;
	Position.TopScreenLine = StartEdInfo.TopScreenLine;
	Position.LeftPos = StartEdInfo.LeftPos;
	Position.Overtype = StartEdInfo.Overtype;
	EctlForceSetPosition(&Position);
}

bool EPreparePattern(tstring &SearchText)
{
	ECleanup(true);

	if (SearchText.empty()) return false;

	if (!CheckUsage(SearchText, ERegExp!=0, ESeveralLine!=0)) return false;

	REErrorField  = MSearchFor;
	if (ERegExp) {
		EditorFillNamedParameters();

#ifdef UNICODE
		tstring FillSearchText = REParam.FillNamedReferences(SearchText, true);
		bool Result = PreparePattern(&EPattern,&EPatternExtra,FillSearchText,ECaseSensitive,NULL);
		if (Result) {
			PreparePattern(&EPattern16,&EPattern16Extra,FillSearchText,ECaseSensitive);
		}
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

		string OEMLine = SearchText;
		OEMToEditor(OEMLine);
		OEMLine = REParam.FillNamedReferences(OEMLine);

		bool Result = PreparePattern(&EPattern,&EPatternExtra,OEMLine,ECaseSensitive,ECharacterTables);
#endif
		return Result;
	} else {
		ETextUpcase = (ECaseSensitive) ? SearchText : UpCaseString(SearchText);
		PrepareBMHSearch(ETextUpcase.data(), ETextUpcase.length());
		REErrorOffset = -1;
		return true;
	}
}

void ECleanup(bool PatternOnly)
{
	REParam.BackupParam();
	PCRE_FREE(EPattern);
	PCRE_FREE(EPatternExtra);
#ifdef UNICODE
	PCRE_FREE(EPattern16);
	PCRE_FREE(EPattern16Extra);
#endif

	REErrorOffset = -1;

	if (!PatternOnly) {
		delete ESPresets;
		delete ERPresets;
		delete EPPresets;
		delete EFPresets;
		delete ETPresets;
	}
}

void SynchronizeWithFile(bool bReplace)
{
	ECaseSensitive = FCaseSensitive;
	EReverse = false;

	if (FSearchAs <= SA_MULTILINE) {
		ERegExp = (FSearchAs != SA_PLAINTEXT);
		ESeveralLine = (FSearchAs == SA_SEVERALLINE) || (FSearchAs == SA_MULTILINE);
		EText = FText;
	} else {
		ERegExp = (FSearchAs == SA_MULTIREGEXP);
		ESeveralLine = false;
		EText = FSWords.empty() ? tstring() : FSWords[0];
		if (!EText.empty() && ((EText[0] == '-') || (EText[0] == '+'))) EText.erase(0,1);
	}

	if (bReplace) {
		ERReplace = FRReplace;
		EREvaluate = FREvaluate;
		LastAction = 1;
	} else
		LastAction = 0;
}

int LastLine=0;
bool ClockPresent=false;

void FindIfClockPresent()
{
#ifdef FAR3
	ClockPresent = false;
#else
	HKEY Key;
#ifdef UNICODE
	RegCreateKeyEx(HKEY_CURRENT_USER,_T("Software\\Far2\\Screen"),0,NULL,0,KEY_ALL_ACCESS,NULL,&Key,NULL);
#else
	RegCreateKeyEx(HKEY_CURRENT_USER,_T("Software\\Far\\Screen"),0,NULL,0,KEY_ALL_ACCESS,NULL,&Key,NULL);
#endif
	if (Key==INVALID_HANDLE_VALUE) return;
	QueryRegBoolValue(Key,_T("ViewerEditorClock"),&ClockPresent,false);
	RegCloseKey(Key);
#endif
}

void ShowCurrentLine(int CurLine,int TotalLines,int TotalColumns)
{
	if ((CurLine < LastLine-1000) || (CurLine>=LastLine+1000) || (GetTickCount() > LastTickCount+250)) {
		int Position = (ClockPresent) ? 19 : 25;
		if (TotalColumns > 80) Position += (TotalColumns-81);
#ifdef UNICODE
#ifdef FAR3
		Position+=9;
#else
		Position+=17;
#endif
#else
		Position+=22;
#endif

		tstring strTotal = FormatStr(_T("%d"), TotalLines);
		int nWidth  = (strTotal.length() < 6) ? 13 : strTotal.length()*2+2;
		tstring strText = FormatStr(_T("%*s"), nWidth, FormatStr(_T("%d/%d"), CurLine, TotalLines).c_str());

#ifdef FAR3
		FarColor Color;
		Color.Flags = FCF_4BITMASK;
		Color.BackgroundColor = (BarColor >> 4) & 0x0F;
		Color.ForegroundColor =  BarColor       & 0x0F;
		StartupInfo.Text(Position, 0, &Color, strText.c_str());
		StartupInfo.Text(0, 0, &Color, NULL);
#else
		StartupInfo.Text(Position, 0, BarColor & 0xFF, strText.c_str());
		StartupInfo.Text(0, 0, 0, NULL);
#endif
		LastLine=CurLine;

		SetConsoleTitle(FormatStr(GetMsg(MEditConsoleTitle), CurLine, TotalLines).c_str());

		LastTickCount=GetTickCount();
	}
}

tstring PickupSelection()
{
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

tstring PickupMultilineSelection()
{
	EditorGetString String;
	tstring strSelection;

	RefreshEditorInfo();
	if (EdInfo.BlockType == BTYPE_NONE) return _T("");

	for (String.StringNumber = EdInfo.BlockStartLine; String.StringNumber < EdInfo.TotalLines; String.StringNumber++)
	{
		EctlGetString(&String);
		if ((String.SelStart < 0) || (String.SelStart == String.SelEnd)) break;

		if (String.SelEnd < 0)
		{
			strSelection = strSelection + ToStringEOL(String, String.SelStart).substr(String.SelStart);
		}
		else
		{
			strSelection = strSelection + ToString(String, String.SelEnd).substr(String.SelStart, String.SelEnd - String.SelStart);
			if (EdInfo.BlockType == BTYPE_COLUMN)
			{
				strSelection += GetEOL(String);
			}
			else
				break;
		}
	}

	return strSelection;
}

#ifdef UNICODE
bool IsWordChar(TCHAR C) {
	return iswalnum(C)||(C=='_');
}
#else
bool IsWordChar(char C) {
	WCHAR WChar;
	MultiByteToWideChar(CP_OEMCP,0,&C,1,&WChar,1);
	return iswalnum(WChar)||(C=='_');
}
#endif

tstring PickupWord()
{
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

int EctlGetString(EditorGetString *String)
{
#ifdef FAR3
	String->StructSize = sizeof(EditorGetString);
#endif
	return StartupInfo.EditorControl(ECTL_GETSTRING, String);
}

tstring EctlGetString(int nLine)
{
	if (nLine >= 0) {
		EditorSetPosition Position = {ITEM_SS(EditorSetPosition) nLine,-1,-1,-1,-1,-1};
		EctlSetPosition(&Position);
	}
	EditorGetString String = {ITEM_SS(EditorGetString) -1};
	StartupInfo.EditorControl(ECTL_GETSTRING, &String);
	return ToString(String);
}

tstring ToString(const EditorGetString &String, int nAssureLength)
{
	return CSO::AssureLength(CSO::MakeString(String.StringText, String.StringLength), nAssureLength);
}

tstring ToStringEOL(const EditorGetString &String, int nAssureLength)
{
	return ToString(String, nAssureLength) + GetEOL(String);
}

tstring GetEOL(const EditorGetString &String)
{
	return (String.StringEOL != NULL) ? String.StringEOL : _T("");
}

void ToArray(const EditorGetString &String, vector<TCHAR> &arrBuffer)
{
	arrBuffer.assign(String.StringText, String.StringText + String.StringLength);
}

void ToArray(const EditorSetString &String, vector<TCHAR> &arrBuffer)
{
	arrBuffer.assign(String.StringText, String.StringText + String.StringLength);
}

void EctlSetString(EditorSetString *String)
{
#ifdef FAR3
	String->StructSize = sizeof(EditorSetString);
#endif
	StartupInfo.EditorControl(ECTL_SETSTRING, String);
}

void EctlSetStringWithWorkarounds(EditorSetString *String)
{
	EditorGetString GetString = {ITEM_SS(EditorGetString) String->StringNumber};
	EctlGetString(&GetString);

#ifdef FAR3
	String->StructSize = sizeof(EditorSetString);
#endif

	if ((CSO::cstrlen(GetString.StringEOL) == 0) && (CSO::cstrlen(String->StringEOL) > 0))
	{
		int StringNumber = String->StringNumber;
		if (StringNumber < 0) {
			RefreshEditorInfo();
			StringNumber = EdInfo.CurLine;
		}

		StartupInfo.EditorControl(ECTL_INSERTSTRING, String);
		RefreshEditorInfo();
		CEditorSetString EmptyString(-1, _T(""), _T(""), 0);
		StartupInfo.EditorControl(ECTL_SETSTRING, &EmptyString);
		RefreshEditorInfo();

		EditorSetPosition Position = {ITEM_SS(EditorSetPosition) StringNumber, -1, -1, -1, -1, -1};
		EctlForceSetPosition(&Position);
		StartupInfo.EditorControl(ECTL_SETSTRING, String);
	}
	else
	{
		StartupInfo.EditorControl(ECTL_SETSTRING, String);
	}
}

int _nOldLine = -2;

void EctlSetPosition(EditorSetPosition *Position)
{
#ifdef FAR3
	Position->StructSize = sizeof(EditorSetPosition);
#endif
	if (Position->CurLine == _nOldLine) return;
	StartupInfo.EditorControl(ECTL_SETPOSITION, Position);
//	OutputDebugString(FormatStr("ESP %d\r\n", Position->CurLine).c_str());
	_nOldLine = Position->CurLine;
}

void EctlForceSetPosition(EditorSetPosition *Position)
{
	if (Position) {
#ifdef FAR3
		Position->StructSize = sizeof(EditorSetPosition);
#endif
		StartupInfo.EditorControl(ECTL_SETPOSITION, Position);
		_nOldLine = Position->CurLine;
	} else {
		_nOldLine = -2;
	}
}

bool RefreshEditorInfo()
{
#ifdef FAR3
	EdInfo.StructSize = sizeof(EditorInfo);
#endif
	if (StartupInfo.EditorControl(ECTL_GETINFO, &EdInfo) == 0) return false;
#ifdef UNICODE
	size_t FileNameSize=StartupInfo.EditorControl(ECTL_GETFILENAME, NULL);
	if (FileNameSize) {
		vector<TCHAR> arrFileName(FileNameSize+1);
#ifdef FAR3
		((PluginStartupInfo &)StartupInfo).EditorControl(-1, ECTL_GETFILENAME, FileNameSize+1, &arrFileName[0]);
#else
		StartupInfo.EditorControl(ECTL_GETFILENAME, &arrFileName[0]);
#endif
		EditorFileName = &arrFileName[0];
	} else {
		EditorFileName = _T("");
	}
#endif

	return true;
}

void RefreshEditorColorInfo()
{
	HANDLE hBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD coord = {1, 0};
	DWORD dwRead;
	ReadConsoleOutputAttribute(hBuffer, &BarColor, 1, coord, &dwRead);
}

void EditorFillNamedParameters()
{
	RefreshEditorInfo();
#ifdef UNICODE
	FillDefaultNamedParameters(EditorFileName.c_str());
#else
	FillDefaultNamedParameters(EdInfo.FileName);
#endif
}

void EditorSeekToBeginEnd()
{
	if (EReverse) {
		RefreshEditorInfo();

		EditorSetPosition Position = {ITEM_SS(EditorSetPosition) EdInfo.TotalLines, 0, -1, -1, -1, -1};
		EctlForceSetPosition(&Position);

		CEditorSetString String(-1);
		EctlSetString(&String);

		Position.CurPos = String.StringLength;
		EctlForceSetPosition(&Position);
	} else {
		EditorSetPosition Position = {ITEM_SS(EditorSetPosition) 0, 0, 0, -1, -1, -1};
		EctlForceSetPosition(&Position);
	}
}

bool EditorUpdateSelectionPosition()
{
	RefreshEditorInfo();

	switch (EInSelectionPreset)
	{
	case 0:
		EInSelection = false;
		return true;
	case 1:
		if (EdInfo.BlockType == BTYPE_NONE)
			return false;
		EInSelection = true;
		return true;
	case 2:
		EInSelection = (EdInfo.BlockType != BTYPE_NONE);
		return true;
	}

	return false;
}

void EditorUpdatePresetPosition()
{
	if (EFromCurrentPosition) {
		EFromCurrentPosition = false;
	} else {
		EditorSeekToBeginEnd();
	}
}
