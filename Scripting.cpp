#include "StdAfx.h"
#include "RESearch.h"
#include "RESearchIDL.h"

_COM_SMARTPTR_TYPEDEF(IActiveScript, __uuidof(IActiveScript));
_COM_SMARTPTR_TYPEDEF(IActiveScriptParse, __uuidof(IActiveScriptParse));
_COM_SMARTPTR_TYPEDEF(ICatInformation, __uuidof(ICatInformation));
_COM_SMARTPTR_TYPEDEF(IEnumCLSID, __uuidof(IEnumCLSID));

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
		ProgIDFromCLSID(Script.m_clsid, &bstrName);
		Script.m_strName = (const char *)_bstr_t(bstrName);
		m_arrEngines.push_back(Script);
		m_lstEngines.Append(Script.m_strName.c_str());
	} while (true);
}

// ---------------------------------------------------

class CReplaceParameters : public IReplaceParameters {
public:
	CReplaceParameters(const char *Matched, int *Match, int Count, const char *EOL, int *Numbers)
		: m_nCounter(0)
		, m_szMatched(Matched)
		, m_pMatch(Match)
		, m_nCount(Count)
		, m_szEOL(EOL)
		, m_pNumbers(Numbers)
		, m_pOuter(NULL)
	{
	}

	string Result() {return m_strResult;}
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
		if ((lPos < 0) || (lPos >= m_nCount)) return E_INVALIDARG;
		*pbstrMatch = _bstr_t(string(m_szMatched + m_pMatch[lPos*2], m_pMatch[lPos*2+1] - m_pMatch[lPos*2]).c_str()).copy();
		return S_OK;
	}

	STDMETHOD(get_eol)(BSTR *pbstrEOL) {
		*pbstrEOL = _bstr_t(m_szEOL).copy();
		return S_OK;
	}

	STDMETHOD(get_l)(long *pValue) {
		*pValue = m_pNumbers[0];
		return S_OK;
	}

	STDMETHOD(get_n)(long *pValue) {
		*pValue = m_pNumbers[1];
		return S_OK;
	}

	STDMETHOD(get_r)(long *pValue) {
		*pValue = m_pNumbers[2];
		return S_OK;
	}

	STDMETHOD(result)(BSTR bstrResult) {
		m_strResult = _bstr_t(bstrResult);
		return S_OK;
	}

private:
	ULONG m_nCounter;
	const char *m_szMatched;
	int *m_pMatch;
	int m_nCount;
	const char *m_szEOL;
	int *m_pNumbers;
	string m_strResult;

	IDispatch *m_pOuter;
};

// --------------------------------------------------

class CReplaceScriptSite : public IActiveScriptSite {
public:
	CReplaceScriptSite(CReplaceParameters *pParams) : m_nCounter(0), m_pParams(pParams), m_pDispatch(NULL) {
		char szModule[MAX_PATH];
		GetModuleFileName(GetModuleHandle("RESearch.dll"), szModule, MAX_PATH);
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

string EvaluateReplaceString(const char *Matched,int *Match,int Count,const char *Replace,const char *EOL,int *Numbers,int Engine) {
	EXCEPINFO ExcepInfo;
	HRESULT hResult;

	if (Interrupted()) return NULL;

	IActiveScriptPtr spEngine;
	hResult = spEngine.CreateInstance(m_arrEngines[Engine].m_clsid);
	if (FAILED(hResult)) {
		ShowHResultError(MErrorCreatingEngine, hResult);
		g_bInterrupted = TRUE;
		return NULL;
	}

	CReplaceParameters *pParams = new CReplaceParameters(Matched, Match, Count, EOL, Numbers);
	pParams->AddRef();
	CReplaceScriptSite *pSite = new CReplaceScriptSite(pParams);
	pSite->AddRef();
	spEngine->SetScriptSite(pSite);

	IActiveScriptParsePtr spParser = spEngine;
	spParser->InitNew();
	hResult = spParser->ParseScriptText(_bstr_t(Replace), NULL, NULL, NULL, 0, 0, 0, NULL, &ExcepInfo);
	if (FAILED(hResult)) {
		ShowHResultError(MErrorParsingText, hResult);
		g_bInterrupted = TRUE;
		return NULL;
	}

	spEngine->AddNamedItem(L"research", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_NOCODE|SCRIPTITEM_GLOBALMEMBERS);
	spEngine->SetScriptState(SCRIPTSTATE_CONNECTED);

	string strResult = pParams->Result();
	spParser = NULL;
	spEngine = NULL;
	pSite->Release();
	pParams->Release();

	return strResult;
}
