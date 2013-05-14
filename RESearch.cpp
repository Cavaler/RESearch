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
	Settings.OpenRoot(_T("RESearch"));
	ReadRegistry();

	g_pszOKButton = GetMsg(MOk);
	g_pszErrorTitle = GetMsg(MError);
	CFarIntegerRangeValidator::s_szErrorMsg = GetMsg(MInvalidNumber);
	CFarIntegerRangeValidator::s_szHelpTopic = _T("REInvalidNumber");
	CFarDialog::AutoHotkeys = true;

	CoInitialize(NULL);
	ReadActiveScripts();

/*	BMH Performance check
	LARGE_INTEGER Freq, Start, End;
	QueryPerformanceFrequency(&Freq);

	int nSize = 512*1024*1024;
	char *szBuffer = (char *)malloc(nSize);

	PrepareBMHSearch("ksbnrikavgéãôêåíãøùç", 20, 0);
	QueryPerformanceCounter(&Start);
	int nResult = BMHSearch(szBuffer, nSize, "ksbnrikavgéãôêåíãøùç", 20, NULL, 0);
	QueryPerformanceCounter(&End);

	double mMsec = (End.QuadPart - Start.QuadPart)/(Freq.QuadPart / 1000.0);
	*/
}

void BadCmdLine() {
	const TCHAR *Lines[]={GetMsg(MRESearch),GetMsg(MInvalidCmdLine),GetMsg(MOk)};
	StartupInfo.Message(FMSG_WARNING,_T("REInvalidCmdLine"),Lines,3,1);
}

BOOL ProcessFFLine(const TCHAR *Line, BOOL *ShowDialog, INT_PTR *Item)
{
	*Item = 0;
	TCHAR Switch = Line[0];
	if (Switch == 0) { *ShowDialog = TRUE; return TRUE; }

	Line++;
	*ShowDialog = FALSE;

	if ((Switch==' ')||(Switch=='\t')) {
		FText=Line;
		*Item=0;
		return TRUE;
	}

	const TCHAR *NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return FALSE; }

	FMask = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcsrchr(Line,Switch);
	if (NextSwitch) {
		FText = tstring(Line, NextSwitch-Line);
		Line  = NextSwitch+1;
	} else {
		FText=Line;
	}

	while (NextSwitch&&*Line) {
		switch (*Line) {
		case 'c':FCaseSensitive=TRUE;break;
		case 'C':FCaseSensitive=FALSE;break;
		case 'i':FSInverse=TRUE;break;
		case 'I':FSInverse=FALSE;break;
		case 'p':case 'P':FSearchAs=SA_PLAINTEXT;break;
		case 'r':case 'R':FSearchAs=SA_REGEXP;break;
		case 's':case 'S':FSearchAs=SA_SEVERALLINE;break;
		case 'm':case 'M':FSearchAs=SA_MULTILINE;break;
		case 'a':case 'A':FSearchAs=SA_MULTITEXT;break;
		case 'l':case 'L':FSearchAs=SA_MULTIREGEXP;break;
		case 'd':*ShowDialog=TRUE;break;
		case 'D':*ShowDialog=FALSE;break;
		case '0':FSearchIn=SI_ALLDRIVES;break;
		case '1':FSearchIn=SI_ALLLOCAL;break;
		case '2':FSearchIn=SI_FROMROOT;break;
		case '3':FSearchIn=SI_FROMCURRENT;break;
		case '4':FSearchIn=SI_CURRENTONLY;break;
		case '5':FSearchIn=SI_SELECTED;break;
		default:BadCmdLine();return FALSE;
		}
		Line++;
	}
	return TRUE;
}

