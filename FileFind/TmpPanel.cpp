#include "StdAfx.h"
#include "..\RESearch.h"

void WINAPI GetOpenPluginInfo(HANDLE hPlugin,OpenPluginInfo *Info) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	if (Panel) Panel->GetOpenPluginInfo(Info);
}

int WINAPI GetFindData(HANDLE hPlugin,PluginPanelItem **PanelItem,int *ItemsNumber,int OpMode) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	return (Panel)?Panel->GetFindData(PanelItem,ItemsNumber,OpMode):0;
}

int WINAPI SetDirectory(HANDLE hPlugin,char *Name,int OpMode) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	return (Panel)?Panel->_SetDirectory(Name,OpMode):0;
}

int WINAPI PutFiles(HANDLE hPlugin,PluginPanelItem *AddItems,int AddNumber,int Move,int OpMode) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	return (Panel)?Panel->PutFiles(AddItems,AddNumber,Move,OpMode):0;
}

int WINAPI ProcessKey(HANDLE hPlugin,int Key,unsigned int ControlState) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	return (Panel)?Panel->ProcessKey(Key,ControlState):0;
}

void WINAPI ClosePlugin(HANDLE hPlugin) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	if (Panel) Panel->ClosePlugin();
}

// CTemporaryPanel

CTemporaryPanel::CTemporaryPanel(PluginPanelItem *NewItems,int NewCount,char *CalledFolder)
: Items(NewItems),Count(NewCount),Folder(_strdup(CalledFolder)),m_bActive(true)
{
	for (int I=0;I<Count;I++) {
		if (!Items[I].UserData) Items[I].UserData = (DWORD)new TempUserData();
	}

	if (LastTempPanel && !LastTempPanel->m_bActive) delete LastTempPanel;
	LastTempPanel = this;
}

CTemporaryPanel::~CTemporaryPanel() {
	for (int I=0;I<Count;I++) {
		if (Items[I].UserData) delete (TempUserData *)Items[I].UserData;
	}

	free(Items);
	if (Folder) free(Folder);
}

void CTemporaryPanel::GetOpenPluginInfo(OpenPluginInfo *Info) {
	static char CurrentDir[MAX_PATH];
	Info->StructSize=sizeof(*Info);
	Info->Flags=OPIF_USEFILTER|OPIF_USESORTGROUPS|OPIF_USEHIGHLIGHTING|
				OPIF_ADDDOTS|OPIF_SHOWRIGHTALIGNNAMES|
				OPIF_REALNAMES;
	Info->HostFile=NULL;
	Info->CurDir="";

	Info->Format=GetMsg(MSearchResults);
	Info->PanelTitle=GetMsg(MSearchResults);
	Info->InfoLines=NULL;
	Info->InfoLinesNumber=0;
	Info->DescrFiles=NULL;
	Info->DescrFilesNumber=0;

	Info->PanelModesArray=NULL;
	Info->PanelModesNumber=0;
	Info->StartPanelMode='4';

	memset(&KeyBar,0,sizeof(KeyBar));
	KeyBar.Titles[7-1]=(char *)GetMsg(MF7);
	Info->KeyBar=&KeyBar;
	Info->ShortcutData=Info->CurDir;
}

#define Deleted(i) ((TempUserData *)Items[I].UserData)->ToBeDeleted

void CTemporaryPanel::UpdateList() {
	int I,J,NewCount=Count;

	for (I=0;I<Count;I++) {
		if (Deleted(I)) NewCount--; else {
			char SaveName[MAX_PATH];
			strcpy(SaveName,Items[I].FindData.cFileName);
			HANDLE Find=FindFirstFile(Items[I].FindData.cFileName,&Items[I].FindData);
			FindClose(Find);
			if (Deleted(I)=(Find==INVALID_HANDLE_VALUE)) NewCount--; else {
				strcpy(Items[I].FindData.cFileName,SaveName);
				for (J=0;J<I;J++) {
					if (!_stricmp(Items[I].FindData.cFileName,Items[J].FindData.cFileName)) {
						Deleted(I)=TRUE;NewCount--;
					}
				}
			}
		}
	}

	struct PluginPanelItem *NewItems;
	NewItems=(PluginPanelItem *)malloc(NewCount*sizeof(PluginPanelItem));
	J=0;
	for (I=0;I<Count;I++) {
		if (!Deleted(I)) NewItems[J++]=Items[I];
	}
	free(Items);Items=NewItems;Count=NewCount;	
}

