#include "StdAfx.h"
#include "..\RESearch.h"

struct sFindOneInfo {
	int FirstLine;
	int StartPos;
	int LastLine;
	int EndPos;
};

struct sFindAllInfo {
	vector<tstring> arrString;
	vector<sFindOneInfo> arrLines;
};
map<tstring, sFindAllInfo> FindAllInfos;

tstring CanonicalLCName(const TCHAR *szName) {
	tstring strResult = GetFullFileName(szName);
	CharLower((LPTSTR)strResult.c_str());		// Lazy!
	return strResult;
}

bool EditorListAllAgain()
{
	RefreshEditorInfo();
	RefreshEditorColorInfo();
	EctlForceSetPosition(NULL);
	StartEdInfo = EdInfo;

	CDebugTimer tm(_T("EditListAll() took %d ms"));

#ifdef UNICODE
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EditorFileName.c_str())];
#else
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
#endif
	Info.arrString.clear();
	Info.arrLines.clear();

	for (int CurrentLine = 0; CurrentLine < EdInfo.TotalLines; CurrentLine++) {
		if (Interrupted()) break;
		int FirstLine = CurrentLine, StartPos = 0, EndPos = -1;
		int LastLine = (ESeveralLine) ? EdInfo.TotalLines-1 : CurrentLine;

		if (SearchInText(FirstLine, StartPos, LastLine, EndPos)) {
			EditorSetPosition Position={ITEM_SS(EditorSetPosition) FirstLine,-1,-1,-1,-1,-1};
			EctlSetPosition(&Position);
			EditorGetString String = {ITEM_SS(EditorGetString) -1};
			EctlGetString(&String);

			TCHAR szNumber[8];
			_stprintf_s(szNumber, 8, _T("%3d|"), FirstLine+1);

			tstring str = tstring(szNumber) + String.StringText;
#ifndef UNICODE
			EditorToOEM(str);
#endif
			Info.arrString.push_back(str);
			sFindOneInfo OneInfo;
			OneInfo.FirstLine = FirstLine;
			OneInfo.StartPos  = StartPos;
			OneInfo.LastLine  = LastLine;
			OneInfo.EndPos    = EndPos;
			Info.arrLines.push_back(OneInfo);
			CurrentLine = FirstLine;
		} else {
			CurrentLine = LastLine;
		}
	}

	tm.Stop();

	if (!g_bInterrupted && (Info.arrLines.size() == 0)) {
		const TCHAR *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(FMSG_WARNING,_T("ECannotFind"),Lines,4,1);
		RestorePosition(StartEdInfo);
		return true;
	}

	EdInfo = StartEdInfo;
	return EditorListAllShowResults(true);
}

bool EditorListAllHasResults() {
	RefreshEditorInfo();

#ifdef UNICODE
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EditorFileName.c_str())];
#else
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
#endif

	return !Info.arrLines.empty();
}

bool EditorListAllShowResults(bool bImmediate)
{
	if (!bImmediate) RefreshEditorInfo();

	EditorSetPosition Position = {ITEM_SS(EditorSetPosition) EdInfo.CurLine, EdInfo.CurPos, EdInfo.CurTabPos, 
		EdInfo.TopScreenLine, EdInfo.LeftPos, -1};
	EctlForceSetPosition(&Position);

#ifdef UNICODE
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EditorFileName.c_str())];
#else
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
#endif
	if (Info.arrLines.size() == 0) return true;

	int nBreakKeys[] = {VK_CTRL_RETURN, 0};

	tstring strTotal = FormatStr(GetMsg(MTotalLines), Info.arrLines.size());

	int nBreakKey = 0, nResult;
	for (nResult = 0; nResult < Info.arrLines.size(); nResult++)
	{
		if (Info.arrLines[nResult].FirstLine > EdInfo.CurLine)
			break;
	}

	do {
		nResult = ChooseMenu(Info.arrString, GetMsg(MListAllLines), strTotal.c_str(), _T("ListAll"), nResult, FMENU_WRAPMODE|FMENU_SHOWAMPERSAND|FMENU_RETURNCODE, nBreakKeys, &nBreakKey);
		if (nResult < 0)
			break;

		sFindOneInfo &OneInfo = Info.arrLines[nResult];
		EditorSearchOK(OneInfo.FirstLine, OneInfo.StartPos, OneInfo.LastLine, OneInfo.EndPos);
		StartupInfo.EditorControl(ECTL_REDRAW, NULL);

	} while (nBreakKey == VK_CTRL_RETURN);

	return true;
}
