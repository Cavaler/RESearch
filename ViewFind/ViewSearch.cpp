#include "StdAfx.h"
#include "..\RESearch.h"

struct ViewerSearchInfo {
	__int64  CurPos;	// Line
	int      LeftPos;	// Column;
};

map<int, ViewerSearchInfo> g_ViewerInfo;

#ifndef UNICODE
string ToOEM(ViewerInfo &VInfo, const char *szData, int nLength) {
	if (nLength == 0) return "";

	if (VInfo.CurMode.AnsiMode) {
		vector<char> arrData(szData, szData+nLength);
		CharToOemBuff(&arrData[0], &arrData[0], nLength);
		return string(&arrData[0], arrData.size());
	}
	if (!VInfo.CurMode.UseDecodeTable || (VInfo.CurMode.TableNum >= (int)XLatTables.size())) return string(szData, nLength);

	string strResult;
	strResult.reserve(nLength);
	while (nLength--) {
		strResult += XLatTables[VInfo.CurMode.TableNum].DecodeTable[(BYTE)(*szData)];
		szData++;
	}
	return strResult;
}
#endif

tstring GetNextLine(ViewerInfo &VInfo, char *szData, int nLength, int &nSkip) {
#ifdef UNICODE
	if (VInfo.CurMode.CodePage == 1200) {
#else
	if (VInfo.CurMode.Unicode) {
#endif
		const wchar_t *wszData = (const wchar_t *)szData;
		const wchar_t *wszCur = wszData;
		while (nLength && !wcschr(L"\r\n", *wszCur)) {wszCur++; nLength--;}

		nSkip = (nLength == 0) ? 0 :
			((nLength > 1) && (wszCur[0] == '\r') && (wszCur[1] == '\n')) ? 2 : 1;

		nSkip = (nSkip + (wszCur - wszData)) * 2;
		if (wszCur == wszData) return _T("");

#ifdef UNICODE
		return wstring(wszData, wszCur);
#else
		vector<char> arrData(wszCur - wszData);
		WideCharToMultiByte(CP_OEMCP, 0, wszData, wszCur-wszData, &arrData[0], wszCur-wszData, " ", NULL);
		return string(&arrData[0], arrData.size());
#endif
	} else {
		const char *szCur = szData;
		while (nLength && !strchr("\r\n", *szCur)) {szCur++; nLength--;}

		nSkip = (nLength == 0) ? 0 :
			((nLength > 1) && (szCur[0] == '\r') && (szCur[1] == '\n')) ? 2 : 1;

		nSkip = nSkip + (szCur - szData);
#ifdef UNICODE
		return StrToUnicode(string(szData, szCur), VInfo.CurMode.CodePage);
#else
		return ToOEM(VInfo, szData, szCur-szData);
#endif
	}
}

void SetViewerSelection(__int64 nStart, int nLength) {
	ViewerSelect VSelect = {nStart, nLength};
	StartupInfo.ViewerControl(VCTL_SELECT, &VSelect);

	ViewerSetPosition VPos = {/*VSP_NOREDRAW*/0, (nStart > 256) ? nStart-256 : 0, 0};
	StartupInfo.ViewerControl(VCTL_SETPOSITION, &VPos);
	g_bInterrupted = TRUE;
}

BOOL ViewerSearchAgain() {
	ViewerInfo VInfo;
	VInfo.StructSize=sizeof(VInfo);
	StartupInfo.ViewerControl(VCTL_GETINFO, &VInfo);
	g_bInterrupted = FALSE;

	map<int, ViewerSearchInfo>::iterator it = g_ViewerInfo.find(VInfo.ViewerID);
	if (it == g_ViewerInfo.end()) {
		ViewerSearchInfo Info;
#ifdef UNICODE
		Info.CurPos = VInfo.FilePos;
		Info.LeftPos = (int)VInfo.LeftPos;
#else
		Info.CurPos = VInfo.FilePos.i64;
		Info.LeftPos = VInfo.LeftPos;
#endif
		g_ViewerInfo[VInfo.ViewerID] = Info;
	}
	ViewerSearchInfo &Info = g_ViewerInfo[VInfo.ViewerID];

	CFileMapping mapInput(VInfo.FileName);
	if (!mapInput) return FALSE;

	char *szData = mapInput;
	if (!szData) return FALSE;
#ifdef UNICODE
	szData += (VInfo.CurMode.CodePage == 1200) ? Info.CurPos*2 : Info.CurPos;
#else
	szData += (VInfo.CurMode.Unicode) ? Info.CurPos*2 : Info.CurPos;
#endif

	long nOffset = (long)Info.CurPos;
	int nMatchCount = ERegExp ? pcre_info(EPattern, NULL, NULL) + 1 : 0;
	int *pMatch = ERegExp ? new int[nMatchCount*3] : NULL;

	if (ESeveralLine) {
	} else {
		tstring strLine;
		int nCurrentLine = 0;
		int nSkip;

		int nLineOffset = Info.LeftPos;
		do {
			if (Interrupted256(nCurrentLine)) break;
			strLine = GetNextLine(VInfo, szData, mapInput.Size()-nOffset, nSkip);
			if (strLine.empty() && (nSkip == 0)) break;
			if (!ECaseSensitive) strLine = UpCaseString(strLine);

			if (ERegExp) {
				if (pcre_exec(EPattern, EPatternExtra, strLine.data(), strLine.length()-nLineOffset, nLineOffset, 0, pMatch, nMatchCount*3)>=0) {
					SetViewerSelection(nOffset + pMatch[0], pMatch[1] - pMatch[0]);
					Info.CurPos = nOffset;
					Info.LeftPos = pMatch[0] + pMatch[1];
					break;
				}
			} else {
				int nPos = BMHSearch(strLine.data()+nLineOffset, strLine.length()-nLineOffset, ETextUpcase.data(), ETextUpcase.length(), NULL);
				if (nPos >= 0) {
					SetViewerSelection(nOffset + nLineOffset + nPos, EText.length());
					Info.CurPos = nOffset;
					Info.LeftPos = nLineOffset + nPos + EText.length();
					break;
				}
			}

			szData += nSkip;
			nOffset += nSkip;
			nLineOffset = 0;
			nCurrentLine++;
		} while (nOffset < (long)mapInput.Size());
	}

	if (ERegExp) delete[] pMatch;
	if (!g_bInterrupted) {
		const TCHAR *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("VCannotFind"),Lines,4,1);
	}
	return TRUE;
}

