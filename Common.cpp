#include "StdAfx.h"
#define DEFINE_VARS
#include "RESearch.h"

HKEY OpenRegistry(const TCHAR *szSubKey, bool bCreate) {
	TCHAR szCurrentKey[512];
	_tcscat(_tcscpy(szCurrentKey, StartupInfo.RootKey), _T("\\RESearch"));
	if (szSubKey) _tcscat(_tcscat(szCurrentKey, _T("\\")), szSubKey);

	if (bCreate) {
		return RegCreateSubkey(HKEY_CURRENT_USER, szCurrentKey);
	} else {
		return RegOpenSubkey(HKEY_CURRENT_USER, szCurrentKey);
	}
}

void ReadRegistry() {
	CHKey hKey = OpenRegistry();
#define DECLARE_PERSIST_LOAD hKey
#include "PersistVars.h"

	EReadRegistry(hKey);
	VReadRegistry(hKey);
	FReadRegistry(hKey);
	FTReadRegistry(hKey);

	g_pEditorBatchType = new CBatchType(MEditorBatches, ESPresets, ERPresets, EFPresets, ETPresets, NULL);
	hKey = OpenRegistry(_T("EditorBatches"));
	g_pEditorBatches = new CBatchActionCollection(*g_pEditorBatchType, hKey);

	g_pPanelBatchType = new CBatchType(MPanelBatches, FSPresets, FRPresets, RnPresets, QRPresets, FGPresets, NULL);
	hKey = OpenRegistry(_T("PanelBatches"));
	g_pPanelBatches = new CBatchActionCollection(*g_pPanelBatchType, hKey);
}

void WriteRegistry() {
	CHKey hKey = OpenRegistry();

#define DECLARE_PERSIST_SAVE hKey
#include "PersistVars.h"

	EWriteRegistry(hKey);
	VWriteRegistry(hKey);
	FWriteRegistry(hKey);
	FTWriteRegistry(hKey);

	hKey = OpenRegistry(_T("EditorBatches"));
	g_pEditorBatches->Save(hKey);

	hKey = OpenRegistry(_T("PanelBatches"));
	g_pPanelBatches->Save(hKey);
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

	if (bSeveralLine) {
		if ((strText.find(_T("\\r\\n")) != string::npos) || (strText.find(_T("\r\n")) != string::npos)) {
			int nResult = Message(FMSG_WARNING, NULL, 5, 2,
				GetMsg(MWarning), GetMsg(MWarnRNInSeveralLine), GetMsg(MWarnContinue), GetMsg(MOk), GetMsg(MCancel));

			if (nResult == 1) return false;
		}
	}

	return true;
}

BOOL PreparePattern(pcre **Pattern,pcre_extra **PatternExtra,const tstring &Text,int CaseSensitive,BOOL bUTF8,const unsigned char *pTables) {
	if (Text.empty()) return FALSE;		// WAS: Not needed if empty NOW: what is search for nothing?
	const TCHAR *ErrPtr;
	int ErrOffset;
	int iFlags=PCRE_MULTILINE;
	if (DotMatchesNewline) iFlags |= PCRE_DOTALL;
	if (!CaseSensitive) iFlags |= PCRE_CASELESS;
	if (bUTF8) iFlags |= PCRE_UTF8;

	*Pattern=pcre_compile(Text.c_str(),iFlags,&ErrPtr,&ErrOffset,pTables);
	if (!(*Pattern)) {
		tstring ErrPos(Text.length(),' ');
		const TCHAR *Lines[]={GetMsg(MRegExpError),ErrPtr,_T("\x01"),Text.c_str(),ErrPos.c_str(),GetMsg(MOk)};
		ErrPos[ErrOffset]='^';
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("RegExpError"),Lines,6,1);
		return FALSE;
	} else {
		if (PatternExtra) {
			*PatternExtra=pcre_study(*Pattern,0,&ErrPtr);
		}
		return TRUE;
	}
}

enum ECaseConvert {CCV_NONE,CCV_UPPER,CCV_LOWER,CCV_FLIP};
ECaseConvert CaseConvert;
ECaseConvert OneCaseConvert;

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

