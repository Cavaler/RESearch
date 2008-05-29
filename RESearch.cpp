#include "StdAfx.h"
#include "RESearch.h"

void WINAPI GetPluginInfo(PluginInfo *Info) {
	static const char *ConfigStrings[1];
	static const char *MenuStrings[1];
	ConfigStrings[0] = GetMsg(MRESearch);
	MenuStrings[0] = GetMsg(MRESearch);

	Info->StructSize=sizeof(PluginInfo);
	Info->Flags=PF_EDITOR|PF_VIEWER|PF_FULLCMDLINE;
//	Info->Flags=PF_EDITOR|PF_FULLCMDLINE;
	Info->DiskMenuStringsNumber=0;
	Info->PluginMenuStrings=MenuStrings;
	Info->PluginMenuStringsNumber=1;
	Info->PluginConfigStrings=ConfigStrings;
	Info->PluginConfigStringsNumber=1;
	Info->CommandPrefix="ff:fr:rn:qr";
}

void WINAPI SetStartupInfo(const PluginStartupInfo *Info) {
	StartupInfo=*Info;
	SetANSILocale();
	ReadRegistry();

	g_pszOKButton = GetMsg(MOk);
	g_pszErrorTitle = GetMsg(MError);
	CFarIntegerRangeValidator::s_szErrorMsg = GetMsg(MInvalidNumber);
	CFarIntegerRangeValidator::s_szHelpTopic = "REInvalidNumber";

	CoInitialize(NULL);
	ReadActiveScripts();
}

void BadCmdLine() {
	const char *Lines[]={GetMsg(MRESearch),GetMsg(MInvalidCmdLine),GetMsg(MOk)};
	StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"REInvalidCmdLine",Lines,3,1);
}

