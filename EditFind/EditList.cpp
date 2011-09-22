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

BOOL EditorListAllAgain() {
	RefreshEditorInfo();
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
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("ECannotFind"),Lines,4,1);
		RestorePosition(StartEdInfo);
		return TRUE;
	}

	EdInfo = StartEdInfo;
	return EditorListAllShowResults(true);
}

BOOL EditorListAllHasResults() {
	RefreshEditorInfo();

#ifdef UNICODE
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EditorFileName.c_str())];
#else
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
#endif

	return !Info.arrLines.empty();
}

BOOL EditorListAllShowResults(bool bImmediate) {
	if (!bImmediate) RefreshEditorInfo();

#ifdef UNICODE
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EditorFileName.c_str())];
#else
	sFindAllInfo &Info = FindAllInfos[CanonicalLCName(EdInfo.FileName)];
#endif
	if (Info.arrLines.size() == 0) return TRUE;

	tstring strTotal = FormatStr(GetMsg(MTotalLines), Info.arrLines.size());
	int nResult = ChooseMenu(Info.arrString, GetMsg(MListAllLines), strTotal.c_str(), _T("ListAll"), 0, FMENU_WRAPMODE|FMENU_SHOWAMPERSAND);
	if (nResult >= 0) {
		sFindOneInfo &OneInfo = Info.arrLines[nResult];
		EditorSearchOK(OneInfo.FirstLine, OneInfo.StartPos, OneInfo.LastLine, OneInfo.EndPos);
	} else {
		EditorSetPosition Position = {EdInfo.CurLine, EdInfo.CurPos, EdInfo.CurTabPos, 
			EdInfo.TopScreenLine, EdInfo.LeftPos, -1};
		EctlForceSetPosition(&Position);
	}

	return TRUE;
}