#pragma message("ConvertCase unimplemented for UNICODE")

	return C;
}
#endif

void AddChar(tstring &String, TCHAR C) {
	String += ConvertCase(C);
}

void AddString(tstring &String, const TCHAR *Add, int Len) {
	for (int I=0;I<Len;I++) String += ConvertCase(Add[I]);
}

int GetDelta(TCHAR *&String) {
	int Number=0;
	BOOL Minus=FALSE;
	String++;
	if (*String=='-') {Minus=TRUE;String++;} else
	if (*String=='+') {String++;}
	while (*String) {
		if (isdigit((unsigned char)*String)) Number=Number*10+(*String)-'0'; else
			if (*String=='$') {String++;break;} else break;
		String++;
	}
	String--;
	return (Minus)?(-Number):Number;
}

bool GetNumber(const tstring &str, int &nValue) {
	static CRegExp reNumber(_T("^[-+]?\\d+$"));
	if (reNumber.Match(str)) {
		nValue = _ttoi(str.c_str());
		return true;
	} else return false;
}

BOOL ExpandParameter(const TCHAR *Matched,tstring &String,tstring Param,int *Match,int Count,int *Numbers) {
	int Number=0;
	if (Param.size()==0) return TRUE;
	if (isdigit((UTCHAR)Param[0])) {
		if (Count) {
			Number = Param[0] - '0';
			size_t nPos = 1;
			while ((nPos < Param.length()) && isdigit((UTCHAR)Param[nPos])) {
				Number = Number*10 + Param[nPos] - '0';
				nPos++;
			}
			if ((Number >= Count) || (Match[Number*2] == -1)) return FALSE;

			tstring strMatch = tstring(Matched+Match[Number*2], Match[Number*2+1]-Match[Number*2]);
			if ((nPos < Param.length()) && ((Param[nPos] == '+') || (Param[nPos] == '-'))) {
				int nMatch, nAdd;
				if (GetNumber(strMatch, nMatch) && GetNumber(Param.substr(nPos), nAdd)) {
					int MinLen = 1;
					if (Param.length() >= nPos+1) MinLen = Param.length() - nPos - 1;

					TCHAR sz[16];
					_stprintf_s(sz, 16, _T("%0*d"), MinLen, nMatch + nAdd);
					strMatch = sz;
				} else {
					strMatch += Param.substr(nPos);
				}
			}
			AddString(String, strMatch.data(), strMatch.length());
		} else AddChar(String, '$');
		return TRUE;
	}

	if (!Numbers) return FALSE;
	int MinLen = 1;
	if ((Param.size() >= 2) && ((Param[1]=='+') || (Param[1]=='-'))){
		MinLen = Param.length()-2;
		_stscanf(Param.c_str()+1, _T("%d"), &Number);
	}
	switch (toupper(Param[0])) {
	case 'L':Number+=Numbers[0];break;
	case 'N':Number+=Numbers[1];break;
	case 'R':Number+=Numbers[2];break;
	default:return FALSE;
	}
	TCHAR S[16];
	_stprintf_s(S, 16, _T("%0*d"), MinLen, Number);
	AddString(String, S, _tcslen(S));
	return TRUE;
}

