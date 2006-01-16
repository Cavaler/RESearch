#include "EditFind.h"

BOOL EditorTransliterateAgain() {
	EditorInfo EdInfo;
	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);

	EditorSetPosition Position = {0, 0, 0, -1, -1, -1};
	EditorGetString String = {-1};
	EditorSetString SetString = {-1};

	for (int nLine = 0; nLine < EdInfo.TotalLines; nLine++) {
		Position.CurLine = nLine;
		StartupInfo.EditorControl(ECTL_SETPOSITION, &Position);

		StartupInfo.EditorControl(ECTL_GETSTRING,&String);
		string strData(String.StringText, String.StringLength);

		for (int nChar = 0; nChar < strData.length(); nChar++) {
			int nIndex = ETSource.find(strData[nChar]);
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

		SetString.StringText = strData.data();
		SetString.StringLength = strData.length();
		SetString.StringEOL = String.StringEOL;
		StartupInfo.EditorControl(ECTL_SETSTRING, &SetString);
	}

	return TRUE;
}

BOOL EditorTransliterate() {
	CFarDialog Dialog(76,13,"TransliterateDlg");
	Dialog.AddFrame(MTransliterate);
	Dialog.Add(new CFarTextItem(5,2,0,MTransSource));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"SourceChars",SearchText));
	Dialog.Add(new CFarTextItem(5,4,0,MTransTarget));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"TargetChars",ReplaceText));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,7,0,0,MPresets));

	SearchText = ETSource;
	ReplaceText = ETTarget;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(2, -3, -1)) {
		case 0:
			break;
		case -1:
			return FALSE;
		}
	} while (ExitCode >= 1);

	ETSource = SearchText;
	ETTarget = ReplaceText;

	Interrupt=FALSE;
	if (!ETSource.empty()) EditorTransliterateAgain();
	return TRUE;
}