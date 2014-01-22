#include "StdAfx.h"
#include "..\RESearch.h"

void EditorSearchOK(int FirstLine,int StartPos,int LastLine,int EndPos)
{
	RefreshEditorInfo();

	EditorSelect Select={ITEM_SS(EditorSelect) BTYPE_STREAM,FirstLine,StartPos,EndPos-StartPos,LastLine-FirstLine+1};
	if ((Select.BlockWidth == 0) && (Select.BlockHeight == 1)) {
		Select.BlockWidth++;
		if (Select.BlockStartPos>0) {Select.BlockStartPos--;Select.BlockWidth++;}
	}
	StartupInfo.EditorControl(ECTL_SELECT,&Select);

	EditorSetPosition Position;
	GetHighlightPosition(Position, FirstLine, StartPos, LastLine, EndPos);
	EctlForceSetPosition(&Position);
}

void PatchEditorInfo(EditorInfo &EdInfo)
{
	// Skipping over selection - for "Search Again inverse"

	if (ESearchAgainCalled && (EdInfo.BlockType == BTYPE_STREAM)) {
		EditorGetString String = {ITEM_SS(EditorGetString) EdInfo.BlockStartLine};
		EctlGetString(&String);
		int BlockStartPos = String.SelStart;
		while (String.SelEnd == -1) {
			String.StringNumber++;
			EctlGetString(&String);
		}
		int BlockEndLine = String.StringNumber;
		int BlockEndPos = String.SelEnd;

		if (EReverse) {
			if ((EdInfo.CurLine == BlockEndLine) && (EdInfo.CurPos == BlockEndPos)) {
				EdInfo.CurLine = EdInfo.BlockStartLine;
				EdInfo.CurPos = BlockStartPos;
			}
		} else {
			if ((EdInfo.CurLine == EdInfo.BlockStartLine) && (EdInfo.CurPos == BlockStartPos)) {
				EdInfo.CurLine = BlockEndLine;
				EdInfo.CurPos = BlockEndPos;
			} else

			// Skipping selection for zero-width search
			if ((EdInfo.CurLine == EdInfo.BlockStartLine) && (EdInfo.BlockStartLine == BlockEndLine) &&
				(BlockStartPos == EdInfo.CurPos-1) && (BlockEndPos == EdInfo.CurPos+1)) {
				EdInfo.CurLine = BlockEndLine;
				EdInfo.CurPos = BlockEndPos;
			}
		}
	}
}

