#include "StdAfx.h"
#define DEFINE_VARS
#include "RESearch.h"

CFarSettingsKey OpenSettings(const TCHAR *szSubKey, bool bCreate)
{
	CFarSettingsKey Key;
	Key.OpenRoot(_T("RESearch"));

	if (szSubKey == NULL) {
		return Key;
	} else {
		CFarSettingsKey SubKey;
		SubKey.Open(Key, szSubKey, bCreate);
		return SubKey;
	}
}

HKEY OpenRegistry(const TCHAR *szSubKey, bool bCreate)
{
	TCHAR szCurrentKey[512];
	_tcscat(_tcscpy(szCurrentKey, StartupInfo.RootKey), _T("\\RESearch"));
	if (szSubKey) _tcscat(_tcscat(szCurrentKey, _T("\\")), szSubKey);

	if (bCreate) {
		return RegCreateSubkey(HKEY_CURRENT_USER, szCurrentKey);
	} else {
		return RegOpenSubkey(HKEY_CURRENT_USER, szCurrentKey);
	}
}

void ReadRegistry()
{
	CFarSettingsKey hKey = OpenSettings();

#define DECLARE_PERSIST_LOAD hKey
#include "PersistVars.h"

#ifdef UNICODE
	vector<BYTE> arrCPs;
#ifdef FAR3
	QuerySettingsBinaryValue(hKey, _T("AllCP"), arrCPs);
#else
	QueryRegBinaryValue(hKey, _T("AllCP"), arrCPs);
#endif
	if (arrCPs.empty()) {
		g_setAllCPs.insert(GetOEMCP());
		g_setAllCPs.insert(GetACP());
		g_setAllCPs.insert(CP_UTF8);
		g_setAllCPs.insert(CP_UNICODE);
	} else {
		for (size_t nCP = 0; nCP < arrCPs.size()-3; nCP+=4) {
			DWORD *dwPtr = (DWORD *)(&arrCPs[0]+nCP);
			g_setAllCPs.insert(*dwPtr);
		}
	}
#endif

	EReadRegistry(hKey);
	VReadRegistry(hKey);
	FReadRegistry(hKey);
	FTReadRegistry(hKey);

	CHKey chKey;
	g_pEditorBatchType = new CBatchType(MEditorBatches, ESPresets, ERPresets, EFPresets, ETPresets, NULL);
	chKey = OpenRegistry(_T("EditorBatches"));
	g_pEditorBatches = new CBatchActionCollection(*g_pEditorBatchType, chKey);

	g_pPanelBatchType = new CBatchType(MPanelBatches, FSPresets, FRPresets, RnPresets, QRPresets, FGPresets, NULL);
	chKey = OpenRegistry(_T("PanelBatches"));
	g_pPanelBatches = new CBatchActionCollection(*g_pPanelBatchType, chKey);
}

void WriteRegistry()
{
	CFarSettingsKey hKey = OpenSettings();

#define DECLARE_PERSIST_SAVE hKey
#include "PersistVars.h"

#ifdef UNICODE
	vector<DWORD> arrCPs;
	for (cp_set::iterator it = g_setAllCPs.begin(); it != g_setAllCPs.end(); it++)
		arrCPs.push_back(*it);
#ifdef FAR3
	SetSettingsBinaryValue(hKey, _T("AllCP"), arrCPs.empty() ? NULL : &arrCPs[0], arrCPs.size()*sizeof(DWORD));
#else
	SetRegBinaryValue(hKey, _T("AllCP"), arrCPs.empty() ? NULL : &arrCPs[0], arrCPs.size()*sizeof(DWORD));
#endif
#endif

	EWriteRegistry(hKey);
	VWriteRegistry(hKey);
	FWriteRegistry(hKey);
	FTWriteRegistry(hKey);

	g_pEditorBatches->Save(hKey.Open(_T("EditorBatches")));
	g_pPanelBatches->Save (hKey.Open(_T("PanelBatches" )));
}

