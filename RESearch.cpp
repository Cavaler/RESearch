#include "StdAfx.h"
#include "RESearch.h"
#include <initguid.h>
DEFINE_GUID(GUID_RESearch,			0xf250c12a, 0x78e2, 0x4abc, 0xa7, 0x84, 0x3f, 0xdd, 0x31, 0x56, 0xe4, 0x15);
DEFINE_GUID(GUID_RESearchConfig,	0x25a28ca1, 0x23e7, 0x4019, 0xad, 0x5e, 0xd3, 0xf3, 0x1b, 0x7c, 0x2c, 0xfe);
DEFINE_GUID(GUID_RESearchRun,		0x9340f8e5, 0xf4a1, 0x4be2, 0xa1, 0x46, 0x0a, 0x49, 0x68, 0xa3, 0xd1, 0x6c);
#include "version.h"

CComModule _Module;

BOOL __stdcall DllMain(HINSTANCE hInst, ULONG reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		g_hInstance = hInst;
		_Module.Init(NULL, hInst, &__uuidof(__RESearchLib));
	}
	else
	if (reason == DLL_PROCESS_DETACH)
	{
		_Module.Term();
	}

	return TRUE;
}

void WINAPI FAR_EXPORT(GetPluginInfo)(PluginInfo *Info)
{
	static const TCHAR *ConfigStrings[1];
	static const TCHAR *MenuStrings[1];
	ConfigStrings[0] = GetMsg(MRESearch);
	MenuStrings[0]   = GetMsg(MRESearch);

	Info->StructSize=sizeof(PluginInfo);
	Info->Flags=PF_EDITOR|PF_VIEWER|PF_FULLCMDLINE;

#ifdef FAR3
	static const GUID   ConfigGuids[1] = { GUID_RESearchConfig };
	static const GUID   MenuGuids[1]   = { GUID_RESearchRun };

	Info->PluginConfig.Strings = ConfigStrings;
	Info->PluginConfig.Guids   = ConfigGuids;
	Info->PluginConfig.Count   = 1;
	Info->PluginMenu.Strings   = MenuStrings;
	Info->PluginMenu.Guids     = MenuGuids;
	Info->PluginMenu.Count     = 1;
	Info->DiskMenu.Count       = 0;
#else
	Info->DiskMenuStrings=NULL;
#ifndef UNICODE
	Info->DiskMenuNumbers=NULL;
#endif
	Info->DiskMenuStringsNumber=0;
	Info->PluginMenuStrings=MenuStrings;
	Info->PluginMenuStringsNumber=1;
	Info->PluginConfigStrings=ConfigStrings;
	Info->PluginConfigStringsNumber=1;
#endif

	Info->CommandPrefix=_T("ff:fr:rn:qr");
}

void WINAPI FAR_EXPORT(SetStartupInfo)(const PluginStartupInfo *Info)
{
	StartupInfo=*Info;
#ifdef FAR3
	StartupInfo.m_GUID = GUID_RESearch;
#endif

	PrepareLocaleStuff();
	ReadRegistry();

	g_pszOKButton = GetMsg(MOk);
	g_pszErrorTitle = GetMsg(MError);
	CFarIntegerRangeValidator::s_szErrorMsg = GetMsg(MInvalidNumber);
	CFarIntegerRangeValidator::s_szHelpTopic = _T("REInvalidNumber");
	CFarDialog::AutoHotkeys = true;
	CFarDialog::SetDefaultCancelID(MCancel);

	CoInitialize(NULL);
	EnumActiveScripts();
}

int ShowFileMenu(int &nBreakCode)
{
	vector<CFarMenuItemEx> MenuItems;

	MenuItems.push_back(CFarMenuItemEx(MMenuSearch));
	MenuItems.push_back(CFarMenuItemEx(MMenuReplace));
	MenuItems.push_back(CFarMenuItemEx(MMenuGrep));
	MenuItems.push_back(CFarMenuItemEx(true));
	MenuItems.push_back(CFarMenuItemEx(MMenuSelect));
	MenuItems.push_back(CFarMenuItemEx(MMenuUnselect));
	MenuItems.push_back(CFarMenuItemEx(MMenuFlipSelection));
	MenuItems.push_back(CFarMenuItemEx(true));
	MenuItems.push_back(CFarMenuItemEx(MMenuRename));
	MenuItems.push_back(CFarMenuItemEx(MMenuRenameSelected));
	MenuItems.push_back(CFarMenuItemEx(MMenuRenumber));
	MenuItems.push_back(CFarMenuItemEx(MMenuUndoRename));
	MenuItems.push_back(CFarMenuItemEx(true));
	MenuItems.push_back(CFarMenuItemEx(MMenuShowLastResults));
	MenuItems.push_back(CFarMenuItemEx(MMenuClearVariables));
	MenuItems.push_back(CFarMenuItemEx(true));

	FSPresets->FillMenuItems(MenuItems);
	FRPresets->FillMenuItems(MenuItems);
	FGPresets->FillMenuItems(MenuItems);
	RnPresets->FillMenuItems(MenuItems);
	QRPresets->FillMenuItems(MenuItems);

	MenuItems.push_back(CFarMenuItemEx(true));
	MenuItems.push_back(CFarMenuItemEx(MMenuBatches));

	g_pPanelBatches->FillMenuItems(MenuItems);

	static int nLastSelection = 0;
	if (nLastSelection >= (int)MenuItems.size()) nLastSelection = 0;
	MenuItems[nLastSelection].Flags |= MIF_SELECTED;

	if (m_arrLastRename.empty()) MenuItems[11].Flags |= MIF_DISABLE;
	if (LastTempPanel == NULL) MenuItems[13].Flags |= MIF_DISABLE;

	int nBreakKeys[] = {VK_F4, 0};

	int nResult = StartupInfo.Menu(-1, -1, 0, FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT|FMENU_USEEXT,
		GetMsg(MMenuHeader), NULL, _T("FileMenu"), nBreakKeys, &nBreakCode, 
		(const FarMenuItem *)&MenuItems[0], MenuItems.size());

	if (nResult >= 0) nLastSelection = nResult;

	return nResult;
}

