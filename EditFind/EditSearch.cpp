#include "StdAfx.h"
#include "..\RESearch.h"

void EditorSearchOK(int FirstLine,int StartPos,int LastLine,int EndPos) {
	RefreshEditorInfo();

	EditorSelect Select={BTYPE_STREAM,FirstLine,StartPos,EndPos-StartPos,LastLine-FirstLine+1};
	if ((Select.BlockWidth == 0) && (Select.BlockHeight == 1)) {
		Select.BlockWidth++;
		if (Select.BlockStartPos>0) {Select.BlockStartPos--;Select.BlockWidth++;}
	}
	StartupInfo.EditorControl(ECTL_SELECT,&Select);

	EditorSetPosition Position;
	GetHighlightPosition(Position, FirstLine, StartPos, LastLine, EndPos);
	EctlForceSetPosition(&Position);
}

void PatchEditorInfo(EditorInfo &EdInfo) {
	// Skipping over selection - for "Search Again inverse"

	if (ESearchAgainCalled && (EdInfo.BlockType == BTYPE_STREAM)) {
		EditorGetString String = {EdInfo.BlockStartLine};
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

BOOL EditorSearchAgain()
{
	RefreshEditorInfo();
	PatchEditorInfo(EdInfo);
	EctlForceSetPosition(NULL);
	if (!EPreparePattern(SearchText)) return FALSE;

	CDebugTimer tm(_T("EditSearch() took %d ms"));

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
			return TRUE;
		}
	} else {
		if (EReverse) {
			for (int Line = LastLine; !g_bInterrupted && (Line >= FirstLine); Line--) {
				int CurLastLine = Line;
				int CurStartPos = (Line == FirstLine) ? StartPos : 0;
				int CurEndPos = (Line == LastLine) ? EndPos : -1;
				if (SearchInText(Line, CurStartPos, CurLastLine, CurEndPos)) {
					EditorSearchOK(Line, CurStartPos, CurLastLine, CurEndPos);
					return TRUE;
				}
			}
		} else {
			for (int Line = FirstLine; !g_bInterrupted && (Line<=LastLine); Line++) {
				int CurLastLine = Line;
				int CurStartPos = (Line == FirstLine) ? StartPos : 0;
				int CurEndPos = (Line == LastLine) ? EndPos : -1;
				if (SearchInText(Line, CurStartPos, CurLastLine, CurEndPos)) {
					EditorSearchOK(Line, CurStartPos, CurLastLine, CurEndPos);
					return TRUE;
				}
			}
		}
	}

	RestorePosition(StartEdInfo);

	tm.Stop();

	if (!g_bInterrupted) {
		const TCHAR *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("ECannotFind"),Lines,4,1);
	}

	if (EInSelection) RestoreSelection();
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////

void UpdateESDialog(CFarDialog *pDlg, bool bCheckSel) {
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

LONG_PTR WINAPI EditorSearchDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2) {
	int nCtlID = pDlg->GetID(nParam1);

	switch (nMsg) {
	case DN_INITDIALOG:
		UpdateESDialog(pDlg);
		HighlightREError(pDlg);
		break;
	case DN_BTNCLICK:
		switch (nCtlID) {
		case MSeveralLine:
			UpdateESDialog(pDlg, false);
			break;
		case MInSelection:
		case MRegExp:
			UpdateESDialog(pDlg, true);
			break;
		}
		break;
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

BOOL EditorSearch()
{
	EditorFillNamedParameters();

	EInSelection = EAutoFindInSelection && (EdInfo.BlockType != BTYPE_NONE);

	CFarDialog Dialog(76,13,_T("SearchDlg"));
	Dialog.SetWindowProc(EditorSearchDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

	Dialog.AddFrame(MRESearch);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"),SearchText));
	Dialog.Add(new CFarButtonItem(67,3,0,0,MQuoteSearch));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(30,5,0,MSeveralLine,&ESeveralLine));
	Dialog.Add(new CFarButtonItem(48,5,0,0,MEllipsis));

	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MReverseSearch,&EReverse));
	Dialog.Add(new CFarCheckBoxItem(30,7,(EdInfo.BlockType != BTYPE_NONE) ? 0 : DIF_DISABLE, MInSelection, &EInSelection));
	Dialog.AddButtons(MOk, MShowAll);
	Dialog.AddButton(MCancel);
	Dialog.Add(new CFarButtonItem(60,5,0,0,MBtnPresets));

	SearchText=PickupText();
	if (SearchText.empty()) SearchText=EText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(-1)) {
		case MOk:
			break;
		case MShowAll:
			// Show All
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
			return FALSE;
		}
	} while (((ExitCode != MOk) && (ExitCode != MShowAll)) || !EPreparePattern(SearchText));

	EText=SearchText;
	g_bInterrupted=FALSE;
	if (!EText.empty()) (ExitCode == MOk) ? EditorSearchAgain() : EditorListAllAgain();
	return TRUE;
}

OperationResult EditorSearchExecutor() {
	if (!EPreparePattern(SearchText)) return OR_FAILED;
	EText = SearchText;

//	Search from current position is more handy
//	EditorSeekToBeginEnd();

	return (EListAllFromPreset) ?
		EditorListAllAgain() ? OR_OK : OR_CANCEL :
		EditorSearchAgain() ? OR_OK : OR_CANCEL;
}

BOOL CESPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,16,_T("ESPresetDlg"));
	Dialog.AddFrame(MESPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"),pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(35,7,0,MSeveralLine,&pPreset->m_mapInts["SeveralLine"]));
	Dialog.Add(new CFarCheckBoxItem(5,10,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.Add(new CFarCheckBoxItem(35,10,0,MListAllFromPreset,&pPreset->m_mapInts["ListAll"]));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(1, -2)) {
		case 0:
			return TRUE;
		default:
			return FALSE;
		}
	} while (true);
}
