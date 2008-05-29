#include "StdAfx.h"
#include "..\RESearch.h"

struct sFindAllInfo {
	vector<string> arrString;
	vector<pair<int, int> > arrLines;
};
map<string, sFindAllInfo> FindAllInfos;

string CanonicalLCName(const char *szName) {
	string strResult = GetFullFileName(szName);
	CharLower((LPSTR)strResult.c_str());		// Lazy!
	return strResult;
}

BOOL EditorListAllAgain() {
	RefreshEditorInfo();
	EctlForceSetPosition(NULL);

	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
	Info.arrString.clear();
	Info.arrLines.clear();

	for (int CurrentLine = 0; CurrentLine < EdInfo.TotalLines; CurrentLine++) {
		if (Interrupted256(CurrentLine)) break;
		int FirstLine = CurrentLine, StartPos = 0, EndPos = -1;
		int LastLine = (ESeveralLine) ? EdInfo.TotalLines-1 : CurrentLine;

		if (SearchInText(FirstLine, StartPos, LastLine, EndPos, FALSE)) {
			EditorSetPosition Position={FirstLine,-1,-1,-1,-1,-1};
			EctlSetPosition(&Position);
			EditorGetString String = {-1};
			EctlGetString(&String);

			char szNumber[8];
			sprintf(szNumber, "%3d|", FirstLine+1);

			string str = string(szNumber) + String.StringText;
			EditorToOEM(str);
			Info.arrString.push_back(str);
			Info.arrLines.push_back(pair<int, int>(FirstLine, StartPos));
			CurrentLine = FirstLine;
		}
	}

	if (!g_bInterrupted && (Info.arrLines.size() == 0)) {
		const char *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"ECannotFind",Lines,4,1);
		return TRUE;
	}

	return EditorListAllShowResults();
}

BOOL EditorListAllShowResults() {
	RefreshEditorInfo();

	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
	if (Info.arrLines.size() == 0) return TRUE;

	int nResult = ChooseMenu(Info.arrString, GetMsg(MListAllLines), NULL, "ListAll", 0, FMENU_WRAPMODE);
	if (nResult >= 0) {
		EditorSetPosition Position = {Info.arrLines[nResult].first, Info.arrLines[nResult].second, -1,
			TopLine(Info.arrLines[nResult].first, EdInfo.WindowSizeY, EdInfo.TotalLines),
			LeftColumn(Info.arrLines[nResult].second, EdInfo.WindowSizeY), -1};
		EctlForceSetPosition(&Position);
	} else {
		EditorSetPosition Position = {EdInfo.CurLine, EdInfo.CurPos, EdInfo.CurTabPos, 
			EdInfo.TopScreenLine, EdInfo.LeftPos, -1};
		EctlForceSetPosition(&Position);
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
			ELPresets->ShowMenu(g_ELBatch);
			break;
		case 3:
			UTF8Converter(SearchText);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!EPreparePattern(SearchText));

	EText=SearchText;
	g_bInterrupted=FALSE;
	if (!EText.empty()) EditorListAllAgain();
	return TRUE;
}

BOOL CELPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,16,"ELPresetDlg");
	Dialog.AddFrame(MEFPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"RESearch.PresetName",pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"SearchText",pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(30,8,0,"",&pPreset->m_mapInts["UTF8"]));
	Dialog.Add(new CFarButtonItem(34,8,0,0,MUTF8));
	Dialog.Add(new CFarCheckBoxItem(5,10,0,MAddToMenu,&pPreset->m_bAddToMenu));
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

OperationResult EditorListAllExecutor() {
	return OR_CANCEL;
}
