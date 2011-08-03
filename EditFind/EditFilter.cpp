#include "StdAfx.h"
#include "..\RESearch.h"

BOOL EditorFilterAgain() {
	RefreshEditorInfo();
	EctlForceSetPosition(NULL);
	EditorSetPosition Position={0,-1,-1,-1,-1,-1};
	EditorGetString String={-1};

	EditorStartUndo();

	if (EReverse) {
		for (Position.CurLine=EdInfo.CurLine; Position.CurLine>=0; Position.CurLine--) {
			EctlSetPosition(&Position);
			EctlGetString(&String);

			if (SearchInLine(String.StringText, String.StringLength, 0, -1, NULL, NULL) != EFLeaveFilter) {
				StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
				Position.CurLine--;
				EdInfo.TotalLines--;
			}
		}
	} else {
		for (Position.CurLine=EdInfo.CurLine; Position.CurLine<EdInfo.TotalLines; Position.CurLine++) {
			EctlSetPosition(&Position);
			EctlGetString(&String);

			if (SearchInLine(String.StringText, String.StringLength, 0, -1, NULL, NULL) != EFLeaveFilter) {
				StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
				Position.CurLine--;
				EdInfo.TotalLines--;
			}
		}
	}

	EditorEndUndo();

	return TRUE;
}

BOOL EditorFilter()
{
	EditorFillNamedParameters();

	CFarDialog Dialog(76,13,_T("FilterDlg"));
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
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,5,0,0,MBtnPresets));

	SearchText=PickupText();
	if (SearchText.empty()) SearchText=EText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(-1)) {
		case MOk:
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
	} while ((ExitCode != MOk) || !EPreparePattern(SearchText));

	EText=SearchText;
	if (!EText.empty()) EditorFilterAgain();
	return TRUE;
}

OperationResult EditorFilterExecutor() {
	if (!EPreparePattern(SearchText)) return OR_FAILED;

	EText = SearchText;
	return EditorFilterAgain() ? OR_OK : OR_CANCEL;
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
	Dialog.Add(new CFarCheckBoxItem(5,11,0,MAddToMenu,&pPreset->m_bAddToMenu));
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