int ShowEditorMenu(int &nBreakCode)
{
	vector<CFarMenuItemEx> MenuItems;

	MenuItems.push_back(CFarMenuItemEx(MMenuSearch));
	MenuItems.push_back(CFarMenuItemEx(MMenuReplace));
	MenuItems.push_back(CFarMenuItemEx(MMenuFilterText));
	MenuItems.push_back(CFarMenuItemEx(MMenuTransliterate));
	MenuItems.push_back(CFarMenuItemEx(true));
	MenuItems.push_back(CFarMenuItemEx(MMenuSearchReplaceAgain));
	MenuItems.push_back(CFarMenuItemEx(MMenuSearchReplaceAgainRev));
	MenuItems.push_back(CFarMenuItemEx(true));
	MenuItems.push_back(CFarMenuItemEx(MMenuShowLastResults));
	MenuItems.push_back(CFarMenuItemEx(MMenuClearVariables));

	MenuItems.push_back(CFarMenuItemEx(true));
	ESPresets->FillMenuItems(MenuItems);
	ERPresets->FillMenuItems(MenuItems);
	EFPresets->FillMenuItems(MenuItems);
	ETPresets->FillMenuItems(MenuItems);

	MenuItems.push_back(CFarMenuItemEx(true));
	MenuItems.push_back(CFarMenuItemEx(MMenuBatches));

	g_pEditorBatches->FillMenuItems(MenuItems);

	static int nLastSelection = 0;
	if (nLastSelection >= (int)MenuItems.size()) nLastSelection = 0;
	MenuItems[nLastSelection].Flags |= MIF_SELECTED;

	if (!EditorListAllHasResults()) MenuItems[8].Flags |= MIF_DISABLE;

	int nBreakKeys[] = {VK_F4, 0};

	int nResult = StartupInfo.Menu(-1, -1, 0, FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT|FMENU_USEEXT,
		GetMsg(MMenuHeader), NULL, _T("EditorMenu"), nBreakKeys, &nBreakCode,
		(const FarMenuItem *)&MenuItems[0], MenuItems.size());

	if (nResult >= 0) nLastSelection = nResult;

	return nResult;
}

int ShowViewerMenu(int &nBreakCode)
{
	vector<CFarMenuItemEx> MenuItems;

	MenuItems.push_back(CFarMenuItemEx(MMenuSearch));
	MenuItems.push_back(CFarMenuItemEx(MMenuSearchAgain));
//	MenuItems.push_back(CFarMenuItemEx(MMenuSearchAgainRev));
	MenuItems.push_back(CFarMenuItemEx(MMenuClearVariables));

	MenuItems.push_back(CFarMenuItemEx(true));
	VSPresets->FillMenuItems(MenuItems);

	static int nLastSelection = 0;
	if (nLastSelection >= (int)MenuItems.size()) nLastSelection = 0;
	MenuItems[nLastSelection].Flags |= MIF_SELECTED;

	int nBreakKeys[] = {VK_F4, 0};

	int nResult = StartupInfo.Menu(-1,-1,0,FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT|FMENU_USEEXT,
		GetMsg(MMenuHeader), NULL, _T("ViewerMenu"), nBreakKeys, &nBreakCode, 
		(const FarMenuItem *)&MenuItems[0], MenuItems.size());

	if (nResult >= 0) nLastSelection = nResult;

	return nResult;
}

bool FindRunPreset(CPresetCollection *pColl, int &nItem, int nBreakCode, OperationResult &Result)
{
	CPreset *pPreset = pColl->FindMenuPreset(nItem);
	if (pPreset == NULL) return false;

	switch (nBreakCode) {
	case -1:
		Result = pPreset->ExecutePreset();
		return true;
	case 0:
		if (pColl->EditPreset(pPreset)) pColl->Save();
		Result = OR_OK;
		break;
	}

	return true;
}