bool CheckUsage(const tstring &strText, bool bRegExp, bool bSeveralLine) {
	if (!g_bShowUsageWarnings) return true;

	if (!bRegExp) {
		if ((strText.find(_T("\\r")) != string::npos) || (strText.find(_T("\\n")) != string::npos)) {
			int nResult = Message(FMSG_WARNING, NULL, 5, 2,
				GetMsg(MWarning), GetMsg(MWarnMacrosInPlainText), GetMsg(MWarnContinue), GetMsg(MOk), GetMsg(MCancel));
			
			if (nResult == 1) return false;
		}
	}

	if (bSeveralLine && !g_bUseRealEOL) {
		if ((strText.find(_T("\\r\\n")) != string::npos) || (strText.find(_T("\r\n")) != string::npos)) {
			int nResult = Message(FMSG_WARNING, NULL, 5, 2,
				GetMsg(MWarning), GetMsg(MWarnRNInSeveralLine), GetMsg(MWarnContinue), GetMsg(MOk), GetMsg(MCancel));

			if (nResult == 1) return false;
		}
	}

	return true;
}

BOOL PreparePattern(pcre **Pattern,pcre_extra **PatternExtra,const tstring &Text,int CaseSensitive,const unsigned char *pTables) {
	if (Text.empty()) return FALSE;		// WAS: Not needed if empty NOW: what is search for nothing?
	const TCHAR *ErrPtr;
	int ErrOffset;
#ifdef UNICODE
	int iFlags=PCRE_MULTILINE|PCRE_UCP;
#else
	int iFlags=PCRE_MULTILINE;
#endif
	if (DotMatchesNewline) iFlags |= PCRE_DOTALL;
	if (!CaseSensitive) iFlags |= PCRE_CASELESS;

	*Pattern=pcre_compile(Text.c_str(),iFlags,&ErrPtr,&ErrOffset,pTables);
	if (!(*Pattern)) {
		tstring ErrPos((ErrOffset >= (int)Text.length()) ? ErrOffset+1 : Text.length(), ' ');
		ErrPos[ErrOffset]='^';

		const TCHAR *Lines[]={GetMsg(MRegExpError),ErrPtr,_T("\x01"),Text.c_str(),ErrPos.c_str(),GetMsg(MOk)};
		StartupInfo.Message(FMSG_WARNING,_T("RegExpError"),Lines,6,1);

		REErrorOffset = ErrOffset;
		return FALSE;
	} else {
		if (PatternExtra) {
			*PatternExtra=pcre_study(*Pattern,0,&ErrPtr);
		}
		return TRUE;
	}
}

#ifdef UNICODE
BOOL PreparePattern(pcre **Pattern,pcre_extra **PatternExtra,const string &Text,int CaseSensitive,const unsigned char *pTables) {
	if (Text.empty()) return FALSE;		// WAS: Not needed if empty NOW: what is search for nothing?
	const char *ErrPtr;
	int ErrOffset;
	int iFlags=PCRE_MULTILINE|PCRE_UCP;
	if (DotMatchesNewline) iFlags |= PCRE_DOTALL;
	if (!CaseSensitive) iFlags |= PCRE_CASELESS;

	*Pattern=pcre_compile(Text.c_str(),iFlags,&ErrPtr,&ErrOffset,pTables);
	if (!(*Pattern)) {
		tstring ErrPos(Text.length(),' ');
		tstring strErrPtr = OEMToUnicode(ErrPtr);
		tstring strText = OEMToUnicode(Text);
		const TCHAR *Lines[]={GetMsg(MRegExpError),strErrPtr.c_str(),_T("\x01"),strText.c_str(),ErrPos.c_str(),GetMsg(MOk)};
		ErrPos[ErrOffset]='^';
		StartupInfo.Message(FMSG_WARNING,_T("RegExpError"),Lines,6,1);
		return FALSE;
	} else {
		if (PatternExtra) {
			*PatternExtra=pcre_study(*Pattern,0,&ErrPtr);
		}
		return TRUE;
	}
}
#endif

void FillDefaultNamedParameters(const TCHAR *szFileName)
{
	CRegExpParam<TCHAR> reParseName;
	reParseName.Compile(CSO::_T2("(?<_fullname>^(?<_fpath>(?<_path>.*)\\\\)(?<_filename>.*)$)"));

	if (reParseName.Match(szFileName)) {
		reParseName.CopyParam(REParam);

		const CSO::cstring strName = reParseName.m_mapStrParam[CSO::_T2("_filename")];

		//	Easier to just manually check than invent complicated RE
		if (strName.find('.') != tstring::npos) {
			reParseName.Compile(CSO::_T2("^(?<_name>.*)\\.(?<_ext>.*)$"));
			reParseName.Match(strName);
			reParseName.CopyParam(REParam);

			reParseName.Compile(CSO::_T2("^(?<_sname>.*?)\\.(?<_fext>.*)$"));
			reParseName.Match(strName);
			reParseName.CopyParam(REParam);
		} else {
			REParam.SetParam(CSO::_T2("_name"), strName);
			REParam.SetParam(CSO::_T2("_sname"), strName);
		}
	}
}