BOOL ProcessFRLine(const TCHAR *Line,BOOL *ShowDialog,INT_PTR *Item)
{
	*Item = 1;
	TCHAR Switch = Line[0];
	if (!Switch) { *ShowDialog = TRUE; return TRUE; }

	Line++;
	*ShowDialog = FALSE;

	if ((Switch==' ')||(Switch=='\t')) {
		FText=Line;*Item=0;return TRUE;
	}

	const TCHAR *NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return FALSE; }
	FMask = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return FALSE; }
	FText = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch=_tcsrchr(Line,Switch);
	if (NextSwitch) {
		FRReplace = tstring(Line, NextSwitch-Line);
		Line = NextSwitch+1;
	} else {
		FRReplace = Line;
	}

	while (NextSwitch&&*Line) {
		switch (*Line) {
		case 'c':FCaseSensitive=TRUE;break;
		case 'C':FCaseSensitive=FALSE;break;
		case 'p':case 'P':FSearchAs=SA_PLAINTEXT;break;
		case 'r':case 'R':FSearchAs=SA_REGEXP;break;
		case 's':case 'S':FSearchAs=SA_SEVERALLINE;break;
		case 'v':FROpenModified=TRUE;break;
		case 'V':FROpenModified=FALSE;break;
		case 'f':FRConfirmFile=TRUE;break;
		case 'F':FRConfirmFile=FALSE;break;
		case 'l':FRConfirmLine=TRUE;break;
		case 'L':FRConfirmLine=FALSE;break;
		case 'o':FRSaveOriginal=TRUE;break;
		case 'O':FRSaveOriginal=FALSE;break;
		case 'b':FROverwriteBackup=TRUE;break;
		case 'B':FROverwriteBackup=FALSE;break;
		case 'w':FRReplaceToNew=TRUE;break;
		case 'W':FRReplaceToNew=FALSE;break;
		case 'd':*ShowDialog=TRUE;break;
		case 'D':*ShowDialog=FALSE;break;
		case '0':FSearchIn=SI_ALLDRIVES;break;
		case '1':FSearchIn=SI_ALLLOCAL;break;
		case '2':FSearchIn=SI_FROMROOT;break;
		case '3':FSearchIn=SI_FROMCURRENT;break;
		case '4':FSearchIn=SI_CURRENTONLY;break;
		case '5':FSearchIn=SI_SELECTED;break;
		default:BadCmdLine();return FALSE;
		}
		Line++;
	}
	return TRUE;
}

BOOL ProcessRNLine(const TCHAR *Line,BOOL *ShowDialog,INT_PTR *Item)
{
	*Item = 7;
	TCHAR Switch = Line[0];
	if (!Switch) { *ShowDialog = TRUE; return TRUE; }

	Line++;
	*ShowDialog = FALSE;

	if ((Switch==' ')||(Switch=='\t')) {
		FText=Line;
		*Item=0;
		return TRUE;
	}

	const TCHAR *NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return FALSE; }
	FMask = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return FALSE; }
	FText = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcsrchr(Line,Switch);
	if (NextSwitch) {
		FRReplace = tstring(Line, NextSwitch-Line);
		Line = NextSwitch+1;
	} else {
		FRReplace = Line;
	}

	while (NextSwitch&&*Line) {
		switch (*Line) {
		case 'c':FCaseSensitive=TRUE;break;
		case 'C':FCaseSensitive=FALSE;break;
		case 'r':FSearchAs=SA_REGEXP;break;
		case 'R':FSearchAs=SA_PLAINTEXT;break;
		case 'p':FRepeating=TRUE;break;
		case 'P':FRepeating=FALSE;break;
		case 'f':FRConfirmFile=TRUE;break;
		case 'F':FRConfirmFile=FALSE;break;
		case 'l':FRConfirmLine=TRUE;break;
		case 'L':FRConfirmLine=FALSE;break;
		case 'd':*ShowDialog=TRUE;break;
		case 'D':*ShowDialog=FALSE;break;
		case '0':FSearchIn=SI_ALLDRIVES;break;
		case '1':FSearchIn=SI_ALLLOCAL;break;
		case '2':FSearchIn=SI_FROMROOT;break;
		case '3':FSearchIn=SI_FROMCURRENT;break;
		case '4':FSearchIn=SI_CURRENTONLY;break;
		case '5':FSearchIn=SI_SELECTED;break;
		default:BadCmdLine();return FALSE;
		}
		Line++;
	}
	return TRUE;
}