BOOL ProcessFFLine(char *Line,BOOL *ShowDialog,int *Item) {
	char Switch=Line[0];
	if (!Switch) {BadCmdLine();return FALSE;}
	Line++;*ShowDialog=FALSE;*Item=0;

	if ((Switch==' ')||(Switch=='\t')) {
		FText=Line;*Item=0;return TRUE;
	}

	char *NextSwitch=strchr(Line,Switch);
	if (!NextSwitch) {BadCmdLine();return FALSE;}
	*NextSwitch=0;FMask=Line;
	*NextSwitch=Switch;Line=NextSwitch+1;

	NextSwitch=strrchr(Line,Switch);
	if (NextSwitch) {
		*NextSwitch=0;FText=Line;
		*NextSwitch=Switch;Line=NextSwitch+1;
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
		case 'u':FUTF8=TRUE;break;
		case 'U':FUTF8=FALSE;break;
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

BOOL ProcessFRLine(char *Line,BOOL *ShowDialog,int *Item) {
	char Switch=Line[0];
	if (!Switch) {BadCmdLine();return FALSE;}
	Line++;*ShowDialog=FALSE;*Item=1;

	if ((Switch==' ')||(Switch=='\t')) {
		FText=Line;*Item=0;return TRUE;
	}

	char *NextSwitch=strchr(Line,Switch);
	if (!NextSwitch) {BadCmdLine();return FALSE;}
	*NextSwitch=0;FMask=Line;
	*NextSwitch=Switch;Line=NextSwitch+1;

	NextSwitch=strchr(Line,Switch);
	if (!NextSwitch) {BadCmdLine();return FALSE;}
	*NextSwitch=0;FText=Line;
	*NextSwitch=Switch;Line=NextSwitch+1;

	NextSwitch=strrchr(Line,Switch);
	if (NextSwitch) {
		*NextSwitch=0;FRReplace=Line;
		*NextSwitch=Switch;Line=NextSwitch+1;
	} else {
		FRReplace=Line;
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
		case 'd':*ShowDialog=TRUE;break;
		case 'D':*ShowDialog=FALSE;break;
		case 'u':FUTF8=TRUE;break;
		case 'U':FUTF8=FALSE;break;
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

BOOL ProcessRNLine(char *Line,BOOL *ShowDialog,int *Item) {
	char Switch=Line[0];
	if (!Switch) {BadCmdLine();return FALSE;}
	Line++;*ShowDialog=FALSE;*Item=7;

	if ((Switch==' ')||(Switch=='\t')) {
		FText=Line;*Item=0;return TRUE;
	}

	char *NextSwitch=strchr(Line,Switch);
	if (!NextSwitch) {BadCmdLine();return FALSE;}
	*NextSwitch=0;FMask=Line;
	*NextSwitch=Switch;Line=NextSwitch+1;

	NextSwitch=strchr(Line,Switch);
	if (!NextSwitch) {BadCmdLine();return FALSE;}
	*NextSwitch=0;FText=Line;
	*NextSwitch=Switch;Line=NextSwitch+1;

	NextSwitch=strrchr(Line,Switch);
	if (NextSwitch) {
		*NextSwitch=0;FRReplace=Line;
		*NextSwitch=Switch;Line=NextSwitch+1;
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

BOOL ProcessQRLine(char *Line,BOOL *ShowDialog,int *Item) {
	char Switch=Line[0];
	if (!Switch) {BadCmdLine();return FALSE;}
	Line++;*ShowDialog=FALSE;*Item=8;

	char *NextSwitch=strchr(Line,Switch);
	if (!NextSwitch) {BadCmdLine();return FALSE;}
	*NextSwitch=0;FText=Line;
	*NextSwitch=Switch;Line=NextSwitch+1;

	NextSwitch=strrchr(Line,Switch);
	if (NextSwitch) {
		*NextSwitch=0;FRReplace=Line;
		*NextSwitch=Switch;Line=NextSwitch+1;
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

BOOL ProcessCommandLine(char *Line,BOOL *ShowDialog,int *Item) {
//	f?:/mask/findtext/options
//	f?:/mask/findtext/replacetext/options
//	f?: FindText
//	ff:/mask/findtext/options
//	fr:/mask/findtext/replacetext/options
//	rn:/mask/findtext/replacetext/options
//	qr:/findtext/replacetext/options
	if (_strnicmp(Line,"ff:",3)==0) return ProcessFFLine(Line+3,ShowDialog,Item);
	if (_strnicmp(Line,"fr:",3)==0) return ProcessFRLine(Line+3,ShowDialog,Item);
	if (_strnicmp(Line,"rn:",3)==0) return ProcessRNLine(Line+3,ShowDialog,Item);
	if (_strnicmp(Line,"qr:",3)==0) return ProcessQRLine(Line+3,ShowDialog,Item);

	char Switch=Line[0];
	if (!Switch) {BadCmdLine();return FALSE;}

	char *NextSwitch;
	if ((Switch!=' ')&&(Switch!='\t')) {
		if (NextSwitch=strchr(Line+1,Switch))
			if (NextSwitch=strchr(NextSwitch+1,Switch))
				if (NextSwitch=strchr(NextSwitch+1,Switch)) 
					return ProcessFRLine(Line+1,ShowDialog,Item);
	}
	return ProcessFFLine(Line+1,ShowDialog,Item);
}

int ShowFileMenu() {
	vector<FarMenuItem> MenuItems(15);

	strcpy(MenuItems[0].Text,GetMsg(MMenuSearch));
	strcpy(MenuItems[1].Text,GetMsg(MMenuReplace));
	strcpy(MenuItems[2].Text,GetMsg(MMenuGrep));
	MenuItems[3].Separator=TRUE;
	strcpy(MenuItems[4].Text,GetMsg(MMenuSelect));
	strcpy(MenuItems[5].Text,GetMsg(MMenuUnselect));
	strcpy(MenuItems[6].Text,GetMsg(MMenuFlipSelection));
	MenuItems[7].Separator=TRUE;
	strcpy(MenuItems[8].Text,GetMsg(MMenuRename));
	strcpy(MenuItems[9].Text,GetMsg(MMenuRenameSelected));
	strcpy(MenuItems[10].Text,GetMsg(MMenuRenumber));
	MenuItems[11].Separator=TRUE;
	strcpy(MenuItems[12].Text,GetMsg(MMenuUTF8Converter));
	strcpy(MenuItems[13].Text,GetMsg(MMenuShowLastResults));

	MenuItems[14].Separator=TRUE;
	FSPresets->FillMenuItems(MenuItems);
	FRPresets->FillMenuItems(MenuItems);
	FGPresets->FillMenuItems(MenuItems);
	RPresets->FillMenuItems(MenuItems);
	QRPresets->FillMenuItems(MenuItems);

	return StartupInfo.Menu(StartupInfo.ModuleNumber,-1,-1,0,FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT,GetMsg(MMenuHeader),
		NULL,"FileMenu",NULL,NULL,&MenuItems[0],MenuItems.size());
}

int ShowEditorMenu() {
	vector<FarMenuItem> MenuItems(11);

	strcpy(MenuItems[0].Text,GetMsg(MMenuSearch));
	strcpy(MenuItems[1].Text,GetMsg(MMenuReplace));
	strcpy(MenuItems[2].Text,GetMsg(MMenuFilterText));
	strcpy(MenuItems[3].Text,GetMsg(MMenuTransliterate));
	MenuItems[4].Separator=TRUE;
	strcpy(MenuItems[5].Text,GetMsg(MMenuSearchReplaceAgain));
	strcpy(MenuItems[6].Text,GetMsg(MMenuSearchReplaceAgainRev));
	MenuItems[7].Separator=TRUE;
	strcpy(MenuItems[8].Text,GetMsg(MMenuUTF8Converter));
	strcpy(MenuItems[9].Text,GetMsg(MMenuShowLastResults));

	MenuItems[10].Separator=TRUE;
	ESPresets->FillMenuItems(MenuItems);
	ERPresets->FillMenuItems(MenuItems);
	EFPresets->FillMenuItems(MenuItems);
	ELPresets->FillMenuItems(MenuItems);

	return StartupInfo.Menu(StartupInfo.ModuleNumber,-1,-1,0,FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT,GetMsg(MMenuHeader),
		NULL,"EditorMenu",NULL,NULL,&MenuItems[0],MenuItems.size());
}

int ShowViewerMenu() {
	vector<FarMenuItem> MenuItems(5);

	strcpy(MenuItems[0].Text,GetMsg(MMenuSearch));
	strcpy(MenuItems[1].Text,GetMsg(MMenuSearchAgain));
//	strcpy(MenuItems[2].Text,GetMsg(MMenuSearchAgainRev));
	MenuItems[2].Separator=TRUE;
	strcpy(MenuItems[3].Text,GetMsg(MMenuUTF8Converter));

	MenuItems[4].Separator=TRUE;
	VSPresets->FillMenuItems(MenuItems);

	return StartupInfo.Menu(StartupInfo.ModuleNumber,-1,-1,0,FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT,GetMsg(MMenuHeader),
		NULL,"EditorMenu",NULL,NULL,&MenuItems[0],MenuItems.size());
}

OperationResult OpenPluginFromFilePreset(int Item) {
	CPreset *pPreset;
	if (pPreset = FSPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_FSBatch);
		pPreset->Apply(g_FSBatch);
		return g_FSBatch.m_Executor();
	}

	if (pPreset = FRPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_FRBatch);
		pPreset->Apply(g_FRBatch);
		return g_FRBatch.m_Executor();
	}

	if (pPreset = FGPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_FGBatch);
		pPreset->Apply(g_FGBatch);
		return g_FGBatch.m_Executor();
	}

	if (pPreset = RPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_RBatch);
		pPreset->Apply(g_RBatch);
		return g_RBatch.m_Executor();
	}

	if (pPreset = QRPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_QRBatch);
		pPreset->Apply(g_QRBatch);
		return g_QRBatch.m_Executor();
	}

	return OR_CANCEL;
}

HANDLE OpenPluginFromFileMenu(int Item, BOOL ShowDialog) {
	OperationResult Result=OR_CANCEL;
	if (!g_bFromCmdLine) {
		Item=ShowFileMenu();
		if (Item==-1) return INVALID_HANDLE_VALUE;
	}
	switch (Item) {
		case 0:
			Result=FileFind(&PanelItems,&ItemsNumber,ShowDialog);
			break;
		case 1:
			Result=FileReplace(&PanelItems,&ItemsNumber,ShowDialog);
			break;
		case 2:
			Result=FileGrep(ShowDialog);
			break;
		case 4:ChangeSelection(MMenuSelect);break;
		case 5:ChangeSelection(MMenuUnselect);break;
		case 6:ChangeSelection(MMenuFlipSelection);break;
		case 8:
			Result=RenameFiles(&PanelItems,&ItemsNumber,ShowDialog);
			break;
		case 9:
			Result=RenameSelectedFiles(&PanelItems,&ItemsNumber,ShowDialog);
			break;
		case 10:
			RenumberFiles();
			Result=OR_CANCEL;
			break;
		case 12:
			UTF8Converter();
			Result=OR_CANCEL;
			break;
		case 13:
			if (LastTempPanel) {
				LastTempPanel->m_bActive = true;
				return (HANDLE)LastTempPanel;
			} else {
				Result=OR_CANCEL;
				break;
			}
	}
	if (Item >= 15) {
		Item -= 15;
		Result = OpenPluginFromFilePreset(Item);
	}

	if (Result!=OR_CANCEL) SynchronizeWithFile(Item);
	if (Result==OR_PANEL) {
		PanelInfo PInfo;
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
		CTemporaryPanel *Panel=new CTemporaryPanel(PanelItems,ItemsNumber,PInfo.CurDir);
		PanelItems=NULL;ItemsNumber=0;
		return (HANDLE)Panel;
	} else {
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_UPDATEPANEL,(void *)~NULL);
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_REDRAWPANEL,NULL);
		return INVALID_HANDLE_VALUE;
	}
}

OperationResult OpenPluginFromEditorPreset(int Item) {
	CPreset *pPreset;
	if (pPreset = ESPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_ESBatch);
		pPreset->Apply(g_ESBatch);
		return g_ESBatch.m_Executor();
	}
	if (pPreset = ERPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_ERBatch);
		pPreset->Apply(g_ERBatch);
		return g_ERBatch.m_Executor();
	}
	if (pPreset = EFPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_EFBatch);
		pPreset->Apply(g_EFBatch);
		return g_EFBatch.m_Executor();
	}
	if (pPreset = ELPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_ELBatch);
		pPreset->Apply(g_ELBatch);
		return g_ELBatch.m_Executor();
	}
	return OR_CANCEL;
}

