#ifndef __STRINGTEMPLATES_H
#define __STRINGTEMPLATES_H

template<class CHAR>
class CREParameters;

template<class CHAR>
class CStringOperations : public CStringT<CHAR> {
public:
	static void AddChar(cstring &String, CHAR c);
	static void AddString(cstring &String, const CHAR *Add, int Len);

	static bool GetNumber(const cstring &str, int &nValue);

	static void QuoteAny(cstring &str, const CHAR *szList);

	static cstring ExpandParameter(const CHAR *&Replace, CREParameters<CHAR> &Param);
	static CHAR    ExpandHexDigits(const CHAR *&Replace);
	static cstring CreateReplaceString(const CHAR *Replace, const CHAR *EOL, int Engine, CREParameters<CHAR> &Param);

	static void QuoteRegExpString (cstring &strText) {
		QuoteAny(strText, _T2("()[]{}\\^$+*.?|"));
	}

	static void QuoteReplaceString(cstring &strText) {
		QuoteAny(strText, _T2("\\$"));
	}

};

typedef CStringOperations<TCHAR>    CSO;
typedef CStringOperations<char>     CSOA;
typedef CStringOperations<wchar_t>  CSOW;

template<class CHAR>
class CREParameters {
public:
	typedef CStringOperations<CHAR> CSO;
	typedef typename CSO::cstring cstring;
	typedef map<cstring, cstring> named_parameters;
	typedef typename named_parameters::iterator named_parameter_ptr;
	typedef set<cstring> parameters_list;

	CREParameters();

	void Clear();
	bool Empty();

	void AddENumbers(int nL, int nN, int nS, int nR);
	void AddFNumbers(int nF, int nS, int nR);
	void AddSource (const CHAR *szString, size_t nLength);
	void CopySource(const CHAR *szString, size_t nLength);
	void CopySource(const cstring &strString);
	void AddRE(pcre *re);

	vector<int> m_arrMatch;
	int *Match() { return m_arrMatch.empty() ? NULL : &m_arrMatch[0]; }
	int  Count() { return (int)m_arrMatch.size(); }

	cstring GetParam (int nNumber);
	cstring GetParam (const cstring &strName, bool bCheckNumber = false);
	int     FindParam(const cstring &strName, bool bCheckNumber = false);
	void    SetParam (const cstring &strName, const cstring &strValue);
	void    InitParam(const cstring &strName, const cstring &strValue);

	void    BackupParam();
	template<class CHAR2> void CopyParam(CREParameters<CHAR2> &Param2);

	void FillStartLength(int *MatchStart, int *MatchLength);
	cstring Original();

	cstring FillNamedReferences(const cstring &strPattern, bool bQuote=true);

	void AddSingleCharParam(char C, const CHAR *sz);

public:
	named_parameters m_mapStrParam;
	named_parameter_ptr	m_StrParamPtr[26];
	parameters_list  m_setInitParam;	// for script::init()

	pcre *m_re;

	cstring m_strString;
	const CHAR *m_szString;
	size_t m_nLength;
};

template<class CHAR>
class CRegExpParam : public CREParameters<CHAR> {
public:
	bool Compile(cstring strPattern, int iCompileFlags = 0);
	bool Match(cstring strAnalyze, int iExecFlags = 0);

protected:
};

#endif
