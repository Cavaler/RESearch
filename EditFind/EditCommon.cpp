#include "StdAfx.h"
#include "..\RESearch.h"

CParameterBatch g_ESBatch(1, 4,
	"Text", &SearchText,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine, "UTF8", &EUTF8
					 );
CParameterBatch g_ERBatch(2, 6,
	"Text", &SearchText, "Replace", &ReplaceText,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine,
	"UTF8", &EUTF8, "RemoveEmpty", &ERRemoveEmpty, "RemoveNoMatch", &ERRemoveNoMatch
					 );
CParameterBatch g_EFBatch(1, 4,
	"Text", &SearchText,
	"LeaveFilter", &EFLeaveFilter, "IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "UTF8", &EUTF8
					 );
CParameterBatch g_ELBatch(1, 3,
	"Text", &SearchText,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "UTF8", &EUTF8
					 );

void EReadRegistry(HKEY Key) {
	QueryRegStringValue(Key,"EText",EText,"");
	QueryRegIntValue(Key,"ERegExp",&ERegExp,0,0,1);
	QueryRegIntValue(Key,"ESeveralLine",&ESeveralLine,1,0,1);
	QueryRegIntValue(Key,"ECaseSensitive",&ECaseSensitive,0,0,1);
	QueryRegStringValue(Key,"ERReplace",ERReplace,"");
	QueryRegBoolValue(Key,"EFLeaveFilter",&EFLeaveFilter,TRUE);
	QueryRegIntValue(Key,"EREvaluateScript",&EREvaluateScript,0,0);
	QueryRegStringValue(Key,"ETSource",ETSource,"");
	QueryRegStringValue(Key,"ETTarget",ETTarget,"");
	SelType=BTYPE_NONE;

	ESPresets=new CESPresetCollection();
	ERPresets=new CERPresetCollection();
	EFPresets=new CEFPresetCollection();
	ELPresets=new CELPresetCollection();

	ERBatch=new CPresetBatchCollection(ERPresets);
	EFBatch=new CPresetBatchCollection(EFPresets);
}

void EWriteRegistry(HKEY Key) {
	SetRegStringValue(Key,"EText",EText);
	SetRegIntValue(Key,"ERegExp",ERegExp);
	SetRegIntValue(Key,"ESeveralLine",ESeveralLine);
	SetRegIntValue(Key,"ECaseSensitive",ECaseSensitive);
	SetRegStringValue(Key,"ERReplace",ERReplace);
	SetRegBoolValue(Key,"EFLeaveFilter",EFLeaveFilter);
	SetRegIntValue(Key,"EREvaluateScript",EREvaluateScript);
	SetRegStringValue(Key,"ETSource",ETSource);
	SetRegStringValue(Key,"ETTarget",ETTarget);
}

