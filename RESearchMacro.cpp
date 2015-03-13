#include "StdAfx.h"
#include "RESearch.h"
#include "resource.h"

#ifdef FAR3

static LPCWSTR g_szPanelCommands[] = {
	L"Search", L"Replace", L"Grep", NULL,
	L"Select", L"Unselect", L"FlipSelection", NULL,
	L"Rename", L"RenameSelected", L"Renumber", L"UndoRename", NULL,
	L"ShowLastResults", L"ClearVariables"
};

static LPCWSTR g_szEditorCommands[] = {
	L"Search", L"Replace", L"Filter", L"Repeat", L"Transliterate", NULL,
	L"SRAgain", L"SRAgainRev", NULL,
	L"ShowLastResults", L"ClearVariables"
};

static LPCWSTR g_szViewerCommands[] = {
	L"Search", L"SRAgain", L"ClearVariables"
};

bool GetIntValue(const FarMacroValue &Value, int &nValue)
{
	switch (Value.Type) {
	case FMVT_INTEGER:
		nValue = (int)Value.Integer;
		return true;
	case FMVT_DOUBLE:
		nValue = (int)Value.Double;
		return true;
	case FMVT_STRING:
		return swscanf(Value.String, L"%d", &nValue) == 1;
	default:
		return false;
	}
}

bool GetStringValue(const FarMacroValue &Value, string &strValue)
{
	switch (Value.Type) {
	case FMVT_INTEGER:
		strValue = FormatStrA("%d", (int)Value.Integer);
		return true;
	case FMVT_DOUBLE:
		strValue = FormatStrA("%g", Value.Double);
		return true;
	case FMVT_STRING:
		strValue = FormatStrA("%S", Value.String);
		return true;
	default:
		return false;
	}
}

bool GetStringValue(const FarMacroValue &Value, wstring &strValue)
{
	switch (Value.Type) {
	case FMVT_INTEGER:
		strValue = FormatStrW(L"%d", (int)Value.Integer);
		return true;
	case FMVT_DOUBLE:
		strValue = FormatStrW(L"%g", Value.Double);
		return true;
	case FMVT_STRING:
		strValue = Value.String;
		return true;
	default:
		return false;
	}
}

wstring GetStringValue(const FarMacroValue &Value)
{
	wstring strValue;
	GetStringValue(Value, strValue);
	return strValue;
}

bool GetVariantValue(const FarMacroValue &Value, _variant_t &vtValue)
{
	switch (Value.Type) {
	case FMVT_INTEGER:
		vtValue = (int)Value.Integer;
		return true;
	case FMVT_DOUBLE:
		vtValue = Value.Double;
		return true;
	case FMVT_STRING:
		vtValue = Value.String;
		return true;
	default:
		return false;
	}
}

_variant_t GetVariantValue(const FarMacroValue &Value)
{
	_variant_t vtValue;
	GetVariantValue(Value, vtValue);
	return vtValue;
}

HANDLE RunFilePreset(LPCWSTR szPreset)
{
	OperationResult Result = OR_FAILED;

	FindRunPreset(FSPresets, szPreset, Result) ||
	FindRunPreset(FRPresets, szPreset, Result) ||
	FindRunPreset(FGPresets, szPreset, Result) ||
	FindRunPreset(RnPresets, szPreset, Result) ||
	FindRunPreset(QRPresets, szPreset, Result);

	return HandleFromOpResult(Result);
}

HANDLE RunEditorPreset(LPCWSTR szPreset)
{
	OperationResult Result = OR_FAILED;

	FindRunPreset(ESPresets, szPreset, Result) ||
	FindRunPreset(ERPresets, szPreset, Result) ||
	FindRunPreset(EFPresets, szPreset, Result) ||
	FindRunPreset(ETPresets, szPreset, Result);

	return NO_PANEL_HANDLE;
}

HANDLE RunViewerPreset(LPCWSTR szPreset)
{
	OperationResult Result = OR_FAILED;

	FindRunPreset(VSPresets, szPreset, Result);

	return NO_PANEL_HANDLE;
}

HANDLE RunBatch(CBatchActionCollection *pBatches, LPCWSTR szBatch)
{
	CBatchAction *pAction;

	if (pAction = pBatches->FindMenuAction(szBatch)) {
		pAction->Execute();
	} else {
		ShowError(GetMsg(MBatchNotFound), szBatch);
	}

	return NO_PANEL_HANDLE;
}