void ClearVariables()
{
	REParam.m_mapStrParam.clear();
	REParamA.m_mapStrParam.clear();
}

void MatchDone()
{
	REParam.BackupParam();
#ifdef UNICODE
	REParam.CopyParam(REParamA);
#endif
}

void OEMMatchDone()
{
	REParamA.BackupParam();
#ifdef UNICODE
	REParamA.CopyParam(REParam);
#endif
}

void HighlightREError(CFarDialog *pDlg) {
	if (REErrorOffset < 0) return;

	int nIndex = pDlg->GetIndex(REErrorField);
	if (nIndex <= 0) nIndex = pDlg->GetIndex(MSearchFor);
	if (nIndex <= 0) nIndex = pDlg->GetIndex(MText);
	if (nIndex <= 0) return;

	COORD coord = {REErrorOffset, 0};
	StartupInfo.SendDlgMessage(pDlg->hDlg(), DM_SETCURSORPOS, nIndex+1, (LONG_PTR)&coord);
	StartupInfo.SendDlgMessage(pDlg->hDlg(), DM_SETFOCUS, nIndex+1, NULL);
}

#ifndef UNICODE
TCHAR ConvertCase_OEM(TCHAR C) {
	TCHAR Ansi,Oem;

	OemToCharBuff(&C,&Ansi,1);
	CharToOemBuff(&Ansi,&Oem,1);
	if (Oem!=C) return C;

	switch (OneCaseConvert) {
	case CCV_UPPER:
		Ansi=(TCHAR)CharUpper((TCHAR *)(UTCHAR)Ansi);break;
	case CCV_LOWER:
		Ansi=(TCHAR)CharLower((TCHAR *)(UTCHAR)Ansi);break;
	case CCV_FLIP:{
		char Lower=(TCHAR)CharLower((TCHAR *)(UTCHAR)Ansi);
		Ansi=(Lower==Ansi)?(TCHAR)CharUpper((TCHAR *)(UTCHAR)Ansi):Lower;
		break;
				  }
	}

	CharToOemBuff(&Ansi,&C,1);
	return C;
}

TCHAR ConvertCase(TCHAR C) {
	if (OneCaseConvert == CCV_NONE) return C;

	if (m_pReplaceTable) {
		char cUp = m_pReplaceTable->UpperTable[(UTCHAR)C];
		char cDn = m_pReplaceTable->LowerTable[(UTCHAR)C];
		switch (OneCaseConvert) {
		case CCV_UPPER:
			C = cUp;
			break;
		case CCV_LOWER:
			C = cDn;
			break;
		case CCV_FLIP:
			C = (C == cUp) ? cDn : cUp;
			break;
		}
	} else {
		C = ConvertCase_OEM(C);
	}

	OneCaseConvert=CaseConvert;
	return C;
}
#else
TCHAR ConvertCase(TCHAR C) {
	if (OneCaseConvert == CCV_NONE) return C;

	TCHAR cUp = (TCHAR)CharUpper((LPTSTR)C);
	TCHAR cDn = (TCHAR)CharLower((LPTSTR)C);

	switch (OneCaseConvert) {
	case CCV_UPPER:
		C = cUp;
		break;
	case CCV_LOWER:
		C = cDn;
		break;
	case CCV_FLIP:
		C = (C == cUp) ? cDn : cUp;
		break;
	}

	return C;
}
#endif

//////////////////////////////////////////////////////////////////////////

#ifdef UNICODE

string EvaluateReplaceString(CREParameters<CHAR> &Param, const CHAR *Replace, const CHAR *EOL, int Engine) {
	return string();
}