bool FindRunPreset(CPresetCollection *pColl, LPCTSTR szName, OperationResult &Result)
{
	CPreset *pPreset = pColl->FindMenuPreset(szName);
	if (pPreset == NULL) {
		ShowError(GetMsg(MPresetNotFound), szName);
		return false;
	}

	Result = pPreset->ExecutePreset();
	return true;
}

OperationResult OpenPluginFromFilePreset(int nItem, int nBreakCode)
{
	OperationResult Result = OR_CANCEL;

	if (FindRunPreset(FSPresets, nItem, nBreakCode, Result) ||
		FindRunPreset(FRPresets, nItem, nBreakCode, Result) ||
		FindRunPreset(FGPresets, nItem, nBreakCode, Result) ||
		FindRunPreset(RnPresets, nItem, nBreakCode, Result) ||
		FindRunPreset(QRPresets, nItem, nBreakCode, Result)
		) return Result;

	nItem--;
	if (nItem == 0) {
		g_pPanelBatches->ShowMenu();
		return OR_OK;
	} else nItem--;

	CBatchAction *pAction;
	if (pAction = g_pPanelBatches->FindMenuAction(nItem)) {
		switch (nBreakCode) {
		case -1:
			pAction->Execute();
			return OR_OK;
		case 0:
			if (pAction->EditItems())
				WriteRegistry();
			return OR_OK;
		}
	}

	return OR_CANCEL;
}

HANDLE HandleFromOpResult(OperationResult Result)
{
	if (Result==OR_PANEL) {
#ifdef UNICODE
		wchar_t szCurDir[MAX_PATH];
		StartupInfo.Control(PANEL_ACTIVE, FCTL_GETCURRENTDIRECTORY, MAX_PATH, (LONG_PTR)szCurDir);
		CTemporaryPanel *Panel=new CTemporaryPanel(g_PanelItems,szCurDir);
#else
		PanelInfo PInfo;
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
		CTemporaryPanel *Panel=new CTemporaryPanel(g_PanelItems,PInfo.CurDir);
#endif
		g_PanelItems.clear();
		return (HANDLE)Panel;
	} else {
#ifdef UNICODE
		StartupInfo.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, true, NULL);
		StartupInfo.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, NULL);
#else
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_UPDATEPANEL,(void *)~NULL);
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_REDRAWPANEL,NULL);
#endif
		return NO_PANEL_HANDLE;
	}
}

HANDLE OpenPluginFromFileMenu(int Item, bool ShowDialog)
{
	SetCurrentDirectory(CPanelInfo(false).CurDir);

	OperationResult Result = OR_CANCEL;
	int nBreakCode = -1;

	do {
		if (!g_bFromCmdLine) {
			do {
				Item = ShowFileMenu(nBreakCode);
				if (Item == -1) return NO_PANEL_HANDLE;
			} while ((nBreakCode >= 0) && (Item < 15));
		}

		switch (Item) {
		case 0:
			Result = FileFind(g_PanelItems,ShowDialog);
			if (Result != OR_CANCEL) SynchronizeWithFile(false);
			break;
		case 1:
			Result = FileReplace(g_PanelItems,ShowDialog);
			if (Result != OR_CANCEL) SynchronizeWithFile(true);
			break;
		case 2:
			Result = FileGrep(ShowDialog);
			if (Result != OR_CANCEL) SynchronizeWithFile(false);
			break;
		case 4:ChangeSelection(MSelect);break;
		case 5:ChangeSelection(MUnselect);break;
		case 6:ChangeSelection(MFlipSelection);break;
		case 8:
			Result=RenameFiles(g_PanelItems,ShowDialog);
			break;
		case 9:
			Result=RenameSelectedFiles(g_PanelItems,ShowDialog);
			break;
		case 10:
			Result=RenumberFiles();
			break;
		case 11:
			Result=UndoRenameFiles();
			break;
		case 13:
			if (LastTempPanel) {
				LastTempPanel->m_bActive = true;
				return (HANDLE)LastTempPanel;
			} else {
				Result=OR_CANCEL;
				break;
			}
		case 14:
			ClearVariables();
			break;
		}
		if (Item >= 16) {
			Item -= 16;
			Result = OpenPluginFromFilePreset(Item, nBreakCode);
		}

	} while (nBreakCode >= 0);

	return HandleFromOpResult(Result);
}

OperationResult OpenPluginFromEditorPreset(int nItem, int nBreakCode)
{
	OperationResult Result = OR_CANCEL;

	if (FindRunPreset(ESPresets, nItem, nBreakCode, Result) ||
		FindRunPreset(ERPresets, nItem, nBreakCode, Result) ||
		FindRunPreset(EFPresets, nItem, nBreakCode, Result) ||
		FindRunPreset(ETPresets, nItem, nBreakCode, Result)
		) return OR_OK;

	nItem--;
	if (nItem == 0) {
		g_pEditorBatches->ShowMenu();
		return OR_OK;
	} else nItem--;

	CBatchAction *pAction;
	if (pAction = g_pEditorBatches->FindMenuAction(nItem)) {
		switch (nBreakCode) {
		case -1:
			pAction->Execute();
			return OR_OK;
		case 0:
			if (pAction->EditItems())
				WriteRegistry();
			return OR_OK;
		}
	}

	return OR_CANCEL;
}

