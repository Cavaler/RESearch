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
	StartEdInfo = EdInfo;

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
		} else {
			CurrentLine = LastLine;
		}
	}

	if (!g_bInterrupted && (Info.arrLines.size() == 0)) {
		const char *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"ECannotFind",Lines,4,1);
		RestorePosition(StartEdInfo);
		return TRUE;
	}

	EdInfo = StartEdInfo;
	return EditorListAllShowResults(true);
}

BOOL EditorListAllShowResults(bool bImmediate) {
	if (!bImmediate) RefreshEditorInfo();

	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
	if (Info.arrLines.size() == 0) return TRUE;

	int nResult = ChooseMenu(Info.arrString, GetMsg(MListAllLines), NULL, "ListAll", 0, FMENU_WRAPMODE);
	if (nResult >= 0) {
		EditorSetPosition Position = {Info.arrLines[nResult].first, Info.arrLines[nResult].second, -1,
			TopLine(Info.arrLines[nResult].first, EdInfo.WindowSizeY, EdInfo.TotalLines,StartEdInfo.TopScreenLine),
			LeftColumn(Info.arrLines[nResult].second, EdInfo.WindowSizeY), -1};
		EctlForceSetPosition(&Position);
	} else {
		EditorSetPosition Position = {EdInfo.CurLine, EdInfo.CurPos, EdInfo.CurTabPos, 
			EdInfo.TopScreenLine, EdInfo.LeftPos, -1};
		EctlForceSetPosition(&Position);
	}

	return TRUE;
}
