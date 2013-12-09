#include "StdAfx.h"
#include "..\RESearch.h"

struct ViewerSearchInfo {
	__int64  CurPos;	// Line offset in BYTES from file start
	int      LeftPos;	// Column in CHARS
};

map<int, ViewerSearchInfo> g_ViewerInfo;

bool IsUnicode(const ViewerMode &Mode) {
#ifdef UNICODE
	return (Mode.CodePage == 1200) || (Mode.CodePage == 1201);
#else
	return Mode.Unicode != 0;
#endif
}

#ifndef UNICODE
string ToOEM(ViewerInfo &VInfo, const char *szData, int nLength)
{
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

tstring GetNextLine(ViewerInfo &VInfo, char *szData, int nLength, int &nSkip)
{
	if (IsUnicode(VInfo.CurMode)) {
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

void SetViewerSelection(__int64 nStart, int nLength, int nCharSize)
{
	ViewerSelect VSelect = {ITEM_SS(ViewerSelect) nStart / nCharSize, nLength / nCharSize};
	StartupInfo.ViewerControl(VCTL_SELECT, &VSelect);

	ViewerSetPosition VPos = {ITEM_SS(ViewerSetPosition) /*VSP_NOREDRAW*/0, (nStart > 256) ? nStart-256 : 0, 0};
	StartupInfo.ViewerControl(VCTL_SETPOSITION, &VPos);
	g_bInterrupted = TRUE;
}

BOOL ViewerSearchAgain()
{
	ViewerInfo VInfo;
	VInfo.StructSize=sizeof(VInfo);
	StartupInfo.ViewerControl(VCTL_GETINFO, &VInfo);
	g_bInterrupted = FALSE;

	CDebugTimer tm(_T("ViewSearch() took %d ms"));

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

	bool bUnicode  = IsUnicode(VInfo.CurMode);
	int  nCharSize = bUnicode ? 2 : 1;

#ifdef FAR3
	wchar_t szFileName[MAX_PATH];
	StartupInfo.ViewerControl(VCTL_GETFILENAME, szFileName);
	CFileMapping mapInput(szFileName);
#else
	CFileMapping mapInput(VInfo.FileName);
#endif

	if (!mapInput) return FALSE;
	char *szData = mapInput;
	if (!szData) return FALSE;

	long nOffset = (long)Info.CurPos;
	if (bUnicode) {
		if (nOffset == 0) {
			if (mapInput.Size() >= 2) {
				WORD wSig = *((WORD *)szData);
				if ((wSig == 0xFEFF) || (wSig == 0xFFFE)) nOffset += 2;
			}
		}
	}
	szData += nOffset;

	REParam.Clear();
	if (ERegExp) REParam.AddRE(EPattern);

	if (ESeveralLine) {
	} else {
		tstring strLine;
		int nCurrentLine = 0;
		int nSkip;

		int nLineOffset = Info.LeftPos;
		do {
			if (Interrupted()) break;
			strLine = GetNextLine(VInfo, szData, mapInput.Size()-nOffset, nSkip);
			if (strLine.empty() && (nSkip == 0)) break;
			if (!ECaseSensitive) strLine = UpCaseString(strLine);

			if (ERegExp) {
				if (pcre_exec(EPattern, EPatternExtra, strLine.data(), strLine.length(), nLineOffset, 0, REParam.Match(), REParam.Count())>=0) {
					SetViewerSelection(nOffset + REParam.m_arrMatch[0]*nCharSize, (REParam.m_arrMatch[1] - REParam.m_arrMatch[0])*nCharSize, nCharSize);
					Info.CurPos  = nOffset;
					Info.LeftPos = REParam.m_arrMatch[1];
					break;
				}
			} else {
				int nPos = BMHSearch(strLine.data()+nLineOffset, strLine.length()-nLineOffset, ETextUpcase.data(), ETextUpcase.length(), NULL);
				if (nPos >= 0) {
					SetViewerSelection(nOffset + (nLineOffset + nPos)*nCharSize, EText.length()*nCharSize, nCharSize);
					Info.CurPos  = nOffset;
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

	tm.Stop();

	if (!g_bInterrupted) {
		const TCHAR *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(FMSG_WARNING,_T("VCannotFind"),Lines,4,1);
	}
	return TRUE;
}

bool RefreshViewerInfo()
{
	VInfo.StructSize=sizeof(VInfo);
	if (StartupInfo.ViewerControl(VCTL_GETINFO, &VInfo) == 0) return false;

	return true;
}

BOOL ViewerSearch()
{
	RefreshViewerInfo();
	g_ViewerInfo.erase(VInfo.ViewerID);

	CFarDialog Dialog(76,13,_T("SearchDlg"));
	Dialog.SetWindowProc(EditorSearchDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

	Dialog.AddFrame(MRESearch);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"),SearchText));
	Dialog.Add(new CFarButtonItem(67,3,0,0,MQuoteSearch));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(30,5,DIF_DISABLE,MSeveralLine,&ESeveralLine));
	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,7,DIF_DISABLE,MReverseSearch,&EReverse));
	Dialog.AddButtons(MOk,MCancel,MBtnClose);
	Dialog.Add(new CFarButtonItem(60,9,0,0,MBtnPresets));

	SearchText = EText;
	ESeveralLine = FALSE;
	EReverse = FALSE;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display()) {
		case MOk:
		case MBtnClose:
			break;
		case MQuoteSearch:
			if (ERegExp) CSO::QuoteRegExpString(SearchText);
			break;
		case MBtnPresets:
			VSPresets->ShowMenu(true);
			break;
		default:
			return FALSE;
		}
	} while (!IsOKApply(ExitCode) || !EPreparePattern(SearchText));

	EText=SearchText;
	if ((ExitCode == MOk) && !EText.empty()) ViewerSearchAgain();

	return TRUE;
}

BOOL CVSPresetCollection::EditPreset(CPreset *pPreset)
{
	CFarDialog Dialog(76,16,_T("VSPresetDlg"));
	Dialog.AddFrame(MVSPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"),pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(30,7,0,MSeveralLine,&pPreset->m_mapInts["SeveralLine"]));
	Dialog.Add(new CFarCheckBoxItem(5,10,0,MAddToMenu,&pPreset->m_bAddToMenu));
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

OperationResult ViewSearchExecutor()
{
	return OR_CANCEL;
}