HANDLE OpenPluginFromEditorMenu(int nItem)
{
	FindIfClockPresent();

	int nBreakCode = -1;

	do {
		if (!g_bFromCmdLine) {
			do {
				nItem = ShowEditorMenu(nBreakCode);
				if (nItem == -1) return NO_PANEL_HANDLE;
			} while ((nBreakCode >= 0) && (nItem < 10));
		}

		switch (nItem) {
		case 0:
			if (EditorSearch()) LastAction=0;
			break;
		case 1:
			if (EditorReplace()) LastAction=1;
			break;
		case 2:
			if (EditorFilter()) LastAction=2;
			break;
		case 3:
			if (EditorTransliterate()) LastAction=3;
			break;
		case 6:
			EReverse = !EReverse;
			//	fall-through
		case 5:
			ESearchAgainCalled = true;
			EPreparePattern(EText);		// In case codepage changed etc

			switch (LastAction) {
			case -1:
				ESearchAgainCalled = false;
				if (EditorSearch()) LastAction=0;
				break;
			case 0:
				EditorSearchAgain();
				break;
			case 1:
				_EditorReplaceAgain();
				break;
			case 2:
				EditorFilterAgain();
				break;
			case 3:
				EditorTransliterateAgain();
				break;
			case 4:
				EditorListAllAgain();
				break;
			};
			if (nItem == 6) EReverse = !EReverse;
			break;
		case 8:
			EditorListAllShowResults(false);
			break;
		case 9:
			ClearVariables();
			break;
		}

		if (nItem >= 11) {
			nItem -= 11;
			OpenPluginFromEditorPreset(nItem, nBreakCode);
		}
	} while (nBreakCode >= 0);

	return NO_PANEL_HANDLE;
}

OperationResult OpenPluginFromViewerPreset(int nItem, int nBreakCode)
{
	OperationResult Result = OR_CANCEL;

	if (FindRunPreset(VSPresets, nItem, nBreakCode, Result)) return OR_OK;

	return OR_CANCEL;
}

HANDLE OpenPluginFromViewerMenu(int nItem)
{
	int nBreakCode = -1;

	do {
		if (!g_bFromCmdLine) {
			do {
				nItem = ShowViewerMenu(nBreakCode);
				if (nItem == -1) return NO_PANEL_HANDLE;
			} while ((nBreakCode >= 0) && (nItem < 3));
		}

		switch (nItem) {
		case 0:
			if (ViewerSearch()) LastAction = 0;
			break;
	//	case 2:
	//		EReverse = !EReverse;
		case 1:
			ESearchAgainCalled = true;
			switch (LastAction) {
			case -1:
				ESearchAgainCalled = false;
				if (ViewerSearch()) LastAction = 0;
				break;
			default:
				if (ViewerSearchAgain()) LastAction = 0;
				break;
			}
	//		if (nMenu == 2) EReverse = !EReverse;
			break;
		case 2:
			ClearVariables();
			break;
		}

		if (nItem >= 4) {
			nItem -= 4;
			OpenPluginFromViewerPreset(nItem, nBreakCode);
		}
	} while (nBreakCode >= 0);

	return NO_PANEL_HANDLE;
}