BOOL ViewerSearch() {
	ViewerInfo VInfo;
	VInfo.StructSize=sizeof(VInfo);
	StartupInfo.ViewerControl(VCTL_GETINFO, &VInfo);
	g_ViewerInfo.erase(VInfo.ViewerID);

	CFarDialog Dialog(76,13,_T("SearchDlg"));
	Dialog.AddFrame(MRESearch);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"),SearchText));
	Dialog.Add(new CFarButtonItem(67,3,0,0,_T("&\\")));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(30,5,DIF_DISABLE,MSeveralLine,&ESeveralLine));
	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,7,DIF_DISABLE,MReverseSearch,&EReverse));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,5,0,0,MBtnPresets));

	SearchText = EText;
	ESeveralLine = FALSE;
	EReverse = FALSE;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(3,-3,3,-1)) {
		case 0:
			break;
		case 1:
			if (ERegExp) QuoteRegExpString(SearchText);
			break;
		case 2:
			VSPresets->ShowMenu(true);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!EPreparePattern(SearchText));

	EText=SearchText;
	if (!EText.empty()) ViewerSearchAgain();
	return TRUE;
}

BOOL CVSPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,14,_T("VSPresetDlg"));
	Dialog.AddFrame(MVSPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"),pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(30,7,0,MSeveralLine,&pPreset->m_mapInts["SeveralLine"]));
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

OperationResult ViewSearchExecutor() {
	return OR_CANCEL;
}
