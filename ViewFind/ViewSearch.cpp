#include "ViewFind.h"
#include "..\EditFind\EditFind.h"

struct ViewerSearchInfo {
	__int64  CurPos;	// Line
	int      LeftPos;	// Column;
};

map<int, ViewerSearchInfo> g_ViewerInfo;

string ToOEM(ViewerInfo &VInfo, const char *szData, int nLength) {
	if (VInfo.CurMode.AnsiMode) {
		vector<char> arrData(szData, szData+nLength);
		CharToOemBuff(&arrData[0], &arrData[0], nLength);
		return string(&arrData[0], arrData.size());
	}
	if (!VInfo.CurMode.UseDecodeTable || (VInfo.CurMode.TableNum >= XLatTableCount)) return string(szData, nLength);

	string strResult;
	strResult.reserve(nLength);
	while (nLength--) {
		strResult += XLatTables[VInfo.CurMode.TableNum][(BYTE)(*szData)];
		szData++;
	}
	return strResult;
}

string GetNextLine(ViewerInfo &VInfo, const char *szData, int nLength, int *pEOLLen = NULL) {
	if (VInfo.CurMode.Unicode) {
		const wchar_t *wszData = (const wchar_t *)szData;
		const wchar_t *wszCur = wszData;
		while (nLength && !wcschr(L"\r\n", *wszCur)) {wszCur++; nLength++;}

		vector<char> arrData(wszCur - wszData);
		WideCharToMultiByte(CP_OEMCP, 0, wszData, wszCur-wszData, &arrData[0], wszCur-wszData, " ", NULL);
		if (pEOLLen) *pEOLLen = (nLength == 0) ? 0 :
			((nLength > 1) && (wszCur[0] == '\r') && (wszCur[1] == '\n')) ? 2 : 1;
		return string(&arrData[0], arrData.size());
	} else {
		const char *szCur = szData;
		while (nLength && !strchr("\r\n", *szCur)) {szCur++; nLength++;}
		if (pEOLLen) *pEOLLen = (nLength == 0) ? 0 :
			((nLength > 1) && (szCur[0] == '\r') && (szCur[1] == '\n')) ? 2 : 1;
		return ToOEM(VInfo, szData, szCur-szData);
	}
}

void SetViewerSelection(__int64 nStart, int nLength) {
	ViewerSelect VSelect = {nStart, nLength};
	StartupInfo.ViewerControl(VCTL_SELECT, &VSelect);

	ViewerSetPosition VPos = {/*VSP_NOREDRAW*/0, (nStart > 256) ? nStart-256 : 0, 0};
	StartupInfo.ViewerControl(VCTL_SETPOSITION, &VPos);
	Interrupt = TRUE;
}

BOOL ViewerSearchAgain() {
	ViewerInfo VInfo;
	VInfo.StructSize=sizeof(VInfo);
	StartupInfo.ViewerControl(VCTL_GETINFO, &VInfo);
	Interrupt = FALSE;

	map<int, ViewerSearchInfo>::iterator it = g_ViewerInfo.find(VInfo.ViewerID);
	if (it == g_ViewerInfo.end()) {
		ViewerSearchInfo Info;
		Info.CurPos = VInfo.FilePos.i64;
		Info.LeftPos = VInfo.LeftPos;
		g_ViewerInfo[VInfo.ViewerID] = Info;
	}
	ViewerSearchInfo &Info = g_ViewerInfo[VInfo.ViewerID];

	CFileMapping mapInput;
	const char *szData = (const char *)mapInput.Open(VInfo.FileName);
	if (!szData) return FALSE;
	szData += VInfo.CurMode.Unicode ? Info.CurPos*2 : Info.CurPos;

	long nOffset = Info.CurPos;
	int nMatchCount = ERegExp ? pcre_info(EPattern, NULL, NULL) + 1 : 0;
	int *pMatch = ERegExp ? new int[nMatchCount*3] : NULL;

	if (ESeveralLine) {
	} else {
		string strLine;
		int nEOLLen;

		int nLineOffset = Info.LeftPos;
		do {
			if (Interrupted()) break;
			strLine = GetNextLine(VInfo, szData, VInfo.FileSize.i64-nOffset, &nEOLLen);
			if (strLine.empty() && !nEOLLen) break;
			if (!ECaseSensitive)
				for (int n=0; n<strLine.size(); n++) strLine[n] = UpCaseTable[(BYTE)strLine[n]];

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
			szData += strLine.length()+nEOLLen;
			nOffset += strLine.length()+nEOLLen;
			nLineOffset = 0;
		} while (true);
	}

	if (ERegExp) delete[] pMatch;
	if (!Interrupt) {
		const char *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"ECannotFind",Lines,4,1);
	}
	return TRUE;
}

BOOL ViewerSearch() {
	ViewerInfo VInfo;
	VInfo.StructSize=sizeof(VInfo);
	StartupInfo.ViewerControl(VCTL_GETINFO, &VInfo);
	g_ViewerInfo.erase(VInfo.ViewerID);

	CFarDialog Dialog(76,13,"SearchDlg");
	Dialog.AddFrame(MRESearch);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY,"SearchText",SearchText));
	Dialog.Add(new CFarButtonItem(67,3,0,0,"&\\"));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,""));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(30,5,0,MSeveralLine,&ESeveralLine));
	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(30,6,0,"",&EUTF8));
	Dialog.Add(new CFarButtonItem(34,6,0,0,MUTF8));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MReverseSearch,&EReverse));
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
//			VSPresets->ShowMenu(g_ESBatch);
			break;
		case 3:
			UTF8Converter(SearchText);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=1)||!EPreparePattern(SearchText));

	EText=SearchText;
	if (!EText.empty()) ViewerSearchAgain();
	return TRUE;
}
