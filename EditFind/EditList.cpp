#include "StdAfx.h"
#include "..\RESearch.h"

struct sFindAllInfo {
	vector<tstring> arrString;
	vector<pair<int, int> > arrLines;
};
map<tstring, sFindAllInfo> FindAllInfos;

tstring CanonicalLCName(const TCHAR *szName) {
	tstring strResult = GetFullFileName(szName);
	CharLower((LPTSTR)strResult.c_str());		// Lazy!
	return strResult;
}

BOOL EditorListAllAgain() {
	RefreshEditorInfo();
	EctlForceSetPosition(NULL);
	StartEdInfo = EdInfo;

#ifdef UNICODE
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EditorFileName.c_str())];
#else
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
#endif
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

			TCHAR szNumber[8];
			_stprintf_s(szNumber, 8, _T("%3d|"), FirstLine+1);

			tstring str = tstring(szNumber) + String.StringText;
#ifndef UNICODE
			EditorToOEM(str);
#endif
			Info.arrString.push_back(str);
			Info.arrLines.push_back(pair<int, int>(FirstLine, StartPos));
			CurrentLine = FirstLine;
		} else {
			CurrentLine = LastLine;
		}
	}

	if (!g_bInterrupted && (Info.arrLines.size() == 0)) {
		const TCHAR *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("ECannotFind"),Lines,4,1);
		RestorePosition(StartEdInfo);
		return TRUE;
	}

	EdInfo = StartEdInfo;
	return EditorListAllShowResults(true);
}

BOOL EditorListAllShowResults(bool bImmediate) {
	if (!bImmediate) RefreshEditorInfo();

#ifdef UNICODE
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EditorFileName.c_str())];
#else
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
#endif
	if (Info.arrLines.size() == 0) return TRUE;

	int nResult = ChooseMenu(Info.arrString, GetMsg(MListAllLines), NULL, _T("ListAll"), 0, FMENU_WRAPMODE|FMENU_SHOWAMPERSAND);
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
