#ifndef __STRINGTEMPLATES_H
#define __STRINGTEMPLATES_H

template<class CHAR>
class CREParameters;

template<class CHAR>
class CStringOperations {
public:
	typedef basic_string<CHAR> cstring;
	static int ctoi(const CHAR *sz);
	static CHAR *ctoa(int n, CHAR *sz);
	static int cstrlen(const CHAR *sz);
	static int csprintf_s(CHAR *sz, size_t count, const CHAR *szFormat, ...);

	static CHAR *__T2(char *sz, wchar_t *wsz);

	static void AddChar(cstring &String, CHAR c);
	static void AddString(cstring &String, const CHAR *Add, int Len);
	static int GetDelta(CHAR *&String);

	static bool GetNumber(const cstring &str, int &nValue);
	static BOOL ExpandParameter(const CHAR *Matched, cstring &String, const cstring &Param, int *Match, int Count, int *Numbers);
	static cstring CreateReplaceString(const CHAR *Matched,int *Match,int Count,const CHAR *Replace,const CHAR *EOL,int *Numbers,int Engine, BOOL bRegExp);

	static cstring ExpandParameter(const CHAR *&Replace, CREParameters<CHAR> &Param);
	static cstring CreateReplaceString(const CHAR *Replace, const CHAR *EOL, int Engine, CREParameters<CHAR> &Param);
};

#define _T2(sz) __T2(sz, L ## sz)

template<class CHAR>
class CREParameters {
public:
	typedef CStringOperations<CHAR> CSO;
	typedef typename CSO::cstring cstring;

	CREParameters();

	void Clear();
	void AddENumbers(int nL, int nN, int nR);
	void AddFNumbers(int nF, int nR);
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