void BuildUpCaseTable(UINT nCP, char *pTable) {
	for (unsigned short nChar=0; nChar<256; nChar++) {
		WCHAR wChar, wCharU, wCharL;
		MultiByteToWideChar(nCP, 0, (char *)&nChar, 1, &wChar, 1);
		wCharU = (WCHAR)CharUpper((LPWSTR)wChar);
		wCharL = (WCHAR)CharLower((LPWSTR)wCharU);
		if ((wChar==wCharU) || (wChar==wCharL)) {
			char cChar;
			WideCharToMultiByte(nCP, 0, &wCharU, 1, &cChar, 1, NULL, NULL);
			pTable[nChar] = cChar;
		} else pTable[nChar] = (char)nChar;
	}
}

char *GetUpCaseTable(int nCP) {
	if (nCP == -1) nCP = (g_bDefaultOEM ? CP_OEMCP : CP_ACP);

	upcase_map::iterator it = UpCaseTables.find(nCP);

	if (it == UpCaseTables.end()) {
		UpCaseTables[nCP] = new char[256];
		it = UpCaseTables.find(nCP);
		BuildUpCaseTable(nCP, it->second);
	}

	return it->second;
}

void PrepareLocaleStuff() {
	for (int nChar=0;nChar<65536;nChar++) UpCaseTable[nChar]=nChar;
	UpCaseTable[65536]=0;
	CharUpper(UpCaseTable+1);

	UpCaseTables[GetOEMCP()] = GetUpCaseTable(CP_OEMCP);
	UpCaseTables[GetACP()]   = GetUpCaseTable(CP_ACP);

	setlocale(LC_ALL, FormatStrA(".%d", GetDefCP()).c_str());
	OEMCharTables = pcre_maketables();
	setlocale(LC_ALL, FormatStrA(".%d", GetACP()).c_str());
	ANSICharTables = pcre_maketables();
}

tstring UpCaseString(const tstring &strText) {
	tstring strUpCase = strText;

	CharUpper((LPTSTR)strUpCase.c_str());	// Bad but efficient

	return strUpCase;
}

#else

void PrepareLocaleStuff() {
//	Not .ACP / .OCP - they set locale based on "Standarts and Formats",
//	not "Language for non-Unicode programs"
	setlocale(LC_ALL, FormatStr(".%d", GetOEMCP()).c_str());
	OEMCharTables = pcre_maketables();
	setlocale(LC_ALL, FormatStr(".%d", GetACP()).c_str());
	ANSICharTables = pcre_maketables();

	for (unsigned short nChar=0; nChar<256; nChar++) {
		WCHAR wChar, wCharU, wCharL;
		MultiByteToWideChar(CP_OEMCP, 0, (char *)&nChar, 1, &wChar, 1);
		wCharU = (WCHAR)CharUpperW((LPWSTR)wChar);
		wCharL = (WCHAR)CharLowerW((LPWSTR)wCharU);
		if ((wChar==wCharU) || (wChar==wCharL)) {
			char cChar;
			WideCharToMultiByte(CP_OEMCP, 0, &wCharU, 1, &cChar, 1, NULL, NULL);
			UpCaseTable[nChar] = cChar;
		} else UpCaseTable[nChar] = (char)nChar;
	}
}

string UpCaseString(const string &strText) {
	string strUpCase = strText;

	for (size_t I=0; I<strUpCase.size(); I++)
		strUpCase[I] = UpCaseTable[(unsigned char)strUpCase[I]];

	return strUpCase;
}

#endif

#define BufCased(nChar) ((XLatTable)?(UTCHAR)XLatTable[Buf[nChar]]:Buf[nChar])
#define BufCasedA(nChar) ((XLatTable)?(BYTE)XLatTable[Buf[nChar]]:Buf[nChar])

#ifdef UNICODE
typedef int BMHTable[65536];
typedef int BMHTableA[256];
BMHTableA g_BMHTableA;
#else
typedef int BMHTable[256];
#endif

typedef struct {BMHTable m_Table;} BMHTableRec;
vector<BMHTableRec> g_BMHTables;

void PrepareBMHSearch(const TCHAR *String,int StringLength,size_t nPattern) {
	if (nPattern >= g_BMHTables.size()) g_BMHTables.resize(nPattern+1);

	BMHTable &Table = g_BMHTables[nPattern].m_Table;
#ifdef UNICODE
	for (int I=0;I<65536;I++) Table[I]=StringLength;
#else
	for (int I=0;I<256;I++) Table[I]=StringLength;
#endif

	if (EReverse)
		for (int I=StringLength-1;I>0;I--) Table[((UTCHAR *)String)[I]]=I;
	else
		for (int I=0;I<StringLength-1;I++) Table[((UTCHAR *)String)[I]]=StringLength-I-1;
}

