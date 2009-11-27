#include "StdAfx.h"
#include "..\RESearch.h"

BOOL NoAsking;
int  ReplaceStartLine, ReplaceNumber;
enum eReplaceResult {RR_OK, RR_SKIP, RR_CANCEL};

int LastReplaceLine, LastReplacePos;

//	Start, length
typedef map<int, int> part_map;

tstring MatchingRE(const tstring &strSource)
{
	if (CRegExp::Match(strSource, _T("^\\d+$"), 0, NULL)) return  _T("\\d+");
	if (CRegExp::Match(strSource, _T("^\\s+$"), 0, NULL)) return  _T("\\s+");
	if (CRegExp::Match(strSource, _T("^\\w+$"), 0, NULL)) return  _T("\\w+");
	if (CRegExp::Match(strSource, _T("^\\S+$"), 0, NULL)) return  _T("\\S+");
	return _T(".+");
}

tstring BuildRE(const tstring &strSource, const part_map &mapParts)
{
	tstring strResult = strSource;
	int nSkip = 0;

	for (part_map::const_iterator it = mapParts.begin(); it != mapParts.end(); it++) {
		tstring strPart = strResult.substr(it->first - nSkip, it->second);
		tstring strRE = _T("(") + MatchingRE(strPart) + _T(")");
		strResult.replace(it->first - nSkip, it->second, strRE);
		nSkip += strPart.length()-strRE.length();
	}

	return strResult;
}

struct sREData {
	part_map mapParts;
	int      nCurrentPart;
};

tstring GetDialogText(HANDLE hDlg, int nItem) {
#ifdef UNICODE
	return (const wchar_t *)StartupInfo.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, nItem, 0);
#else
	FarDialogItem Item;
	StartupInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, nItem, (LONG_PTR)&Item);
	return Item.Data;
#endif
}

void UpdateStrings(HANDLE hDlg, sREData *pData) {
	tstring strSource = GetDialogText(hDlg, 2);
	tstring strRE = BuildRE(strSource, pData->mapParts);
	StartupInfo.SendDlgMessage(hDlg, DM_SETTEXTPTR, 4, (LONG_PTR)strRE.data());

	pcre *re;
	if (!PreparePattern(&re, NULL, strRE, ECaseSensitive)) return;

	int nMatch = pcre_info(re, NULL, NULL)+1;
	vector<int> arrMatch(nMatch*3);
	if (pcre_exec(re, NULL, strSource.c_str(), strSource.length(), 0, 0, &arrMatch[0], nMatch*3) < 0) return;

	tstring strReplace = GetDialogText(hDlg, 6);
	tstring strResult = CreateReplaceString(strSource.c_str(), &arrMatch[0], nMatch, strReplace.c_str(), _T("\n"), NULL, -1);
	StartupInfo.SendDlgMessage(hDlg, DM_SETTEXTPTR, 8, (LONG_PTR)strResult.data());
}

long WINAPI REBuilderDialogProc(HANDLE hDlg, int nMsg, int nParam1, long lParam2) {
	sREData *pData = (sREData *)StartupInfo.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);

	switch (nMsg) {
	case DN_INITDIALOG:
		StartupInfo.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, lParam2);
		break;
	case DN_DRAWDLGITEM:
		if (nParam1 == 2) {
			EditorSelect Select;
			StartupInfo.SendDlgMessage(hDlg, DM_GETSELECTION, 2, (LONG_PTR)&Select);
			pData->mapParts.erase(pData->nCurrentPart);

			if ((Select.BlockType == BTYPE_STREAM) && (Select.BlockWidth > 0)) {
				int BlockEndPos = Select.BlockStartPos + Select.BlockWidth;
				bool bNewBlock = true;
				for (part_map::const_iterator it = pData->mapParts.begin(); bNewBlock && (it != pData->mapParts.end()); it++) {
					if (Select.BlockStartPos < it->first) {
						bNewBlock = BlockEndPos <= it->first;
					} else if (Select.BlockStartPos < it->first+it->second) {
						bNewBlock = false;
					}
				}
				if (bNewBlock) {
					pData->nCurrentPart = Select.BlockStartPos;
					pData->mapParts[pData->nCurrentPart] = Select.BlockWidth;
				}
			} else {
				pData->nCurrentPart = -1;
			}

			UpdateStrings(hDlg, pData);
		}
		break;
	case DN_KEY:
		if ((nParam1 == 2) && (lParam2 == KEY_ENTER)) {
			pData->nCurrentPart = -1;
			return TRUE;
		}
		if ((nParam1 == 2) && (lParam2 == KEY_ESC)) {
			pData->nCurrentPart = -1;
			pData->mapParts.clear();
			return TRUE;
		}
		break;
	}
	return StartupInfo.DefDlgProc(hDlg, nMsg, nParam1, lParam2);
}