tstring CreateReplaceString(const TCHAR *Matched,int *Match,int Count,const TCHAR *Replace,const TCHAR *EOL,int *Numbers,int Engine) {
	if ((Engine >= 0) && (Engine < (int)m_arrEngines.size()))
		return EvaluateReplaceString(Matched, Match, Count, Replace, EOL, Numbers, Engine);

	tstring String;
	OneCaseConvert=CaseConvert=CCV_NONE;
	while (*Replace) {
		if (OneCaseConvert==CCV_NONE) OneCaseConvert=CaseConvert;
		switch (*Replace) {
		case '\\':
			Replace++;
			switch (*Replace) {
			case 0  :AddChar(String,'\\');Replace--;break;
			case 'a':AddChar(String,'\a');break;
			case 'b':AddChar(String,'\b');break;
			case 'e':AddChar(String,'\x1B');break;
			case 'f':AddChar(String,'\f');break;
			case 'n':AddString(String,EOL,_tcslen(EOL));break;
			case 'r':AddChar(String,'\r');break;
			case 't':AddChar(String,'\t');break;
			case 'v':AddChar(String,'\v');break;

			case '0':case '1':case '2':case '3':case '4':
			case '5':case '6':case '7':case '8':case '9':{
				int Char=0;
				for (int I=0;I<3;I++) {
					if (isdigit((unsigned char)*Replace)) {
						Char=Char*8+(*Replace-'0');Replace++;
					} else break;
				}
				Replace--;AddChar(String, Char);
					 }
			case 'x':{
				int Char=0;Replace++;
				for (int I=0;I<2;I++) {
					if (isxdigit((unsigned char)*Replace)) {
						Char=Char*16+*Replace;
						if (*Replace<='9') Char-='0'; else
						if (*Replace<='F') Char+=10-'A'; else Char+=10-'a';
						Replace++;
					} else break;
				}
				Replace--;AddChar(String, Char);break;
					 }

			case 'l':OneCaseConvert=CCV_LOWER;break;
			case 'u':OneCaseConvert=CCV_UPPER;break;
			case 'c':OneCaseConvert=CCV_FLIP;break;
			case 'L':CaseConvert=OneCaseConvert=CCV_LOWER;break;
			case 'U':CaseConvert=OneCaseConvert=CCV_UPPER;break;
			case 'C':CaseConvert=OneCaseConvert=CCV_FLIP;break;
			case 'E':CaseConvert=OneCaseConvert=CCV_NONE;break;

			default:AddChar(String, *Replace);
			}
			break;
		case '$':{
			int I=2;
			if (Replace[1]=='{') {
				while (Replace[I]&&(Replace[I]!='}')) I++;
				TCHAR Save=Replace[I];
				ExpandParameter(Matched,String,tstring(Replace+2,I-2),Match,Count,Numbers);
				Replace+=I;
				break;
			}
			while (isdigit((UTCHAR)Replace[I])) I++;
			TCHAR Save=Replace[I];
			ExpandParameter(Matched,String,tstring(Replace+1,I-1),Match,Count,Numbers);
			Replace+=I-1;
			break;
				 }
		default:AddChar(String, *Replace);
		}
		if (*Replace) Replace++; else break;
	}
	return String;
}

#ifdef UNICODE

void PrepareLocaleStuff() {
	for (int I=0;I<65536;I++) UpCaseTable[I]=I;
	CharUpper(UpCaseTable);
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

	for (unsigned short I=0;I<256;I++) {
		WCHAR W,U,L;
		unsigned char C;
		MultiByteToWideChar(CP_OEMCP,0,(char *)&I,1,&W,1);
		U=towupper(W);
		L=towlower(U);
		if ((W==U)||(W==L)) {
			WideCharToMultiByte(CP_OEMCP,0,&U,1,(char *)&C,1,NULL,NULL);
			UpCaseTable[I]=C;
		} else UpCaseTable[I]=(char)I;
	}
}

string UpCaseString(const string &strText) {
	string strUpCase = strText;

	for (size_t I=0; I<strUpCase.size(); I++)
		strUpCase[I] = UpCaseTable[(unsigned char)strUpCase[I]];

	return strUpCase;
}

#endif

#define BufCased(I) ((XLatTable)?(unsigned char)XLatTable[Buf[I]]:Buf[I])

#ifdef UNICODE
typedef int BMHTable[65536];
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

#ifndef UNICODE
tstring UTF8ToHex(string &strUTF8) {
	return UniToHex(DecodeUTF8(strUTF8));
}

string HexToUTF8(tstring &strHex) {
	return EncodeUTF8(HexToUni(strHex));
}
#endif