int BMHSearch(const TCHAR *Buffer,int BufferLength,const TCHAR *String,int StringLength,TCHAR *XLatTable,int nPattern) {
	UTCHAR *Buf=(UTCHAR *)Buffer;
	UTCHAR *Str=(UTCHAR *)String;
	BMHTable &Table = g_BMHTables[nPattern].m_Table;
	int I;

	if (BufferLength<StringLength) return -1;

	int J=StringLength-1,K;
	while (J<BufferLength) {
		I=J;K=StringLength-1;
		while ((K>=0)&&(BufCased(I)==Str[K])) {I--;K--;}
		if (K<0) return I+1; else J+=Table[BufCased(J)];
	}
	return -1;
}

#ifdef UNICODE

void PrepareBMHSearchA(const char *String,int StringLength) {
	BMHTableA &Table = g_BMHTableA;//g_BMHTables[nPattern].m_Table;
	for (int I=0;I<256;I++) Table[I]=StringLength;

	if (EReverse)
		for (int I=StringLength-1;I>0;I--) Table[((BYTE *)String)[I]]=I;
	else
		for (int I=0;I<StringLength-1;I++) Table[((BYTE *)String)[I]]=StringLength-I-1;
}

int BMHSearchA(const char *Buffer,int BufferLength,const char *String,int StringLength,char *XLatTable) {
	BYTE *Buf=(BYTE *)Buffer;
	BYTE *Str=(BYTE *)String;
	BMHTableA &Table = g_BMHTableA;//g_BMHTables[nPattern].m_Table;
	int I;

	if (BufferLength<StringLength) return -1;

	int J=StringLength-1,K;
	while (J<BufferLength) {
		I=J;K=StringLength-1;
		while ((K>=0)&&(BufCasedA(I)==Str[K])) {I--;K--;}
		if (K<0) return I+1; else J+=Table[BufCasedA(J)];
	}
	return -1;
}
#endif

int ReverseBMHSearch(const TCHAR *Buffer,int BufferLength,const TCHAR *String,int StringLength,TCHAR *XLatTable,int nPattern) {
	UTCHAR *Buf=(UTCHAR*)Buffer;
	UTCHAR *Str=(UTCHAR *)String;
	BMHTable &Table = g_BMHTables[nPattern].m_Table;
	int I;

	if (BufferLength<StringLength) return -1;

	int J=StringLength-1,K;
	while (J<BufferLength) {
		I=J;K=StringLength-1;
		while ((K>=0)&&(BufCased(BufferLength-1-I)==Str[StringLength-1-K])) {I--;K--;}
		if (K<0) return BufferLength-StringLength-I-1; else J+=Table[BufCased(BufferLength-1-J)];
	}
	return -1;
}

tstring UniToHex(wstring &wstrUnicode) {
	TCHAR szBuffer[6];
	tstring strHex;
	for (size_t nIndex = 0; nIndex < wstrUnicode.length(); nIndex++) {
		_stprintf_s(szBuffer, 6, _T("%04X "), wstrUnicode[nIndex]);
		strHex += szBuffer;
	}
	return strHex;
}

wstring HexToUni(tstring strHex) {
	wstring wstrUnicode;
	size_t nStart = 0;
	while (nStart < strHex.length()) {
		while (isspace(strHex[nStart])) nStart++;
		int nEnd = nStart;
		while (isxdigit(strHex[nEnd]) && (nEnd - nStart < 4)) nEnd++;
		if (nStart == nEnd) break;
		while (isspace(strHex[nStart])) nStart++;
		int chSymbol;
		_stscanf(strHex.data() + nStart, _T("%4X"), &chSymbol);
		wstrUnicode += (wchar_t)chSymbol;
		nStart = nEnd;
	}

	return wstrUnicode;
}

void EditorStartUndo() {
#ifdef UNICODE
	EditorUndoRedo UR;
	UR.Command = EUR_BEGIN;

	StartupInfo.EditorControl(ECTL_UNDOREDO, &UR);
#endif
}

