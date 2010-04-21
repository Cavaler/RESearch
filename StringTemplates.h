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

	static cstring ExpandParameter(const CHAR *&Replace, CREParameters<CHAR> &Param);
	static cstring CreateReplaceString(const CHAR *Replace, const CHAR *EOL, int Engine, CREParameters<CHAR> &Param);
};

typedef CStringOperations<TCHAR>    CSO;
typedef CStringOperations<char>     CSOA;
typedef CStringOperations<wchar_t>  CSOW;

template<class CHAR>
class CREParameters {
public:
	typedef CStringOperations<CHAR> CSO;
	typedef typename CSO::cstring cstring;

	CREParameters();

	void Clear();
	void AddENumbers(int nL, int nN, int nS, int nR);
	void AddFNumbers(int nF, int nS, int nR);
	void AddSource(const CHAR *szString);
	void AddRE(pcre *re);

	vector<int> m_arrMatch;
	int *Match() { return m_arrMatch.empty() ? NULL : &m_arrMatch[0]; }
	int  Count() { return m_arrMatch.size(); }

	cstring GetParam(int nNumber);
	cstring GetParam(const cstring &strName, bool bCheckNumber = false);

	void FillStartLength(int *MatchStart, int *MatchLength);
	cstring Original();

public:
	map<cstring, cstring> m_mapStrParam;

	pcre *m_re;
	const CHAR *m_szString;
};

#endif