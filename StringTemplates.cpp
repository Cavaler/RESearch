#include "StdAfx.h"
#include "RESearch.h"

template<class CHAR>
void CStringOperations<CHAR>::AddChar(cstring &String, CHAR c) {
	String += (CHAR)::ConvertCase(c);
}
 
template<class CHAR>
void CStringOperations<CHAR>::AddString(cstring &String, const CHAR *Add, int Len) {
	for (int I=0;I<Len;I++) String += (CHAR)::ConvertCase(Add[I]);
}

template<class CHAR>
typename CStringOperations<CHAR>::cstring
CStringOperations<CHAR>::CreateReplaceString(const CHAR *Replace, const CHAR *EOL, int Engine, CREParameters<CHAR> &Param)
{
	if ((Engine >= 0) && (Engine < (int)m_arrEngines.size()))
		return EvaluateReplaceString(Param, Replace, EOL, Engine);

	if (!Param.m_re && !g_bEscapesInPlainText)
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

		default:
			AddChar(String, *Replace);
			}
			break;
		case '$':{
			cstring strParam = ExpandParameter(Replace, Param);
			AddString(String, strParam.c_str(), strParam.length());

			//	Not a break - to skip Replace++
			continue;
				 }
		default:
			AddChar(String, *Replace);
		}
		if (*Replace) Replace++; else break;
	}
	return String;
}

template<class CHAR>
typename CStringOperations<CHAR>::cstring
CStringOperations<CHAR>::ExpandParameter(const CHAR *&Replace, CREParameters<CHAR> &Param)
{
	vector<cstring> arrMatch;

	//	Named/formatted parameters: {name}
	if (CRegExpT<CHAR>::Match(Replace+1, _T2("^\\{([^}]+)\\}"), 0, 0, &arrMatch)) {
		cstring strParam = arrMatch[1];
		Replace = Replace + strParam.length()+3;

		//	Parameters with offset: {name+32}
		if (CRegExpT<CHAR>::Match(strParam, _T2("^(.*?)((\\+|-)\\d+)"), 0, 0, &arrMatch)) {
			strParam = arrMatch[1];
			int nAdd = ctoi(arrMatch[2].c_str());
			size_t nAlign = arrMatch[2].length()-1;

			cstring strValue = Param.GetParam(strParam, true);
			int nValue;
			if (GetNumber(strValue, nValue)) {
				CHAR szNumber[32];
				strValue = ctoa(nValue + nAdd, szNumber);
				cstring strAppend = (nAlign <= strValue.length()) ? cstring() : cstring(nAlign - strValue.length(), '0');
				return strAppend + strValue;
			} else {
				return strValue + arrMatch[2];
			}

		//	Parameters with alignment: {name>32}
		} else if (CRegExpT<CHAR>::Match(strParam, _T2("^(.*?)((<|>)(\\d+))"), 0, 0, &arrMatch)) {
			strParam = arrMatch[1];
			CHAR cAlign = arrMatch[3][0];
			size_t nAlign = ctoi(arrMatch[4].c_str());

			cstring strValue = Param.GetParam(strParam, true);
			cstring strAppend = (nAlign <= strValue.length()) ? cstring() : cstring(nAlign - strValue.length(), ' ');
			return (cAlign == '<') ? strValue + strAppend : strAppend + strValue;
		} else {
			return Param.GetParam(strParam, true);
		}
		//	Simple parameters: $32 or $R
	} else if (CRegExpT<CHAR>::Match(Replace+1, _T2("^\\d+|[A-Z]"), 0, 0, &arrMatch)) {
		cstring strParam = arrMatch[0];
		Replace = Replace + strParam.length()+1;

		return Param.GetParam(strParam, true);
	}

	Replace++;
	return cstring();
}

template<class CHAR>
bool CStringOperations<CHAR>   ::GetNumber(const cstring &str, int &nValue) {
	static CRegExpT<CHAR> reNumber(_T2("^[-+]?\\d+$"));
	if (reNumber.Match(str)) {
		nValue = ctoi(str.c_str());
		return true;
	} else return false;
}

