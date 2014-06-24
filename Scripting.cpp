#include "StdAfx.h"
#include "RESearch.h"

#undef DEFINE_GUID	//	No other chance, since guiddef is in StdAfx
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#include <activscp.h>

_COM_SMARTPTR_TYPEDEF(IActiveScript, __uuidof(IActiveScript));
_COM_SMARTPTR_TYPEDEF(IActiveScriptParse, __uuidof(IActiveScriptParse));
_COM_SMARTPTR_TYPEDEF(ICatInformation, __uuidof(ICatInformation));
_COM_SMARTPTR_TYPEDEF(IEnumCLSID, __uuidof(IEnumCLSID));

#ifdef FAR3
LPOLESTR g_szFarLUA = L"{40308C27-BCE7-4244-B5FF-5DFB32F9E778}";
LPOLESTR g_szFarMS  = L"{1ED8D46A-36D9-4542-9160-C8A8535CB683}";
#endif

void AddCustomScriptEngines()
{
	sActiveScript Script;
#ifdef FAR3
	CLSIDFromString(g_szFarLUA, &Script.m_clsid);
	Script.m_strName = L"FarLUA";
	m_arrEngines.push_back(Script);
	m_lstEngines.Append(Script.m_strName.c_str());
/*
	CLSIDFromString(g_szFarMS, &Script.m_clsid);
	Script.m_strName = L"FarMoonScript";
	m_arrEngines.push_back(Script);
	m_lstEngines.Append(Script.m_strName.c_str());*/
#endif
}

void EnumActiveScripts()
{
	m_arrEngines.clear();

	ICatInformationPtr spCatInfo;
	spCatInfo.CreateInstance(CLSID_StdComponentCategoriesMgr);

	IEnumCLSIDPtr pEnumCLSID;
	spCatInfo->EnumClassesOfCategories(1, (CATID *)&CATID_ActiveScriptParse, -1, NULL, &pEnumCLSID);

	do {
		sActiveScript Script;
		ULONG nFetched;
		pEnumCLSID->Next(1, &Script.m_clsid, &nFetched);
		if (!nFetched) break;

		BSTR bstrName;
		if (SUCCEEDED(ProgIDFromCLSID(Script.m_clsid, &bstrName))) {
			Script.m_strName = (LPCTSTR)_bstr_t(bstrName);
		} else {
			OLECHAR szGuid[42];
			if (FAILED(StringFromGUID2(Script.m_clsid, szGuid, 42))) continue;
			Script.m_strName = (LPCTSTR)_bstr_t(szGuid);
		}
		m_arrEngines.push_back(Script);
		m_lstEngines.Append(Script.m_strName.c_str());
	} while (true);

	AddCustomScriptEngines();
}

LPCTSTR ScriptEngine(bool bEnabled)
{
	return bEnabled ? EREvaluateScript.c_str() : NULL;
}

int EngineIndex(const tstring &strValue)
{
	CLSID clsid;
	HRESULT hr = CLSIDFromString(_bstr_t(strValue.c_str()), &clsid);

	if (SUCCEEDED(hr)) {
		for (size_t nEngine = 0; nEngine < m_arrEngines.size(); nEngine++)
		{
			if (memcmp(&m_arrEngines[nEngine].m_clsid, &clsid, sizeof(CLSID)) == 0) return nEngine;
		}
		return 0;
	} else {
		return _ttoi(strValue.c_str());
	}
}

void SetEngineIndex(int nIndex, tstring &strValue)
{
	OLECHAR szGuid[64];
	StringFromGUID2(m_arrEngines[nIndex].m_clsid, szGuid, 64);
#ifdef UNICODE
	strValue = szGuid;
#else
	strValue = ANSIFromUnicode(szGuid);
#endif
}

void SanitateEngine()
{
	SetEngineIndex(EngineIndex(EREvaluateScript), EREvaluateScript);
}

CFarEngineStorage::CFarEngineStorage(tstring &strEngine)
: CFarIntegerStorage(m_nEngine)
, m_strEngine(strEngine)
{
}

void CFarEngineStorage::Get(TCHAR *pszBuffer, int nSize) const
{
	m_nEngine = EngineIndex(m_strEngine);
	__super::Get(pszBuffer, nSize);
}

void CFarEngineStorage::Put(int nValue)
{
	__super::Put(nValue);
	SetEngineIndex(m_nEngine, m_strEngine);
}

// ---------------------------------------------------