BOOL ProcessQRLine(const TCHAR *Line,BOOL *ShowDialog,INT_PTR *Item)
{
	*Item = 8;
	TCHAR Switch = Line[0];
	if (!Switch) { *ShowDialog = TRUE; return TRUE; }

	Line++;
	*ShowDialog = FALSE;

	const TCHAR *NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return FALSE; }
	FText = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcsrchr(Line,Switch);
	if (NextSwitch) {
		FRReplace = tstring(Line, NextSwitch-Line);
		Line = NextSwitch+1;
	} else {
		FRReplace=Line;
	}

	while (NextSwitch&&*Line) {
		switch (*Line) {
		case 'c':FCaseSensitive=TRUE;break;
		case 'C':FCaseSensitive=FALSE;break;
		case 'r':FSearchAs=SA_REGEXP;break;
		case 'R':FSearchAs=SA_PLAINTEXT;break;
		case 'p':FRepeating=TRUE;break;
		case 'P':FRepeating=FALSE;break;
		case 'f':FRConfirmFile=TRUE;break;
		case 'F':FRConfirmFile=FALSE;break;
		case 'l':FRConfirmLine=TRUE;break;
		case 'L':FRConfirmLine=FALSE;break;
		case 'd':*ShowDialog=TRUE;break;
		case 'D':*ShowDialog=FALSE;break;
		default:BadCmdLine();return FALSE;
		}
		Line++;
	}
	return TRUE;
}

BOOL ProcessCommandLine(const TCHAR *Line,BOOL *ShowDialog,INT_PTR *Item)
{
//	f?:/mask/findtext/options
//	f?:/mask/findtext/replacetext/options
//	f?: FindText
//	ff:/mask/findtext/options
//	fr:/mask/findtext/replacetext/options
//	rn:/mask/findtext/replacetext/options
//	qr:/findtext/replacetext/options
	if (_tcsnicmp(Line,_T("ff:"),3)==0) return ProcessFFLine(Line+3,ShowDialog,Item);
	if (_tcsnicmp(Line,_T("fr:"),3)==0) return ProcessFRLine(Line+3,ShowDialog,Item);
	if (_tcsnicmp(Line,_T("rn:"),3)==0) return ProcessRNLine(Line+3,ShowDialog,Item);
	if (_tcsnicmp(Line,_T("qr:"),3)==0) return ProcessQRLine(Line+3,ShowDialog,Item);

	TCHAR Switch=Line[0];
	if (!Switch) { BadCmdLine(); return FALSE; }

	const TCHAR *NextSwitch;
	if ((Switch!=' ')&&(Switch!='\t')) {
		if (NextSwitch=_tcschr(Line+1,Switch))
			if (NextSwitch=_tcschr(NextSwitch+1,Switch))
				if (NextSwitch=_tcschr(NextSwitch+1,Switch)) 
					return ProcessFRLine(Line+1,ShowDialog,Item);
	}
	return ProcessFFLine(Line+1,ShowDialog,Item);
}

static LPCWSTR g_szPanelCommands[] = {
	L"Search", L"Replace", L"Grep", NULL,
	L"Select", L"Unselect", L"FlipSelection", NULL,
	L"Rename", L"RenameSelected", L"Renumber", L"UndoRename", NULL,
	L"ShowLastResults", L"ClearVariables"
};

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

static LPCWSTR g_szEditorCommands[] = {
	L"Search", L"Replace", L"Filter", L"Transliterate", NULL,
	L"SRAgain", L"SRAgainRev", NULL,
	L"ShowLastResults", L"ClearVariables"
};

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

static LPCWSTR g_szViewerCommands[] = {
	L"Search", L"SRAgain", L"ClearVariables"
};

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

HANDLE OpenPluginFromFileMenu(int Item, BOOL ShowDialog = TRUE)
{
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
		StartupInfo.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, TRUE, NULL);
		StartupInfo.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, NULL);
