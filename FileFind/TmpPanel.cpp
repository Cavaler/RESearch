#include "StdAfx.h"
#include "..\RESearch.h"

void WINAPI FAR_EXPORT(GetOpenPluginInfo)(HANDLE hPlugin,OpenPluginInfo *Info) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	if (Panel) Panel->GetOpenPluginInfo(Info);
}

int WINAPI FAR_EXPORT(GetFindData)(HANDLE hPlugin,PluginPanelItem **PanelItem,int *ItemsNumber,int OpMode) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	return (Panel)?Panel->GetFindData(PanelItem,ItemsNumber,OpMode):0;
}

int WINAPI FAR_EXPORT(SetDirectory)(HANDLE hPlugin,TCHAR *Name,int OpMode) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	return (Panel)?Panel->_SetDirectory(Name,OpMode):0;
}

int WINAPI FAR_EXPORT(PutFiles)(HANDLE hPlugin,PluginPanelItem *AddItems,int AddNumber,int Move,int OpMode) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	return (Panel)?Panel->PutFiles(AddItems,AddNumber,Move,OpMode):0;
}

int WINAPI FAR_EXPORT(ProcessKey)(HANDLE hPlugin,int Key,unsigned int ControlState) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	return (Panel)?Panel->ProcessKey(Key,ControlState):0;
}

void WINAPI FAR_EXPORT(ClosePlugin)(HANDLE hPlugin) {
	CTemporaryPanel *Panel=(CTemporaryPanel *)hPlugin;
	if (Panel) Panel->ClosePlugin();
}

// CTemporaryPanel

CTemporaryPanel::CTemporaryPanel(panelitem_vector &PanelItems,TCHAR *CalledFolder)
: m_arrItems(PanelItems),m_strBaseFolder(CalledFolder),m_bActive(true)
{
	for (size_t I=0; I<m_arrItems.size(); I++) {
		m_arrItems[I].UserData = (DWORD)new TempUserData();
	}

	if (LastTempPanel && !LastTempPanel->m_bActive) delete LastTempPanel;
	LastTempPanel = this;

	m_Mode.Assign(INVALID_HANDLE_VALUE);
}

CTemporaryPanel::~CTemporaryPanel() {
	for (size_t I=0; I<m_arrItems.size(); I++) {
		if (m_arrItems[I].UserData) delete (TempUserData *)m_arrItems[I].UserData;
	}
}

void CTemporaryPanel::GetOpenPluginInfo(OpenPluginInfo *Info) {
	static TCHAR CurrentDir[MAX_PATH];

	CPanelInfo PInfo;
	PInfo.GetInfo((HANDLE)this);
	if (PInfo.CurrentItem > 0) {
		m_strCurFolder = FarFileName(PInfo.PanelItems[PInfo.CurrentItem].FindData);
		size_t nPos = m_strCurFolder.rfind('\\');
		if (nPos != tstring::npos) m_strCurFolder.erase(nPos);
	} else {
		m_strCurFolder = m_strBaseFolder;
	}

	Info->StructSize=sizeof(*Info);
	Info->Flags=OPIF_USEFILTER|OPIF_USESORTGROUPS|OPIF_USEHIGHLIGHTING|
				OPIF_ADDDOTS|OPIF_SHOWRIGHTALIGNNAMES|
				OPIF_REALNAMES;
	Info->HostFile=NULL;
	Info->CurDir = m_strCurFolder.c_str();

	Info->Format=GetMsg(MSearchResults);
	Info->PanelTitle=GetMsg(MSearchResults);
	Info->InfoLines=NULL;
	Info->InfoLinesNumber=0;
	Info->DescrFiles=NULL;
	Info->DescrFilesNumber=0;

	Info->PanelModesArray=NULL;
	Info->PanelModesNumber=0;

	Info->StartPanelMode=TPPanelMode+'0';
	Info->StartSortMode=TPSortMode;
	Info->StartSortOrder=TPSortOrder;

	memset(&KeyBar,0,sizeof(KeyBar));
	KeyBar.Titles[7-1]=(LPTSTR)GetMsg(MF7);
	Info->KeyBar=&KeyBar;
	Info->ShortcutData=Info->CurDir;
}

#define Deleted(i) ((TempUserData *)m_arrItems[i].UserData)->ToBeDeleted

void CTemporaryPanel::UpdateList() {
	for (int nItem = m_arrItems.size()-1; nItem >= 0; nItem--) {
		if (Deleted(nItem)) {
			m_arrItems.erase(m_arrItems.begin()+nItem);
			continue;
		}

		WIN32_FIND_DATA FindData;
		HANDLE hFind = FindFirstFile(FarFileName(m_arrItems[nItem].FindData), &FindData);
		FindClose(hFind);
		if (hFind == INVALID_HANDLE_VALUE) {
			m_arrItems.erase(m_arrItems.begin()+nItem);
		}
	}
}

int CTemporaryPanel::GetFindData(PluginPanelItem **PanelItem,int *ItemsNumber,int OpMode) {
	UpdateList();
	*PanelItem  = m_arrItems.empty() ? NULL : &m_arrItems[0];
	*ItemsNumber = m_arrItems.size();
	return TRUE;
}

