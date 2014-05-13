#include "StdAfx.h"

#include "LuaScript.h"

#define OLESCRIPT_E_SYNTAX _HRESULT_TYPEDEF_(0x80020101L)

const CLSID CLSID_LuaScript = {0x4B014691,0x33A9,0x429d,{0xA7,0x4D,0x4A,0x19,0xD9,0xAF,0xE8,0x4F}};	// 4B014691-33A9-429d-A74D-4A19D9AFE84F

CLuaScript::CLuaScript()
: m_eState(SCRIPTSTATE_UNINITIALIZED)
, m_bLuaLoaded(false)
{
	m_pLua = luaL_newstate();
	luaL_openlibs(m_pLua);
	luacom_open(m_pLua);
}

CLuaScript::~CLuaScript()
{
	luacom_close(m_pLua);
	lua_close(m_pLua);
}

//////////////////////////////////////////////////////////////////////////
//	IActiveScript

STDMETHODIMP CLuaScript::SetScriptSite(IActiveScriptSite *pSite)
{
	m_spSite = pSite;

	return S_OK;
}
    
STDMETHODIMP CLuaScript::GetScriptSite(REFIID riid, void **ppvObject)
{
	return E_NOTIMPL;
}
    
STDMETHODIMP CLuaScript::SetScriptState(SCRIPTSTATE ss)
{
	HRESULT hr = S_OK;

	switch (ss) {
	case SCRIPTSTATE_UNINITIALIZED:
		m_spSite = NULL;
		break;

	case SCRIPTSTATE_STARTED:
	case SCRIPTSTATE_CONNECTED:
		for (std::map<CComBSTR,DWORD>::iterator i = m_oGlobalNames.begin(); i != m_oGlobalNames.end(); ++i)
		{
			lua_pushnil(m_pLua);
			lua_setglobal(m_pLua, ATL::CW2A(i->first));
		}
		hr = m_bLuaLoaded ? Run() : S_OK;
		break;
	}

	m_eState = ss;

	return hr;
}

STDMETHODIMP CLuaScript::GetScriptState(SCRIPTSTATE *pssState)
{
	*pssState = m_eState;

	return S_OK;
}

STDMETHODIMP CLuaScript::Close()
{
	return E_NOTIMPL;
}

STDMETHODIMP CLuaScript::AddNamedItem(LPCOLESTR pstrName, DWORD dwFlags)
{
	m_oGlobalNames.insert(std::map<CComBSTR,DWORD>::value_type(pstrName, dwFlags));

	return S_OK;
}

STDMETHODIMP CLuaScript::AddTypeLib(REFGUID rguidTypeLib, DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP CLuaScript::GetScriptDispatch(LPCOLESTR pstrItemName, IDispatch **ppDisp)
{
	*ppDisp = NULL;//(*ppdisp = static_cast<IDispatch*>(this))->AddRef();

	return S_OK;
}

STDMETHODIMP CLuaScript::GetCurrentScriptThreadID(SCRIPTTHREADID *pstidThread)
{
	return E_NOTIMPL;
}

STDMETHODIMP CLuaScript::GetScriptThreadID(DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
	return E_NOTIMPL;
}

STDMETHODIMP CLuaScript::GetScriptThreadState(SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
	return E_NOTIMPL;
}

STDMETHODIMP CLuaScript::InterruptScriptThread(SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP CLuaScript::Clone(IActiveScript **ppScript)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//	IActiveScriptParse

STDMETHODIMP CLuaScript::InitNew()
{
	return S_OK;
}

STDMETHODIMP CLuaScript::AddScriptlet(LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName, LPCOLESTR pstrSubItemName,
									  LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter, DWCOOKIE dwSourceContextCookie, ULONG ulStartingLineNumber,
									  DWORD dwFlags, BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP CLuaScript::ParseScriptText(LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext, LPCOLESTR pstrDelimiter,
										 DWCOOKIE dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
	HRESULT hr = S_OK;

	string strCode = OEMFromUnicode(pstrCode);

	if(luaL_loadstring(m_pLua, strCode.c_str()))
	{
		m_sLastError = lua_tostring(m_pLua, -1);
		m_spSite->OnScriptError(this);
	}
	else
	{
//		lua_setglobal(m_pLua, "_");
		m_bLuaLoaded = true;
	}

	if (m_bLuaLoaded && m_eState == SCRIPTSTATE_STARTED)
	{
		hr = Run();
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////
//	IActiveScriptError
STDMETHODIMP CLuaScript::GetExceptionInfo(EXCEPINFO *pexcepinfo)
{
	::memset(pexcepinfo, 0, sizeof(EXCEPINFO));
	pexcepinfo->bstrSource = ::SysAllocString(L"LuaScript");
	pexcepinfo->bstrDescription = ::SysAllocString(m_sLastError);
	pexcepinfo->scode = OLESCRIPT_E_SYNTAX;

	return S_OK;
}

STDMETHODIMP CLuaScript::GetSourcePosition(DWORD *pdwSourceContext, ULONG *pulLineNumber, LONG *plCharacterPosition)
{
	if(pdwSourceContext) *pdwSourceContext = 0;
	*pulLineNumber = 0;
	*plCharacterPosition = 0;
	return S_OK;
}

STDMETHODIMP CLuaScript::GetSourceLineText(BSTR *pbstrSourceLine)
{
	*pbstrSourceLine = NULL;
	return E_NOTIMPL;
}

HRESULT CLuaScript::Run()
{
	for (std::map<CComBSTR,DWORD>::iterator i = m_oGlobalNames.begin(); i != m_oGlobalNames.end(); ++i)
	{
		LPCOLESTR pstrName = (i->first).m_str;

		IUnknownPtr spUnknown;
		ITypeInfoPtr spTypeInfo;
		IDispatchPtr spDispatch;

		if (SUCCEEDED(m_spSite->GetItemInfo(pstrName, SCRIPTINFO_IUNKNOWN, &spUnknown, &spTypeInfo)) &&
			((spDispatch = spUnknown) != NULL))
		{
			luacom_IDispatch2LuaCOM(m_pLua, spDispatch);
			lua_setglobal(m_pLua, ATL::CW2A(pstrName));
		}
	}

	m_spSite->OnEnterScript();

	if (lua_pushvalue(m_pLua, 1), lua_pcall(m_pLua, 0, 0, 0))
	{
		m_sLastError = lua_tostring(m_pLua, -1);
		m_spSite->OnScriptError( static_cast<IActiveScriptError*>(this) );
	}

	m_spSite->OnLeaveScript();

	return S_OK;
}