HANDLE OpenFromStringMacro(int nType, LPCWSTR szParam)
{
	if (nType == 0) {
		for (int nValue = 0; nValue < sizeof(g_szPanelCommands)/sizeof(g_szPanelCommands[0]); nValue++) {
			if (g_szPanelCommands[nValue] == NULL) continue;
			if (_wcsicmp(g_szPanelCommands[nValue], szParam) == 0)
				return OpenPluginFromFileMenu(nValue);
		}

		if (_wcsicmp(szParam, L"Menu") == 0) {
			g_bFromCmdLine = false;
			return OpenPluginFromFileMenu(0);
		}
		if (_wcsicmp(szParam, L"Batch") == 0) {
			g_pPanelBatches->ShowMenu();
		}

	} else if (nType == 1) {
		for (int nValue = 0; nValue < sizeof(g_szEditorCommands)/sizeof(g_szEditorCommands[0]); nValue++) {
			if (g_szEditorCommands[nValue] == NULL) continue;
			if (_wcsicmp(g_szEditorCommands[nValue], szParam) == 0)
				return OpenPluginFromEditorMenu(nValue);
		}

		if (_wcsicmp(szParam, L"Menu") == 0) {
			g_bFromCmdLine = false;
			return OpenPluginFromEditorMenu(0);
		}
		if (_wcsicmp(szParam, L"Batch") == 0) {
			g_pEditorBatches->ShowMenu();
		}

	} else if (nType == 2) {
		for (int nValue = 0; nValue < sizeof(g_szViewerCommands)/sizeof(g_szViewerCommands[0]); nValue++) {
			if (g_szViewerCommands[nValue] == NULL) continue;
			if (_wcsicmp(g_szViewerCommands[nValue], szParam) == 0)
				return OpenPluginFromViewerMenu(nValue);
		}

		if (_wcsicmp(szParam, L"Menu") == 0) {
			g_bFromCmdLine = false;
			return OpenPluginFromViewerMenu(0);
		}
		if (_wcsicmp(szParam, L"Batch") == 0) {
//			g_pViewerBatches->ShowMenu();
		}

	}

	return NO_PANEL_HANDLE;
}

HANDLE OpenFromStringMacro(int nType, LPCWSTR szParam1, LPCWSTR szParam2)
{
	if (nType == 0) {
		if (_wcsicmp(szParam1, L"Preset") == 0) {
			return RunFilePreset(szParam2);
		}
		if (_wcsicmp(szParam1, L"Batch") == 0) {
			return RunBatch(g_pPanelBatches, szParam2);
		}

	} else if (nType == 1) {
		if (_wcsicmp(szParam1, L"Preset") == 0) {
			return RunEditorPreset(szParam2);
		}
		if (_wcsicmp(szParam1, L"Batch") == 0) {
			return RunBatch(g_pEditorBatches, szParam2);
		}

	} else if (nType == 2) {
		if (_wcsicmp(szParam1, L"Preset") == 0) {
			return RunViewerPreset(szParam2);
		}

	}

	return NO_PANEL_HANDLE;
}

int GetAreaType()
{
	int nArea = StartupInfo.MacroControl(MCTL_GETAREA, 0, NULL);

	switch (nArea) {
	case MACROAREA_SHELL:
		return 0;
	case MACROAREA_EDITOR:
		return 1;
	case MACROAREA_VIEWER:
		return 2;
	}

	return -1;
}

bool ApplyMacroParameters(CParameterSet &Set, CPresetCollection *pColl, const OpenMacroInfo *MInfo)
{
	for (size_t nItem = 1; nItem < MInfo->Count-1; nItem += 2)
	{
		string strName;
		if (!GetStringValue(MInfo->Values[nItem], strName))
			return false;

		if (strName == "Preset")
		{
			if (pColl == NULL)
				return false;

			wstring strValue;
			if (!GetStringValue(MInfo->Values[nItem+1], strValue))
				return false;

			CPreset *pPreset = pColl->FindMenuPreset(strValue.c_str());
			if (pPreset == NULL) {
				ShowError(GetMsg(MPresetNotFound), strValue.c_str());
				return false;
			}

			pPreset->Apply();

			continue;
		}

		map<string, tstring *>::iterator its = Set.m_mapStrings.find(strName);
		if (its != Set.m_mapStrings.end())
		{
			wstring strValue;
			if (!GetStringValue(MInfo->Values[nItem+1], strValue))
				return false;
			*its->second = strValue;
			continue;
		}

		map<string, int *>::iterator iti = Set.m_mapInts.find(strName);
		if (iti != Set.m_mapInts.end())
		{
			int nValue;
			if (!GetIntValue(MInfo->Values[nItem+1], nValue))
				return false;
			*iti->second = nValue;
			continue;
		}

		map<string, bool *>::iterator itb = Set.m_mapBools.find(strName);
		if (itb != Set.m_mapBools.end())
		{
			int nValue;
			if (!GetIntValue(MInfo->Values[nItem+1], nValue))
				return false;
			*itb->second = nValue != 0;
			continue;
		}

		return false;
	}

	return true;
}