#ifdef FAR3
HANDLE WINAPI OpenW(const struct OpenInfo *Info)
{
	g_bFromCmdLine = false;
	g_bInterrupted = false;
	ESearchAgainCalled = false;

	switch (Info->OpenFrom) {
	case OPEN_PLUGINSMENU:
		return OpenPluginFromFileMenu(0);

	case OPEN_EDITOR:
		return OpenPluginFromEditorMenu(0);

	case OPEN_VIEWER:
		return OpenPluginFromViewerMenu(0);

	case OPEN_COMMANDLINE:{
		OpenCommandLineInfo *CmdInfo = (OpenCommandLineInfo *)Info->Data;
		bool ShowDialog = true;
		INT_PTR Item;
		if (ProcessCommandLine(CmdInfo->CommandLine, &ShowDialog, &Item)) {
			g_bFromCmdLine = true;
			return OpenPluginFromFileMenu(Item, ShowDialog);
		}
		break;
						  }
	case OPEN_FROMMACRO:{
		bool bRawReturn = false;

		bool bFromCmdLine = g_bFromCmdLine;
		g_bFromCmdLine = true;
		HANDLE hPlugin = OpenFromMacro((const OpenMacroInfo *)Info->Data, bRawReturn);
		g_bFromCmdLine = false;

		if (bRawReturn) return hPlugin;

		if (hPlugin != NO_PANEL_HANDLE)
		{
			static FarMacroValue Value;
			Value.Type = FMVT_PANEL;
			Value.Pointer = hPlugin;

			static FarMacroCall Call = { sizeof(FarMacroCall), 1, &Value, NULL, NULL };
			return &Call;
		}

		return NO_PANEL_HANDLE;
						}
	}

	return NO_PANEL_HANDLE;
}
#else
#ifdef UNICODE
HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
#else
HANDLE WINAPI OpenPlugin(int OpenFrom, INT_PTR Item)
#endif
{
	bool ShowDialog = true;
	g_bFromCmdLine = false;
	g_bInterrupted = false;

	ESearchAgainCalled = false;

	switch (OpenFrom) {
	case OPEN_COMMANDLINE:
		g_bFromCmdLine = true;
		if (!ProcessCommandLine((TCHAR *)Item,&ShowDialog,&Item)) return INVALID_HANDLE_VALUE;
		else // fall-through

	case OPEN_PLUGINSMENU:
		return OpenPluginFromFileMenu(Item, ShowDialog);

	case OPEN_EDITOR:
		return OpenPluginFromEditorMenu(Item);

	case OPEN_VIEWER:
		return OpenPluginFromViewerMenu(Item);

	case OPEN_SHORTCUT:
#ifdef UNICODE
		StartupInfo.Control(PANEL_ACTIVE, FCTL_SETPANELDIR, 0, (LONG_PTR)Item);
		StartupInfo.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
		StartupInfo.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, NULL);
#else
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_SETPANELDIR,(void *)Item);
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_UPDATEPANEL,NULL);
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_REDRAWPANEL,NULL);
#endif
		return NO_PANEL_HANDLE;

	default:
		return NO_PANEL_HANDLE;
	}
}
#endif

bool AtoI(char *String,int *Number,int Min,int Max) {
	int I;
	if ((sscanf(String,"%d",&I)==1)&&(I>=Min)&&(I<=Max)) {
		*Number=I;
		return true;
	} else return false;
}

int ConfigureSeveralLines()
{
	CFarDialog Dialog(64 ,9, _T("CommonConfig"));
	Dialog.AddFrame(MSeveralLineSettings);

	Dialog.Add(new CFarTextItem(5,3,0,MSeveralLinesIs));
	Dialog.Add(new CFarEditItem(32,3,38,0,NULL,(int &)SeveralLines,new CFarIntegerRangeValidator(1, 0x100000)));
	Dialog.Add(new CFarTextItem(40,3,0,MLinesOr));
	Dialog.Add(new CFarEditItem(50,3,56,0,NULL,(int &)SeveralLinesKB,new CFarIntegerRangeValidator(1, 128*1024)));
	Dialog.Add(new CFarTextItem(57,3,DIF_NOAUTOHOTKEY,MKB));

	Dialog.AddButtons(MOk,MCancel);
	return Dialog.Display(-1);
}

#ifdef UNICODE

vector<int> g_arrEnumCPs;

BOOL CALLBACK EnumCPProc(LPTSTR lpCodePageString) {
	UINT codePage = _ttoi(lpCodePageString);

	if ((codePage == GetOEMCP()) || (codePage == GetACP())) return TRUE;

	CPINFOEX cpiex;
	if (!GetCPInfoEx(codePage, 0, &cpiex)) return TRUE;
	if (cpiex.MaxCharSize != 1) return TRUE;

	g_arrEnumCPs.push_back(codePage);

	return TRUE;
}

void AddCP(int nCP, LPCTSTR szText, vector<int> &arrCPs, vector<CFarMenuItemEx> &arrCPItems)
{
	arrCPs.push_back(nCP);
	arrCPItems.push_back(FormatStrW(L"%5d | %s", nCP, szText));

	if (g_setAllCPs.find(nCP) != g_setAllCPs.end()) {
		arrCPItems[arrCPItems.size()-1].Check();
	}
}