void UTF8Converter(tstring strInit) {
	tstring strANSI = strInit;
	tstring strUTF8 = strInit;
	tstring strHex;

	CFarDialog Dialog(70,13,_T("UTF8Converter"));
	Dialog.AddFrame(MUTF8Converter);
	Dialog.Add(new CFarTextItem(5,3,0,MConverterANSI));
	Dialog.Add(new CFarEditItem(11,3,57,DIF_HISTORY,_T("RESearch.ANSI"),strANSI));

	Dialog.Add(new CFarTextItem(5,5,0,MConverterUTF8));
	Dialog.Add(new CFarEditItem(11,5,57,DIF_HISTORY,_T("RESearch.UTF8"),strUTF8));

	Dialog.Add(new CFarTextItem(5,7,0,MConverterHex));
	Dialog.Add(new CFarEditItem(11,7,57,DIF_HISTORY,_T("RESearch.Hex"),strHex));

	Dialog.Add(new CFarButtonItem(60,3,0,0,MConvert));
	Dialog.Add(new CFarButtonItem(60,5,0,0,MConvert));
	Dialog.Add(new CFarButtonItem(60,7,0,0,MConvert));
	Dialog.Add(new CFarButtonItem(60,9,DIF_CENTERGROUP,1,MOk));

	int iResult;
	do {
		switch (iResult = Dialog.Display(4, -4, -3, -2, -1)) {
		case 0:
#ifdef UNICODE
			strUTF8 = OEMToUnicode(EncodeUTF8(strANSI));
			strHex = UniToHex(strANSI);
#else
			strUTF8 = EncodeUTF8(strANSI, CP_OEMCP);
			strHex = UTF8ToHex(strUTF8);
#endif
			break;
		case 1:
#ifdef UNICODE
			strANSI = DecodeUTF8(OEMFromUnicode(strUTF8));
			strHex = UniToHex(strANSI);
#else
			strANSI = DecodeUTF8A(strUTF8, CP_OEMCP);
			strHex = UTF8ToHex(strUTF8);
#endif
			break;
		case 2:
#ifdef UNICODE
			strANSI = HexToUni(strHex);
			strUTF8 = OEMToUnicode(EncodeUTF8(strANSI));
#else
			strUTF8 = HexToUTF8(strHex);
			strANSI = DecodeUTF8A(strUTF8, CP_OEMCP);
#endif
			break;
		}
	} while ((iResult == 0) || (iResult == 1) || (iResult == 2));
}

void QuoteRegExpString(tstring &strText) {
	for (size_t I=0; I<strText.length();I++) {
		if (_tcschr(_T("()[]{}\\^$+*.?|"), strText[I])) {
			strText.insert(I++, 1, '\\');
			continue;
		}
	}
}

void QuoteReplaceString(tstring &strText) {
	for (size_t I=0; I<strText.length();I++) {
		if ((strText[I] == '\\') || (strText[I] == '$')) {
			strText.insert(I++, 1, '\\');
			continue;
		}
	}
}

void ShowErrorMsg(const TCHAR *sz1, const TCHAR *sz2, const TCHAR *szHelp) {
	const TCHAR *Lines[]={GetMsg(MREReplace),sz1,sz2,GetMsg(MOk)};
	StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,szHelp,Lines,4,1);
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
HANDLE g_hREDone = CreateSemaphore(NULL, 0, 1, NULL);

const pcre *g_external_re;
const pcre_extra *g_extra_data;
const TCHAR *g_subject;
int g_length;
int g_start_offset;
int g_options;
int *g_offsets;
int g_offsetcount;
int g_result;

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
		mapFile.Open(szName, true, strText.length());
		memmove((BYTE *)mapFile, strText.data(), strText.length());
		mapFile.Close();

#ifdef UNICODE
		StartupInfo.Editor(szName, NULL, 0, 0, -1, -1, EF_DISABLEHISTORY, 0, 1, CP_AUTODETECT);
#else
		StartupInfo.Editor(szName, NULL, 0, 0, -1, -1, EF_DISABLEHISTORY, 0, 1);
#endif

		mapFile.Open(szName);
		strText = tstring(mapFile, mapFile.Size());
		mapFile.Close();
		DeleteFile(szName);
	}
}