void EditorEndUndo() {
#ifdef UNICODE
	EditorUndoRedo UR;
	UR.Command = EUR_END;

	StartupInfo.EditorControl(ECTL_UNDOREDO, &UR);
#endif
}

void ShowErrorMsg(const TCHAR *sz1, const TCHAR *sz2, const TCHAR *szHelp) {
	const TCHAR *Lines[]={GetMsg(MREReplace),sz1,sz2,GetMsg(MOk)};
	StartupInfo.Message(FMSG_WARNING,szHelp,Lines,4,1);
}

void ShowHResultError(int nError, HRESULT hResult, const TCHAR *szHelp) {
	TCHAR *szMessage;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, hResult, 0, (LPTSTR)&szMessage, 0, NULL);

	TCHAR szFullMsg[1024];

	if (szMessage) {
		TCHAR *szEnd = _tcschr(szMessage, '\r');
		if (szEnd) *szEnd = 0;
		_stprintf_s(szFullMsg, 1024, _T("%s (0x%08X)"), szMessage, hResult);
	} else {
		_stprintf_s(szFullMsg, 1024, _T("0x%08X"), hResult);
	}

	ShowErrorMsg(GetMsg(nError), szFullMsg, szHelp);
	LocalFree(szMessage);
}

HANDLE g_hREThread = NULL;
HANDLE g_hREReady = CreateSemaphore(NULL, 0, 1, NULL);
#ifdef UNICODE
HANDLE g_hREReadyA = CreateSemaphore(NULL, 0, 1, NULL);
#endif
HANDLE g_hREDone = CreateSemaphore(NULL, 0, 1, NULL);

const pcre *g_external_re;
const pcre_extra *g_extra_data;
const TCHAR *g_subject;
#ifdef UNICODE
const char *g_subjectA;
#endif
int g_length;
int g_start_offset;
int g_options;
int *g_offsets;
int g_offsetcount;
int g_result;

#ifdef UNICODE
DWORD WINAPI REThreadProc(LPVOID lpParameter) {
	HANDLE hRE[] = {g_hREReady, g_hREReadyA};

	while (true) {
		DWORD dwResult = WaitForMultipleObjects(2, hRE, FALSE, 60000);
		if (dwResult == WAIT_TIMEOUT) {
			CloseHandle(g_hREThread);
			g_hREThread = NULL;
			return 0;
		}
		if (dwResult == WAIT_OBJECT_0) {
			g_result = pcre_exec(g_external_re, g_extra_data, g_subject, g_length,
				g_start_offset, g_options, g_offsets, g_offsetcount);
		} else {
			g_result = pcre_exec(g_external_re, g_extra_data, g_subjectA, g_length,
				g_start_offset, g_options, g_offsets, g_offsetcount);
		}
		ReleaseSemaphore(g_hREDone, 1, NULL);
	}
}
#else
DWORD WINAPI REThreadProc(LPVOID lpParameter) {
	while (true) {
		if (WaitForSingleObject(g_hREReady, 60000) == WAIT_TIMEOUT) {
			CloseHandle(g_hREThread);
			g_hREThread = NULL;
			return 0;
		}
		g_result = pcre_exec(g_external_re, g_extra_data, g_subject, g_length,
			g_start_offset, g_options, g_offsets, g_offsetcount);
		ReleaseSemaphore(g_hREDone, 1, NULL);
	}
}
#endif

void StartREThread() {
	DWORD dwThreadID;
	g_hREThread = CreateThread(NULL, g_nThreadStackMB*1024*1024, REThreadProc, NULL, /*CREATE_SUSPENDED*/0, &dwThreadID);
}

void StopREThread() {
	TerminateThread(g_hREThread, 0);
	CloseHandle(g_hREThread);
	g_hREThread = NULL;
	CloseHandle(g_hREReady);
	CloseHandle(g_hREDone);
}

int do_pcre_exec(const pcre *external_re, const pcre_extra *extra_data,
	const TCHAR *subject, int length, int start_offset, int options, int *offsets,
	int offsetcount)
{
	if (g_bUseSeparateThread && (length > g_nMaxInThreadLength)) {
		if (!g_hREThread)
			StartREThread();
		if (g_hREThread) {
			g_external_re = external_re;
			g_extra_data = extra_data;
			g_subject = subject;
			g_length = length;
			g_start_offset = start_offset;
			g_options = options;
			g_offsets = offsets;
			g_offsetcount = offsetcount;
			ReleaseSemaphore(g_hREReady, 1, NULL);
			WaitForSingleObject(g_hREDone, INFINITE);
			return g_result;
		}
	}
	return pcre_exec(external_re, extra_data, subject, length, start_offset, options, offsets, offsetcount);
}