bool EditorSearchAgain()
{
	RefreshEditorInfo();
	RefreshEditorColorInfo();
	EctlForceSetPosition(NULL);
	ClearLineBuffer();

	if (!EPreparePattern(SearchText)) return false;

	CDebugTimer tm(_T("EditSearch() took %d ms"));

	PatchEditorInfo(EdInfo);
	StartEdInfo = EdInfo;

	g_bInterrupted = false;

	int FirstLine,StartPos,LastLine,EndPos;

	if (EInSelection) {		// ***************** SEARCH IN SELECTION
		if (ESearchAgainCalled) {
			if (EReverse) {
				FirstLine = SelStartLine;
				StartPos = SelStartPos;

				EditorGetString String;
				String.StringNumber = LastLine = EdInfo.BlockStartLine;
				EctlGetString(&String);

				EndPos = String.SelEnd-1;
			} else {
				FirstLine = EdInfo.CurLine;
				StartPos = EdInfo.CurPos;
				LastLine = SelEndLine;
				EndPos = SelEndPos;
			}
		} else {
			SaveSelection();
			FirstLine = SelStartLine;
			StartPos = SelStartPos;
			LastLine = SelEndLine;
			EndPos = SelEndPos;
		}
	} else {
		if (EReverse) {
			FirstLine=0;StartPos=0;
			LastLine=EdInfo.CurLine;EndPos=EdInfo.CurPos;
		} else {
			FirstLine=EdInfo.CurLine;StartPos=EdInfo.CurPos;
			LastLine=EdInfo.TotalLines-1;EndPos=-1;
		}
	}

	if (ESeveralLine) {
		if (SearchInText(FirstLine,StartPos,LastLine,EndPos)) {
			EditorSearchOK(FirstLine,StartPos,LastLine,EndPos);
			return true;
		}
	} else {
		if (EReverse) {
			for (int Line = LastLine; !g_bInterrupted && (Line >= FirstLine); Line--) {
				int CurLastLine = Line;
				int CurStartPos = (Line == FirstLine) ? StartPos : 0;
				int CurEndPos = (Line == LastLine) ? EndPos : -1;
				if (SearchInText(Line, CurStartPos, CurLastLine, CurEndPos)) {
					EditorSearchOK(Line, CurStartPos, CurLastLine, CurEndPos);
					return true;
				}
			}
		} else {
			for (int Line = FirstLine; !g_bInterrupted && (Line<=LastLine); Line++) {
				int CurLastLine = Line;
				int CurStartPos = (Line == FirstLine) ? StartPos : 0;
				int CurEndPos = (Line == LastLine) ? EndPos : -1;
				if (SearchInText(Line, CurStartPos, CurLastLine, CurEndPos)) {
					EditorSearchOK(Line, CurStartPos, CurLastLine, CurEndPos);
					return true;
				}
			}
		}
	}

	RestorePosition(StartEdInfo);

	tm.Stop();

	if (!g_bInterrupted && !EIncremental) {
		const TCHAR *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(FMSG_WARNING,_T("ECannotFind"),Lines,4,1);
	}

	if (EInSelection) RestoreSelection();

	return false;
}

//////////////////////////////////////////////////////////////////////////

void UpdateESDialog(CFarDialog *pDlg, bool bCheckSel)
{
	bool bIncremental = pDlg->IsDlgItemChecked(MIncrementalSearch);
	if (bIncremental) {
		pDlg->CheckDlgItem (MInSelection,   false);
		pDlg->EnableDlgItem(MInSelection,   false);
		pDlg->CheckDlgItem (MReverseSearch, false);
		pDlg->EnableDlgItem(MReverseSearch, false);
	} else {
		pDlg->EnableDlgItem(MInSelection,   true);
		pDlg->EnableDlgItem(MReverseSearch, true);
	}

	if (EdInfo.BlockType == BTYPE_COLUMN) {
		if (bCheckSel) {
			if (pDlg->IsDlgItemChecked(MInSelection)) {
				pDlg->CheckDlgItem (MSeveralLine, false);
				pDlg->EnableDlgItem(MSeveralLine, false);
			} else {
				pDlg->EnableDlgItem(MSeveralLine, true);
			}
		} else {
			if (pDlg->IsDlgItemChecked(MSeveralLine)) {
				pDlg->CheckDlgItem (MInSelection, false);
				pDlg->EnableDlgItem(MInSelection, false);
			} else {
				pDlg->EnableDlgItem(MInSelection, true);
			}
		}
	}

	bool bRegExp = pDlg->IsDlgItemChecked(MRegExp);
	pDlg->EnableDlgItem(MQuoteSearch, bRegExp);

	if (pDlg->HasItem(MQuoteReplace)) {
		pDlg->EnableDlgItem(MQuoteReplace, bRegExp || g_bEscapesInPlainText);
	}
}

bool FetchEditorSearchSettings(CFarDialog *pDlg)
{
	EIncremental = pDlg->IsDlgItemChecked(MIncrementalSearch);
	if (!EIncremental) return false;

	SearchText     = EText = pDlg->GetDlgItemText(pDlg->MakeID(MSearchFor, 1));
	ERegExp        = pDlg->IsDlgItemChecked(MRegExp);
	ESeveralLine   = pDlg->IsDlgItemChecked(MSeveralLine);
	ECaseSensitive = pDlg->IsDlgItemChecked(MCaseSensitive);
	EInSelection   = false;
	EReverse       = false;

	return true;
}