template<class CHAR, class CONVERT>
class CReplaceParametersT
	: public CComObjectRootEx<CComSingleThreadModel>
	, public IDispatchImpl<IReplaceParameters, &__uuidof(IReplaceParameters), &__uuidof(__RESearchLib), -1, -1>
{
public:
	typedef CStringOperations<CHAR> CSO;
	typedef typename CSO::cstring cstring;

	BEGIN_COM_MAP(CReplaceParametersT)
		COM_INTERFACE_ENTRY(IReplaceParameters)
		COM_INTERFACE_ENTRY(IDispatch)
	END_COM_MAP()

	void Init(CREParameters<CHAR> *pParam, const CHAR *szEOL)
	{
		m_pParam = pParam;
		m_szEOL  = szEOL;
	}

	cstring Result() { return m_strResult; }

	// IReplaceParameters methods
	STDMETHOD(match)(long lPos, BSTR *pbstrMatch) {
		*pbstrMatch = CONVERT::ToBstr(m_pParam->GetParam(lPos).c_str());
		return S_OK;
	}

	STDMETHOD(named)(BSTR strParam, BSTR *pbstrMatch) {
		*pbstrMatch = CONVERT::ToBstr(m_pParam->GetParam(CONVERT::FromBSTR<CHAR>(strParam)).c_str());
		return S_OK;
	}

	STDMETHOD(eol)(BSTR *pbstrEOL) {
		*pbstrEOL = CONVERT::ToBstr(m_szEOL);
		return S_OK;
	}

	STDMETHOD(l)(long *pValue) {
		*pValue = CSO::ctoi(m_pParam->GetParam(CSO::__T2("L", L"L")).c_str());
		return S_OK;
	}

	STDMETHOD(n)(long *pValue) {
		*pValue = CSO::ctoi(m_pParam->GetParam(CSO::__T2("N", L"N")).c_str());
		return S_OK;
	}

	STDMETHOD(s)(long *pValue) {
		*pValue = CSO::ctoi(m_pParam->GetParam(CSO::__T2("S", L"S")).c_str());
		return S_OK;
	}

	STDMETHOD(r)(long *pValue) {
		*pValue = CSO::ctoi(m_pParam->GetParam(CSO::__T2("R", L"R")).c_str());
		return S_OK;
	}

	STDMETHOD(result)(BSTR bstrResult) {
		m_strResult = CONVERT::FromBSTR<CHAR>(bstrResult);
		return S_OK;
	}

	STDMETHOD(init )(BSTR strParam, BSTR strValue) {
		m_pParam->InitParam(CONVERT::FromBSTR<CHAR>(strParam), CONVERT::FromBSTR<CHAR>(strValue));
		return S_OK;
	}

	STDMETHOD(store)(BSTR strParam, BSTR strValue) {
		m_pParam->SetParam(CONVERT::FromBSTR<CHAR>(strParam), CONVERT::FromBSTR<CHAR>(strValue));
		return S_OK;
	}

	STDMETHOD(skip)() {
		g_bSkipReplace = true;
		return S_OK;
	}

private:
	CREParameters<CHAR> *m_pParam;
	const CHAR *m_szEOL;

	cstring m_strResult;
};

// --------------------------------------------------

class CReplaceScriptSite
	: public CComObjectRootEx < CComSingleThreadModel >
	, public IActiveScriptSite
{
public:
	BEGIN_COM_MAP(CReplaceScriptSite)
		COM_INTERFACE_ENTRY(IActiveScriptSite)
	END_COM_MAP()

	void Init(IReplaceParameters *pParams)
	{
		m_pParams = pParams;
	}

	// IActiveScriptSite methods
	STDMETHOD(GetLCID)(LCID *plcid) {return E_NOTIMPL;}

	STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti) {
		if ((dwReturnMask & SCRIPTINFO_IUNKNOWN) && !ppiunkItem)
			return E_INVALIDARG;
		else
			if (ppiunkItem) *ppiunkItem = NULL;

		if (dwReturnMask & SCRIPTINFO_ITYPEINFO) {
			if (!ppti) return E_INVALIDARG;
			*ppti=NULL;
		} else {
			if (ppti) *ppti=NULL;
		}

		if (m_pParams && (_wcsicmp(pstrName, L"research") == 0)) {
			if (dwReturnMask & SCRIPTINFO_ITYPEINFO) {
				HRESULT hr = m_pParams->GetTypeInfo(0, 0, ppti);
				if (FAILED(hr)) return hr;
			}
			if (dwReturnMask & SCRIPTINFO_IUNKNOWN) {
				*ppiunkItem = m_pParams;
				(*ppiunkItem)->AddRef();
			}
			return S_OK;
		}

		return TYPE_E_ELEMENTNOTFOUND;
	}

	STDMETHOD(GetDocVersionString)(BSTR *pszVersion) {return E_NOTIMPL;}
	STDMETHOD(OnScriptTerminate)(const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo) {
		return S_OK;
	}
	STDMETHOD(OnStateChange)(SCRIPTSTATE ssScriptState) {return S_OK;}
	STDMETHOD(OnScriptError)(IActiveScriptError *pscripterror) {
		EXCEPINFO ExcepInfo;
		if (SUCCEEDED(pscripterror->GetExceptionInfo(&ExcepInfo))) {
			if (ExcepInfo.pfnDeferredFillIn) ExcepInfo.pfnDeferredFillIn(&ExcepInfo);
			ShowErrorMsg(GetMsg(MErrorExecutingScript), _bstr_t(ExcepInfo.bstrDescription));
		} else {
			ShowErrorMsg(GetMsg(MErrorExecutingScript));
		}
		g_bInterrupted = true;
		return S_OK;
	}
	STDMETHOD(OnEnterScript)() {return S_OK;}
	STDMETHOD(OnLeaveScript)() {return S_OK;}
