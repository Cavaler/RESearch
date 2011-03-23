#include "StdAfx.h"
#include "..\RESearch.h"

BOOL NoAsking;
int  ReplaceStartLine;
enum eReplaceResult {RR_OK, RR_SKIP, RR_CANCEL};

int LastReplaceLine, LastReplacePos;

void DoEditReplace(int FirstLine, int StartPos, int &LastLine, int &EndPos, const tstring &Replace) {
	EditorSetPosition Position = {-1,-1,-1,-1,-1,-1};
	EditorGetString GetString = {-1};
	int I;

	// Quite a special case
	if ((FirstLine == LastLine) && (StartPos == EndPos) && Replace.empty()) {
		EndPos++;
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
	tstring EndEOL = DefEOL;

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
		EndEOL = GetString.StringEOL;

		OriginalEndLength = GetString.StringLength-EndPos;

		NewString = strGetString.substr(0, StartPos) + Replace + strGetString2.substr(EndPos);

		Position.CurLine = FirstLine;
		EctlSetPosition(&Position);
		StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
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
	SetString.StringEOL = EndEOL.empty() ? NULL : EndEOL.c_str();
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
	Position.LeftPos = (EReverse)?-1:LeftColumn(Position.CurPos);
	if (NoAsking) {
		EctlSetPosition(&Position);
	} else {
		EctlForceSetPosition(&Position);
		EditorSelect Select = {BTYPE_NONE};
		StartupInfo.EditorControl(ECTL_REDRAW, NULL);
		StartupInfo.EditorControl(ECTL_SELECT, &Select);
	}
}

void QuoteString(const TCHAR *Source, int Length, vector<tstring> &arrQuoted, int MaxWidth) {
	tstring str;

	if (Length>MaxWidth) {
		str = tstring(Source, (MaxWidth-5)/2) + _T("...") + tstring(Source + Length-(MaxWidth-5)/2);
	} else {
		str = tstring(Source, Length);
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
eReplaceResult EditorReplaceOK(int FirstLine, int StartPos, int &LastLine, int &EndPos, const TCHAR *Original, const tstring &Replace) {
#else
eReplaceResult EditorReplaceOK(int FirstLine, int StartPos, int &LastLine, int &EndPos, const TCHAR *Original, const tstring &Replace, const tstring &Replace_O2E) {
#endif
	RefreshEditorInfo();

	LastReplaceLine = LastLine;LastReplacePos = EndPos;

	EditorSetPosition Position;
	GetHighlightPosition(Position, FirstLine, StartPos, LastLine, EndPos);
	LastReplaceLine = Position.CurLine;
	LastReplacePos = Position.CurPos;

	EditorSelect Select = {BTYPE_STREAM, FirstLine, StartPos, EndPos-StartPos, LastLine-FirstLine + 1};

	int Result;
	if (!NoAsking) {
		int Width = 0;
		if (FirstLine != LastLine) {
			const TCHAR *NewOriginal = Original + StartPos;
			for (int I = 0; I < LastLine-FirstLine; I++) {
				const TCHAR *CR = _tcschr(NewOriginal, '\n');
				if (CR) {
					Width += (CR-NewOriginal) + 1;
					NewOriginal = CR + 1;
				} else {
					Width += _tcslen(NewOriginal)-EndPos;
					break;
				}
			}
			Width += EndPos;
		} else {
			Width = EndPos-StartPos;
		}

		if (g_bIgnoreIdentReplace && (tstring(Original+StartPos, Width) == Replace)) {
			ReplaceNumber++;
			return RR_SKIP;
		}

		vector<tstring> arrFound;
		QuoteStrings(tstring(Original + StartPos, Width).c_str(), arrFound, EdInfo.WindowSizeX-12);

		vector<tstring> arrReplaced;
		QuoteStrings(Replace.c_str(), arrReplaced, EdInfo.WindowSizeX-12);

		int L, H, TotalCount = arrFound.size() + arrReplaced.size();					// Calculate dialog width
		size_t Len = 40;
		for (size_t I = 0; I<arrFound.size();I++)
			if (arrFound[I].length()>Len) Len = arrFound[I].length();

		for (size_t I = 0; I<arrReplaced.size();I++)
			if (arrReplaced[I].length()>Len) Len = arrReplaced[I].length();

		if ((int)Len > EdInfo.WindowSizeX-2) Len = EdInfo.WindowSizeX-2;

		L = Position.CurLine-Position.TopScreenLine;		// Calculate dialog position
		if (L<1 + EdInfo.WindowSizeY/2) {
			H = (EdInfo.WindowSizeY + L-9)/2;
		} else {
			H = (L-9)/2;
		}

		EctlForceSetPosition(&Position);

		//	Moved here - so that it is skipped upon IgnoreIdentReplace
		if ((Select.BlockWidth == 0) && (Select.BlockHeight == 1)) {
			Select.BlockWidth++;
			if (Select.BlockStartPos>0) {Select.BlockStartPos--;Select.BlockWidth++;}
		}
		StartupInfo.EditorControl(ECTL_SELECT, &Select);
		StartupInfo.EditorControl(ECTL_REDRAW, NULL);

		CFarDialog Dialog(-1, H + 1, Len + 8, H + 8+TotalCount, _T("ERAskReplace"));
		Dialog.AddFrame(MREReplace);
		Dialog.Add(new CFarTextItem(-1, 2, 0, MAskReplace));
		for (size_t I = 0; I<arrFound.size();I++)
			Dialog.Add(new CFarTextItem(-1, 3 + I, DIF_SETCOLOR|0x30, arrFound[I]));
		Dialog.Add(new CFarTextItem(-1, 3 + arrFound.size(), 0, MAskWith));
		for (size_t I = 0; I<arrReplaced.size();I++)
			Dialog.Add(new CFarTextItem(-1, 4 + arrFound.size() + I, DIF_SETCOLOR|0xB0, arrReplaced[I]));
		Dialog.Add(new CFarButtonItem(0, 5 + TotalCount, DIF_CENTERGROUP|DIF_NOBRACKETS, TRUE, MReplace));
		Dialog.Add(new CFarButtonItem(0, 5 + TotalCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MAll));
		Dialog.Add(new CFarButtonItem(0, 5 + TotalCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MSkip));
		Dialog.Add(new CFarButtonItem(0, 5 + TotalCount, DIF_CENTERGROUP|DIF_NOBRACKETS, FALSE, MCancel));
		Result = Dialog.Display(4,-4,-3,-2,-1);

	} else {	//	!NoAsking
		EctlSetPosition(&Position);
		Result = 0;
	}

	switch (Result) {
	case 1:
		NoAsking = TRUE;
		Select.BlockType = BTYPE_NONE;
		StartupInfo.EditorControl(ECTL_SELECT, &Select);
	case 0:
#ifdef UNICODE
		DoEditReplace(FirstLine, StartPos, LastLine, EndPos, Replace);
#else
		DoEditReplace(FirstLine, StartPos, LastLine, EndPos, Replace_O2E);
#endif
		ReplaceNumber++;
		return RR_OK;
	case 2:
		return RR_SKIP;
	default:
		NoAsking = TRUE;
		return RR_CANCEL;
	}
}

BOOL ReplaceInText(int FirstLine, int StartPos, int LastLine, int EndPos) {
	do {
		int MatchFirstLine = FirstLine, MatchStartPos = StartPos;
		int MatchLastLine = LastLine, MatchEndPos = EndPos;
		if (!SearchInText(MatchFirstLine, MatchStartPos, MatchLastLine, MatchEndPos)) return FALSE;

		// Assuming that MatchedLine starts from the needed line
		RefreshEditorInfo();

		int FoundLastLine = MatchLastLine;
		BOOL ZeroMatch = (MatchFirstLine == MatchLastLine)&&(MatchStartPos == MatchEndPos);

		REParam.AddENumbers(MatchFirstLine, MatchFirstLine-ReplaceStartLine, FindNumber, ReplaceNumber);
#ifdef UNICODE
		tstring Replace = CSO::CreateReplaceString(ERReplace.c_str(),_T("\n"), (EREvaluate ? EREvaluateScript : -1), REParam);
#else
		string Replace_O2E = CSO::CreateReplaceString(ERReplace_O2E.c_str(),"\n", (EREvaluate ? EREvaluateScript : -1), REParam);
#endif
		if (g_bInterrupted) return FALSE;	// Script failed

#ifdef UNICODE
		eReplaceResult Result = EditorReplaceOK(MatchFirstLine, MatchStartPos, MatchLastLine, MatchEndPos, REParam.m_szString, Replace);
#else
		string Replace = Replace_O2E;
		EditorToOEM(Replace);
		tstring Original = REParam.Original();
		EditorToOEM(Original);

		eReplaceResult Result = EditorReplaceOK(MatchFirstLine, MatchStartPos, MatchLastLine, MatchEndPos, Original.c_str(), Replace, Replace_O2E);
#endif

		if (Result == RR_CANCEL) return TRUE;
		if (!EReverse) LastLine += MatchLastLine-FoundLastLine;

		if (EReverse) {
			LastLine = MatchFirstLine;EndPos = MatchStartPos-((ZeroMatch)?1:0);
		} else {
			FirstLine = MatchLastLine;StartPos = MatchEndPos + ((ZeroMatch)?1:0);
		}

		FindNumber++;
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

		while (SearchInText(MatchFirstLine, FoundStartPos, MatchLastLine, FoundEndPos)) {
			Matched = TRUE;
			// Assuming that MatchedLine starts from the needed line
			RefreshEditorInfo();

			REParam.AddENumbers(MatchFirstLine, MatchFirstLine-ReplaceStartLine, FindNumber, ReplaceNumber);
#ifdef UNICODE
			tstring Replace = CSO::CreateReplaceString(ERReplace.c_str(),_T("\n"), (EREvaluate ? EREvaluateScript : -1), REParam);
#else
			string Replace_O2E = CSO::CreateReplaceString(ERReplace_O2E.c_str(),"\n", (EREvaluate ? EREvaluateScript : -1), REParam);
#endif
			if (g_bInterrupted) return FALSE;	// Script failed

#ifndef UNICODE
			string Replace = Replace_O2E;
			EditorToOEM(Replace);
			tstring Original = REParam.Original();
			EditorToOEM(Original);
#endif

			int TailLength = MatchEndPos-FoundEndPos;
			int FoundLastLine = MatchLastLine;
			BOOL ZeroMatch = (FoundStartPos == FoundEndPos);
#ifdef UNICODE
			eReplaceResult Result = EditorReplaceOK(MatchFirstLine, FoundStartPos, MatchLastLine, FoundEndPos, REParam.m_szString, Replace);
#else
			eReplaceResult Result = EditorReplaceOK(MatchFirstLine, FoundStartPos, MatchLastLine, FoundEndPos, Original.c_str(), Replace, Replace_O2E);
#endif

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

			FindNumber++;
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

void EditorFillNamedParameters()
{
	CRegExpParam<TCHAR> reParseName;
	reParseName.Compile(_T("(?<_fullname>^(?<_path>.*)\\\\(?<_name>.*)\\.(?<_ext>.*)$)"));

	FileMaskNamedParameters.clear();
	if (reParseName.Match(EdInfo.FileName))
		reParseName.BackupParam(FileMaskNamedParameters);
}

BOOL _EditorReplaceAgain() {
	NoAsking = FALSE;
	return EditorReplaceAgain();
}

BOOL EditorReplaceAgain() {
	RefreshEditorInfo();
	PatchEditorInfo(EdInfo);
	EctlForceSetPosition(NULL);
	EditorFillNamedParameters();

	EditorStartUndo();

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
	EditorSetPosition Position = {LastReplaceLine, LastReplacePos, -1,
		TopLine(LastReplaceLine), LeftColumn(LastReplacePos),-1};
	EctlForceSetPosition(&Position);

	EditorEndUndo();

	if (NoAsking||g_bInterrupted) return TRUE;
	ShowErrorMsg(GetMsg(MCannotFind), EText.c_str(), _T("ECannotFind"));
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////

void UpdateERDialog(CFarDialog *pDlg, bool bCheckSel = true) {
	bool bSeveralLines = pDlg->IsDlgItemChecked(MSeveralLine);

	if (bSeveralLines) {
		pDlg->CheckDlgItem(MRemoveEmpty,   false);
		pDlg->CheckDlgItem(MRemoveNoMatch, false);
	}
	pDlg->EnableDlgItem(MRemoveEmpty,   !bSeveralLines);
	pDlg->EnableDlgItem(MRemoveNoMatch, !bSeveralLines);

	UpdateESDialog(pDlg, bCheckSel);
}

LONG_PTR WINAPI EditorReplaceDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2) {
	int nCtlID = pDlg->GetID(nParam1);

	switch (nMsg) {
	case DN_INITDIALOG:
		UpdateERDialog(pDlg);
		HighlightREError(pDlg);
		break;
	case DN_BTNCLICK:
		switch (nCtlID) {
		case MSeveralLine:
			UpdateERDialog(pDlg, false);
			break;
		case MInSelection:
		case MRegExp:
			UpdateERDialog(pDlg, true);
			break;
		}
		break;
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}


BOOL EditorReplace() {
	RefreshEditorInfo();
	EInSelection = EAutoFindInSelection && (EdInfo.BlockType != BTYPE_NONE);

	CFarDialog Dialog(76, 17, _T("ReplaceDlg"));
	Dialog.SetWindowProc(EditorReplaceDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

	Dialog.AddFrame(MREReplace);
	Dialog.Add(new CFarTextItem(5, 2, 0, MSearchFor));
	Dialog.Add(new CFarEditItem(5, 3, 65, DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), SearchText));

	Dialog.Add(new CFarTextItem(5, 4, 0, MReplaceWith));
	Dialog.Add(new CFarEditItem(5, 5, 65, DIF_HISTORY|DIF_VAREDIT,_T("ReplaceText"), ReplaceText));

	Dialog.Add(new CFarButtonItem(67, 3, 0, 0, MQuoteSearch));
	Dialog.Add(new CFarButtonItem(67, 5, 0, 0, MQuoteReplace));

	Dialog.Add(new CFarTextItem(5, 6, DIF_BOXCOLOR|DIF_SEPARATOR, _T("")));

	Dialog.Add(new CFarCheckBoxItem(5, 7, 0, MRegExp, &ERegExp));
	Dialog.Add(new CFarCheckBoxItem(30, 7, 0, MSeveralLine, &ESeveralLine));
	Dialog.Add(new CFarButtonItem(48, 7, 0, 0, MEllipsis));

	Dialog.Add(new CFarCheckBoxItem(5, 8, 0, MCaseSensitive, &ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5, 9, 0, MReverseSearch, &EReverse));
	Dialog.Add(new CFarCheckBoxItem(30, 9, (EdInfo.BlockType != BTYPE_NONE) ? 0 : DIF_DISABLE, MInSelection, &EInSelection));
	Dialog.Add(new CFarCheckBoxItem(5, 10, 0, MRemoveEmpty, &ERRemoveEmpty));
	Dialog.Add(new CFarCheckBoxItem(30, 10, 0, MRemoveNoMatch, &ERRemoveNoMatch));
	Dialog.Add(new CFarCheckBoxItem(5, 11, 0, MEvaluateAsScript, &EREvaluate));
	Dialog.Add(new CFarComboBoxItem(30, 11, 55, 0, new CFarListData(m_lstEngines, false), EREvaluateScript));
	Dialog.Add(new CFarButtonItem(60, 11, 0, FALSE, MRunEditor));

	Dialog.Add(new CFarButtonItem(0, 13, DIF_CENTERGROUP, TRUE, MReplace));
	Dialog.Add(new CFarButtonItem(0, 13, DIF_CENTERGROUP, FALSE, MAll));
	Dialog.Add(new CFarButtonItem(0, 13, DIF_CENTERGROUP, FALSE, MCancel));
	Dialog.Add(new CFarButtonItem(60, 7, 0, FALSE, MBtnPresets));
	Dialog.Add(new CFarButtonItem(58, 8, 0, FALSE, MBtnREBuilder));

	SearchText = PickupText();
	if (SearchText.empty()) SearchText = EText;
	ReplaceText = ERReplace;

	int ExitCode;
	do {
		switch (ExitCode = Dialog.Display(-1)) {
		case MReplace:
		case MAll:
			break;
		case MQuoteSearch:
			if (ERegExp) QuoteRegExpString(SearchText);
			break;
		case MQuoteReplace:
			QuoteReplaceString(ReplaceText);
			break;
		case MEllipsis:
			ConfigureSeveralLines();
			break;
		case MBtnPresets:
			ERPresets->ShowMenu(true);
			break;
		case MRunEditor:
			RunExternalEditor(ReplaceText);
			break;
		case MBtnREBuilder:
			if (RunREBuilder(SearchText, ReplaceText)) {
				ERegExp = TRUE;
			}
			break;
		case -1:
			return FALSE;
		}
	} while (((ExitCode != MReplace) && (ExitCode != MAll)) || !EPreparePattern(SearchText));

	EText = SearchText;
#ifdef UNICODE
	ERReplace = ReplaceText;
#else
	ERReplace = ERReplace_O2E = ReplaceText;
	OEMToEditor(ERReplace_O2E);
#endif

	NoAsking = (ExitCode == MAll);
	ReplaceStartLine = -1;
	FindNumber = ReplaceNumber = 0;
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
	FindNumber = ReplaceNumber = 0;

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
	Dialog.Add(new CFarCheckBoxItem(5, 11, 0, MRemoveEmpty, &pPreset->m_mapInts["RemoveEmpty"]));
	Dialog.Add(new CFarCheckBoxItem(30, 11, 0, MRemoveNoMatch, &pPreset->m_mapInts["RemoveNoMatch"]));
	Dialog.Add(new CFarCheckBoxItem(5, 12, 0, MEvaluateAsScript, &pPreset->m_mapInts["AsScript"]));
	Dialog.Add(new CFarComboBoxItem(30, 12, 55, 0, new CFarListData(m_lstEngines, false), &pPreset->m_mapInts["Script"]));
	Dialog.Add(new CFarButtonItem(60, 12, 0, FALSE, MRunEditor));

	Dialog.Add(new CFarCheckBoxItem(5, 14, 0, MAddToMenu, &pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk, MCancel);

	do {
		switch (Dialog.Display(2, -2, -4)) {
		case 0:
			return TRUE;
		case 1:
			RunExternalEditor(pPreset->m_mapStrings["Replace"]);
			break;

		default:
			return FALSE;
		}
	} while (true);
}
