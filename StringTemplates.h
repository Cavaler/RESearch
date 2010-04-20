#ifndef __STRINGTEMPLATES_H
#define __STRINGTEMPLATES_H

template<class CHAR>
class CStringOperations {
public:
	typedef basic_string<CHAR> cstring;
	static int ctoi(const CHAR *sz);
	static CHAR *ctoa(int n, CHAR *sz);
	static int cstrlen(const CHAR *sz);
	static int csprintf_s(CHAR *sz, size_t count, const CHAR *szFormat, ...);
	static CHAR *_T2(char *sz, wchar_t *wsz);

	static void AddChar(cstring &String, CHAR c);
	static void AddString(cstring &String, const CHAR *Add, int Len);
	static int GetDelta(CHAR *&String);

	static bool GetNumber(const cstring &str, int &nValue);
	static BOOL ExpandParameter(const CHAR *Matched, cstring &String, const cstring &Param, int *Match, int Count, int *Numbers);
	static cstring CreateReplaceString(const CHAR *Matched,int *Match,int Count,const CHAR *Replace,const CHAR *EOL,int *Numbers,int Engine, BOOL bRegExp);
};

#endif