int ConfigureCP()
{
	vector<int> arrCPs;
	vector<CFarMenuItemEx> arrCPItems;

	AddCP(GetOEMCP(),    L"OEM",   arrCPs, arrCPItems);
	AddCP(GetACP(),      L"ANSI",  arrCPs, arrCPItems);
	arrCPItems.push_back(true);	arrCPs.push_back(0);
	AddCP(CP_UTF7,       L"UTF-7", arrCPs, arrCPItems);
	AddCP(CP_UTF8,       L"UTF-8", arrCPs, arrCPItems);
	AddCP(CP_UNICODE,    L"UTF-16 (Little endian)", arrCPs, arrCPItems);
	AddCP(CP_REVERSEBOM, L"UTF-16 (Big endian)",    arrCPs, arrCPItems);
	arrCPItems.push_back(true);	arrCPs.push_back(0);

	g_arrEnumCPs.clear();
	EnumSystemCodePages(EnumCPProc, CP_INSTALLED);

	sort(g_arrEnumCPs.begin(), g_arrEnumCPs.end());
	for each (int nCP in g_arrEnumCPs) {
		CPINFOEX cpiex;
		GetCPInfoEx(nCP, 0, &cpiex);
		AddCP(nCP, cpiex.CodePageName, arrCPs, arrCPItems);
	}

	int nBreakKeys[] = {VK_INSERT, 0};
	int nBreakCode;

	cp_set setCPs = g_setAllCPs;

	int nItem = 0;
	do {
		arrCPItems[nItem].Select(true);
		int nResult = StartupInfo.Menu(-1, -1, 0, FMENU_WRAPMODE|FMENU_USEEXT, GetMsg(MAllCPMenu), L"Ins, Enter/Esc",
			L"Help", nBreakKeys, &nBreakCode, (FarMenuItem *)&arrCPItems[0], arrCPItems.size());
		arrCPItems[nItem].Select(false);
		nItem = nResult;

		if (nBreakCode == -1) {
			if (nResult >= 0) {
				g_setAllCPs = setCPs;
			}
			return true;
		}

		if (arrCPs[nItem] != 0) {
			if (arrCPItems[nItem].Checked()) {
				arrCPItems[nItem].Check(false);
				setCPs.erase(arrCPs[nItem]);
			} else {
				arrCPItems[nItem].Check(true);
				setCPs.insert(arrCPs[nItem]);
			}
		}
	} while (true);
}
#endif

void ConfigureCommon()
{
	CFarDialog Dialog(64, 21, _T("CommonConfig"));
	Dialog.SetUseID(true);
	Dialog.AddFrame(MCommonSettings);

	Dialog.Add(new CFarTextItem(5,3,0,MSeveralLinesIs));
	Dialog.Add(new CFarEditItem(32,3,38,0,NULL,(int &)SeveralLines,new CFarIntegerRangeValidator(1, 0x100000)));
	Dialog.Add(new CFarTextItem(40,3,0,MLinesOr));
	Dialog.Add(new CFarEditItem(50,3,56,0,NULL,(int &)SeveralLinesKB,new CFarIntegerRangeValidator(1, 128*1024)));
	Dialog.Add(new CFarTextItem(57,3,DIF_NOAUTOHOTKEY,MKB));

	Dialog.Add(new CFarCheckBoxItem(5,5,0,MDotMatchesNewline,&DotMatchesNewline));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MUseSeparateThread,&g_bUseSeparateThread));
	Dialog.Add(new CFarTextItem(9,8,0,MMaxInThreadLength));
	Dialog.Add(new CFarEditItem(34,8,40,0,NULL,(int &)g_nMaxInThreadLength,new CFarIntegerRangeValidator(0,0x7FFFFFFF)));
	Dialog.Add(new CFarTextItem(9,9,0,MThreadStackMB));
	Dialog.Add(new CFarEditItem(34,9,40,0,NULL,(int &)g_nThreadStackMB,new CFarIntegerRangeValidator(0,1024)));
	Dialog.Add(new CFarTextItem(44,9,DIF_NOAUTOHOTKEY,MKB));

	Dialog.Add(new CFarCheckBoxItem(5,11,0,MShowUsageWarnings,&g_bShowUsageWarnings));
	Dialog.Add(new CFarCheckBoxItem(5,12,0,MUseEscapesInPlainText,&g_bEscapesInPlainText));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MIgnoreIdentReplace,&g_bIgnoreIdentReplace));

	Dialog.AddButtons(MOk, MCancel);

	Dialog.Display();
}