HANDLE OpenPluginFromFileParameters(const OpenMacroInfo *MInfo)
{
	if (MInfo->Values[0].Type != FMVT_STRING)
		return NO_PANEL_HANDLE;

	CParameterSet *pSet = NULL;
	CPresetCollection *pColl = NULL;

	if (_wcsicmp(MInfo->Values[0].String, L"Search") == 0) {
		pSet  = &g_FSParamSet;
		pColl =    FSPresets;
	} else if (_wcsicmp(MInfo->Values[0].String, L"Replace") == 0) {
		pSet  = &g_FRParamSet;
		pColl =    FRPresets;
	} else if (_wcsicmp(MInfo->Values[0].String, L"Grep") == 0) {
		pSet  = &g_FGParamSet;
		pColl =    FGPresets;
	} else if (_wcsicmp(MInfo->Values[0].String, L"Rename") == 0) {
		pSet  = &g_RnParamSet;
		pColl =    RnPresets;
	} else if (_wcsicmp(MInfo->Values[0].String, L"RenameSelected") == 0) {
		pSet  = &g_QRParamSet;
		pColl =    QRPresets;
	}

	if (pSet == NULL)
		return NO_PANEL_HANDLE;

	CParameterBackup _Backup(*pSet);

	if (!ApplyMacroParameters(*pSet, pColl, MInfo))
		return NO_PANEL_HANDLE;

	return HandleFromOpResult(pSet->m_Executor());
}

HANDLE OpenPluginFromEditorParameters(const OpenMacroInfo *MInfo)
{
	if (MInfo->Values[0].Type != FMVT_STRING)
		return NO_PANEL_HANDLE;

	CParameterSet *pSet = NULL;
	CPresetCollection *pColl = NULL;

	if (_wcsicmp(MInfo->Values[0].String, L"Search") == 0) {
		pSet  = &g_ESParamSet;
		pColl =    ESPresets;
	} else if (_wcsicmp(MInfo->Values[0].String, L"Replace") == 0) {
		pSet  = &g_ERParamSet;
		pColl =    ERPresets;
	} else if (_wcsicmp(MInfo->Values[0].String, L"Filter") == 0) {
		pSet  = &g_EFParamSet;
		pColl =    EFPresets;
	} else if (_wcsicmp(MInfo->Values[0].String, L"Transliterate") == 0) {
		pSet  = &g_ETParamSet;
		pColl =    ETPresets;
	}

	if (pSet == NULL)
		return NO_PANEL_HANDLE;

	//	Some sensible defaults
	EFromCurrentPosition = true;
	EListAllFromPreset   = false;
	ECountAllFromPreset  = false;

	CParameterBackup _Backup(*pSet);

	if (!ApplyMacroParameters(*pSet, pColl, MInfo))
		return NO_PANEL_HANDLE;

	return HandleFromOpResult(pSet->m_Executor());
}

HANDLE OpenPluginFromViewerParameters(const OpenMacroInfo *MInfo)
{
	if (MInfo->Values[0].Type != FMVT_STRING)
		return NO_PANEL_HANDLE;

	CParameterSet *pSet = NULL;
	CPresetCollection *pColl = NULL;

	if (_wcsicmp(MInfo->Values[0].String, L"Search") == 0) {
		pSet  = &g_VSParamSet;
		pColl =    VSPresets;
	}

	if (pSet == NULL)
		return NO_PANEL_HANDLE;

	CParameterBackup _Backup(*pSet);

	if (!ApplyMacroParameters(*pSet, pColl, MInfo))
		return NO_PANEL_HANDLE;

	return HandleFromOpResult(pSet->m_Executor());
}

HANDLE OpenFromScriptMacro(const OpenMacroInfo *MInfo);

