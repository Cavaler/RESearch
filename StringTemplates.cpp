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
CStringOperations<CHAR>::CreateReplaceString(const CHAR *Replace, const CHAR *EOL, LPCTSTR szEngine, CREParameters<CHAR> &Param)
{
	g_bSkipReplace = false;

	if (szEngine != NULL)
		return EvaluateReplaceString(Param, Replace, EOL, szEngine);

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
			Replace++;
			CHAR c = ExpandHexDigits(Replace);
			Replace--;AddChar(String, c);
			break;
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
int GetHexDigit(const CHAR c)
{
	if ((c >= '0') && (c <= '9')) return c - '0';
	if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
	if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
	return -1;
}

template<class CHAR>
typename CHAR CStringOperations<CHAR>::ExpandHexDigits(const CHAR *&Replace)
{
	int nChar = 0;

	if (*Replace == '{') {
		Replace++;
		bool bOK = true;
		while ((*Replace != '}') && (*Replace != 0)) {
			if (bOK) {
				int nHex = GetHexDigit(*Replace);
				if (nHex >= 0)
					nChar = nChar*16 + nHex;
				else
					bOK = false;
			}
			Replace++;
		}
		if (*Replace != 0) Replace++;
	} else {
		for (int nDigit=0; nDigit < 2; nDigit++) {
			int nHex = GetHexDigit(*Replace);
			if (nHex >= 0)
				nChar = nChar*16 + nHex;
			else
				break;
			Replace++;
		}
	}

	return (CHAR)nChar;
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
bool CStringOperations<CHAR>::GetNumber(const cstring &str, int &nValue) {
	static CRegExpT<CHAR> reNumber(_T2("^[-+]?\\d+$"));
	if (reNumber.Match(str)) {
		nValue = ctoi(str.c_str());
		return true;
	} else return false;
}

template<class CHAR>
void CStringOperations<CHAR>::QuoteAny(cstring &str, const CHAR *szList) {
	for (size_t i=0; i < str.length(); i++) {
		if (cstrchr(_T2("()[]{}\\^$+*.?|"), str[i])) {
			str.insert(i++, 1, '\\');
			continue;
		}
	}
}

#ifdef UNICODE
template class CStringOperations<wchar_t>;
#else
template class CStringOperations<char>;
#endif

//////////////////////////////////////////////////////////////////////////

template<class CHAR>
CREParameters<CHAR>::CREParameters()
: m_re(NULL), m_szString(NULL)
{
	for (int n = 0; n < 25; n++)
		m_StrParamPtr[n] = m_mapStrParam.end();
}

template<class CHAR>
void CREParameters<CHAR>::Clear()
{
	if (m_re) BackupParam();
	m_arrMatch.clear();
	m_re = NULL;
	m_szString = NULL;
	m_nLength  = 0;
}

template<class CHAR>
bool CREParameters<CHAR>::Empty()
{
	return ((m_re == NULL) || (m_szString == NULL));
}

template<class CHAR>
void CREParameters<CHAR>::AddSingleCharParam(char C, const CHAR *sz)
{
	named_parameter_ptr &ptr = m_StrParamPtr[C - 'A'];
	if (ptr == m_mapStrParam.end()) {
		ptr = m_mapStrParam.insert(named_parameters::value_type(cstring(1, C), sz)).first;
	} else {
		ptr->second = sz;
	}
}

template<class CHAR>
void CREParameters<CHAR>::AddSingleCharParam(char C, int n)
{
	CHAR szNumber[32];
	AddSingleCharParam(C, CSO::ctoa(n, szNumber));
}

template<class CHAR>
void CREParameters<CHAR>::RebuildSingleCharParam()
{
	for (char c = 'A'; c <= 'Z'; c++)
	{
		m_StrParamPtr[c - 'A'] = m_mapStrParam.find(cstring(1, c));
	}
}

template<class CHAR>
void CREParameters<CHAR>::AddENumbers(int nL, int nN, int nS, int nR)
{
	AddSingleCharParam('L', nL);
	AddSingleCharParam('N', nN);
	AddSingleCharParam('S', nS);
	AddSingleCharParam('R', nR);
}

template<class CHAR>
void CREParameters<CHAR>::AddFNumbers(int nN, int nF, int nS, int nR)
{
	AddSingleCharParam('N', nN);
	AddSingleCharParam('F', nF);
	AddSingleCharParam('S', nS);
	AddSingleCharParam('R', nR);
}

template<class CHAR>
void CREParameters<CHAR>::AddSource (const CHAR *szString, size_t nLength) {
	m_szString = szString;
	m_nLength  = nLength;
}

template<class CHAR>
void CREParameters<CHAR>::CopySource(const CHAR *szString, size_t nLength) {
	m_strString = cstring(szString, nLength);
	m_szString  = m_strString.data();
	m_nLength   = m_strString.size();
}

template<class CHAR>
void CREParameters<CHAR>::CopySource(const cstring &strString) {
	m_strString = strString;
	m_szString  = m_strString.data();
	m_nLength   = m_strString.size();
}

template<class CHAR>
void CREParameters<CHAR>::AddRE(pcre *re)
{
	m_re = re;

	if (m_re == NULL) {
		m_arrMatch.clear();
		return;
	}

	int count;
	pcre_fullinfo(re, NULL, PCRE_INFO_CAPTURECOUNT, &count);
	m_arrMatch.resize((count+1)*3);

	vector<cstring> arrNames;
	pcre_get_stringlist(m_re, arrNames);

	for (size_t nParam = 0; nParam < arrNames.size(); nParam++) {
		named_parameters::iterator it = m_mapStrParam.find(arrNames[nParam]);
		if (it != m_mapStrParam.end()) m_mapStrParam.erase(it);
	}
}

template<class CHAR>
void CREParameters<CHAR>::AddPlainTextMatch(int nStart, int nLength)
{
	m_arrMatch.resize(3);
	m_arrMatch[0] = nStart;
	m_arrMatch[1] = nStart + nLength;
}

template<class CHAR>
int CREParameters<CHAR>::ParamCount()
{
	return (m_szString == NULL) ? 0 : Count()/3;
}

template<class CHAR>
bool CREParameters<CHAR>::HasParam(int nNumber)
{
	return (nNumber >= 0) && (nNumber < ParamCount());
}

template<class CHAR>
typename CREParameters<CHAR>::cstring CREParameters<CHAR>::GetParam(int nNumber)
{
	if (!HasParam(nNumber)) return cstring();

	return cstring(m_szString+m_arrMatch[nNumber*2], m_arrMatch[nNumber*2+1]-m_arrMatch[nNumber*2]);
}

template<class CHAR>
int CREParameters<CHAR>::FindParam(const cstring &strName, bool bCheckNumber)
{
	if (m_re == NULL) return -1;

	int nNumber;
	if (bCheckNumber && CSO::GetNumber(strName, nNumber))
		return nNumber;

	return pcre_get_stringnumber(m_re, strName.c_str());
}

template<class CHAR>
typename CREParameters<CHAR>::cstring
CREParameters<CHAR>::GetParam(const cstring &strName, bool bCheckNumber)
{
	int nNumber;
	if (bCheckNumber && CSO::GetNumber(strName, nNumber))
		return GetParam(nNumber);

	named_parameters::iterator it = m_mapStrParam.find(strName);
	if (it != m_mapStrParam.end()) return it->second;

	if (Empty()) return cstring();

	nNumber = pcre_get_stringnumber(m_re, strName.c_str());

	cstring strSubStr = GetParam(nNumber);
	m_mapStrParam[strName] = strSubStr;
	return strSubStr;
}

template<class CHAR>
void CREParameters<CHAR>::SetParam(const cstring &strName, const cstring &strValue)
{
	m_mapStrParam[strName] = strValue;
}

template<class CHAR>
void CREParameters<CHAR>::InitParam(const cstring &strName, const cstring &strValue)
{
	parameters_list::iterator it = m_setInitParam.find(strName);
	if (it == m_setInitParam.end()) {
		SetParam(strName, strValue);
		m_setInitParam.insert(strName);
	}
}

template<class CHAR>
void CREParameters<CHAR>::BackupParam()
{
	if (Empty()) return;

	vector<cstring> arrNames;
	pcre_get_stringlist(m_re, arrNames);

	for (size_t nParam = 0; nParam < arrNames.size(); nParam++) {
		int nNumber = pcre_get_stringnumber(m_re, arrNames[nParam].c_str());
		m_mapStrParam[arrNames[nParam]] = GetParam(nNumber);
	}
}

template<class CHAR> template<class CHAR2>
void CREParameters<CHAR>::CopyParam(CREParameters<CHAR2> &Param2)
{
	BackupParam();

	for (named_parameters::iterator it = m_mapStrParam.begin(); it != m_mapStrParam.end(); it++) {
		Param2.m_mapStrParam[CREParameters<CHAR2>::CSO::Convert(it->first, CP_OEMCP)] =
			CREParameters<CHAR2>::CSO::Convert(it->second, CP_OEMCP);
	}
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

template<class CHAR>
typename CREParameters<CHAR>::cstring
CREParameters<CHAR>::FillNamedReferences(const cstring &strPattern, bool bQuote)
{
	cstring strResult = strPattern;
	int nStart = 0;
	CRegExpT<CHAR> re(CSO::_T2("\\$\\{(.*?)\\}"));

	vector<cstring> arrRefs;
	while (re.Match(strResult.substr(nStart), 0, &arrRefs)) {
		cstring strReplace = GetParam(arrRefs[1], false);
		if (bQuote) CSO::QuoteRegExpString(strReplace);
		strResult.replace(nStart+re.RefStart(0), arrRefs[0].length(), strReplace);
		nStart += strReplace.length();
	}

	return strResult;
}

template<class CHAR>
bool CRegExpParam<CHAR>::Compile(cstring strPattern, int iCompileFlags)
{
	Clear();

	const CHAR *pszErrPtr;
	int iErrOffset;

	m_re = pcre_compile(strPattern.c_str(), iCompileFlags, &pszErrPtr, &iErrOffset, NULL);
	if (!m_re) return false;

	AddRE(m_re);
	return true;
}

template<class CHAR>
bool CRegExpParam<CHAR>::Match(cstring strAnalyze, int iExecFlags)
{
	CopySource(strAnalyze);

	return pcre_exec(m_re, NULL, strAnalyze.data(), strAnalyze.size(), 0, 0, __super::Match(), Count()) >= 0;
}

template class CREParameters<char>;
template void CREParameters<char>::CopyParam(CREParameters<char> &);
template class CRegExpParam<char>;
#ifdef UNICODE
template class CREParameters<wchar_t>;
template void CREParameters<char   >::CopyParam(CREParameters<wchar_t> &);
template void CREParameters<wchar_t>::CopyParam(CREParameters<char   > &);
template void CREParameters<wchar_t>::CopyParam(CREParameters<wchar_t> &);
template class CRegExpParam<wchar_t>;
#endif