void ConfigureFile()
{
#ifdef UNICODE
	CFarDialog Dialog(70, 25, _T("FileConfig"));
#else
	CFarDialog Dialog(70, 21, _T("FileConfig"));
#endif
	Dialog.SetUseID(true);
	Dialog.AddFrame(MFileSearchSettings);

	Dialog.Add(new CFarBoxItem(false,5,3,33,7,DIF_LEFTTEXT,MDefaultMaskCase));
	Dialog.Add(new CFarRadioButtonItem(7,4,DIF_GROUP,MMaskSensitive,(int *)&FMaskCase,MC_SENSITIVE));
	Dialog.Add(new CFarRadioButtonItem(7,5,0,MMaskInsensitive,(int *)&FMaskCase,MC_INSENSITIVE));
	Dialog.Add(new CFarRadioButtonItem(7,6,0,MMaskVolumeDependent,(int *)&FMaskCase,MC_VOLUME));

	Dialog.Add(new CFarBoxItem(false,35,3,64,7,DIF_LEFTTEXT,MReplaceReadonly));
	Dialog.Add(new CFarRadioButtonItem(37,4,DIF_GROUP,MNever,(int *)&FRReplaceReadonly,RR_NEVER));
	Dialog.Add(new CFarRadioButtonItem(37,5,0,MAsk,(int *)&FRReplaceReadonly,RR_ASK));
	Dialog.Add(new CFarRadioButtonItem(37,6,0,MAlways,(int *)&FRReplaceReadonly,RR_ALWAYS));

	Dialog.Add(new CFarCheckBoxItem(5, 9,  0, MSkipSystemFolders, &FASkipSystemFolders));
	Dialog.Add(new CFarEditItem    (9, 10, 60, DIF_HISTORY,_T("RESearch.SystemFolders"), FASystemFolders));
	Dialog.Add(new CFarCheckBoxItem(5, 11, 0, MUseShortFilenames, &FUseShortFilenames));
	Dialog.Add(new CFarCheckBoxItem(5, 13, 0, MUseSingleCR, &FSUseSingleCR));
	Dialog.Add(new CFarCheckBoxItem(5, 14, 0, MEditSrchAfterFile, &FSEditSrchAfterFile));

	Dialog.Add(new CFarTextItem(5,  16, 0, MBufferSize));
	Dialog.Add(new CFarEditItem(34, 16, 40, 0, NULL, FBufferSize, new CFarIntegerRangeValidator(1, 1024)));
	Dialog.Add(new CFarTextItem(43, 16, DIF_NOAUTOHOTKEY, MMB));

#ifdef UNICODE
	Dialog.Add(new CFarTextItem  (5,  18, 0, MDefaultCP));
	Dialog.Add(new CFarRadioButtonItem(35, 18, 0, MDefaultOEM,  &g_bDefaultOEM, true ));
	Dialog.Add(new CFarRadioButtonItem(45, 18, 0, MDefaultANSI, &g_bDefaultOEM, false));
	Dialog.Add(new CFarTextItem  (5,  19, 0, MAllCPInclude));
	Dialog.Add(new CFarButtonItem(35, 19, 0, false, MAllCPSelect));

	Dialog.AddButtons(MOk, MCancel);
	do {
		int nResult = Dialog.Display();

		switch (nResult) {
		case MAllCPSelect:
			ConfigureCP();
			break;
		default:
			return;
		}
	} while (true);

#else
	Dialog.AddButtons(MOk, MCancel);
	Dialog.Display();
#endif
}

void ConfigureGrep()
{
	CFarDialog Dialog(62, 10, _T("GrepConfig"));
	Dialog.EnableAutoHotkeys(true);

	Dialog.AddFrame(MGrepSettings);

	Dialog.Add(new CFarTextItem(5,3,0,MGrepFileNamePrepend));
	Dialog.Add(new CFarEditItem(30,3,55,DIF_HISTORY,_T("FGPrepend"), FGFileNamePrepend));
	Dialog.Add(new CFarTextItem(5,4,0,MGrepFileNameAppend));
	Dialog.Add(new CFarEditItem(30,4,55,DIF_HISTORY,_T("FGAppend"), FGFileNameAppend));

	Dialog.AddButtons(MOk,MCancel);

	Dialog.Display(-1);
}

void ConfigureRenumbering(bool bRuntime)
{
	CFarDialog Dialog(70, bRuntime ? 18 : 16, _T("RenumberConfig"));
	Dialog.SetUseID(true);
	Dialog.AddFrame(MRenumberSettings);

	Dialog.Add(new CFarCheckBoxItem(5,3,0,MStripFromBeginning, g_bStripRegExp));
	Dialog.Add(new CFarEditItem(42,3,61,DIF_HISTORY,_T("RESearch.Strip"), g_strStrip));
	Dialog.Add(new CFarCheckBoxItem(5,4,0,MStripCommonPart, g_bStripCommon));
	Dialog.Add(new CFarTextItem(5,6,0,MPrefix));
	Dialog.Add(new CFarEditItem(30,6,42,DIF_HISTORY,_T("RESearch.Prefix"), g_strPrefix));
	Dialog.Add(new CFarTextItem(5,7,0,MPostfix));
	Dialog.Add(new CFarEditItem(30,7,42,DIF_HISTORY,_T("RESearch.Postfix"), g_strPostfix));
	Dialog.Add(new CFarTextItem(5,9,0,MStartFrom));
	Dialog.Add(new CFarEditItem(30,9,38,0, NULL, (int &)g_nStartWith,new CFarIntegerRangeValidator(0,0x7FFFFFFF)));
	Dialog.Add(new CFarTextItem(5,10,0,MWidth));
	Dialog.Add(new CFarEditItem(30,10,38,0, NULL, (int &)g_nWidth,new CFarIntegerRangeValidator(0,MAX_PATH)));

	if (bRuntime) {
		Dialog.Add(new CFarCheckBoxItem(5,12,0,MLeaveSelection,&FRLeaveSelection));
	}

	Dialog.AddButtons(MOk, MCancel);

	Dialog.Display();
}