HANDLE OpenFromMacro(const OpenMacroInfo *MInfo, bool &bRawReturn)
{
	int nType = GetAreaType();

	int nValue;
	if (MInfo->Count == 0)
	{
		return OpenFromStringMacro(nType, L"Menu");
	}

	if ((MInfo->Values[0].Type == FMVT_STRING) && (_wcsicmp(MInfo->Values[0].String, L"Script") == 0))
	{
		bRawReturn = true;
		return OpenFromScriptMacro(MInfo);
	}

	if (MInfo->Count == 1)
	{
		if (nType < 0) return NO_PANEL_HANDLE;

		if (MInfo->Values[0].Type == FMVT_STRING) {
			return OpenFromStringMacro(nType, MInfo->Values[0].String);
		}

		if (!GetIntValue(MInfo->Values[0], nValue)) return NO_PANEL_HANDLE;
	}
	else if (MInfo->Count == 2)
	{
		if ((MInfo->Values[0].Type == FMVT_STRING) && (MInfo->Values[1].Type == FMVT_STRING)) {
			if (nType < 0) return NO_PANEL_HANDLE;
			return OpenFromStringMacro(nType, MInfo->Values[0].String, MInfo->Values[1].String);
		}

		if (!GetIntValue(MInfo->Values[0], nType )) return NO_PANEL_HANDLE;
		if (!GetIntValue(MInfo->Values[1], nValue)) return NO_PANEL_HANDLE;
	}
	else
	{
		switch (nType) {
		case 0:
			return OpenPluginFromFileParameters(MInfo);
		case 1:
			return OpenPluginFromEditorParameters(MInfo);
		case 2:
			return OpenPluginFromViewerParameters(MInfo);
		}
		return NO_PANEL_HANDLE;
	}

	switch (nType) {
	case 0:
		return OpenPluginFromFileMenu  (nValue);
	case 1:
		return OpenPluginFromEditorMenu(nValue);
	case 2:
		return OpenPluginFromViewerMenu(nValue);
	default:
		return NO_PANEL_HANDLE;
	}
}

void FillMacroValueStrings(const OpenMacroInfo *MInfo, vector<_variant_t> &arrStrings)
{
	if (MInfo->Count <= 1) return;
	if (g_spREParam == NULL) return;

	wstring strFunc = GetStringValue(MInfo->Values[1]);
	LPOLESTR bstrName = (LPOLESTR)strFunc.c_str();
	DISPID dispID;
	if (g_spREParam->GetIDsOfNames(IID_NULL, &bstrName, 1, LOCALE_SYSTEM_DEFAULT, &dispID) != S_OK)
		return;

	vector<_variant_t> arrParams;
	for (size_t nParam = 2; nParam < MInfo->Count; nParam++)
	{
		arrParams.insert(arrParams.begin(), GetVariantValue((MInfo->Values[nParam])));
	}

	DISPPARAMS dispParams = { !arrParams.empty() ? &arrParams[0] : NULL, NULL, arrParams.size(), 0 };
	_variant_t vtResult;
	if (g_spREParam->Invoke(dispID, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispParams, &vtResult, NULL, NULL) != S_OK)
		return;

	arrStrings.push_back(vtResult);
}

HANDLE OpenFromScriptMacro(const OpenMacroInfo *MInfo)
{
	static FarMacroCall Call = { sizeof(FarMacroCall), 0, NULL, NULL, NULL };
	static vector<FarMacroValue> arrValues;
	static vector<_variant_t> arrVariants;
	static vector<wstring> arrStrings;

	arrVariants.clear();
	FillMacroValueStrings(MInfo, arrVariants);

	arrValues .resize(arrVariants.size());
	arrStrings.resize(arrVariants.size());
	if (arrStrings.empty())
		return NULL;

	for (size_t nParam = 0; nParam < arrVariants.size(); nParam++)
	{
		_variant_t &vt = arrVariants[nParam];

		switch (vt.vt)
		{
		case VT_BSTR:
			arrStrings[nParam] = vt.bstrVal;
			arrValues[nParam].Type   = FMVT_STRING;
			arrValues[nParam].String = arrStrings[nParam].c_str();
			break;
		case VT_BOOL:
			arrValues[nParam].Type    = FMVT_BOOLEAN;
			arrValues[nParam].Boolean = vt.boolVal ? 1 : 0;
			break;
		case VT_R4:
		case VT_R8:
			arrValues[nParam].Type    = FMVT_DOUBLE;
			arrValues[nParam].Double  = (double)vt;
			break;
		case VT_I1:		case VT_I2:		case VT_I4:		case VT_I8:		case VT_INT:
		case VT_UI1:	case VT_UI2:	case VT_UI4:	case VT_UI8:	case VT_UINT:
			arrValues[nParam].Type    = FMVT_INTEGER;
			arrValues[nParam].Integer = (__int64)vt;
			break;
		case VT_EMPTY:
			arrValues[nParam].Type    = FMVT_NIL;
			break;
		default:
			arrStrings[nParam] = (LPCWSTR)_bstr_t(vt);
			arrValues[nParam].Type   = FMVT_STRING;
			arrValues[nParam].String = arrStrings[nParam].c_str();
			break;
		}
	}

	Call.Count  = arrValues.size();
	Call.Values = &arrValues[0];

	return &Call;
}

