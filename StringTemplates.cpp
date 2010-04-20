#include "StdAfx.h"
#include "RESearch.h"

template<class CHAR>
void CStringOperations<CHAR>::AddChar(cstring &String, CHAR c) {
	String += (CHAR)ConvertCase(c);
}
 
template<class CHAR>
void CStringOperations<CHAR>::AddString(cstring &String, const CHAR *Add, int Len) {
	for (int I=0;I<Len;I++) String += (CHAR)ConvertCase(Add[I]);
}

template<class CHAR>
int CStringOperations<CHAR>::GetDelta(CHAR *&String) {
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

template<class CHAR>
BOOL CStringOperations<CHAR>::ExpandParameter(const CHAR *Matched, cstring &String, const cstring &Param, int *Match, int Count, int *Numbers) {
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

			cstring strMatch = cstring(Matched+Match[Number*2], Match[Number*2+1]-Match[Number*2]);
			if ((nPos < Param.length()) && ((Param[nPos] == '+') || (Param[nPos] == '-'))) {
				int nMatch, nAdd;
				if (GetNumber(strMatch, nMatch) && GetNumber(Param.substr(nPos), nAdd)) {
					int MinLen = 1;
					if (Param.length() >= nPos+1) MinLen = Param.length() - nPos - 1;

					CHAR sz[16];
					csprintf_s(sz, 16, _T2("%0*d", L"%0*d"), MinLen, nMatch + nAdd);
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
		Number = ctoi(Param.c_str()+1);
	}
	switch (toupper(Param[0])) {
	case 'L':Number+=Numbers[0];break;
	case 'N':Number+=Numbers[1];break;
	case 'R':Number+=Numbers[2];break;
	default:return FALSE;
	}
	CHAR S[16];
	csprintf_s(S, 16, _T2("%0*d", L"%0*d"), MinLen, Number);
	AddString(String, S, cstrlen(S));
	return TRUE;
}

template<class CHAR>
typename CStringOperations<CHAR>::cstring CStringOperations<CHAR>::CreateReplaceString(const CHAR *Matched,int *Match,int Count,const CHAR *Replace,const CHAR *EOL,int *Numbers,int Engine, BOOL bRegExp) {
	if ((Engine >= 0) && (Engine < (int)m_arrEngines.size()))
		return EvaluateReplaceString(Matched, Match, Count, Replace, EOL, Numbers, Engine);

	if (!bRegExp && !g_bEscapesInPlainText)
		return Replace;

	cstring String;
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
		case 'n':AddString(String,EOL,cstrlen(EOL));break;
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
			Replace--;AddChar(String, (CHAR)Char);
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
			Replace--;AddChar(String, (CHAR)Char);break;
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
				ExpandParameter(Matched,String,cstring(Replace+2,I-2),Match,Count,Numbers);
				Replace+=I;
				break;
			}
			while (isdigit((UTCHAR)Replace[I])) I++;
			TCHAR Save=Replace[I];
			ExpandParameter(Matched,String,cstring(Replace+1,I-1),Match,Count,Numbers);
			Replace+=I-1;
			break;
				 }
		default:AddChar(String, *Replace);
		}
		if (*Replace) Replace++; else break;
	}
	return String;
}

template<> static bool CStringOperations<char>   ::GetNumber(const cstring &str, int &nValue) {
	static CRegExp reNumber(_T("^[-+]?\\d+$"));
#ifdef UNICODE
	if (reNumber.Match(OEMToUnicode(str))) {
#else
	if (reNumber.Match(str)) {
#endif
		nValue = ctoi(str.c_str());
		return true;
	} else return false;
}

template<> static bool CStringOperations<wchar_t>::GetNumber(const cstring &str, int &nValue) {
	static CRegExp reNumber(_T("^[-+]?\\d+$"));
#ifdef UNICODE
	if (reNumber.Match(str)) {
#else
	if (reNumber.Match(OEMFromUnicode(str))) {
#endif
		nValue = ctoi(str.c_str());
		return true;
	} else return false;
}

template<> static int CStringOperations<char>   ::ctoi(const char    *sz) { return  atoi(sz); }
template<> static int CStringOperations<wchar_t>::ctoi(const wchar_t *sz) { return _wtoi(sz); }

template<> static char *    CStringOperations<char>   ::ctoa(int n, char    *sz) { return _itoa(n, sz, 10); }
template<> static wchar_t * CStringOperations<wchar_t>::ctoa(int n, wchar_t *sz) { return _itow(n, sz, 10); }

template<> static int CStringOperations<char>   ::cstrlen(const char   *sz)  { return strlen(sz); }
template<> static int CStringOperations<wchar_t>::cstrlen(const wchar_t *sz) { return wcslen(sz); }

template<> static int CStringOperations<char>   ::csprintf_s(char    *sz, size_t count, const char    *szFormat, ...) {
	va_list vaList;
	va_start(vaList, szFormat);
	return vsprintf_s(sz, count, szFormat, vaList);
}
template<> static int CStringOperations<wchar_t>::csprintf_s(wchar_t *sz, size_t count, const wchar_t *szFormat, ...) {
	va_list vaList;
	va_start(vaList, szFormat);
	return vswprintf_s(sz, count, szFormat, vaList);
}

template<> static char    * CStringOperations<char>   ::_T2(char *sz, wchar_t *wsz) { return  sz; }
template<> static wchar_t * CStringOperations<wchar_t>::_T2(char *sz, wchar_t *wsz) { return wsz; }

template class CStringOperations<char>;
#ifdef UNICODE
template class CStringOperations<wchar_t>;
#endif