bool RunREBuilder(tstring &strSearch, tstring &strReplace)
{
	tstring strSource = strSearch, strRE, strResult;
	
	sREData Data;
	Data.nCurrentPart = -1;

	CFarDialog Dialog(76, 15, _T("REBuilder"));
	Dialog.SetWindowProc(REBuilderDialogProc, (LONG_PTR)&Data);
	Dialog.AddFrame(MREBuilder);
	Dialog.Add(new CFarTextItem(5, 2, 0, MRBSourceText));
	Dialog.Add(new CFarEditItem(5, 3, 70, DIF_HISTORY,_T("SearchText"), strSource));
	Dialog.Add(new CFarTextItem(5, 4, 0, MRBResultRE));
	Dialog.Add(new CFarEditItem(5, 5, 70, DIF_HISTORY|DIF_READONLY,_T("RESearch.RBResultRE"), strRE));
	Dialog.Add(new CFarTextItem(5, 6, 0, MRBReplaceText));
	Dialog.Add(new CFarEditItem(5, 7, 70, DIF_HISTORY,_T("ReplaceText"), strReplace));
	Dialog.Add(new CFarTextItem(5, 8, 0, MRBResultText));
	Dialog.Add(new CFarEditItem(5, 9, 70, DIF_HISTORY|DIF_READONLY,_T("RESearch.RBResultText"), strResult));
	Dialog.AddButtons(MOk, MCancel);

	do {
		switch (Dialog.Display(1, -2)) {
		case 0:
			strSearch = strRE;
			return true;
		case -1:
			return false;
		}
	} while (true);
}