wstring LoadStartupScript()
{
	HRSRC hRsrc = FindResource(g_hInstance, MAKEINTRESOURCE(IDR_INIT_SCRIPT), L"SCRIPT");
	if (hRsrc == NULL) return L"";

	HGLOBAL hResource = LoadResource(g_hInstance, hRsrc);
	if (hResource == NULL) return L"";

	wstring strInitScript = wstring((LPCWSTR)LockResource(hResource), SizeofResource(g_hInstance, hRsrc)/2);

	UnlockResource(hResource);

	return strInitScript;
}

extern LPOLESTR g_szFarLUA;

bool CompileLUAString(const tstring &strReplace, LPCTSTR szEngine)
{
	g_bFinalChecked = false;

	if ((szEngine == NULL) || (wcscmp(g_szFarLUA, szEngine) != 0))
		return true;

	_bstr_t bstrReplace = ExpandScriptText(strReplace.c_str());

	MacroSendMacroText SendText;
	SendText.StructSize = sizeof(MacroSendMacroText);
	SendText.Flags = KMFLAGS_LUA;
	SendText.SequenceText = bstrReplace;
	if (!StartupInfo.MacroControl(MCTL_SENDSTRING, MSSC_CHECK, &SendText))
	{
		g_bInterrupted = true;
		return false;
	}

	static wstring strInitScript = LoadStartupScript();
	if (strInitScript.empty()) return false;

	wstring strInitReplace = strInitScript + L" " + (LPCWSTR)bstrReplace;
	SendText.SequenceText = strInitReplace.c_str();
	if (!StartupInfo.MacroControl(MCTL_SENDSTRING, MSSC_CHECK, &SendText))
	{
		g_bInterrupted = true;
		return false;
	}

	wstring strStartup = L"_G.RESearch_f = setfenv(loadstring(...), setmetatable({}, { __index=_G }))";

	FarMacroValue StartupParam(strInitReplace.c_str());

	MacroExecuteString StartupMacro = {sizeof(MacroExecuteString), KMFLAGS_LUA, strStartup.c_str(), 1, &StartupParam, 0, NULL};
	if (!StartupInfo.MacroControl(MCTL_EXECSTRING, 0, &StartupMacro))
	{
		g_bInterrupted = true;
		return false;
	}

	return true;
}

wstring EvaluateLUAString(CREParameters<wchar_t> &Param, LPCWSTR szReplace, FARKEYMACROFLAGS Flags)
{
	vector<FarMacroValue> arrValues (Param.ParamCount());
	vector<wstring>       arrStrings(Param.ParamCount());
	for (int nParam = 0; nParam < Param.ParamCount(); nParam++)
	{
		arrStrings[nParam] = Param.GetParam(nParam);
		arrValues[nParam].Type   = FMVT_STRING;
		arrValues[nParam].String = arrStrings[nParam].c_str();
	}

	MacroExecuteString Macro = {sizeof(MacroExecuteString), Flags, L"return RESearch_f(...)", arrValues.size(), !arrValues.empty() ? &arrValues[0] : NULL, 0, NULL};
	if (!StartupInfo.MacroControl(MCTL_EXECSTRING, 0, &Macro))
	{
		g_bInterrupted = true;
		return L"";
	}

	wstring strResult;

	IReplaceParametersInternalPtr spREInt = g_spREParam;

	if (spREInt->HasResult())
	{
		strResult = spREInt->Result();
	}
	else
	{
		for (size_t nItem = 0; nItem < Macro.OutCount; nItem++)
			strResult += GetStringValue(Macro.OutValues[nItem]);

		if (Macro.OutCount == 0)
			g_bSkipReplace = true;
	}

	return strResult;
}

bool EvaluateLUAStringCleanup()
{
	wstring strCleanup = L"_G.RESearch_f = nil";

	MacroExecuteString CleanupMacro = {sizeof(MacroExecuteString), KMFLAGS_LUA, strCleanup.c_str(), 0, NULL, 0, NULL};
	if (!StartupInfo.MacroControl(MCTL_EXECSTRING, 0, &CleanupMacro))
	{
		g_bInterrupted = true;
		return false;
	}

	return true;
}

#else

bool CompileLUAString(const tstring &strReplace, LPCTSTR szEngine)
{
	g_bFinalChecked = false;

	return true;
}

#endif