void ConfigureEditor()
{
	CFarDialog Dialog(61, 31, _T("EditorConfig"));
	Dialog.SetUseID(true);
	Dialog.AddFrame(MEditorSearchSettings);

	Dialog.Add(new CFarTextItem(5,3,0,MShowPositionOffset));
	Dialog.Add(new CFarEditItem(41,3,45,0,NULL,(int &)EShowPositionOffset,new CFarIntegerRangeValidator(-1024,1024)));

	Dialog.Add(new CFarTextItem(5,4,0,Mfrom));
	Dialog.Add(new CFarRadioButtonItem(10,4,DIF_GROUP,MTop,(int *)&EShowPosition,SP_TOP));
	Dialog.Add(new CFarRadioButtonItem(10,5,0,MCenter,(int *)&EShowPosition,SP_CENTER));
	Dialog.Add(new CFarRadioButtonItem(10,6,0,MBottom,(int *)&EShowPosition,SP_BOTTOM));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MKeepLineIfVisible,&EKeepLineIfVisible));

	Dialog.Add(new CFarTextItem(5,9,0,MLRSideOffset));
	Dialog.Add(new CFarEditItem(41,9,45,0,NULL,(int &)ELRSideOffset,new CFarIntegerRangeValidator(0, 1024)));
	Dialog.Add(new CFarTextItem(47,9,DIF_NOAUTOHOTKEY,MLROffsetChars));
	Dialog.Add(new CFarTextItem(5, 10,0,MTDSideOffset));
	Dialog.Add(new CFarEditItem(41,10,45,0,NULL,(int &)ETDSideOffset,new CFarIntegerRangeValidator(0, 1024)));
	Dialog.Add(new CFarTextItem(47,10,DIF_NOAUTOHOTKEY,MTDOffsetLines));

	Dialog.Add(new CFarTextItem(5,12,0,MFindTextAtCursor));
	Dialog.Add(new CFarRadioButtonItem(7,13,DIF_GROUP,MNone,(int *)&EFindTextAtCursor,FT_NONE));
	Dialog.Add(new CFarRadioButtonItem(7,14,0,MWordOnly,(int *)&EFindTextAtCursor,FT_WORD));
	Dialog.Add(new CFarRadioButtonItem(7,15,0,MAnyText,(int *)&EFindTextAtCursor,FT_ANY));

	Dialog.Add(new CFarCheckBoxItem(7,16,0,MFindSelection,&EFindSelection));
	Dialog.Add(new CFarCheckBoxItem(5,17,0,MAutoFindInSelection,&EAutoFindInSelection));
	
	Dialog.Add(new CFarCheckBoxItem(5,19,0,MUseRealEOL,&g_bUseRealEOL));

	Dialog.Add(new CFarTextItem(5,21,0,MPositionCursor));
	Dialog.Add(new CFarRadioButtonItem(7,22,DIF_GROUP,MCurPosStart,(int *)&EPositionAt, EP_BEGIN));
	Dialog.Add(new CFarRadioButtonItem(7,23,0,MCurPosMove,(int *)&EPositionAt, EP_DIR));
	Dialog.Add(new CFarRadioButtonItem(7,24,0,MCurPosEnd, (int *)&EPositionAt, EP_END));
	Dialog.Add(new CFarCheckBoxItem(7,25,0,MCurAtNamedRef,&EPositionAtSub));
	Dialog.Add(new CFarEditItem(32,25,45,0,_T("RESearch.SubName"),EPositionSubName));

	Dialog.AddButtons(MOk,MCancel);

	Dialog.Display();
}

#ifdef FAR3
intptr_t WINAPI ConfigureW(const struct ConfigureInfo *Info)
#else
int WINAPI FAR_EXPORT(Configure)(int ItemNumber)
#endif
{
	const TCHAR *ppszItems[]={
		GetMsg(MCommonSettings),
		GetMsg(MFileSearchSettings),
		GetMsg(MGrepSettings),
		GetMsg(MRenumberSettings),
		GetMsg(MEditorSearchSettings)
	};
	int iResult = 0;
	do {
		switch (iResult = ChooseMenu(5,ppszItems,GetMsg(MRESearch),NULL,_T("Config"),iResult)) {
		case 0:ConfigureCommon();break;
		case 1:ConfigureFile();break;
		case 2:ConfigureGrep();break;
		case 3:ConfigureRenumbering(false);break;
		case 4:ConfigureEditor();break;
		default:WriteRegistry();return true;
		}
	} while (true);
}

#ifdef FAR3
void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->StructSize = sizeof(GlobalInfo);
	Info->MinFarVersion = FARMANAGERVERSION;
	Info->Version = MAKEFARVERSION(PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_REVISION, 0, VS_RELEASE);
	Info->Guid = GUID_RESearch;
	Info->Title = _T(PLUGIN_NAME);
	Info->Description = _T(PLUGIN_DESCRIPTION);
	Info->Author = _T(PLUGIN_AUTHOR);
}

void WINAPI ExitFARW(const ExitInfo *Info) {
#else
void WINAPI FAR_EXPORT(ExitFAR)() {
#endif
	WriteRegistry();
	ECleanup(false);
	FCleanup(false);
	FTCleanup(false);
	StopREThread();
	CoUninitialize();
}