void CheckIncrementalSearch(CFarDialog *pDlg, bool bNext = false, bool bPrev = false)
{
	if (!FetchEditorSearchSettings(pDlg)) return;
	ESearchAgainCalled = bNext;
	EReverse           = bPrev;
	EditorSearchAgain();
	StartupInfo.EditorControl(ECTL_REDRAW, NULL);
	StartupInfo.SendDlgMessage(pDlg->hDlg(), DM_REDRAW, 0, 0);
}

LONG_PTR WINAPI EditorSearchDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2)
{
	int nCtlID  = pDlg->GetID(nParam1);

	switch (nMsg) {
	case DN_INITDIALOG:
		UpdateESDialog(pDlg);
		HighlightREError(pDlg);
		break;
	case DN_EDITCHANGE:
		switch (nParam1) {
		case 2:
			CheckIncrementalSearch(pDlg);
			break;
		}
		break;
	case DN_BTNCLICK:
		switch (nCtlID) {
		case MSeveralLine:
			UpdateESDialog(pDlg, false);
			CheckIncrementalSearch(pDlg);
			break;
		case MInSelection:
		case MRegExp:
			UpdateESDialog(pDlg, true);
			CheckIncrementalSearch(pDlg);
			break;
		case MCaseSensitive:
			CheckIncrementalSearch(pDlg);
			break;
		case MIncrementalSearch:
			UpdateESDialog(pDlg, true);
			pDlg->SetFocus(MSearchFor, 1);
			break;
		}
		break;
#ifdef FAR3
	case DN_CONTROLINPUT:
		if (!pDlg->IsDlgItemChecked(MIncrementalSearch)) break;
		if (pDlg->GetFocus() != pDlg->MakeID(MSearchFor, 1)) break;
		INPUT_RECORD *record = (INPUT_RECORD *)lParam2;
		if ((record->EventType == KEY_EVENT) && (record->Event.KeyEvent.bKeyDown)) {
			if ((record->Event.KeyEvent.wVirtualKeyCode == VK_RIGHT) &&
				(record->Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)))
			{
				CheckIncrementalSearch(pDlg, true, false);
				return true;
			}
			if ((record->Event.KeyEvent.wVirtualKeyCode == VK_LEFT) &&
				(record->Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)))
			{
				CheckIncrementalSearch(pDlg, false, true);
				return true;
			}
		}
		break;
#else
	case DN_KEY:
		if (!pDlg->IsDlgItemChecked(MIncrementalSearch)) break;
		if ((nParam1 == 2) && (lParam2 == KEY_ALTRIGHT)) {
			CheckIncrementalSearch(pDlg, true);
			return true;
		}
		break;
#endif
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

bool EditorSearch()
{
	EditorFillNamedParameters();

	EInSelection = EAutoFindInSelection && (EdInfo.BlockType != BTYPE_NONE);

	CFarDialog Dialog(80, 14, _T("SearchDlg"));
	Dialog.SetWindowProc(EditorSearchDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

	Dialog.AddFrame(MRESearch);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,69,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"),SearchText));
	Dialog.Add(new CFarButtonItem(71,3,0,0,MQuoteSearch));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(35,5,0,MSeveralLine,&ESeveralLine));
	Dialog.Add(new CFarButtonItem(53,5,0,0,MEllipsis));

	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MReverseSearch,&EReverse));
	Dialog.Add(new CFarCheckBoxItem(35,7,(EdInfo.BlockType != BTYPE_NONE) ? 0 : DIF_DISABLE, MInSelection, &EInSelection));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MIncrementalSearch,&EIncremental));
	Dialog.AddButtons(MOk, MShowAll, MCountAll, MCancel, MBtnClose);
	Dialog.Add(new CFarButtonItem(64,8,0,0,MBtnPresets));

	SearchText=PickupText();
	if (SearchText.empty()) SearchText=EText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display()) {
		case MOk:
		case MShowAll:
		case MCountAll:
		case MBtnClose:
			break;
		case MQuoteSearch:
			if (ERegExp) CSO::QuoteRegExpString(SearchText);
			break;
		case MEllipsis:
			ConfigureSeveralLines();
			break;
		case MBtnPresets:
			ESPresets->ShowMenu(true);
			break;
		default:
			return false;
		}
	} while (((ExitCode != MOk) && (ExitCode != MShowAll) && (ExitCode != MCountAll) && (ExitCode != MBtnClose)) || !EPreparePattern(SearchText));

	EText = SearchText;
	g_bInterrupted = false;

	if (EText.empty()) return false;

	switch (ExitCode) {
	case MOk:
		return EditorSearchAgain();
	case MShowAll:
		return EditorListAllAgain();
	case MCountAll:
		return EditorCountAllAgain();
	}

	return true;
}