BOOL SearchIn(const char *Line,int Start,int Length,int *MatchStart,int *MatchLength,BOOL NeedMatch) {
	if (Length==0) return FALSE;
	if (ERegExp) {
		MatchCount=pcre_info(EPattern,NULL,NULL)+1;
		Match=new int[MatchCount*3];
		if (pcre_exec(EPattern,EPatternExtra,Line,Start+Length,Start,0,Match,MatchCount*3)>=0) {
			if ((Match[0]!=Match[1])||AllowEmptyMatch) {
				if (MatchStart) *MatchStart=Match[0];
				if (MatchLength) *MatchLength=Match[1]-Match[0];
				if (!NeedMatch) {delete[] Match;Match=NULL;}
				return TRUE;
			}
		}
		if (!NeedMatch) {delete[] Match;Match=NULL;}
	} else {
		char *Table = ECaseSensitive ? NULL : UpCaseTable;
		Match=NULL;
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

BOOL SearchInLine(const char *Line,int Length,int Start,int End,int *MatchStart,int *MatchLength,BOOL NeedMatch) {
	int Len;

	if (Start>Length) return FALSE;
	if (Start==-1) Start=0;
	if ((End==-1)||(End>Length)) Len=Length-Start; else Len=End-Start;
	if (Len<=0) return FALSE;

	if (ERegExp) {
		return SearchIn(Line,Start,Len,MatchStart,MatchLength,NeedMatch);
	} else {
		if (EdInfo.AnsiMode || (EdInfo.TableNum >= 0)) {
			char *OEMLine=(char *)malloc(Length);
			memmove(OEMLine,Line,Length);
			EditorToOEM(OEMLine,Length);
			int Result=SearchIn(OEMLine,Start,Len,MatchStart,MatchLength,NeedMatch);
			free(OEMLine);
			return Result;
		} else {
			return SearchIn(Line, Start, Len, MatchStart, MatchLength, NeedMatch);
		}
	}
}

void Relative2Absolute(int Line,char *Lines,int MatchStart,int MatchLength,int &FirstLine,int &StartPos,int &LastLine,int &EndPos) {
	FirstLine=Line;StartPos=MatchStart;
	do {			// Find first line
		void *NewLine=memchr(Lines,'\n',StartPos);
		if (NewLine) {
			int Pos=(char *)NewLine-Lines;
			Lines+=Pos+1;
			StartPos-=Pos+1;
			FirstLine++;
		} else break;
	} while (TRUE);

	LastLine=FirstLine;EndPos=StartPos+MatchLength;
	do {			// Find last line
		void *NewLine=memchr(Lines,'\n',EndPos);
		if (NewLine) {
			int Pos=(char *)NewLine-Lines;
			Lines+=Pos+1;
			EndPos-=Pos+1;
			LastLine++;
		} else break;
	} while (TRUE);
}

vector<char>	g_LineBuffer;
vector<int>		g_LineOffsets;
size_t			g_FirstLine;

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
		vector<char> NewBuffer;

		for (Position.CurLine = g_FirstLine-1; Position.CurLine >= (int)FirstLine; Position.CurLine--) {
			EctlSetPosition(&Position);
			EctlGetString(&String);

			g_LineOffsets.insert(g_LineOffsets.begin(), 0);
			for (size_t nOff = 1; nOff < g_LineOffsets.size(); nOff++) g_LineOffsets[nOff] += String.StringLength+1;

			NewBuffer.insert(NewBuffer.begin(), String.StringText, String.StringText+String.StringLength);
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
			g_LineBuffer.insert(g_LineBuffer.end(), '\n');

			if (g_LineBuffer.size() >= SeveralLinesKB*1024u) break;
		}
	}

	if ((FirstLine == LastLine) && !g_LineBuffer.empty() && (g_LineBuffer.back() == '\n'))
		g_LineBuffer.erase(g_LineBuffer.end()-1);
}