#ifdef UNICODE
int do_pcre_execA(const pcre *external_re, const pcre_extra *extra_data,
	const char *subject, int length, int start_offset, int options, int *offsets,
	int offsetcount)
{
	if (g_bUseSeparateThread && (length > g_nMaxInThreadLength)) {
		if (!g_hREThread)
			StartREThread();
		if (g_hREThread) {
			g_external_re = external_re;
			g_extra_data = extra_data;
			g_subjectA = subject;
			g_length = length;
			g_start_offset = start_offset;
			g_options = options;
			g_offsets = offsets;
			g_offsetcount = offsetcount;
			ReleaseSemaphore(g_hREReadyA, 1, NULL);
			WaitForSingleObject(g_hREDone, INFINITE);
			return g_result;
		}
	}
	return pcre_exec(external_re, extra_data, subject, length, start_offset, options, offsets, offsetcount);
}
#endif

BOOL SystemToLocalTime(FILETIME &ft) {
	TIME_ZONE_INFORMATION tzi;
	DWORD dwRes = GetTimeZoneInformation(&tzi);
	if (dwRes != TIME_ZONE_ID_INVALID) {
		ULARGE_INTEGER ul;
		ul.HighPart = ft.dwHighDateTime;
		ul.LowPart = ft.dwLowDateTime;
		LONG Bias = tzi.Bias + ((dwRes == TIME_ZONE_ID_DAYLIGHT) ? tzi.DaylightBias : tzi.StandardBias);
		ul.QuadPart -= 600000000i64 * Bias;
		ft.dwHighDateTime = ul.HighPart;
		ft.dwLowDateTime = ul.LowPart;
	}
	return TRUE;
}

BOOL LocalToSystemTime(FILETIME &ft) {
	TIME_ZONE_INFORMATION tzi;
	DWORD dwRes = GetTimeZoneInformation(&tzi);
	if (dwRes != TIME_ZONE_ID_INVALID) {
		ULARGE_INTEGER ul;
		ul.HighPart = ft.dwHighDateTime;
		ul.LowPart = ft.dwLowDateTime;
		LONG Bias = tzi.Bias + ((dwRes == TIME_ZONE_ID_DAYLIGHT) ? tzi.DaylightBias : tzi.StandardBias);
		ul.QuadPart += 600000000i64 * Bias;
		ft.dwHighDateTime = ul.HighPart;
		ft.dwLowDateTime = ul.LowPart;
	}
	return TRUE;
}

void RunExternalEditor(tstring &strText) {
	if ((strText.length() > 3) && (strText[1] == ':') && (strText[2] == '\\')) {
#ifdef UNICODE
		StartupInfo.Editor(strText.c_str(), NULL, 0, 0, -1, -1, 0, 0, 1, CP_AUTODETECT);
#else
		StartupInfo.Editor(strText.c_str(), NULL, 0, 0, -1, -1, 0, 0, 1);
#endif
	} else {
		TCHAR szBuffer[MAX_PATH], szName[MAX_PATH];
		GetTempPath(MAX_PATH, szBuffer);
		GetTempFileName(szBuffer, _T("re"), 0, szName);

		CFileMapping mapFile;
#ifdef UNICODE
		mapFile.Open(szName, true, strText.length()*2+2);
		memmove((BYTE *)mapFile,   "\xFF\xFE", 2);
		memmove(((BYTE *)mapFile)+2, strText.data(), strText.length()*2);
#else
		mapFile.Open(szName, true, strText.length());
		memmove((BYTE *)mapFile, strText.data(), strText.length());
#endif
		mapFile.Close();

#ifdef UNICODE
		StartupInfo.Editor(szName, NULL, 0, 0, -1, -1, EF_DISABLEHISTORY, 0, 1, CP_AUTODETECT);
#else
		StartupInfo.Editor(szName, NULL, 0, 0, -1, -1, EF_DISABLEHISTORY, 0, 1);
#endif

		mapFile.Open(szName);
#ifdef UNICODE
		strText = tstring((LPCWSTR)mapFile+1, mapFile.Size()/2-1);
#else
		strText = tstring(mapFile, mapFile.Size());
#endif
		mapFile.Close();
		DeleteFile(szName);
	}
}