int CTemporaryPanel::GetFindData(PluginPanelItem **PanelItem,int *ItemsNumber,int OpMode) {
	UpdateList();
	*PanelItem=Items;
	*ItemsNumber=Count;
	return TRUE;
}

int CTemporaryPanel::_SetDirectory(char *Name,int OpMode) {
	if ((OpMode&OPM_FIND)||(strcmp(Name,"\\")==0)) return FALSE;
	StartupInfo.Control((HANDLE)this,FCTL_CLOSEPLUGIN,Name);return TRUE;
}

int CTemporaryPanel::PutFiles(PluginPanelItem *AddItems,int AddNumber,int Move,int OpMode) {
	Items=(PluginPanelItem *)realloc(Items,(Count+AddNumber)*sizeof(PluginPanelItem));
	for (int I=0;I<AddNumber;I++) {
		char PathBuffer[MAX_PATH];
		Items[I+Count]=AddItems[I];
		GetFullPathName(Items[I+Count].FindData.cFileName,MAX_PATH,PathBuffer,NULL);
		strcpy(Items[I+Count].FindData.cFileName,PathBuffer);
		Items[I+Count].UserData = (DWORD)new TempUserData();
	}
	Count+=AddNumber;UpdateList();
	return TRUE;
}

int CTemporaryPanel::ProcessKey(int Key,unsigned int ControlState) {
	if ((ControlState==0) && (Key==VK_F3)) {
		struct PanelInfo PInfo;
		StartupInfo.Control((HANDLE)this,FCTL_GETPANELINFO,&PInfo);

		if (PInfo.CurrentItem <= PInfo.ItemsNumber) {
			PluginPanelItem &Item = PInfo.PanelItems[PInfo.CurrentItem];

			if ((Item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				TempUserData *pData = (TempUserData *)Item.UserData;
				StartupInfo.Editor(Item.FindData.cFileName, NULL, 0, 0, -1, -1, EF_NONMODAL|EF_IMMEDIATERETURN|EF_ENABLE_F6,
					pData ? pData->FoundLine : 0, pData ? pData->FoundColumn : 1);

				StartupInfo.EditorControl(ECTL_PROCESSKEY, (void *)KEY_F6);
				return TRUE;
			}
		}
		return FALSE;
	}

	if ((ControlState==0) && (Key==VK_F4)) {
		struct PanelInfo PInfo;
		StartupInfo.Control((HANDLE)this,FCTL_GETPANELINFO,&PInfo);

		if (PInfo.CurrentItem <= PInfo.ItemsNumber) {
			PluginPanelItem &Item = PInfo.PanelItems[PInfo.CurrentItem];

			if ((Item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				TempUserData *pData = (TempUserData *)Item.UserData;
				StartupInfo.Editor(Item.FindData.cFileName, NULL, 0, 0, -1, -1, EF_NONMODAL|EF_IMMEDIATERETURN|EF_ENABLE_F6,
					pData ? pData->FoundLine+1 : 0, pData ? pData->FoundColumn : 1);
				return TRUE;
			}
		}
		return FALSE;
	}

	if ((ControlState==0)&&(Key==VK_F7)) {
		struct PanelInfo PInfo;

		StartupInfo.Control((HANDLE)this,FCTL_GETPANELINFO,&PInfo);
		for (int I=0;I<Count;I++) {
			for (int J=0;J<PInfo.SelectedItemsNumber;J++) {
				if (!strcmp(PInfo.SelectedItems[J].FindData.cFileName,Items[I].FindData.cFileName)) {
					Deleted(I)=TRUE;break;
				}
			}
		}

		StartupInfo.Control((HANDLE)this,FCTL_UPDATEPANEL,NULL);
		StartupInfo.Control((HANDLE)this,FCTL_REDRAWPANEL,NULL);

		StartupInfo.Control((HANDLE)this,FCTL_GETANOTHERPANELINFO,&PInfo);
		if (PInfo.PanelType==PTYPE_QVIEWPANEL) {
			StartupInfo.Control((HANDLE)this,FCTL_UPDATEANOTHERPANEL,NULL);
			StartupInfo.Control((HANDLE)this,FCTL_REDRAWANOTHERPANEL,NULL);
		}
		return TRUE;
	}
	if ((ControlState==0)&&(Key==KEY_BS)) {
		StartupInfo.Control((HANDLE)this,FCTL_CLOSEPLUGIN,Folder);return TRUE;
	}
	return FALSE;
}

void CTemporaryPanel::ClosePlugin() {
	if (LastTempPanel == this) m_bActive = false; else delete this;
}