BOOL SearchInText(int &FirstLine,int &StartPos,int &LastLine,int &EndPos,BOOL NeedMatch) {
	int Line,MatchStart,MatchLength;
	char *Lines;int LinesLength;

	ClearLineBuffer();
	RefreshEditorInfo();

	if (EReverse) {
		for (Line=LastLine;Line>=FirstLine;Line--) {
			ShowCurrentLine(Line,EdInfo.TotalLines,EdInfo.WindowSizeX);
			if (Interrupted256(Line)) return FALSE;

			int CurLastLine = min(LastLine+SeveralLines-1, LastLine);
			FillLineBuffer(Line, CurLastLine);
			Lines = (!g_LineBuffer.empty()) ? &g_LineBuffer[0] : NULL;
			LinesLength = g_LineBuffer.size();

			if (CurLastLine == LastLine) {
				int nLastLength = g_LineBuffer.size()-g_LineOffsets[g_LineOffsets.size()-1];
				if ((EndPos >= 0) && (nLastLength >= EndPos)) {
					LinesLength -= nLastLength-EndPos;
				}
			}

			if (SearchInLine(Lines, LinesLength, (Line==FirstLine) ? StartPos : 0, -1, &MatchStart, &MatchLength, NeedMatch)) {
				Relative2Absolute(Line, Lines, MatchStart, MatchLength, FirstLine, StartPos, LastLine, EndPos);
				if (NeedMatch) {
					MatchedLine=Lines;
					MatchedLineLength=LinesLength;
				}
				return TRUE;
			}
		}
	} else {
		int FirstLineLength;
		for (Line=FirstLine;Line<=LastLine;Line++) {
			ShowCurrentLine(Line,EdInfo.TotalLines,EdInfo.WindowSizeX);
			if (Interrupted256(Line)) return FALSE;

			FillLineBuffer(Line, min(LastLine, Line+SeveralLines-1));
			Lines = (!g_LineBuffer.empty()) ? &g_LineBuffer[0] : NULL;
			LinesLength = g_LineBuffer.size();
			FirstLineLength = (g_LineOffsets.size() <= 1) ? LinesLength : g_LineOffsets[1]-1;

			if ((Line + g_LineOffsets.size() - 1 == LastLine) && (EndPos!=-1)) {
				int nLastLength = g_LineBuffer.size()-g_LineOffsets[g_LineOffsets.size()-1];
				if (nLastLength >= EndPos) {
					LinesLength -= nLastLength-EndPos;
					if (g_LineOffsets.size() <= 1) FirstLineLength -= nLastLength-EndPos;
				}
			}

			if (SearchInLine(Lines,LinesLength,(Line==FirstLine)?StartPos:0,-1,&MatchStart,&MatchLength,NeedMatch)) {
				if (MatchStart<=FirstLineLength) {
					Relative2Absolute(Line,Lines,MatchStart,MatchLength,FirstLine,StartPos,LastLine,EndPos);
					if (NeedMatch) {
						MatchedLine=Lines;
						MatchedLineLength=LinesLength;
					}
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

int TopLine(int Line,int ScreenHeight,int TotalLines) {
	int Top;
	switch (EShowPosition) {
	case SP_TOP:
		Top=Line-EShowPositionOffset;break;
	case SP_CENTER:
		Top=Line-ScreenHeight/2-EShowPositionOffset;break;
	case SP_BOTTOM:
		Top=Line-ScreenHeight+EShowPositionOffset+1;break;
	}
	if (Top<0) Top=0;
	if (Top>=TotalLines) Top=TotalLines-1;
	return Top;
}

int LeftColumn(int RightPosition,int ScreenWidth) {
	EditorConvertPos ConvertPos={-1,RightPosition,0};
	StartupInfo.EditorControl(ECTL_REALTOTAB,&ConvertPos);
	RightPosition=ConvertPos.DestPos;

	if (RightPosition<ScreenWidth-ERightSideOffset) return 0;
	return (RightPosition-ScreenWidth+ERightSideOffset);
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

BOOL EPreparePattern(string &SearchText) {
	ECleanup(TRUE);
	if (ERegExp) {
		char *OEMLine = _strdup(SearchText.c_str());
		OEMToEditor(OEMLine, SearchText.size());

		if (ECharacterTables) pcre_free((void *)ECharacterTables);
		RefreshEditorInfo();

		if (EdInfo.TableNum >= 0) {
			CharTableSet TableSet;
			StartupInfo.CharTable(EdInfo.TableNum, (char *)&TableSet, sizeof(TableSet));
			setlocale(LC_CTYPE, ".OCP");
			ECharacterTables = far_maketables(&TableSet);
		} else if (EdInfo.AnsiMode) {
			setlocale(LC_CTYPE, ".ACP");
			ECharacterTables = pcre_maketables();
		} else {
			setlocale(LC_CTYPE, ".OCP");
			ECharacterTables = NULL;
		}

		BOOL Result = PreparePattern(&EPattern,&EPatternExtra,OEMLine,ECaseSensitive,EUTF8,ECharacterTables);
		free(OEMLine);
		return Result;
	} else {
		ETextUpcase=SearchText;
		if (!ECaseSensitive) {
			for (size_t I=0; I<SearchText.size(); I++)
				ETextUpcase[I] = UpCaseTable[(unsigned char)SearchText[I]];
		}

		PrepareBMHSearch(ETextUpcase.data(), ETextUpcase.length());
		return TRUE;
	}
}

void DeleteMatchInfo() {
	if (Match) {delete[] Match;Match=NULL;}
	MatchCount=0;
	MatchedLine=NULL;
}

void ECleanup(BOOL PatternOnly) {
	if (EPattern) {pcre_free(EPattern);EPattern=NULL;}
	if (EPatternExtra) {pcre_free(EPatternExtra);EPatternExtra=NULL;}
	if (!PatternOnly) {
		DeleteMatchInfo();
		delete ERBatch;
		delete EFBatch;
		delete ESPresets;
		delete ERPresets;
		delete EFPresets;
		delete ELPresets;
	}
}

void SynchronizeWithFile(BOOL Replace) {
	if (FSearchAs<=SA_REGEXP) {
		ERegExp=(FSearchAs==SA_REGEXP);
		EText=FText;EPreparePattern(EText);
		if (Replace) {
			ERReplace=FRReplace;
			LastAction=1;
		} else LastAction=0;
	}
	ECaseSensitive=FCaseSensitive;
	EUTF8=FUTF8;
}

int LastLine=0;
DWORD LastTickCount=0;
BOOL ClockPresent=FALSE;

void FindIfClockPresent() {
	HKEY Key;
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Far\\Screen",0,NULL,0,KEY_ALL_ACCESS,NULL,&Key,NULL);
	if (Key==INVALID_HANDLE_VALUE) return;
	QueryRegIntValue(Key,"ViewerEditorClock",&ClockPresent,0,0,1);
	RegCloseKey(Key);
}

void ShowCurrentLine(int CurLine,int TotalLines,int TotalColumns) {
	if ((CurLine<LastLine) || (CurLine>=LastLine+1000) || (GetTickCount() > LastTickCount+1000)) {
		int Position=(ClockPresent)?19:25;
		if (TotalColumns>80) Position+=(TotalColumns-81);
		Position+=23;

		char LineStr[20];
		sprintf(LineStr,"%d/%d",CurLine,TotalLines);
		StartupInfo.Text(Position+12-strlen(LineStr),0,0x30,LineStr);
		StartupInfo.Text(0,0,0x30,NULL);
		LastLine=CurLine;
		LastTickCount=GetTickCount();
	}
}

string PickupSelection() {
	EditorGetString String;

	RefreshEditorInfo();
	if (EdInfo.BlockType==BTYPE_NONE) return "";
	String.StringNumber=EdInfo.BlockStartLine+1;
	EctlGetString(&String);
	if (String.SelStart>=0) return "";

	String.StringNumber=EdInfo.BlockStartLine;
	EctlGetString(&String);
	if (String.SelEnd==-1) String.SelEnd=String.StringLength;
	char *Word=(char *)malloc(String.SelEnd-String.SelStart+1);
	strncpy(Word,String.StringText+String.SelStart,String.SelEnd-String.SelStart);
	Word[String.SelEnd-String.SelStart]=0;
	EditorToOEM(Word,String.SelEnd-String.SelStart);
	string RetWord = Word;
	free(Word);
	return RetWord;
}

BOOL IsWordChar(char C) {
	WCHAR WChar;
	MultiByteToWideChar(CP_OEMCP,0,&C,1,&WChar,1);
	return iswalnum(WChar)||(C=='_');
}

string PickupWord() {
	if (EFindTextAtCursor==FT_NONE) return "";
	RefreshEditorInfo();

	EditorGetString String;
	String.StringNumber=-1;
	EctlGetString(&String);

	if (EdInfo.CurPos>=String.StringLength) return "";

	char *WordStart=(char *)String.StringText+EdInfo.CurPos,*WordEnd=WordStart;

	if (IsWordChar(*WordStart)) {
		while (IsWordChar(*(WordStart-1))&&(WordStart>String.StringText)) WordStart--;
		while (IsWordChar(*WordEnd)&&(WordEnd<String.StringText+String.StringLength)) WordEnd++;
	} else {
		if (EFindTextAtCursor==FT_WORD) return "";
	}

	if (WordEnd==WordStart) WordEnd++;

	int WordLen=WordEnd-WordStart;
	char *Word=(char *)malloc(WordLen+1);
	strncpy(Word,WordStart,WordLen);Word[WordLen]=0;
	EditorToOEM(Word,WordLen);
	string RetWord = Word;
	free(Word);
	return RetWord;
}

string PickupText() {
	string PickUp;
	if (EFindSelection) {
		PickUp=PickupSelection();
		if (!PickUp.empty()) return PickUp;
	}
	if (EFindTextAtCursor!=FT_NONE) {
		PickUp=PickupWord();
		if (!PickUp.empty()) return PickUp;
	}
	return "";
}

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

void EctlGetString(EditorGetString *String) {
	StartupInfo.EditorControl(ECTL_GETSTRING, String);
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
}
