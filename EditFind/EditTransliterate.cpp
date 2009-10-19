#include "StdAfx.h"
#include "..\RESearch.h"

BOOL EditorTransliterateAgain() {
	RefreshEditorInfo();
	StartEdInfo = EdInfo;

	EditorSetPosition Position = {0, 0, 0, -1, -1, -1};
	EditorGetString String = {-1};
	EditorSetString SetString = {-1};

	for (int nLine = 0; nLine < EdInfo.TotalLines; nLine++) {
		Position.CurLine = nLine;
		EctlSetPosition(&Position);
		EctlGetString(&String);
		tstring strData(String.StringText, String.StringLength);

		for (size_t nChar = 0; nChar < strData.length(); nChar++) {
			size_t nIndex = ETSource.find(strData[nChar]);
			if (nIndex != string::npos) {
				if (nIndex >= ETTarget.length()) {
					if (!ETTarget.empty()) {
						strData[nChar] = ETTarget[ETTarget.length()-1];
					}
				} else {
					strData[nChar] = ETTarget[nIndex];
				}
			}
		}

		if (strData != tstring(String.StringText, String.StringLength)) {
			SetString.StringText = strData.data();
			SetString.StringLength = strData.length();
			SetString.StringEOL = String.StringEOL;
			EctlSetString(&SetString);
		}
	}

	RestorePosition(StartEdInfo);

	return TRUE;
}

BOOL EditorTransliterate() {
	CFarDialog Dialog(76,13,_T("TransliterateDlg"));
	Dialog.AddFrame(MTransliterate);
	Dialog.Add(new CFarTextItem(5,2,0,MTransSource));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("SourceChars"),SearchText));
	Dialog.Add(new CFarTextItem(5,4,0,MTransTarget));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("TargetChars"),ReplaceText));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,7,0,0,MBtnPresets));

	SearchText = ETSource;
	ReplaceText = ETTarget;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(2, -3, -1)) {
		case 0:
			break;
		case 1:
			ETPresets->ShowMenu(true);
			break;
		case -1:
			return FALSE;
		}
	} while (ExitCode >= 1);

	ETSource = SearchText;
	ETTarget = ReplaceText;

	g_bInterrupted=FALSE;
	if (!ETSource.empty()) EditorTransliterateAgain();
	return TRUE;
}

OperationResult EditorTransliterateExecutor() {
	ETSource = SearchText;
	ETTarget = ReplaceText;

	return EditorTransliterateAgain() ? OR_OK : OR_CANCEL;
}

BOOL CETPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,15,_T("ETPresetDlg"));
	Dialog.AddFrame(METPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"),pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MTransSource));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("SourceChars"),SearchText));
	Dialog.Add(new CFarTextItem(5,6,0,MTransTarget));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,_T("TargetChars"),ReplaceText));

	Dialog.Add(new CFarCheckBoxItem(5,9,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	return Dialog.Display(-1) == 0;
}