int CTemporaryPanel::_SetDirectory(TCHAR *Name,int OpMode) {
	if ((OpMode&OPM_FIND) || (_tcscmp(Name,_T("\\"))==0)) return FALSE;

	if (_tcscmp(Name, _T("..")) != 0)
		m_strCurFolder = Name;

#ifdef UNICODE
	StartupInfo.Control((HANDLE)this, FCTL_CLOSEPLUGIN, 0, (LONG_PTR)m_strCurFolder.c_str());
//	StartupInfo.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
#else
	StartupInfo.Control((HANDLE)this, FCTL_CLOSEPLUGIN, (void *)m_strCurFolder.c_str());
//	StartupInfo.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL,  NULL);
#endif

	return TRUE;
}

int CTemporaryPanel::PutFiles(PluginPanelItem *AddItems,int AddNumber,int Move,int OpMode) {
	return TRUE;
}

int CTemporaryPanel::ProcessKey(int Key,unsigned int ControlState) {
	if ((ControlState==0) && (Key==VK_F3)) {
		CPanelInfo PInfo;
		PInfo.GetInfo((HANDLE)this);

		if (PInfo.CurrentItem <= PInfo.ItemsNumber) {
			PluginPanelItem &Item = PInfo.PanelItems[PInfo.CurrentItem];

			if ((Item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				TempUserData *pData = (TempUserData *)Item.UserData;
				StartupInfo.Editor(FarFileName(Item.FindData), NULL, 0, 0, -1, -1, EF_NONMODAL|EF_IMMEDIATERETURN|EF_ENABLE_F6,
					pData ? pData->FoundLine : 0, pData ? pData->FoundColumn : 1
#ifdef UNICODE
					, CP_AUTODETECT
#endif
					);

				StartupInfo.EditorControl(ECTL_PROCESSKEY, (void *)KEY_F6);
				return TRUE;
			}
		}
		return FALSE;
	}

	if ((ControlState==0) && (Key==VK_F4)) {
		CPanelInfo PInfo;
		PInfo.GetInfo((HANDLE)this);

		if (PInfo.CurrentItem <= PInfo.ItemsNumber) {
			PluginPanelItem &Item = PInfo.PanelItems[PInfo.CurrentItem];

			if ((Item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				TempUserData *pData = (TempUserData *)Item.UserData;
				StartupInfo.Editor(FarFileName(Item.FindData), NULL, 0, 0, -1, -1, EF_NONMODAL|EF_IMMEDIATERETURN|EF_ENABLE_F6,
					pData ? pData->FoundLine+1 : 0, pData ? pData->FoundColumn : 1
#ifdef UNICODE
					, CP_AUTODETECT
#endif
					);
				return TRUE;
			}
		}
		return FALSE;
	}

	if ((ControlState==0)&&(Key==VK_F7)) {
		CPanelInfo PInfo;
		PInfo.GetInfo((HANDLE)this);

		for (size_t nItem = 0; nItem < m_arrItems.size(); nItem++) {
			for (int J=0; J<PInfo.SelectedItemsNumber; J++) {
				if (!_tcscmp(FarFileName(PInfo.SelectedItems[J].FindData), FarFileName(m_arrItems[nItem].FindData))) {
					Deleted(nItem)=TRUE;break;
				}
			}
		}

#ifdef UNICODE
		StartupInfo.Control((HANDLE)this, FCTL_UPDATEPANEL, 0, NULL);
		StartupInfo.Control((HANDLE)this, FCTL_REDRAWPANEL, 0, NULL);

		PInfo.GetInfo(true);
		if (PInfo.PanelType==PTYPE_QVIEWPANEL) {
			StartupInfo.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, NULL);
			StartupInfo.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, NULL);
		}
#else
		StartupInfo.Control((HANDLE)this,FCTL_UPDATEPANEL,NULL);
		StartupInfo.Control((HANDLE)this,FCTL_REDRAWPANEL,NULL);

		StartupInfo.Control((HANDLE)this,FCTL_GETANOTHERPANELINFO,&PInfo);
		if (PInfo.PanelType==PTYPE_QVIEWPANEL) {
			StartupInfo.Control((HANDLE)this,FCTL_UPDATEANOTHERPANEL,NULL);
			StartupInfo.Control((HANDLE)this,FCTL_REDRAWANOTHERPANEL,NULL);
		}
#endif
		return TRUE;
	}
	return FALSE;
}

void CTemporaryPanel::ClosePlugin() {
	CPanelInfo PInfo;
	if (PInfo.GetInfo(false)) {
		TPPanelMode = PInfo.ViewMode;
		TPSortMode  = PInfo.SortMode;
		TPSortOrder = (PInfo.Flags & PFLAGS_REVERSESORTORDER) ? 1 : 0;
		WriteRegistry();
	}

	if (LastTempPanel == this) m_bActive = false; else delete this;

	m_Mode.Apply(INVALID_HANDLE_VALUE);
}