OperationResult EditorSearchExecutor()
{
	if (!EPreparePattern(SearchText)) return OR_FAILED;
	EText = SearchText;

	EditorUpdatePresetPosition();

	return (EListAllFromPreset) ?
		(EditorListAllAgain() ? OR_OK : OR_CANCEL) :
		(ECountAllFromPreset) ?
		(EditorCountAllAgain() ? OR_OK : OR_CANCEL) :
		(EditorSearchAgain()   ? OR_OK : OR_CANCEL);
}

void UpdateESPDialog(CFarDialog *pDlg)
{
	bool bListAll = pDlg->IsDlgItemChecked(MListAllFromPreset);
	bool bCountAll = pDlg->IsDlgItemChecked(MCountAllFromPreset);

	pDlg->EnableCheckDlgItem(MFromCurrentPosition, !bListAll && !bCountAll);
	pDlg->EnableCheckDlgItem(MInSelection,         !bListAll && !bCountAll);

	bool bFromCurrent = pDlg->IsDlgItemChecked(MFromCurrentPosition);
	bool bInSelection = pDlg->IsDlgItemChecked(MInSelection);
	bool bAddToMenu   = pDlg->IsDlgItemChecked(MAddToMenu);

	pDlg->EnableCheckDlgItem(MListAllFromPreset,  !bFromCurrent && !bInSelection && !bCountAll && bAddToMenu);
	pDlg->EnableCheckDlgItem(MCountAllFromPreset, !bFromCurrent && !bInSelection && !bListAll  && bAddToMenu);
}

LONG_PTR WINAPI EditorSearchPresetDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2)
{
	int nCtlID  = pDlg->GetID(nParam1);

	switch (nMsg) {
	case DN_INITDIALOG:
		UpdateESPDialog(pDlg);
		break;
	case DN_BTNCLICK:
		UpdateESPDialog(pDlg);
		break;
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

bool CESPresetCollection::EditPreset(CPreset *pPreset)
{
	CFarDialog Dialog(80, 20, _T("ESPresetDlg"));
	Dialog.SetWindowProc(EditorSearchPresetDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

	Dialog.AddFrame(MESPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,74,DIF_HISTORY,_T("RESearch.PresetName"),pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,74,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(35,7,0,MSeveralLine,&pPreset->m_mapInts["SeveralLine"]));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(5,9,0,MReverseSearch,&pPreset->m_mapInts["Reverse"]));
	Dialog.Add(new CFarCheckBoxItem(35,9,0,MInSelection, &pPreset->m_mapInts["InSelection"]));

	Dialog.Add(new CFarCheckBoxItem(5,11,0,MFromCurrentPosition,&pPreset->m_mapInts["FromCurrent"]));
	Dialog.Add(new CFarCheckBoxItem(5,12,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MListAllFromPreset,&pPreset->m_mapInts["ListAll"]));
	Dialog.Add(new CFarCheckBoxItem(5,14,0,MCountAllFromPreset,&pPreset->m_mapInts["CountAll"]));

	Dialog.AddButtons(MOk, MCancel);

	return (Dialog.Display() == MOk);
}