void DoReplace(int FirstLine, int StartPos, int &LastLine, int &EndPos, const tstring &Replace) {
	EditorSetPosition Position = {-1,-1,-1,-1,-1,-1};
	EditorGetString GetString = {-1};
	int I;

	// Quite a special case
	if ((FirstLine == LastLine) && (StartPos == EndPos) && Replace.empty()) {
		EndPos++;
		ReplaceNumber++;
		return;
	}

	if (FirstLine < LastLine) {
		// Delete lines to be fully replaced
		Position.CurLine = FirstLine + 1;
		EctlSetPosition(&Position);
		for (I = LastLine-1; I>FirstLine; I--)
			StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
		if (LastLine>FirstLine + 1) LastLine = FirstLine + 1;
	}

	Position.CurLine = FirstLine;
	EctlSetPosition(&Position);
	EctlGetString(&GetString);
	tstring strGetString = ToString(GetString);
	tstring DefEOL = GetString.StringEOL;

	// Creating full replace line
	int OriginalEndLength;
	tstring NewString;

	if (LastLine == FirstLine) {
		OriginalEndLength = GetString.StringLength-EndPos;

		NewString = tstring(GetString.StringText, StartPos) + Replace +
			tstring(GetString.StringText + EndPos, GetString.StringLength-EndPos);
	} else {
		GetString.StringNumber = -1;
		Position.CurLine = LastLine;
		EctlSetPosition(&Position);
		EctlGetString(&GetString);
		tstring strGetString2 = ToString(GetString);

		OriginalEndLength = GetString.StringLength-EndPos;

		NewString = strGetString.substr(0, StartPos) + Replace + strGetString2.substr(EndPos);

		StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
		Position.CurLine = FirstLine;
		EctlSetPosition(&Position);
	}

	RefreshEditorInfo();
	EditorSetString SetString = {-1, NewString.c_str(), NULL, NULL};
//	Position.CurLine = EdInfo.CurLine;
	while (TRUE) {
		const TCHAR *CR = (const TCHAR *)_tmemchr(SetString.StringText, '\r', NewString.length());
		const TCHAR *LF = (const TCHAR *)_tmemchr(SetString.StringText, '\n', NewString.length());
		const TCHAR *EOL, *NextLine;
		if (CR) {
			EOL = (LF == CR + 1) ? _T("\x0D\x0A") : _T("\x0D");
			NextLine = CR + ((LF == CR + 1)?2:1);
		} else {
			if (!LF) break;
			CR = LF;
			EOL = DefEOL.c_str();
			NextLine = CR + 1;
		}

		SetString.StringLength = CR-SetString.StringText;
		SetString.StringEOL = EOL;
		EctlSetString(&SetString);
		Position.CurPos = SetString.StringLength;
		EctlForceSetPosition(&Position);
		Position.CurLine++;
		StartupInfo.EditorControl(ECTL_INSERTSTRING, NULL);

		NewString.erase(0, NextLine-SetString.StringText);
		SetString.StringText = NewString.c_str();
	}

	SetString.StringLength = NewString.length();
	SetString.StringEOL = DefEOL.c_str();
	EctlSetString(&SetString);

	if (EInSelection) {
		SelEndLine += Position.CurLine-LastLine;
		if ((SelType == BTYPE_STREAM) && (Position.CurLine == SelEndLine)) {
			SelEndPos += NewString.length()-OriginalEndLength-EndPos;
		}
	}
	LastReplaceLine = LastLine = Position.CurLine;
	LastReplacePos = EndPos = NewString.length()-OriginalEndLength;

	Position.CurPos = (EReverse)?StartPos:EndPos;
	Position.LeftPos = (EReverse)?-1:LeftColumn(Position.CurPos, EdInfo.WindowSizeX);
	if (NoAsking) {
		EctlSetPosition(&Position);
	} else {
		EctlForceSetPosition(&Position);
		EditorSelect Select = {BTYPE_NONE};
		StartupInfo.EditorControl(ECTL_REDRAW, NULL);
		StartupInfo.EditorControl(ECTL_SELECT, &Select);
	}
	ReplaceNumber++;
}

void QuoteString(const TCHAR *Source, int Length, vector<tstring> &arrQuoted, int MaxWidth) {
	tstring str;

	if (Length>MaxWidth) {
		str = _T("\"") + tstring(Source, (MaxWidth-5)/2) + _T("...") + tstring(Source + Length-(MaxWidth-5)/2) + _T("\"");
	} else {
		str = _T("\"") + tstring(Source, Length) + _T("\"");
	}
	arrQuoted.push_back(str);
}

void QuoteStrings(const TCHAR *Source, vector<tstring> &arrQuoted, int MaxWidth) {
	do {
		const TCHAR *Pos = _tcschr(Source,'\n');
		if (Pos) {
			QuoteString(Source, Pos-Source, arrQuoted, MaxWidth);
			Source = Pos + 1;
		} else break;
	} while (TRUE);
	QuoteString(Source, _tcslen(Source), arrQuoted, MaxWidth);
}