private:
	ULONG m_nCounter;

	IReplaceParameters *m_pParams;
};

template<class CONVERT>
void EvaluateReplaceStringT(IReplaceParameters *pParams, LPCTSTR szReplace, LPCTSTR szEngine)
{
	EXCEPINFO ExcepInfo;
	HRESULT hResult;

	if (Interrupted()) return;

	CLSID clsid;
	CLSIDFromString(_bstr_t(szEngine), &clsid);

	IActiveScriptPtr spEngine;
	hResult = spEngine.CreateInstance(clsid);
	if (FAILED(hResult)) {
		ShowHResultError(MErrorCreatingEngine, hResult);
		g_bInterrupted = true;
		return;
	}

	CReplaceScriptSite *pSite = new CComObject<CReplaceScriptSite>();
	pSite->Init(pParams);
	pSite->AddRef();

	spEngine->SetScriptSite(pSite);

	_bstr_t bstrText;
	if ((_tcslen(szReplace) > 3) && (szReplace[1] == ':') && (szReplace[2] == '\\')) {
		CFileMapping mapScript;
		if (!mapScript.Open(szReplace)) {
			const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),szReplace,GetMsg(MOk)};
			StartupInfo.Message(FMSG_WARNING,_T("FSOpenError"),Lines,4,1);
			g_bInterrupted = true;
			return;
		}
		bstrText = ANSIToUnicode(string(mapScript, mapScript.Size())).c_str();
	} else {
		bstrText.Attach(CONVERT::ToBstr(szReplace));
	}

	IActiveScriptParsePtr spParser = spEngine;
	spParser->InitNew();
	hResult = spParser->ParseScriptText(bstrText, NULL, NULL, NULL, 0, 0, 0, NULL, &ExcepInfo);
	if (FAILED(hResult)) {
		ShowHResultError(MErrorParsingText, hResult);
		g_bInterrupted = true;
		return;
	}

	spEngine->AddNamedItem(L"research", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_NOCODE|SCRIPTITEM_GLOBALMEMBERS);
	spEngine->SetScriptState(SCRIPTSTATE_CONNECTED);

	spParser = NULL;
	spEngine = NULL;
	pSite->Release();
}

#ifdef UNICODE
class CConverter {
public:
	static BSTR ToBstr(LPCWSTR sz) { return SysAllocString(sz); }
	static BSTR ToBstr(LPCSTR  sz) { return SysAllocString(UTF8ToUnicode(sz).c_str()); }
	template<class CHAR> static basic_string<CHAR> FromBSTR(BSTR str);
};
template<> static basic_string< char  > CConverter::FromBSTR(BSTR str) { return UTF8FromUnicode(str); }
template<> static basic_string<wchar_t> CConverter::FromBSTR(BSTR str) { return str; }
#else
class CConverter {
public:
	static BSTR ToBstr(LPCSTR  sz)   { return SysAllocString(OEMToUnicode(sz).c_str()); }
	template<class CHAR> static string FromBSTR(BSTR str) { return OEMFromUnicode(str); }
};
#endif

template<>
basic_string<char> EvaluateReplaceString(CREParameters<char> &Param, const char *Replace, const char *EOL, LPCTSTR szEngine)
{
	CReplaceParametersT<char, CConverter> *pParams = new CComObject<CReplaceParametersT<char, CConverter> >();
	g_spREParam = pParams;
	pParams->Init(&Param, EOL);

#ifdef UNICODE
	EvaluateReplaceStringT<CConverter>(pParams, UTF8ToUnicode(Replace).c_str(), szEngine);
#else
	EvaluateReplaceStringT<CConverter>(pParams, Replace, szEngine);
#endif

	basic_string<char> strResult = pParams->Result();

	g_spREParam = NULL;

	return strResult;
}

#ifdef UNICODE

template<>
basic_string<wchar_t> EvaluateReplaceString(CREParameters<wchar_t> &Param, const wchar_t *Replace, const wchar_t *EOL, LPCTSTR szEngine)
{
	CReplaceParametersT<wchar_t, CConverter> *pParams = new CComObject<CReplaceParametersT<wchar_t, CConverter> >();
	g_spREParam = pParams;
	pParams->Init(&Param, EOL);

#ifdef FAR3
	if (_wcsicmp(szEngine, g_szFarLUA) == 0)
		return EvaluateLUAString(Param, Replace, KMFLAGS_LUA);
	if (_wcsicmp(szEngine, g_szFarMS ) == 0)
		return EvaluateLUAString(Param, Replace, KMFLAGS_MOONSCRIPT);
#endif

	EvaluateReplaceStringT<CConverter>(pParams, Replace, szEngine);

	basic_string<wchar_t> strResult = pParams->Result();

	g_spREParam = NULL;

	return strResult;
}

#endif