void RefreshConsoleInfo() {
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ConInfo);
}

void QuoteString(const TCHAR *Source, int Length, vector<tstring> &arrQuoted, int MaxWidth)
{
	tstring str;

	if (Length>MaxWidth) {
		size_t nLeft = (MaxWidth-5)/2;
		str = tstring(Source, nLeft) + _T("...") + tstring(Source + Length-nLeft, nLeft);
	} else {
		str = tstring(Source, Length);
	}
	arrQuoted.push_back(str);
}

size_t QuoteStrings(const TCHAR *Source, vector<tstring> &arrQuoted, int MaxWidth, int nMaxHeight)
{
	do {
		const TCHAR *Pos = _tcschr(Source,'\n');
		if (Pos) {
			QuoteString(Source, Pos-Source, arrQuoted, MaxWidth);
			Source = Pos + 1;
		} else break;
	} while (TRUE);

	QuoteString(Source, _tcslen(Source), arrQuoted, MaxWidth);

	if ((nMaxHeight > 0) && ((int)arrQuoted.size() > nMaxHeight)) {
		size_t nOffs = nMaxHeight/2;
		arrQuoted.erase (arrQuoted.begin()+nOffs, arrQuoted.begin()+nOffs+(arrQuoted.size()-nMaxHeight)+1);
		arrQuoted.insert(arrQuoted.begin()+nOffs, _T("..."));
	}

	size_t nMaxLength = 0;
	for each (const tstring &str in arrQuoted)
		nMaxLength = max(str.size(), nMaxLength);

	return nMaxLength;
}

size_t MakeSameWidth(vector<tstring> &arrQuoted)
{
	size_t nMaxLength = 0;
	for each (const tstring &str in arrQuoted)
		nMaxLength = max(str.size(), nMaxLength);

	for (size_t nLine = 0; nLine < arrQuoted.size(); nLine++) {
		arrQuoted[nLine] += tstring(nMaxLength-arrQuoted[nLine].size(), ' ');
	}

	return nMaxLength;
}

hack_string::hack_string(const char *szData, size_t nSize) {
	_Bx._Ptr = (char *)szData;
	_Myres = string::_BUF_SIZE*2;
	_Mysize = nSize;
}
hack_string::~hack_string() {
	_Bx._Ptr = NULL;
	_Myres = string::_BUF_SIZE-1;
	_Mysize = 0;
}

hack_wstring::hack_wstring(const wchar_t *szData, size_t nSize) {
	_Bx._Ptr = (wchar_t *)szData;
	_Myres = wstring::_BUF_SIZE*2;
	_Mysize = nSize;
}
hack_wstring::~hack_wstring() {
	_Bx._Ptr = NULL;
	_Myres = wstring::_BUF_SIZE-1;
	_Mysize = 0;
}


#ifdef UNICODE

UINT GetDefCP() {
	return (g_bDefaultOEM) ? GetOEMCP() : GetACP();
}

UINT IsDefCP(UINT nCP) {
	return (g_bDefaultOEM) ?
		((nCP == CP_OEMCP) || (nCP == GetOEMCP())) :
		((nCP == CP_ACP)   || (nCP == GetACP()));
}

wstring DefToUnicode(const string &strDef) {
	return (g_bDefaultOEM) ? OEMToUnicode(strDef) : ANSIToUnicode(strDef);
}

string DefFromUnicode(const wstring &strUnicode) {
	return (g_bDefaultOEM) ? OEMFromUnicode(strUnicode) : ANSIFromUnicode(strUnicode);
}

bool CanUseCP(UINT nCP, const wstring &strUnicode) {
	return StrToUnicode(StrFromUnicode(strUnicode, nCP), nCP) == strUnicode;
}

const unsigned char *DefCharTables() {
	return (g_bDefaultOEM) ? OEMCharTables : ANSICharTables;
}

#else

UINT GetDefCP() {
	return CP_OEMCP;
}

UINT IsDefCP(UINT nCP) {
	return (nCP == CP_OEMCP) || (nCP == GetOEMCP());
}

#endif
