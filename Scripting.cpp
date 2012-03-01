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

void EnumActiveScripts();

void ReadActiveScripts() {
	CHKey hKey = OpenRegistry(_T("ScriptEngines"));

	TCHAR szKey[256];
	DWORD dwIndex = 0, dwSize;
	while ((dwSize = arrsizeof(szKey)), (RegEnumValue(hKey, dwIndex++, szKey, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)) {
		sActiveScript Script;
		if (FAILED(CLSIDFromString(_bstr_t(szKey), &Script.m_clsid))) continue;
		QueryRegStringValue(hKey, szKey, Script.m_strName, szKey);

		m_arrEngines.push_back(Script);
		m_lstEngines.Append(Script.m_strName.c_str());
	}

	if (m_arrEngines.size() == 0) {
		EnumActiveScripts();
		for (size_t nKey = 0; nKey < m_arrEngines.size(); nKey++) {
			OLECHAR szGuid[42];
			if (FAILED(StringFromGUID2(m_arrEngines[nKey].m_clsid, szGuid, 42))) continue;
			SetRegStringValue(hKey, (LPCTSTR)_bstr_t(szGuid), m_arrEngines[nKey].m_strName);
		}
	}
}

void EnumActiveScripts() {
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
}

// ---------------------------------------------------

class CReplaceParameters
	: public CComObjectRootEx<CComSingleThreadModel>
	, public IDispatchImpl<IReplaceParameters, &__uuidof(IReplaceParameters), &__uuidof(__RESearchLib), -1, -1>
{
public:
	BEGIN_COM_MAP(CReplaceParameters)
		COM_INTERFACE_ENTRY(IReplaceParameters)
		COM_INTERFACE_ENTRY(IDispatch)
	END_COM_MAP()

	void Init(TREParameters *pParam, const TCHAR *szEOL)
	{
		m_pParam = pParam;
		m_szEOL  = szEOL;
	}

	tstring Result() {return m_strResult;}

	// IReplaceParameters methods
	STDMETHOD(match)(long lPos, BSTR *pbstrMatch) {
		*pbstrMatch = _bstr_t(m_pParam->GetParam(lPos).c_str()).Detach();
		return S_OK;
	}

	STDMETHOD(named)(BSTR strParam, BSTR *pbstrMatch) {
		*pbstrMatch = _bstr_t(m_pParam->GetParam((LPCTSTR)_bstr_t(strParam)).c_str()).Detach();
		return S_OK;
	}

	STDMETHOD(eol)(BSTR *pbstrEOL) {
		*pbstrEOL = _bstr_t(m_szEOL).Detach();
		return S_OK;
	}

	STDMETHOD(l)(long *pValue) {
		*pValue = _ttoi(m_pParam->GetParam(_T("L")).c_str());
		return S_OK;
	}

	STDMETHOD(n)(long *pValue) {
		*pValue = _ttoi(m_pParam->GetParam(_T("N")).c_str());
		return S_OK;
	}

	STDMETHOD(r)(long *pValue) {
		*pValue = _ttoi(m_pParam->GetParam(_T("R")).c_str());
		return S_OK;
	}

	STDMETHOD(result)(BSTR bstrResult) {
		m_strResult = _bstr_t(bstrResult);
		return S_OK;
	}

	STDMETHOD(init )(BSTR strParam, BSTR strValue) {
		m_pParam->InitParam((LPCTSTR)_bstr_t(strParam), (LPCTSTR)_bstr_t(strValue));
		return S_OK;
	}

	STDMETHOD(store)(BSTR strParam, BSTR strValue) {
		m_pParam->SetParam((LPCTSTR)_bstr_t(strParam), (LPCTSTR)_bstr_t(strValue));
		return S_OK;
	}

private:
	TREParameters *m_pParam;
	const TCHAR *m_szEOL;
	tstring m_strResult;
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

	void Init(CReplaceParameters *pParams)
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
				*ppiunkItem = m_pParams->GetUnknown();
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
		g_bInterrupted = TRUE;
		return S_OK;
	}
	STDMETHOD(OnEnterScript)() {return S_OK;}
	STDMETHOD(OnLeaveScript)() {return S_OK;}
private:
	ULONG m_nCounter;

	CReplaceParameters *m_pParams;
};

tstring EvaluateReplaceString(TREParameters &Param, const TCHAR *Replace, const TCHAR *EOL, int Engine) {
	EXCEPINFO ExcepInfo;
	HRESULT hResult;

	if (Interrupted()) return _T("");

	IActiveScriptPtr spEngine;
	hResult = spEngine.CreateInstance(m_arrEngines[Engine].m_clsid);
	if (FAILED(hResult)) {
		ShowHResultError(MErrorCreatingEngine, hResult);
		g_bInterrupted = TRUE;
		return _T("");
	}

	CReplaceParameters *pParams = new CComObject<CReplaceParameters>();
	pParams->Init(&Param, EOL);
	pParams->AddRef();

	CReplaceScriptSite *pSite = new CComObject<CReplaceScriptSite>();
	pSite->Init(pParams);
	pSite->AddRef();

	spEngine->SetScriptSite(pSite);

	_bstr_t bstrText;
	if ((_tcslen(Replace) > 3) && (Replace[1] == ':') && (Replace[2] == '\\')) {
		CFileMapping mapScript;
		if (!mapScript.Open(Replace)) {
			const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),Replace,GetMsg(MOk)};
			StartupInfo.Message(FMSG_WARNING,_T("FSOpenError"),Lines,4,1);
			g_bInterrupted = TRUE;
			return _T("");
		}
		bstrText = tstring(mapScript, mapScript.Size()).c_str();
	} else {
		bstrText = Replace;
	}

	IActiveScriptParsePtr spParser = spEngine;
	spParser->InitNew();
	hResult = spParser->ParseScriptText(bstrText, NULL, NULL, NULL, 0, 0, 0, NULL, &ExcepInfo);
	if (FAILED(hResult)) {
		ShowHResultError(MErrorParsingText, hResult);
		g_bInterrupted = TRUE;
		return _T("");
	}

	spEngine->AddNamedItem(L"research", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_NOCODE|SCRIPTITEM_GLOBALMEMBERS);
	spEngine->SetScriptState(SCRIPTSTATE_CONNECTED);

	tstring strResult = pParams->Result();
	spParser = NULL;
	spEngine = NULL;
	pSite->Release();
	pParams->Release();

	return strResult;
}
