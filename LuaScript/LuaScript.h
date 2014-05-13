#pragma once

#include "LuaJIT\lua.hpp"
#include "LuaCOM\luacom.h"

#include <activscp.h>
_COM_SMARTPTR_TYPEDEF(IActiveScriptSite, __uuidof(IActiveScriptSite));

#ifdef _WIN64
#define DWCOOKIE DWORDLONG
#pragma comment(lib, "LuaScript\\LuaJIT\\lua51.x64.lib")
//#pragma comment(lib, "LuaScript\\LuaCOM\\luacom-lua5.1-1.4.x64.lib")
#else
#define DWCOOKIE DWORD
#pragma comment(lib, "LuaScript\\LuaJIT\\lua51.x86.lib")
#pragma comment(lib, "LuaScript\\LuaCOM\\luacom-lua5.1-1.4.x86.lib")
#endif
#pragma comment(lib, "htmlhelp.lib")

class CLuaScript
	: public CComObjectRootEx < CComSingleThreadModel >
	, public IActiveScript
	, public IActiveScriptParse
	, public IActiveScriptError
{
public:
	CLuaScript();
	~CLuaScript();

	BEGIN_COM_MAP(CLuaScript)
		COM_INTERFACE_ENTRY(IActiveScript)
		COM_INTERFACE_ENTRY(IActiveScriptParse)
		COM_INTERFACE_ENTRY(IActiveScriptError)
	END_COM_MAP()

protected:		//	IActiveScript
	STDMETHOD(SetScriptSite)(IActiveScriptSite *pSite);
	STDMETHOD(GetScriptSite)(REFIID riid, void **ppvObject);
	STDMETHOD(SetScriptState)(SCRIPTSTATE ss);
	STDMETHOD(GetScriptState)(SCRIPTSTATE *pssState);
	STDMETHOD(Close)();
	STDMETHOD(AddNamedItem)(LPCOLESTR pstrName, DWORD dwFlags);
	STDMETHOD(AddTypeLib)(REFGUID rguidTypeLib, DWORD dwMajor, DWORD dwMinor, DWORD dwFlags);
	STDMETHOD(GetScriptDispatch)(LPCOLESTR pstrItemName, IDispatch **ppDisp);
	STDMETHOD(GetCurrentScriptThreadID)(SCRIPTTHREADID *pstidThread);
	STDMETHOD(GetScriptThreadID)(DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread);
	STDMETHOD(GetScriptThreadState)(SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState);
	STDMETHOD(InterruptScriptThread)(SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags);
	STDMETHOD(Clone)(IActiveScript **ppScript);

protected:		//	IActiveScriptParse
	STDMETHOD(InitNew)();
	STDMETHOD(AddScriptlet)(LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName, LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter, DWCOOKIE dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, BSTR *pbstrName, EXCEPINFO *pexcepinfo);
	STDMETHOD(ParseScriptText)(LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext, LPCOLESTR pstrDelimiter, DWCOOKIE dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo);

protected:		//	IActiveScriptError
	STDMETHOD(GetExceptionInfo)(EXCEPINFO *pexcepinfo);
	STDMETHOD(GetSourcePosition)(DWORD *pdwSourceContext, ULONG *pulLineNumber, LONG *plCharacterPosition);
	STDMETHOD(GetSourceLineText)(BSTR *pbstrSourceLine);

protected:
	HRESULT Run();

	lua_State*				m_pLua;
	bool					m_bLuaLoaded;

	IActiveScriptSitePtr		m_spSite;
	std::map<CComBSTR, DWORD>	m_oGlobalNames;

	SCRIPTSTATE			m_eState;

	CComBSTR			m_sLastError;
};
