#include "StdAfx.h"
#include "..\RESearch.h"

#ifdef FAR3

void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo *Info)
{
	CTemporaryPanel *Panel = (CTemporaryPanel *)Info->hPanel;
	if (Panel) Panel->GetOpenPanelInfo(Info);
}

int WINAPI GetFindDataW(struct GetFindDataInfo *Info)
{
	CTemporaryPanel *Panel = (CTemporaryPanel *)Info->hPanel;
	return (Panel)?Panel->GetFindData(&Info->PanelItem, (int *)&Info->ItemsNumber, (int)Info->OpMode) : 0;
}

int WINAPI SetDirectoryW(const struct SetDirectoryInfo *Info)
{
	CTemporaryPanel *Panel = (CTemporaryPanel *)Info->hPanel;
	return (Panel)?Panel->_SetDirectory(Info->Dir, (int)Info->OpMode):0;
}

int WINAPI PutFilesW(const struct PutFilesInfo *Info)
{
	CTemporaryPanel *Panel = (CTemporaryPanel *)Info->hPanel;
	return (Panel)?Panel->PutFiles(Info->PanelItem, Info->ItemsNumber, Info->Move, (int)Info->OpMode):0;
}

int WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo *Info)
{
	CTemporaryPanel *Panel = (CTemporaryPanel *)Info->hPanel;
	return (Panel)?Panel->ProcessPanelInput(&Info->Rec):0;
}

void WINAPI ClosePanelW(const struct ClosePanelInfo *Info)
{
	CTemporaryPanel *Panel = (CTemporaryPanel *)Info->hPanel;
	if (Panel) Panel->ClosePlugin();
}

#else

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

#endif

// CTemporaryPanel

CTemporaryPanel::CTemporaryPanel(panelitem_vector &PanelItems,TCHAR *CalledFolder)
: m_arrItems(PanelItems),m_strBaseFolder(CalledFolder),m_bActive(true)
{
	for (size_t I=0; I<m_arrItems.size(); I++) {
		if (m_arrItems[I].UserData == 0)
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

#ifdef FAR3
void CTemporaryPanel::GetOpenPanelInfo(struct OpenPanelInfo *Info)
#else
void CTemporaryPanel::GetOpenPluginInfo(OpenPluginInfo *Info)
#endif
{
	Info->StructSize=sizeof(*Info);
#ifdef FAR3
	Info->Flags=OPIF_ADDDOTS|OPIF_SHOWRIGHTALIGNNAMES|OPIF_REALNAMES;
#else
	Info->Flags=OPIF_USEFILTER|OPIF_USESORTGROUPS|OPIF_USEHIGHLIGHTING|
				OPIF_ADDDOTS|OPIF_SHOWRIGHTALIGNNAMES|
				OPIF_REALNAMES;
#endif
	Info->HostFile=NULL;
	Info->CurDir = _T("");

	Info->Format=GetMsg(MSearchResults);
	Info->PanelTitle=GetMsg(MSearchResults);
	Info->InfoLines=NULL;
	Info->InfoLinesNumber=0;
	Info->DescrFiles=NULL;
	Info->DescrFilesNumber=0;

	Info->PanelModesArray=NULL;
	Info->PanelModesNumber=0;

	Info->StartPanelMode=TPPanelMode+'0';
#ifdef FAR3
	Info->StartSortMode=(OPENPANELINFO_SORTMODES)TPSortMode;
#else
	Info->StartSortMode=TPSortMode;
#endif
	Info->StartSortOrder=TPSortOrder;

#ifdef FAR3
	KeyBar.CountLabels = 1;
	KeyBar.Labels = KeyBarLabels;
	KeyBarLabels[0].Key.VirtualKeyCode = VK_F7;
	KeyBarLabels[0].Text = (LPTSTR)GetMsg(MF7);
#else
	memset(&KeyBar,0,sizeof(KeyBar));
	KeyBar.Titles[7-1]=(LPTSTR)GetMsg(MF7);
#endif
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
		HANDLE hFind = FindFirstFile(FarPanelFileName(m_arrItems[nItem]), &FindData);
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

int CTemporaryPanel::_SetDirectory(const TCHAR *Name,int OpMode) {
//	We don't get here upon '..', Ctrl-PgUp or Ctrl-\

	if (OpMode&OPM_FIND) return FALSE;

#ifdef UNICODE
	StartupInfo.Control((HANDLE)this, FCTL_CLOSEPLUGIN, 0, (LONG_PTR)Name);
#else
	StartupInfo.Control((HANDLE)this, FCTL_CLOSEPLUGIN, (void *)Name);
#endif

	return TRUE;
}

int CTemporaryPanel::PutFiles(PluginPanelItem *AddItems,int AddNumber,int Move,int OpMode) {
	return TRUE;
}

#ifdef FAR3
#pragma message("Not implemented in FAR3")
int CTemporaryPanel::ProcessPanelInput(const INPUT_RECORD *pInput)
{
	return TRUE;
}
#endif

int CTemporaryPanel::ProcessKey(int Key, unsigned int ControlState) {
	if ((ControlState==0) && (Key==VK_F3)) {
		CPanelInfo PInfo;
		PInfo.GetInfo((HANDLE)this);

		if (PInfo.CurrentItem <= PInfo.ItemsNumber) {
			PluginPanelItem &Item = PInfo.PanelItems[PInfo.CurrentItem];

			if ((FarPanelAttr(Item) & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				TempUserData *pData = (TempUserData *)Item.UserData;
				if (!pData || (pData->FoundLine < 0)) return FALSE;

				StartupInfo.Editor(FarPanelFileName(Item), NULL, 0, 0, -1, -1, EF_NONMODAL|EF_IMMEDIATERETURN|EF_ENABLE_F6,
					pData ? pData->FoundLine : 0, pData ? pData->FoundColumn : 1
#ifdef UNICODE
					, CP_AUTODETECT
#endif
					);

#ifdef FAR3
				INPUT_RECORD Input;
				Input.EventType = KEY_EVENT;
				Input.Event.KeyEvent.bKeyDown = true;
				Input.Event.KeyEvent.wRepeatCount = 1;
				Input.Event.KeyEvent.wVirtualKeyCode = VK_F6;
				Input.Event.KeyEvent.dwControlKeyState = 0;
				StartupInfo.EditorControl(ECTL_PROCESSINPUT, &Input);
#else
				StartupInfo.EditorControl(ECTL_PROCESSKEY, (void *)KEY_F6);
#endif
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

			if ((FarPanelAttr(Item) & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				TempUserData *pData = (TempUserData *)Item.UserData;
				if (!pData || (pData->FoundLine < 0)) return FALSE;

				StartupInfo.Editor(FarPanelFileName(Item), NULL, 0, 0, -1, -1,
					EF_NONMODAL|EF_IMMEDIATERETURN|EF_ENABLE_F6,
					pData->FoundLine+1, pData->FoundColumn
#ifdef UNICODE
					, CP_AUTODETECT
#endif
					);

				if (FSEditSrchAfterFile) {
					EditorSetPosition Position = {0, 0, 0, 0, 0, 0};
					StartupInfo.EditorControl(ECTL_SETPOSITION, &Position);
					EditorSearchAgain();
					StartupInfo.EditorControl(ECTL_REDRAW, NULL);
				}

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
				if (!_tcscmp(FarPanelFileName(PInfo.SelectedItems[J]), FarPanelFileName(m_arrItems[nItem]))) {
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
