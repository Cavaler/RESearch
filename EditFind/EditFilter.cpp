#include "StdAfx.h"
#include "..\RESearch.h"

BOOL EditorFilterAgain() {
	RefreshEditorInfo();
	EctlForceSetPosition(NULL);
	EditorSetPosition Position={0,-1,-1,-1,-1,-1};
	EditorGetString String={-1};

	if (EReverse) {
		for (Position.CurLine=EdInfo.CurLine; Position.CurLine>=0; Position.CurLine--) {
			EctlSetPosition(&Position);
			EctlGetString(&String);

			if (SearchInLine(String.StringText, String.StringLength, 0, -1, NULL, NULL, FALSE) != EFLeaveFilter) {
				StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
				Position.CurLine--;
				EdInfo.TotalLines--;
			}
		}
	} else {
		for (Position.CurLine=EdInfo.CurLine; Position.CurLine<EdInfo.TotalLines; Position.CurLine++) {
			EctlSetPosition(&Position);
			EctlGetString(&String);

			if (SearchInLine(String.StringText, String.StringLength, 0, -1, NULL, NULL, FALSE) != EFLeaveFilter) {
				StartupInfo.EditorControl(ECTL_DELETESTRING, NULL);
				Position.CurLine--;
				EdInfo.TotalLines--;
			}
		}
	}

	return TRUE;
}

BOOL EditorFilter() {
	CFarDialog Dialog(76,13,"FilterDlg");
	Dialog.AddFrame(MFilterLines);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY|DIF_VAREDIT,"SearchText",SearchText));
	Dialog.Add(new CFarButtonItem(67,3,0,0,"&\\"));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,""));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MReverseSearch,&EReverse));
	Dialog.Add(new CFarRadioButtonItem(30,5,0,MLeaveMatching,&EFLeaveFilter,TRUE));
	Dialog.Add(new CFarRadioButtonItem(30,6,0,MRemoveMatching,&EFLeaveFilter,FALSE));
	Dialog.Add(new CFarCheckBoxItem(30,7,0,"",&EUTF8));
	Dialog.Add(new CFarButtonItem(34,7,0,0,MUTF8));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,5,0,0,MBtnPresets));

	SearchText=PickupText();
	if (SearchText.empty()) SearchText=EText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(4, -3, 3, -1, -5)) {
		case 0:
			break;
		case 1:
			if (ERegExp) QuoteRegExpString(SearchText);
			break;
		case 2:
			EFPresets->ShowMenu(true);
			break;
		case 3:
			UTF8Converter(SearchText);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!EPreparePattern(SearchText));

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
	CFarDialog Dialog(76,17,"EFPresetDlg");
	Dialog.AddFrame(MEFPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"RESearch.PresetName",pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY|DIF_VAREDIT,"SearchText",pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarRadioButtonItem(30,7,0,MLeaveMatching,&pPreset->m_mapInts["LeaveFilter"],TRUE));
	Dialog.Add(new CFarRadioButtonItem(30,8,0,MRemoveMatching,&pPreset->m_mapInts["LeaveFilter"],FALSE));
	Dialog.Add(new CFarCheckBoxItem(30,9,0,"",&pPreset->m_mapInts["UTF8"]));
	Dialog.Add(new CFarButtonItem(34,9,0,0,MUTF8));
	Dialog.Add(new CFarCheckBoxItem(5,11,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(2, -2, -4)) {
		case 0:
			return TRUE;
		case 1:{		// avoid Internal Error for icl
			string str = pPreset->m_mapStrings["Text"];
			UTF8Converter(str);
			break;
			  }
		default:
			return FALSE;
		}
	} while (true);
}
