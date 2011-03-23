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
	typedef map<cstring, cstring> named_parameters;

	CREParameters();

	void Clear();
	void Clear(const named_parameters &mapParam);

	void AddENumbers(int nL, int nN, int nS, int nR);
	void AddFNumbers(int nF, int nS, int nR);
	void AddSource (const CHAR *szString, size_t nLength);
	void CopySource(const CHAR *szString, size_t nLength);
	void CopySource(const cstring &strString);
	void AddRE(pcre *re);

	vector<int> m_arrMatch;
	int *Match() { return m_arrMatch.empty() ? NULL : &m_arrMatch[0]; }
	int  Count() { return (int)m_arrMatch.size(); }

	cstring GetParam(int nNumber);
	int     FindParam(const cstring &strName, bool bCheckNumber = false);
	cstring GetParam(const cstring &strName, bool bCheckNumber = false);

	void    BackupParam(named_parameters &mapParam);

	void FillStartLength(int *MatchStart, int *MatchLength);
	cstring Original();

public:
	named_parameters m_mapStrParam;

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