HANDLE OpenPluginFromEditorMenu(int Item) {
	FindIfClockPresent();
	switch (int nMenu = ShowEditorMenu()) {
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
		case 9:
			if (EditorListAllShowResults()) LastAction=4;
			break;
		case 6:
			EReverse = !EReverse;
		case 5:
			ESearchAgainCalled = TRUE;
			if (!ERegExp) EPreparePattern(EText);	// Not needed for RegExp

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
			if (nMenu == 6) EReverse = !EReverse;
			break;
		case 8:
			UTF8Converter();
			break;
	}

	if (Item >= 10) {
		Item -= 10;
		OpenPluginFromEditorPreset(Item);
	}

	return INVALID_HANDLE_VALUE;
}

OperationResult OpenPluginFromViewerPreset(int Item) {
	CPreset *pPreset;
	if (pPreset = VSPresets->FindMenuPreset(Item)) {
		CParameterBackup Backup(g_VSBatch);
		pPreset->Apply(g_VSBatch);
		return g_VSBatch.m_Executor();
	}
	return OR_CANCEL;
}

HANDLE OpenPluginFromViewerMenu(int Item) {
	switch (ShowViewerMenu()) {
		case 0:
			if (ViewerSearch()) LastAction = 0;
			break;
//		case 2:
//			EReverse = !EReverse;
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
//			if (nMenu == 2) EReverse = !EReverse;
			break;
		case 3:
			UTF8Converter();
			break;
	}

	if (Item >= 5) {
		Item -= 5;
		OpenPluginFromViewerPreset(Item);
	}

	return INVALID_HANDLE_VALUE;
}

