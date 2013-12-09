#include "StdAfx.h"
#include "..\RESearch.h"

BOOL EditorFilterAgain()
{
	RefreshEditorInfo();
	RefreshEditorColorInfo();
	EctlForceSetPosition(NULL);

	EditorSetPosition Position = {ITEM_SS(EditorSetPosition) 0,-1,-1,-1,-1,-1};
	EditorGetString String = {ITEM_SS(EditorGetString) -1};

	CDebugTimer tm(_T("EditFilter() took %d ms"));

	EditorStartUndo();

	if (EReverse) {
		for (Position.CurLine=EdInfo.CurLine; Position.CurLine>=0; Position.CurLine--)
		{
			ShowCurrentLine(Position.CurLine,EdInfo.TotalLines,EdInfo.WindowSizeX);
			EctlSetPosition(&Position);
			EctlGetString(&String);

			if (SearchInLine(String.StringText, String.StringLength, 0, -1, NULL, NULL) != EFLeaveFilter) {
				StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
				Position.CurLine--;
				EdInfo.TotalLines--;
			}

			if (Interrupted()) break;
		}
	} else {
		for (Position.CurLine=EdInfo.CurLine; Position.CurLine<EdInfo.TotalLines; Position.CurLine++)
		{
			ShowCurrentLine(Position.CurLine,EdInfo.TotalLines,EdInfo.WindowSizeX);
			EctlSetPosition(&Position);
			EctlGetString(&String);

			if (SearchInLine(String.StringText, String.StringLength, 0, -1, NULL, NULL) != EFLeaveFilter) {
				StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
				Position.CurLine--;
				EdInfo.TotalLines--;
			}

			if (Interrupted()) break;
		}
	}

	EditorEndUndo();

	return TRUE;
}

BOOL EditorFilter()
{
	EditorFillNamedParameters();

	CFarDialog Dialog(78, 13, _T("FilterDlg"));
	Dialog.SetWindowProc(EditorSearchDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

	Dialog.AddFrame(MFilterLines);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"),SearchText));
	Dialog.Add(new CFarButtonItem(67,3,0,0,MQuoteSearch));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MReverseSearch,&EReverse));
	Dialog.Add(new CFarRadioButtonItem(30,5,0,MLeaveMatching,&EFLeaveFilter,TRUE));
	Dialog.Add(new CFarRadioButtonItem(30,6,0,MRemoveMatching,&EFLeaveFilter,FALSE));
	Dialog.AddButtons(MOk,MCancel,MBtnClose);
	Dialog.Add(new CFarButtonItem(62,9,0,0,MBtnPresets));

	SearchText=PickupText();
	if (SearchText.empty()) SearchText=EText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display()) {
		case MOk:
		case MBtnClose:
			break;
		case MQuoteSearch:
			if (ERegExp) CSO::QuoteRegExpString(SearchText);
			break;
		case MBtnPresets:
			EFPresets->ShowMenu(true);
			break;
		default:
			return FALSE;
		}
	} while (!IsOKClose(ExitCode) || !EPreparePattern(SearchText));

	EText=SearchText;
	g_bInterrupted = FALSE;

	if ((ExitCode == MOk) && !EText.empty()) EditorFilterAgain();

	return TRUE;
}

OperationResult EditorFilterExecutor()
{
	if (!EPreparePattern(SearchText)) return OR_FAILED;

	EText = SearchText;

	if (!EFromCurrentPosition) EditorSeekToBeginEnd();
	EFromCurrentPosition = false;

	BOOL bResult = EditorFilterAgain();
	StartupInfo.EditorControl(ECTL_REDRAW, NULL);
	return bResult ? OR_OK : OR_CANCEL;
}

BOOL CEFPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,17,_T("EFPresetDlg"));
	Dialog.AddFrame(MEFPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"),pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"),pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarRadioButtonItem(30,7,0,MLeaveMatching,&pPreset->m_mapInts["LeaveFilter"],TRUE));
	Dialog.Add(new CFarRadioButtonItem(30,8,0,MRemoveMatching,&pPreset->m_mapInts["LeaveFilter"],FALSE));
	Dialog.Add(new CFarCheckBoxItem(5,10,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.Add(new CFarCheckBoxItem(5,11,0,MFromCurrentPosition,&pPreset->m_mapInts["FromCurrent"]));
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
