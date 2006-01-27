#include "EditFind.h"

BOOL EditorListAllAgain() {
	EditorInfo EdInfo;
	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);

	vector<string> arrString;
	vector<int>    arrLines;

	for (int CurrentLine = 0; CurrentLine < EdInfo.TotalLines; CurrentLine++) {
		int FirstLine = CurrentLine, StartPos = 0, LastLine = CurrentLine, EndPos = -1;
		if (SearchInText(FirstLine, StartPos, LastLine, EndPos, FALSE)) {
			EditorGetString String = {-1};
			StartupInfo.EditorControl(ECTL_GETSTRING, &String);

			char szNumber[8];
			sprintf(szNumber, "%3d|", CurrentLine);
			arrString.push_back(string(szNumber) + String.StringText);
			arrLines.push_back(CurrentLine);
		}
	}

	int nResult = ChooseMenu(arrString, GetMsg(MListAllLines), NULL, "ListAll", 0, FMENU_WRAPMODE);
	if (nResult >= 0) {
		EditorSetPosition Position = {arrLines[nResult], 0, -1, 
			TopLine(arrLines[nResult], EdInfo.WindowSizeY, EdInfo.TotalLines),
			0, -1};
		StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
	}

	return TRUE;
}

BOOL EditorListAll() {
	CFarDialog Dialog(76,12,"ListAllDlg");
	Dialog.AddFrame(MListAllLines);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY,"SearchText",SearchText));
	Dialog.Add(new CFarButtonItem(67,3,0,0,"&\\"));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,""));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
//	Dialog.Add(new CFarCheckBoxItem(30,5,0,MSeveralLine,&ESeveralLine));
	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(30,6,0,"",&EUTF8));
	Dialog.Add(new CFarButtonItem(34,6,0,0,MUTF8));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,5,0,0,MPresets));

	SearchText=PickupText();
	if (SearchText.empty()) SearchText=EText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(4,-3,3,-1,-5)) {
		case 0:
			break;
		case 1:
			if (ERegExp) QuoteRegExpString(SearchText);
			break;
		case 2:
//			ESPresets->ShowMenu(g_ESBatch);
			break;
		case 3:
			UTF8Converter(SearchText);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!EPreparePattern(SearchText));

	EText=SearchText;
	Interrupt=FALSE;
	if (!EText.empty()) EditorListAllAgain();
	return TRUE;
}