#else
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_UPDATEPANEL,(void *)~NULL);
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_REDRAWPANEL,NULL);
#endif
		return NO_PANEL_HANDLE;
	}
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
			ESearchAgainCalled = TRUE;
			EPreparePattern(EText);		// In case codepage changed etc

			switch (LastAction) {
			case -1:
				ESearchAgainCalled = FALSE;
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
			ESearchAgainCalled = TRUE;
			switch (LastAction) {
			case -1:
				ESearchAgainCalled = FALSE;
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

bool GetIntValue(FarMacroValue &Value, int &nValue)
{
	switch (Value.Type) {
	case FMVT_INTEGER:
		nValue = (int)Value.Integer;
		return true;
	case FMVT_DOUBLE:
		nValue = (int)Value.Double;
		return true;
	default:
		return false;
	}
}

HANDLE OpenFromStringMacro(int nType, LPCWSTR szParam)
{
	if (nType == 0) {
		for (int nValue = 0; nValue < sizeof(g_szPanelCommands)/sizeof(g_szPanelCommands[0]); nValue++) {
			if (g_szPanelCommands[nValue] == NULL) continue;
			if (_wcsicmp(g_szPanelCommands[nValue], szParam) == 0)
				return OpenPluginFromFileMenu(nValue);
		}
	} else if (nType == 1) {
		for (int nValue = 0; nValue < sizeof(g_szEditorCommands)/sizeof(g_szEditorCommands[0]); nValue++) {
			if (g_szEditorCommands[nValue] == NULL) continue;
			if (_wcsicmp(g_szEditorCommands[nValue], szParam) == 0)
				return OpenPluginFromEditorMenu(nValue);
		}
	} else if (nType == 2) {
		for (int nValue = 0; nValue < sizeof(g_szViewerCommands)/sizeof(g_szViewerCommands[0]); nValue++) {
			if (g_szViewerCommands[nValue] == NULL) continue;
			if (_wcsicmp(g_szViewerCommands[nValue], szParam) == 0)
				return OpenPluginFromViewerMenu(nValue);
		}
	}

	return NO_PANEL_HANDLE;
}

HANDLE OpenFromStringMacro(int nType, LPCWSTR szParam1, LPCWSTR szParam2)
{
	if (wcsicmp(szParam1, L"Preset") == 0) {
	}

	if (wcsicmp(szParam1, L"Batch") == 0) {
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

HANDLE OpenFromMacro(const OpenMacroInfo *MInfo)
{
	g_bFromCmdLine = true;

	int nType = GetAreaType();

	int nValue;
	if (MInfo->Count == 1) {
		if (nType < 0) return NO_PANEL_HANDLE;

		if (MInfo->Values[0].Type == FMVT_STRING) {
			return OpenFromStringMacro(nType, MInfo->Values[0].String);
		}

		if (!GetIntValue(MInfo->Values[0], nValue)) return NO_PANEL_HANDLE;
	} else {

		if ((MInfo->Values[0].Type == FMVT_STRING) && (MInfo->Values[1].Type == FMVT_STRING)) {
			if (nType < 0) return NO_PANEL_HANDLE;
			return OpenFromStringMacro(nType, MInfo->Values[0].String, MInfo->Values[1].String);
		}

		if (!GetIntValue(MInfo->Values[0], nType )) return NO_PANEL_HANDLE;
		if (!GetIntValue(MInfo->Values[1], nValue)) return NO_PANEL_HANDLE;
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

HANDLE WINAPI OpenW(const struct OpenInfo *Info)
{
	g_bFromCmdLine = false;
	g_bInterrupted = FALSE;
	ESearchAgainCalled = FALSE;

	switch (Info->OpenFrom) {
	case OPEN_PLUGINSMENU:
		return OpenPluginFromFileMenu(0);

	case OPEN_EDITOR:
		return OpenPluginFromEditorMenu(0);

	case OPEN_VIEWER:
		return OpenPluginFromViewerMenu(0);

	case OPEN_COMMANDLINE:{
		OpenCommandLineInfo *CmdInfo = (OpenCommandLineInfo *)Info->Data;
		BOOL ShowDialog = TRUE;
		INT_PTR Item;
		if (ProcessCommandLine(CmdInfo->CommandLine, &ShowDialog, &Item)) {
			g_bFromCmdLine = true;
			return OpenPluginFromFileMenu(Item, ShowDialog);
		}
		break;
						  }
	case OPEN_FROMMACRO:
		return OpenFromMacro((const OpenMacroInfo *)Info->Data);
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
	BOOL ShowDialog = TRUE;
	g_bFromCmdLine = false;
	g_bInterrupted = FALSE;

	ESearchAgainCalled = FALSE;

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

BOOL AtoI(char *String,int *Number,int Min,int Max) {
	int I;
	if ((sscanf(String,"%d",&I)==1)&&(I>=Min)&&(I<=Max)) {
		*Number=I;
		return TRUE;
	} else return FALSE;
}

int ConfigureSeveralLines() {
	CFarDialog Dialog(60,9,_T("CommonConfig"));
	Dialog.AddFrame(MCommonSettings);

	Dialog.Add(new CFarTextItem(5,3,0,MSeveralLinesIs));
	Dialog.Add(new CFarEditItem(32,3,36,0,NULL,(int &)SeveralLines,new CFarIntegerRangeValidator(1, 65535)));
	Dialog.Add(new CFarTextItem(38,3,0,MLinesOr));
	Dialog.Add(new CFarEditItem(48,3,52,0,NULL,(int &)SeveralLinesKB,new CFarIntegerRangeValidator(1, 1024)));
	Dialog.Add(new CFarTextItem(53,3,0,MKB));

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
			return TRUE;
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
	CFarDialog Dialog(60,19,_T("CommonConfig"));
	Dialog.AddFrame(MCommonSettings);

	Dialog.Add(new CFarTextItem(5,3,0,MSeveralLinesIs));
	Dialog.Add(new CFarEditItem(32,3,36,0,NULL,(int &)SeveralLines,new CFarIntegerRangeValidator(1, 65535)));
	Dialog.Add(new CFarTextItem(38,3,0,MLinesOr));
	Dialog.Add(new CFarEditItem(48,3,52,0,NULL,(int &)SeveralLinesKB,new CFarIntegerRangeValidator(1, 1024)));
	Dialog.Add(new CFarTextItem(53,3,0,MKB));

	Dialog.Add(new CFarCheckBoxItem(5,5,0,MDotMatchesNewline,&DotMatchesNewline));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MUseSeparateThread,&g_bUseSeparateThread));
	Dialog.Add(new CFarTextItem(9,8,0,MMaxInThreadLength));
	Dialog.Add(new CFarEditItem(34,8,40,0,NULL,(int &)g_nMaxInThreadLength,new CFarIntegerRangeValidator(0,0x7FFFFFFF)));
	Dialog.Add(new CFarTextItem(9,9,0,MThreadStackMB));
	Dialog.Add(new CFarEditItem(34,9,40,0,NULL,(int &)g_nThreadStackMB,new CFarIntegerRangeValidator(0,1024)));
	Dialog.Add(new CFarTextItem(43,9,0,MKB));

	Dialog.Add(new CFarCheckBoxItem(5,11,0,MShowUsageWarnings,&g_bShowUsageWarnings));
	Dialog.Add(new CFarCheckBoxItem(5,12,0,MUseEscapesInPlainText,&g_bEscapesInPlainText));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MIgnoreIdentReplace,&g_bIgnoreIdentReplace));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Display(-1);
}

void ConfigureFile()
{
#ifdef UNICODE
	CFarDialog Dialog(70, 24, _T("FileConfig"));
#else
	CFarDialog Dialog(70, 21, _T("FileConfig"));
#endif
	Dialog.AddFrame(MFileSearchSettings);

	Dialog.Add(new CFarBoxItem(FALSE,5,3,33,7,DIF_LEFTTEXT,MDefaultMaskCase));
	Dialog.Add(new CFarRadioButtonItem(7,4,DIF_GROUP,MMaskSensitive,(int *)&FMaskCase,MC_SENSITIVE));
	Dialog.Add(new CFarRadioButtonItem(7,5,0,MMaskInsensitive,(int *)&FMaskCase,MC_INSENSITIVE));
	Dialog.Add(new CFarRadioButtonItem(7,6,0,MMaskVolumeDependent,(int *)&FMaskCase,MC_VOLUME));

	Dialog.Add(new CFarBoxItem(FALSE,35,3,64,7,DIF_LEFTTEXT,MReplaceReadonly));
	Dialog.Add(new CFarRadioButtonItem(37,4,DIF_GROUP,MNever,(int *)&FRReplaceReadonly,RR_NEVER));
	Dialog.Add(new CFarRadioButtonItem(37,5,0,MAsk,(int *)&FRReplaceReadonly,RR_ASK));
	Dialog.Add(new CFarRadioButtonItem(37,6,0,MAlways,(int *)&FRReplaceReadonly,RR_ALWAYS));

	Dialog.Add(new CFarCheckBoxItem(5, 9, 0, MSkipSystemFolders, &FASkipSystemFolders));
	Dialog.Add(new CFarEditItem(9, 10, 60, DIF_HISTORY,_T("RESearch.SystemFolders"), FASystemFolders));
	Dialog.Add(new CFarCheckBoxItem(5, 12, 0, MUseSingleCR, &FSUseSingleCR));
	Dialog.Add(new CFarCheckBoxItem(5, 13, 0, MEditSrchAfterFile, &FSEditSrchAfterFile));

	Dialog.Add(new CFarTextItem(5,15,0,MBufferSize));
	Dialog.Add(new CFarEditItem(34,15,40,0,NULL,FBufferSize,new CFarIntegerRangeValidator(1,1024)));
	Dialog.Add(new CFarTextItem(43,15,0,MMB));

#ifdef UNICODE
	Dialog.Add(new CFarTextItem(5,17,0,MDefaultCP));
	Dialog.Add(new CFarRadioButtonItem(35,17,0,MDefaultOEM,&g_bDefaultOEM,TRUE));
	Dialog.Add(new CFarRadioButtonItem(45,17,0,MDefaultANSI,&g_bDefaultOEM,FALSE));
	Dialog.Add(new CFarTextItem(5,18,0,MAllCPInclude));
	Dialog.Add(new CFarButtonItem(35,18,0,FALSE,MAllCPSelect));

	Dialog.AddButtons(MOk,MCancel);
	do {
		int nResult = Dialog.Display(2, -2, -3);

		switch (nResult) {
		case 1:
			ConfigureCP();
			break;
		default:
			return;
		}
	} while (true);

#else
	Dialog.AddButtons(MOk,MCancel);
	Dialog.Display(-1);
#endif
}

void ConfigureRenumbering(bool bRuntime)
{
	CFarDialog Dialog(70, bRuntime ? 18 : 16, _T("RenumberConfig"));

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

	Dialog.AddButtons(MOk,MCancel);

	Dialog.Display(-1);
}

void ConfigureEditor()
{
	CFarDialog Dialog(61, 31, _T("EditorConfig"));
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
	Dialog.Add(new CFarTextItem(47,9,0,MLROffsetChars));
	Dialog.Add(new CFarTextItem(5, 10,0,MTDSideOffset));
	Dialog.Add(new CFarEditItem(41,10,45,0,NULL,(int &)ETDSideOffset,new CFarIntegerRangeValidator(0, 1024)));
	Dialog.Add(new CFarTextItem(47,10,0,MTDOffsetLines));

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
	Dialog.Display(-1);
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
		GetMsg(MRenumberSettings),
		GetMsg(MEditorSearchSettings)
	};
	int iResult = 0;
	do {
		switch (iResult = ChooseMenu(4,ppszItems,GetMsg(MRESearch),NULL,_T("Config"),iResult)) {
		case 0:ConfigureCommon();break;
		case 1:ConfigureFile();break;
		case 2:ConfigureRenumbering(false);break;
		case 3:ConfigureEditor();break;
		default:WriteRegistry();return TRUE;
		}
	} while (TRUE);
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
	ECleanup(FALSE);
	FCleanup(FALSE);
	FTCleanup(FALSE);
	Settings.Close();
	StopREThread();
	CoUninitialize();
}