#ifdef UNICODE
eReplaceResult EditorReplaceOK(int FirstLine, int StartPos, int &LastLine, int &EndPos, TCHAR *Original, const tstring &Replace) {
#else
eReplaceResult EditorReplaceOK(int FirstLine, int StartPos, int &LastLine, int &EndPos, TCHAR *Original, const tstring &Replace, const tstring &Replace_O2E) {
#endif
	RefreshEditorInfo();

	LastReplaceLine = LastLine;LastReplacePos = EndPos;
	EditorSetPosition Position = {(EReverse)?FirstLine:LastLine,(EReverse)?StartPos:EndPos,-1,
		TopLine(FirstLine, EdInfo.WindowSizeY, EdInfo.TotalLines, StartEdInfo.TopScreenLine),
		LeftColumn((EReverse)?StartPos:EndPos, EdInfo.WindowSizeX),-1};
	EditorSelect Select = {BTYPE_STREAM, FirstLine, StartPos, EndPos-StartPos, LastLine-FirstLine + 1};

	EctlSetPosition(&Position);
	if (!NoAsking) {
		if ((Select.BlockWidth == 0)&&(Select.BlockHeight == 1)) {
			Select.BlockWidth++;
			if (Select.BlockStartPos>0) {Select.BlockStartPos--;Select.BlockWidth++;}
		}

		StartupInfo.EditorControl(ECTL_SELECT, &Select);
		StartupInfo.EditorControl(ECTL_REDRAW, NULL);
	}

	int Result;
	if (NoAsking) Result = 0; else {
		int Width = 0;
		if (FirstLine != LastLine) {
			TCHAR *NewOriginal = Original + StartPos;
			for (int I = 0; I < LastLine-FirstLine; I++) {
				TCHAR *CR = _tcschr(NewOriginal, '\n');
				if (CR) {
					Width += (CR-NewOriginal) + 1;
					NewOriginal = CR + 1;
				} else {
					Width += _tcslen(NewOriginal)-EndPos;
					break;
				}
			}
			Width += EndPos;
		} else Width = EndPos-StartPos;

		vector<tstring> arrFound;
		TCHAR Save = Original[StartPos + Width];
		Original[StartPos + Width] = 0;
		QuoteStrings(Original + StartPos, arrFound, EdInfo.WindowSizeX-12);
		Original[StartPos + Width] = Save;

		vector<tstring> arrReplaced;
		QuoteStrings(Replace.c_str(), arrReplaced, EdInfo.WindowSizeX-12);

		int L, H, TotalCount = arrFound.size() + arrReplaced.size();					// Calculate dialog width
		size_t Len = 30;
		for (size_t I = 0; I<arrFound.size();I++)
			if (arrFound[I].length()>Len) Len = arrFound[I].length();

		for (size_t I = 0; I<arrReplaced.size();I++)
			if (arrReplaced[I].length()>Len) Len = arrReplaced[I].length();

		if ((int)Len > EdInfo.WindowSizeX-2) Len = EdInfo.WindowSizeX-2;

		L = Position.CurLine-Position.TopScreenLine;		// Calclulate dialog position
		if (L<1 + EdInfo.WindowSizeY/2) {
			H = (EdInfo.WindowSizeY + L-9)/2;
		} else {
			H = (L-9)/2;
		}

		CFarDialog Dialog(-1, H + 1, Len + 8, H + 8+TotalCount, _T("ERAskReplace"));
		Dialog.AddFrame(MREReplace);
		Dialog.Add(new CFarTextItem(-1, 2, 0, MAskReplace));
		for (size_t I = 0; I<arrFound.size();I++)
			Dialog.Add(new CFarTextItem(-1, 3 + I, 0, arrFound[I]));
		Dialog.Add(new CFarTextItem(-1, 3 + arrFound.size(), 0, MAskWith));
		for (size_t I = 0; I<arrReplaced.size();I++)
			Dialog.Add(new CFarTextItem(-1, 4 + arrFound.size() + I, 0, arrReplaced[I]));
		Dialog.Add(new CFarButtonItem(0, 5 + TotalCount, DIF_CENTERGROUP|DIF_NOBRACKETS, TRUE, MReplace));
		Dialog.Add(new CFarButtonItem(0, 5 + TotalCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MAll));
		Dialog.Add(new CFarButtonItem(0, 5 + TotalCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MSkip));
		Dialog.Add(new CFarButtonItem(0, 5 + TotalCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MCancel));
		Result = Dialog.Display(4,-4,-3,-2,-1);
	}	//	!NoAsking

	switch (Result) {
	case 1:
		NoAsking = TRUE;
		Select.BlockType = BTYPE_NONE;
		StartupInfo.EditorControl(ECTL_SELECT, &Select);
	case 0:
#ifdef UNICODE
		DoReplace(FirstLine, StartPos, LastLine, EndPos, Replace);
#else
		DoReplace(FirstLine, StartPos, LastLine, EndPos, Replace_O2E);
#endif
		return RR_OK;
	case 2:return RR_SKIP;
	default:
		NoAsking = TRUE;
		return RR_CANCEL;
	}
}

BOOL ReplaceInText(int FirstLine, int StartPos, int LastLine, int EndPos) {
	do {
		int MatchFirstLine = FirstLine, MatchStartPos = StartPos;
		int MatchLastLine = LastLine, MatchEndPos = EndPos;
		if (!SearchInText(MatchFirstLine, MatchStartPos, MatchLastLine, MatchEndPos, TRUE)) return FALSE;

		// Assuming that MatchedLine starts from the needed line
		RefreshEditorInfo();

		int Numbers[3] = {MatchFirstLine, MatchFirstLine-ReplaceStartLine, ReplaceNumber};
		int FoundLastLine = MatchLastLine;
		BOOL ZeroMatch = (MatchFirstLine == MatchLastLine)&&(MatchStartPos == MatchEndPos);

#ifdef UNICODE
		tstring Replace = CreateReplaceString(MatchedLine, Match, MatchCount, ERReplace.c_str(),_T("\n"), Numbers,(EREvaluate ? EREvaluateScript : -1));
#else
		string Replace_O2E = CreateReplaceString(MatchedLine, Match, MatchCount, ERReplace_O2E.c_str(),"\n", Numbers,(EREvaluate ? EREvaluateScript : -1));
#endif
		if (g_bInterrupted) {	// Script failed
			break;
		}

#ifdef UNICODE
		eReplaceResult Result = EditorReplaceOK(MatchFirstLine, MatchStartPos, MatchLastLine, MatchEndPos, MatchedLine, Replace);
#else
		string Replace = Replace_O2E;
		EditorToOEM(Replace);
		EditorToOEM(MatchedLine, MatchedLineLength);

		eReplaceResult Result = EditorReplaceOK(MatchFirstLine, MatchStartPos, MatchLastLine, MatchEndPos, MatchedLine, Replace, Replace_O2E);
#endif
		DeleteMatchInfo();

		if (Result == RR_CANCEL) return TRUE;
		if (!EReverse) LastLine += MatchLastLine-FoundLastLine;

		if (EReverse) {
			LastLine = MatchFirstLine;EndPos = MatchStartPos-((ZeroMatch)?1:0);
		} else {
			FirstLine = MatchLastLine;StartPos = MatchEndPos + ((ZeroMatch)?1:0);
		}
	} while (TRUE);
	return FALSE;
}

BOOL ReplaceInTextByLine(int FirstLine, int StartPos, int LastLine, int EndPos, BOOL EachLineLimited) {
	int Line = (EReverse)?LastLine:FirstLine;

	do {
		BOOL Matched = FALSE;
		int MatchFirstLine = Line, MatchStartPos = (Line == FirstLine)||EachLineLimited?StartPos:0;
		int MatchLastLine = Line, MatchEndPos = (Line == LastLine)||EachLineLimited?EndPos:-1;
		int FoundStartPos = MatchStartPos, FoundEndPos = MatchEndPos;

		while (SearchInText(MatchFirstLine, FoundStartPos, MatchLastLine, FoundEndPos, TRUE)) {
			Matched = TRUE;
			// Assuming that MatchedLine starts from the needed line
			RefreshEditorInfo();

			int Numbers[3] = {MatchFirstLine, MatchFirstLine-ReplaceStartLine, ReplaceNumber};

#ifdef UNICODE
			tstring Replace = CreateReplaceString(MatchedLine, Match, MatchCount, ERReplace.c_str(), _T("\n"), Numbers,(EREvaluate ? EREvaluateScript : -1));
#else
			string Replace_O2E = CreateReplaceString(MatchedLine, Match, MatchCount, ERReplace_O2E.c_str(), "\n", Numbers,(EREvaluate ? EREvaluateScript : -1));
#endif
			if (g_bInterrupted) {	// Script failed
				break;
			}

#ifndef UNICODE
			string Replace = Replace_O2E;
			EditorToOEM(Replace);
			EditorToOEM(MatchedLine, MatchedLineLength);
#endif

			int TailLength = MatchEndPos-FoundEndPos;
			int FoundLastLine = MatchLastLine;
			BOOL ZeroMatch = (FoundStartPos == FoundEndPos);
#ifdef UNICODE
			eReplaceResult Result = EditorReplaceOK(MatchFirstLine, FoundStartPos, MatchLastLine, FoundEndPos, MatchedLine, Replace);
#else
			eReplaceResult Result = EditorReplaceOK(MatchFirstLine, FoundStartPos, MatchLastLine, FoundEndPos, MatchedLine, Replace, Replace_O2E);
#endif
			DeleteMatchInfo();

			if (Result == RR_CANCEL) return TRUE;
			if (!EReverse) LastLine += MatchLastLine-FoundLastLine;

			if (ERRemoveEmpty&&(Result == RR_OK)&&(MatchFirstLine == MatchLastLine)) {
				EditorSetPosition Position = {MatchFirstLine,-1,-1,-1,-1,-1};
				EctlSetPosition(&Position);
				EditorGetString String = {-1};
				EctlGetString(&String);
				if (String.StringLength == 0) {
					StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
					if (!EReverse) {Line--;LastLine--;}
					break;
				}
			}

			if (EReverse) {
				Line = MatchLastLine = MatchFirstLine;
				FoundEndPos = MatchEndPos = FoundStartPos-((ZeroMatch)?1:0);
				FoundStartPos = MatchStartPos-((ZeroMatch)?1:0);
			} else {
				Line = MatchFirstLine = MatchLastLine;
				FoundStartPos = MatchStartPos = FoundEndPos + ((ZeroMatch)?1:0);
				FoundEndPos = MatchEndPos = (MatchEndPos == -1)?-1:FoundEndPos + TailLength;
			}
		}

		if (ERRemoveNoMatch&&!Matched) {
			EditorSetPosition Position = {Line,-1,-1,-1,-1,-1};
			EctlSetPosition(&Position);
			StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
			if (EReverse) Line--; else LastLine--;
		} else (EReverse)?Line--:Line++;
	} while (!g_bInterrupted && ((EReverse)?Line >= FirstLine:Line <= LastLine));
	return FALSE;
}

BOOL _EditorReplaceAgain() {
	NoAsking = FALSE;
	return EditorReplaceAgain();
}

BOOL EditorReplaceAgain() {
	RefreshEditorInfo();
	PatchEditorInfo(EdInfo);
	EctlForceSetPosition(NULL);

	StartEdInfo = EdInfo;

#ifndef UNICODE
	m_pReplaceTable = (EdInfo.AnsiMode) ? &XLatTables[XLatTables.size()-1] :
		(EdInfo.TableNum != -1) ? &XLatTables[EdInfo.TableNum] : NULL;
#endif

	LastReplaceLine = EdInfo.CurLine;
	LastReplacePos = EdInfo.CurPos;
	if (EInSelection) {		// ***************** REPLACE IN SELECTION
		SaveSelection();
		ReplaceStartLine = SelStartLine;
		if ((!ESeveralLine)||ERRemoveEmpty||ERRemoveNoMatch) {
			ReplaceInTextByLine(SelStartLine, SelStartPos, SelEndLine, SelEndPos, SelType == BTYPE_COLUMN);
		} else {
			ReplaceInText(SelStartLine, SelStartPos, SelEndLine, SelEndPos);
		}
		RestoreSelection();
	} else {				// ***************** PLAIN REPLACE
		int FirstLine, StartPos, LastLine, EndPos;
		if (EReverse) {
			FirstLine = 0;StartPos = 0;
			LastLine = EdInfo.CurLine;EndPos = EdInfo.CurPos;
			ReplaceStartLine = 0;
		} else {
			FirstLine = EdInfo.CurLine;StartPos = EdInfo.CurPos;
			LastLine = EdInfo.TotalLines-1;EndPos = -1;
			ReplaceStartLine = EdInfo.CurLine;
		}

		if ((!ESeveralLine)||ERRemoveEmpty||ERRemoveNoMatch) {
			ReplaceInTextByLine(FirstLine, StartPos, LastLine, EndPos, FALSE);
		} else {
			ReplaceInText(FirstLine, StartPos, LastLine, EndPos);
		}
	}

	RefreshEditorInfo();
	EditorSetPosition Position = {LastReplaceLine, LastReplacePos,-1,
		TopLine(LastReplaceLine, EdInfo.WindowSizeY, EdInfo.TotalLines,StartEdInfo.TopScreenLine),
		LeftColumn(LastReplacePos, EdInfo.WindowSizeX),-1};
	EctlForceSetPosition(&Position);

	if (NoAsking||g_bInterrupted) return TRUE;
	ShowErrorMsg(GetMsg(MCannotFind), EText.c_str(), _T("ECannotFind"));
	return FALSE;
}

BOOL EditorReplace() {
	RefreshEditorInfo();
	EInSelection = EAutoFindInSelection && (EdInfo.BlockType != BTYPE_NONE);

	CFarDialog Dialog(76, 17, _T("ReplaceDlg"));
	Dialog.AddFrame(MREReplace);
	Dialog.Add(new CFarTextItem(5, 2, 0, MSearchFor));
	Dialog.Add(new CFarEditItem(5, 3, 65, DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), SearchText));

	Dialog.Add(new CFarTextItem(5, 4, 0, MReplaceWith));
	Dialog.Add(new CFarEditItem(5, 5, 65, DIF_HISTORY|DIF_VAREDIT,_T("ReplaceText"), ReplaceText));

	Dialog.Add(new CFarButtonItem(67, 3, 0, 0, _T("&\\")));
	Dialog.Add(new CFarButtonItem(67, 5, 0, 0, _T("&/")));

	Dialog.Add(new CFarTextItem(5, 6, DIF_BOXCOLOR|DIF_SEPARATOR, _T("")));

	Dialog.Add(new CFarCheckBoxItem(5, 7, 0, MRegExp, &ERegExp));
	Dialog.Add(new CFarCheckBoxItem(30, 7, 0, MSeveralLine, &ESeveralLine));
	Dialog.Add(new CFarButtonItem(48, 7, 0, 0, _T("&...")));

	Dialog.Add(new CFarCheckBoxItem(5, 8, 0, MCaseSensitive, &ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(30, 8, 0, _T(""), &EUTF8));
	Dialog.Add(new CFarButtonItem(34, 8, 0, 0, MUTF8));
	Dialog.Add(new CFarCheckBoxItem(5, 9, 0, MReverseSearch, &EReverse));
	if (EdInfo.BlockType != BTYPE_NONE) Dialog.Add(new CFarCheckBoxItem(30, 9, 0, MInSelection, &EInSelection));
	Dialog.Add(new CFarCheckBoxItem(5, 10, 0, MRemoveEmpty, &ERRemoveEmpty));
	Dialog.Add(new CFarCheckBoxItem(30, 10, 0, MRemoveNoMatch, &ERRemoveNoMatch));
	Dialog.Add(new CFarCheckBoxItem(5, 11, 0, MEvaluateAsScript, &EREvaluate));
	Dialog.Add(new CFarComboBoxItem(30, 11, 55, 0, new CFarListData(m_lstEngines, false), EREvaluateScript));
	Dialog.Add(new CFarButtonItem(60, 11, 0, FALSE, MRunEditor));

	Dialog.Add(new CFarButtonItem(0, 13, DIF_CENTERGROUP, TRUE, MReplace));
	Dialog.Add(new CFarButtonItem(0, 13, DIF_CENTERGROUP, FALSE, MAll));
	Dialog.Add(new CFarButtonItem(0, 13, DIF_CENTERGROUP, FALSE, MCancel));
	Dialog.Add(new CFarButtonItem(60, 7, 0, FALSE, MBtnPresets));
	Dialog.Add(new CFarButtonItem(59, 8, 0, FALSE, MBtnREBuilder));

	SearchText = PickupText();
	if (SearchText.empty()) SearchText = EText;
	ReplaceText = ERReplace;

	int ExitCode;
	do {
		switch (ExitCode = Dialog.Display(9, -5, -4, 5, 6, 10, -2, 13, -6, -1)) {
		case 0:
		case 1:
			break;
		case 2:
			if (ERegExp) QuoteRegExpString(SearchText);
			break;
		case 3:
			QuoteReplaceString(ReplaceText);
			break;
		case 4:
			ConfigureSeveralLines();
			break;
		case 5:
			ERPresets->ShowMenu(true);
			break;
		case 6:
			UTF8Converter(SearchText);
			break;
		case 7:
			RunExternalEditor(ReplaceText);
			break;
		case 8:
			if (RunREBuilder(SearchText, ReplaceText)) {
				ERegExp = TRUE;
			}
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode >= 2)||!EPreparePattern(SearchText));
	
	EText = SearchText;
#ifdef UNICODE
	ERReplace = ReplaceText;
#else
	ERReplace = ERReplace_O2E = ReplaceText;
	OEMToEditor(ERReplace_O2E);
#endif

	NoAsking = (ExitCode == 1);ReplaceStartLine = -1;ReplaceNumber = 0;
	g_bInterrupted = FALSE;
	if (!EText.empty()) EditorReplaceAgain();
	return TRUE;
}

OperationResult EditorReplaceExecutor() {
	if (!EPreparePattern(SearchText)) return OR_FAILED;

	EText = SearchText;
#ifdef UNICODE
	ERReplace = ReplaceText;
#else
	ERReplace = ERReplace_O2E = ReplaceText;
	OEMToEditor(ERReplace_O2E);
#endif

	NoAsking = TRUE;
	ReplaceNumber = 0;

	EditorSeekToBeginEnd();

	return EditorReplaceAgain() ? OR_OK : OR_CANCEL;
}

BOOL CERPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76, 20, _T("ERPresetDlg"));
	Dialog.AddFrame(MERPreset);
	Dialog.Add(new CFarTextItem(5, 2, 0, MPresetName));
	Dialog.Add(new CFarEditItem(5, 3, 70, DIF_HISTORY,_T("RESearch.PresetName"), pPreset->Name()));

	Dialog.Add(new CFarTextItem(5, 4, 0, MSearchFor));
	Dialog.Add(new CFarEditItem(5, 5, 70, DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5, 6, 0, MReplaceWith));
	Dialog.Add(new CFarEditItem(5, 7, 70, DIF_HISTORY|DIF_VAREDIT,_T("ReplaceText"), pPreset->m_mapStrings["Replace"]));

	Dialog.Add(new CFarCheckBoxItem(5, 9, 0, MRegExp, &pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5, 10, 0, MCaseSensitive, &pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(30, 9, 0, MSeveralLine, &pPreset->m_mapInts["SeveralLine"]));
	Dialog.Add(new CFarCheckBoxItem(30, 10, 0, _T(""), &pPreset->m_mapInts["UTF8"]));
	Dialog.Add(new CFarButtonItem(34, 10, 0, 0, MUTF8));
	Dialog.Add(new CFarCheckBoxItem(5, 11, 0, MRemoveEmpty, &pPreset->m_mapInts["RemoveEmpty"]));
	Dialog.Add(new CFarCheckBoxItem(30, 11, 0, MRemoveNoMatch, &pPreset->m_mapInts["RemoveNoMatch"]));
	Dialog.Add(new CFarCheckBoxItem(5, 12, 0, MEvaluateAsScript, &pPreset->m_mapInts["AsScript"]));
	Dialog.Add(new CFarComboBoxItem(30, 12, 55, 0, new CFarListData(m_lstEngines, false), &pPreset->m_mapInts["Script"]));
	Dialog.Add(new CFarButtonItem(60, 12, 0, FALSE, MRunEditor));

	Dialog.Add(new CFarCheckBoxItem(5, 14, 0, MAddToMenu, &pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk, MCancel);

	do {
		switch (Dialog.Display(3, -2, -9, -4)) {
		case 0:
			return TRUE;
		case 1:{		// avoid Internal Error for icl
			tstring str = pPreset->m_mapStrings["Text"];
			UTF8Converter(str);
			break;
			  }
		case 2:
			RunExternalEditor(pPreset->m_mapStrings["Replace"]);
			break;

		default:
			return FALSE;
		}
	} while (true);
}
