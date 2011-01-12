#include "StdAfx.h"
#include "RESearch.h"
#include "RESearchIDL.h"

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

class CReplaceParameters : public IReplaceParameters {
public:
	CReplaceParameters(CREParameters<TCHAR> &Param, const TCHAR *EOL)
		: m_nCounter(0)
		, m_Param(Param)
		, m_szEOL(EOL)
	{
	}

	tstring Result() {return m_strResult;}
	void SetOuter(IDispatch *pOuter) {m_pOuter = pOuter;}
	~CReplaceParameters() {}

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj) {
		if (riid == __uuidof(IUnknown)) {
			*ppvObj=static_cast<IReplaceParameters *>(this);
		} else if (riid == __uuidof(IReplaceParameters)) {
			*ppvObj=static_cast<IReplaceParameters *>(this);
		} else if (m_pOuter) {
			return m_pOuter->QueryInterface(riid, ppvObj);
		} else {
			*ppvObj=NULL;return E_NOINTERFACE;
		}
		static_cast<IUnknown *>(*ppvObj)->AddRef();
		return S_OK;
	}
	STDMETHOD_(ULONG, AddRef)() {
		return ++m_nCounter;
	}
	STDMETHOD_(ULONG, Release)() {
		if (--m_nCounter==0) {
			delete this;return 0;
		} else return m_nCounter;
	}

	// IReplaceParameters methods
	STDMETHOD(match)(long lPos, BSTR *pbstrMatch) {
		*pbstrMatch = _bstr_t(m_Param.GetParam(lPos).c_str()).Detach();
		return S_OK;
	}

	STDMETHOD(named)(BSTR strParam, BSTR *pbstrMatch) {
		*pbstrMatch = _bstr_t(m_Param.GetParam((LPCTSTR)_bstr_t(strParam)).c_str()).Detach();
		return S_OK;
	}

	STDMETHOD(get_eol)(BSTR *pbstrEOL) {
		*pbstrEOL = _bstr_t(m_szEOL).Detach();
		return S_OK;
	}

	STDMETHOD(get_l)(long *pValue) {
		*pValue = _ttoi(m_Param.GetParam(_T("L")).c_str());
		return S_OK;
	}

	STDMETHOD(get_n)(long *pValue) {
		*pValue = _ttoi(m_Param.GetParam(_T("N")).c_str());
		return S_OK;
	}

	STDMETHOD(get_r)(long *pValue) {
		*pValue = _ttoi(m_Param.GetParam(_T("R")).c_str());
		return S_OK;
	}

	STDMETHOD(result)(BSTR bstrResult) {
		m_strResult = _bstr_t(bstrResult);
		return S_OK;
	}

private:
	ULONG m_nCounter;

	CREParameters<TCHAR> &m_Param;
	const TCHAR *m_szEOL;
	tstring m_strResult;

	IDispatch *m_pOuter;
};

// --------------------------------------------------

class CReplaceScriptSite : public IActiveScriptSite {
public:
	CReplaceScriptSite(CReplaceParameters *pParams) : m_nCounter(0), m_pParams(pParams), m_pDispatch(NULL) {
		TCHAR szModule[MAX_PATH];
		GetModuleFileName((HMODULE)g_hInstance, szModule, MAX_PATH);

		ITypeLib *pLib;
		HRESULT hResult = LoadTypeLib(_bstr_t(szModule), &pLib);
		if (FAILED(hResult)) {
			ShowHResultError(MErrorLoadingTypeLib, hResult);
			g_bInterrupted = TRUE;
			return;
		}

		ITypeInfo *pInfo;
		hResult = pLib->GetTypeInfoOfGuid(__uuidof(IReplaceParameters), &pInfo);
		if (FAILED(hResult)) {
			ShowHResultError(MErrorLoadingTypeLib, hResult);
			pLib->Release();
			g_bInterrupted = TRUE;
			return;
		}

		hResult = CreateStdDispatch(pParams, pParams, pInfo, (IUnknown **)&m_pDispatch);
		if (FAILED(hResult)) {
			ShowHResultError(MErrorLoadingTypeLib, hResult);
			g_bInterrupted = TRUE;
		}

		pParams->SetOuter(m_pDispatch);

		pInfo->Release();
		pLib->Release();
	}
	~CReplaceScriptSite() {
		if (m_pDispatch) m_pDispatch->Release();
	}

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj) {
		if (riid==IID_IUnknown) {
			*ppvObj=static_cast<IActiveScriptSite *>(this);
		} else if (riid==IID_IActiveScriptSite) {
			*ppvObj=static_cast<IActiveScriptSite *>(this);
		} else {
			*ppvObj=NULL;return E_NOINTERFACE;
		}
		static_cast<IUnknown *>(*ppvObj)->AddRef();
		return S_OK;
	}
	STDMETHOD_(ULONG, AddRef)() {
		return ++m_nCounter;
	}
	STDMETHOD_(ULONG, Release)() {
		if (--m_nCounter==0) {
			delete this;return 0;
		} else return m_nCounter;
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
				HRESULT hr = m_pDispatch->GetTypeInfo(0, 0, ppti);
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
		g_bInterrupted = TRUE;
		return S_OK;
	}
	STDMETHOD(OnEnterScript)() {return S_OK;}
	STDMETHOD(OnLeaveScript)() {return S_OK;}
private:
	ULONG m_nCounter;

	CReplaceParameters *m_pParams;
	IDispatch *m_pDispatch;
};

tstring EvaluateReplaceString(CREParameters<TCHAR> &Param, const TCHAR *Replace, const TCHAR *EOL, int Engine) {
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

	CReplaceParameters *pParams = new CReplaceParameters(Param, EOL);
	pParams->AddRef();
	CReplaceScriptSite *pSite = new CReplaceScriptSite(pParams);
	pSite->AddRef();
	spEngine->SetScriptSite(pSite);

	_bstr_t bstrText;
	if ((_tcslen(Replace) > 3) && (Replace[1] == ':') && (Replace[2] == '\\')) {
		CFileMapping mapScript;
		if (!mapScript.Open(Replace)) {
			const TCHAR *Lines[]={GetMsg(MREReplace),GetMsg(MFileOpenError),Replace,GetMsg(MOk)};
			StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,_T("FSOpenError"),Lines,4,1);
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