HANDLE WINAPI OpenPlugin(int OpenFrom,int Item) {
	BOOL ShowDialog = TRUE;
	g_bFromCmdLine = false;
	g_bInterrupted = FALSE;

	ESearchAgainCalled = FALSE;

	switch (OpenFrom) {
	case OPEN_COMMANDLINE:
		g_bFromCmdLine = true;
		if (!ProcessCommandLine((char *)Item,&ShowDialog,&Item)) return INVALID_HANDLE_VALUE;
		else // fall-through

	case OPEN_PLUGINSMENU:
		return OpenPluginFromFileMenu(Item, ShowDialog);

	case OPEN_EDITOR:
		return OpenPluginFromEditorMenu(Item);

	case OPEN_VIEWER:
		return OpenPluginFromViewerMenu(Item);

	case OPEN_SHORTCUT:
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_SETPANELDIR,(void *)Item);
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_UPDATEPANEL,NULL);
		StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_REDRAWPANEL,NULL);
		return INVALID_HANDLE_VALUE;

	default:return INVALID_HANDLE_VALUE;
	}
}

BOOL AtoI(char *String,int *Number,int Min,int Max) {
	int I;
	if ((sscanf(String,"%d",&I)==1)&&(I>=Min)&&(I<=Max)) {
		*Number=I;
		return TRUE;
	} else return FALSE;
}