template class CStringOperations<char>;
#ifdef UNICODE
template class CStringOperations<wchar_t>;
#endif

//////////////////////////////////////////////////////////////////////////

template<class CHAR>
CREParameters<CHAR>::CREParameters()
: m_re(NULL), m_szString(NULL)
{
}

template<class CHAR>
void CREParameters<CHAR>::Clear() {
	m_arrMatch.clear();
	m_mapStrParam.clear();
	m_re = NULL;
	m_szString = NULL;
}

template<class CHAR>
void CREParameters<CHAR>::AddENumbers(int nL, int nN, int nS, int nR) {
	CHAR szNumber[16];
	m_mapStrParam[CSO::_T2("L")] = CSO::ctoa(nL, szNumber);
	m_mapStrParam[CSO::_T2("N")] = CSO::ctoa(nN, szNumber);
	m_mapStrParam[CSO::_T2("S")] = CSO::ctoa(nS, szNumber);
	m_mapStrParam[CSO::_T2("R")] = CSO::ctoa(nR, szNumber);
}

template<class CHAR>
void CREParameters<CHAR>::AddFNumbers(int nF, int nS, int nR) {
	CHAR szNumber[16];
	m_mapStrParam[CSO::_T2("F")] = CSO::ctoa(nF, szNumber);
	m_mapStrParam[CSO::_T2("S")] = CSO::ctoa(nS, szNumber);
	m_mapStrParam[CSO::_T2("R")] = CSO::ctoa(nR, szNumber);
}

template<class CHAR>
void CREParameters<CHAR>::AddSource(const CHAR *szString, size_t nLength) {
	m_szString = szString;
	m_nLength = nLength;
}

template<class CHAR>
void CREParameters<CHAR>::AddRE(pcre *re) {
	m_re = re;
	m_arrMatch.resize((pcre_info(re, NULL, NULL)+1)*3);
}

template<class CHAR>
typename CREParameters<CHAR>::cstring CREParameters<CHAR>::GetParam(int nNumber) {
	if ((nNumber < 0) || (nNumber >= Count()/3)) return cstring();

	return cstring(m_szString+m_arrMatch[nNumber*2], m_arrMatch[nNumber*2+1]-m_arrMatch[nNumber*2]);
}

template<class CHAR>
int CREParameters<CHAR>::FindParam(const cstring &strName, bool bCheckNumber) {
	int nNumber;
	if (bCheckNumber && CSO::GetNumber(strName, nNumber))
		return nNumber;

	return pcre_get_stringnumber(m_re, strName.c_str());
}

template<class CHAR>
typename CREParameters<CHAR>::cstring
CREParameters<CHAR>::GetParam(const cstring &strName, bool bCheckNumber) {
	int nNumber;
	if (bCheckNumber && CSO::GetNumber(strName, nNumber))
		return GetParam(nNumber);

	map<cstring, cstring>::iterator it = m_mapStrParam.find(strName);
	if (it != m_mapStrParam.end()) return it->second;

	if (m_re == NULL) return cstring();

	nNumber = pcre_get_stringnumber(m_re, strName.c_str());

	cstring strSubStr = GetParam(nNumber);
	m_mapStrParam[strName] = strSubStr;
	return strSubStr;
}

template<class CHAR>
void CREParameters<CHAR>::FillStartLength(int *MatchStart, int *MatchLength)
{
	if (MatchStart) *MatchStart = m_arrMatch[0];
	if (MatchLength) *MatchLength = m_arrMatch[1]-m_arrMatch[0];
}

template<class CHAR>
typename CREParameters<CHAR>::cstring
CREParameters<CHAR>::Original()
{
	return cstring(m_szString, m_arrMatch.empty() ? m_nLength : m_arrMatch[1]);
}

template class CREParameters<char>;
#ifdef UNICODE
template class CREParameters<wchar_t>;
#endif