int ConfigureSeveralLines() {
	CFarDialog Dialog(60,9,"CommonConfig");
	Dialog.AddFrame(MCommonSettings);

	Dialog.Add(new CFarTextItem(5,3,0,MSeveralLinesIs));
	Dialog.Add(new CFarEditItem(32,3,36,0,NULL,(int &)SeveralLines,new CFarIntegerRangeValidator(1, 65535)));
	Dialog.Add(new CFarTextItem(38,3,0,MLinesOr));
	Dialog.Add(new CFarEditItem(48,3,52,0,NULL,(int &)SeveralLinesKB,new CFarIntegerRangeValidator(1, 1024)));
	Dialog.Add(new CFarTextItem(53,3,0,MKB));

	Dialog.AddButtons(MOk,MCancel);
	return Dialog.Display(-1);
}

int ConfigureCommon() {
	CFarDialog Dialog(60,16,"CommonConfig");
	Dialog.AddFrame(MCommonSettings);

	Dialog.Add(new CFarTextItem(5,3,0,MSeveralLinesIs));
	Dialog.Add(new CFarEditItem(32,3,36,0,NULL,(int &)SeveralLines,new CFarIntegerRangeValidator(1, 65535)));
	Dialog.Add(new CFarTextItem(38,3,0,MLinesOr));
	Dialog.Add(new CFarEditItem(48,3,52,0,NULL,(int &)SeveralLinesKB,new CFarIntegerRangeValidator(1, 1024)));
	Dialog.Add(new CFarTextItem(53,3,0,MKB));

	Dialog.Add(new CFarCheckBoxItem(5,5,0,MAllowEmptyMatch,&AllowEmptyMatch));
	Dialog.Add(new CFarCheckBoxItem(5,6,0,MDotMatchesNewline,&DotMatchesNewline));

	Dialog.Add(new CFarCheckBoxItem(5,8,0,MUseSeparateThread,&g_bUseSeparateThread));
	Dialog.Add(new CFarTextItem(9,9,0,MMaxInThreadLength));
	Dialog.Add(new CFarEditItem(34,9,40,0,NULL,(int &)g_nMaxInThreadLength,new CFarIntegerRangeValidator(0,0x7FFFFFFF)));
	Dialog.Add(new CFarTextItem(9,10,0,MThreadStackMB));
	Dialog.Add(new CFarEditItem(34,10,40,0,NULL,(int &)g_nThreadStackMB,new CFarIntegerRangeValidator(0,1024)));

	Dialog.AddButtons(MOk,MCancel);
	return Dialog.Display(-1);
}

void ConfigureFile() {
	CFarDialog Dialog(70, 23, "FileConfig");
	Dialog.AddFrame(MFileSearchSettings);

	Dialog.Add(new CFarBoxItem(FALSE,5,3,33,7,DIF_LEFTTEXT,MDefaultMaskCase));
	Dialog.Add(new CFarRadioButtonItem(7,4,DIF_GROUP,MMaskSensitive,(int *)&FMaskCase,MC_SENSITIVE));
	Dialog.Add(new CFarRadioButtonItem(7,5,0,MMaskInsensitive,(int *)&FMaskCase,MC_INSENSITIVE));
	Dialog.Add(new CFarRadioButtonItem(7,6,0,MMaskVolumeDependent,(int *)&FMaskCase,MC_VOLUME));

	Dialog.Add(new CFarBoxItem(FALSE,35,3,64,7,DIF_LEFTTEXT,MReplaceReadonly));
	Dialog.Add(new CFarRadioButtonItem(37,4,DIF_GROUP,MNever,(int *)&FRReplaceReadonly,RR_NEVER));
	Dialog.Add(new CFarRadioButtonItem(37,5,0,MAsk,(int *)&FRReplaceReadonly,RR_ASK));
	Dialog.Add(new CFarRadioButtonItem(37,6,0,MAlways,(int *)&FRReplaceReadonly,RR_ALWAYS));

	Dialog.Add(new CFarBoxItem(FALSE, 5,9,64,15,DIF_LEFTTEXT,MRenumberOptions));
	Dialog.Add(new CFarTextItem(7,10,0,MStripFromBeginning));
	Dialog.Add(new CFarEditItem(40,10,61,DIF_HISTORY,"RESearch.Strip", g_strStrip));
	Dialog.Add(new CFarTextItem(7,11,0,MPrefix));
	Dialog.Add(new CFarEditItem(32,11,42,DIF_HISTORY,"RESearch.Prefix", g_strPrefix));
	Dialog.Add(new CFarTextItem(7,12,0,MPostfix));
	Dialog.Add(new CFarEditItem(32,12,42,DIF_HISTORY,"RESearch.Postfix", g_strPostfix));
	Dialog.Add(new CFarTextItem(7,13,0,MStartFrom));
	Dialog.Add(new CFarEditItem(32,13,38,0, NULL, (int &)g_nStartWith,new CFarIntegerRangeValidator(0,0x7FFFFFFF)));
	Dialog.Add(new CFarTextItem(7,14,0,MWidth));
	Dialog.Add(new CFarEditItem(32,14,38,0, NULL, (int &)g_nWidth,new CFarIntegerRangeValidator(0,MAX_PATH)));

	Dialog.Add(new CFarCheckBoxItem(5,16,0,MSkipSystemFolders,&FASkipSystemFolders));
	Dialog.Add(new CFarEditItem(9, 17, 45, DIF_HISTORY,"RESearch.SystemFolders", FASystemFolders));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Display(-1);
}

void ConfigureEditor() {
	CFarDialog Dialog(60,20,"EditorConfig");
	Dialog.AddFrame(MEditorSearchSettings);

	Dialog.Add(new CFarTextItem(5,3,0,MShowPositionOffset));
	Dialog.Add(new CFarEditItem(40,3,47,0,NULL,(int &)EShowPositionOffset,new CFarIntegerRangeValidator(-1024,1024)));

	Dialog.Add(new CFarTextItem(5,4,0,Mfrom));
	Dialog.Add(new CFarRadioButtonItem(10,4,DIF_GROUP,MTop,(int *)&EShowPosition,SP_TOP));
	Dialog.Add(new CFarRadioButtonItem(10,5,0,MCenter,(int *)&EShowPosition,SP_CENTER));
	Dialog.Add(new CFarRadioButtonItem(10,6,0,MBottom,(int *)&EShowPosition,SP_BOTTOM));

	Dialog.Add(new CFarTextItem(5,8,0,MRightSideOffset));
	Dialog.Add(new CFarEditItem(32,8,39,0,NULL,(int &)ERightSideOffset,new CFarIntegerRangeValidator(0,1024)));

	Dialog.Add(new CFarTextItem(5,10,0,MFindTextAtCursor));
	Dialog.Add(new CFarRadioButtonItem(7,11,DIF_GROUP,MNone,(int *)&EFindTextAtCursor,FT_NONE));
	Dialog.Add(new CFarRadioButtonItem(7,12,0,MWordOnly,(int *)&EFindTextAtCursor,FT_WORD));
	Dialog.Add(new CFarRadioButtonItem(7,13,0,MAnyText,(int *)&EFindTextAtCursor,FT_ANY));

	Dialog.Add(new CFarCheckBoxItem(7,14,0,MFindSelection,&EFindSelection));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Display(-1);
}

int WINAPI Configure(int ItemNumber) {
	const char *ppszItems[]={GetMsg(MCommonSettings),GetMsg(MFileSearchSettings),GetMsg(MEditorSearchSettings)};
	int iResult = 0;
	do {
		switch (iResult = ChooseMenu(3,ppszItems,GetMsg(MRESearch),NULL,"Config",iResult)) {
		case 0:ConfigureCommon();break;
		case 1:ConfigureFile();break;
		case 2:ConfigureEditor();break;
		default:WriteRegistry();return TRUE;
		}
	} while (TRUE);
}

void WINAPI ExitFAR() {
	WriteRegistry();
	ECleanup(FALSE);
	FCleanup(FALSE);
	FTCleanup(FALSE);
	StopREThread();
	CoUninitialize();
}
