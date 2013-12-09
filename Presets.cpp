#include "StdAfx.h"
#include "RESearch.h"

#ifdef UNICODE
#define KeyName(name) OEMToUnicode(name)
#define NameKey(name) OEMFromUnicode(name)
#else
#define KeyName(name) name
#define NameKey(name) name
#endif

CParameterSet::CParameterSet(PresetExecutor Executor, int nStringCount, int nIntCount, ...)
: m_Executor(Executor)
{
	va_list List;
	va_start(List, nIntCount);
	
	if (nStringCount >= 0) {
		while (nStringCount--) {
			const char *szName = va_arg(List, const char *);
			m_mapStrings[szName] = va_arg(List, tstring *);
		}
	} else {
		while (true) {
			const char *szName = va_arg(List, const char *);
			if (szName != NULL)
				m_mapStrings[szName] = va_arg(List, tstring *);
			else
				break;
		} 
	}

	if (nIntCount >= 0) {
		while (nIntCount--) {
			const char *szName = va_arg(List, const char *);
			m_mapInts[szName] = va_arg(List, int *);
		}
	} else {
		while (true) {
			const char *szName = va_arg(List, const char *);
			if (szName != NULL)
				m_mapInts[szName] = va_arg(List, int *);
			else
				break;
		} 
	}
	va_end(List);
}

CParameterBackup::CParameterBackup(CParameterSet &Set, bool bAutoRestore)
: m_Set(Set), m_bAutoRestore(bAutoRestore)
{
	for (map<string, tstring *>::iterator its = m_Set.m_mapStrings.begin(); its != m_Set.m_mapStrings.end(); its++) {
		m_mapStrings[its->first] = *(its->second);
	}

	for (map<string, int *>::iterator iti = m_Set.m_mapInts.begin(); iti != m_Set.m_mapInts.end(); iti++) {
		m_mapInts[iti->first] = *(iti->second);
	}
}

void CParameterBackup::Restore() {
	for (map<string, tstring *>::iterator its = m_Set.m_mapStrings.begin(); its != m_Set.m_mapStrings.end(); its++) {
		*(its->second) = m_mapStrings[its->first];
	}

	for (map<string, int *>::iterator iti = m_Set.m_mapInts.begin(); iti != m_Set.m_mapInts.end(); iti++) {
		*(iti->second) = m_mapInts[iti->first];
	}

	m_bAutoRestore = false;
}

CParameterBackup::~CParameterBackup() {
	if (m_bAutoRestore) Restore();
}

//////////////////////////////////////////////////////////////////////////

CPreset::CPreset(CParameterSet &ParamSet)
: m_ParamSet(ParamSet), m_nID(0)
{
	FillDefaults();

	m_mapStrings[""] = _T("New Preset");
	m_bAddToMenu = false;
}

void CPreset::FillDefaults()
{
	map<string, tstring *>::iterator it1 = m_ParamSet.m_mapStrings.begin();
	while (it1 != m_ParamSet.m_mapStrings.end()) {
		m_mapStrings[it1->first] = *(it1->second);
		it1++;
	}

	map<string, int *>::iterator it2 = m_ParamSet.m_mapInts.begin();
	while (it2 != m_ParamSet.m_mapInts.end()) {
		m_mapInts[it2->first] = *(it2->second);
		it2++;
	}
}

void CPreset::CopyFrom(const CPreset &Preset)
{
	m_mapStrings = Preset.m_mapStrings;
	m_mapInts    = Preset.m_mapInts;
	m_bAddToMenu = Preset.m_bAddToMenu;
}

CPreset::CPreset(CParameterSet &ParamSet, const tstring &strName, CFarSettingsKey &hKey)
: m_ParamSet(ParamSet)
, m_nID(0)
, m_bAddToMenu(false)
{
	FillDefaults();

	CFarSettingsKey hOwnKey = hKey.Open(strName.c_str(), false);
	if (!hOwnKey.Valid()) return;

	hOwnKey.QueryIntValue (_T("ID"), m_nID);
	hOwnKey.QueryBoolValue(_T("AddToMenu"), m_bAddToMenu);
	hOwnKey.QueryStringValue(_T(""), m_mapStrings[""]);

	for (map<string, tstring>::iterator it = m_mapStrings.begin(); it != m_mapStrings.end(); it++) {
		hOwnKey.QueryStringValue(KeyName(it->first).c_str(), it->second);
	}
	for (map<string, int>::iterator it = m_mapInts.begin(); it != m_mapInts.end(); it++) {
		hOwnKey.QueryIntValue(KeyName(it->first).c_str(), it->second);
	}
}

OperationResult CPreset::ExecutePreset() {
	CParameterBackup _Backup(m_ParamSet);
	Apply();
	return m_ParamSet.m_Executor();
}

void CPreset::Apply() {
	map<string, tstring *>::iterator it1 = m_ParamSet.m_mapStrings.begin();
	while (it1 != m_ParamSet.m_mapStrings.end()) {
		if (it1->first[0] != '@') {
			map<string, tstring>::iterator it = m_mapStrings.find(it1->first);
			if (it != m_mapStrings.end()) *(it1->second) = it->second;
		}
		it1++;
	}

	map<string, int *>::iterator it2 = m_ParamSet.m_mapInts.begin();
	while (it2 != m_ParamSet.m_mapInts.end()) {
		if (it2->first[0] != '@') {
			map<string, int>::iterator it = m_mapInts.find(it2->first);
			if (it != m_mapInts.end()) *(it2->second) = it->second;
		}
		it2++;
	}
}

void CPreset::FillMenuItem(CFarMenuItemEx &Item) {
#ifdef UNICODE
	Item.Text = _tcsdup(Name().c_str());
#else
	strcat(strcpy(Item.Text.Text, GetMsg(MMenuPreset)), Name().c_str());
	Item.Flags = 0;
#endif
}

void CPreset::Save(CFarSettingsKey &hKey, int nIndex)
{
	CFarSettingsKey hOwnKey = hKey.Open(FormatStr(_T("%04d"), nIndex).c_str());
	if (!hOwnKey.Valid()) return;

	map<string, tstring>::iterator it1 = m_mapStrings.begin();
	while (it1 != m_mapStrings.end()) {
		if (it1->first[0] != '@') hOwnKey.SetStringValue(KeyName(it1->first).c_str(), it1->second);
		it1++;
	}

	map<string, int>::iterator it2 = m_mapInts.begin();
	while (it2 != m_mapInts.end()) {
		if (it2->first[0] != '@') hOwnKey.SetIntValue(KeyName(it2->first).c_str(), it2->second);
		it2++;
	}

	hOwnKey.SetIntValue (_T("ID"), m_nID);
	hOwnKey.SetBoolValue(_T("AddToMenu"), m_bAddToMenu);
}

//////////////////////////////////////////////////////////////////////////

CPresetCollection::CPresetCollection(CParameterSet &ParamSet, const TCHAR *strKey, int nTitle)
: m_ParamSet(ParamSet)
, m_strKey(strKey)
, m_nTitle(nTitle)
, m_nCurrent(0)
{
}

void CPresetCollection::Load()
{
	CFarSettingsKey hRoot = GetSettings();
	CFarSettingsKey hKey = hRoot.Open(FormatStr(_T("%sPresets"), Name()).c_str(), false);
	if (!hKey.Valid()) return;

	hKey.StartEnumKeys();
	do {
		tstring strName;
		if (!hKey.GetNextEnum(strName)) break;
		push_back(LoadPreset(strName.c_str(), hKey));
	} while (TRUE);

	ValidateIDs();
}

CPresetCollection::~CPresetCollection()
{
	for (size_t nPreset=0; nPreset < size(); nPreset++) delete at(nPreset);
}

void CPresetCollection::Save()
{
	CFarSettingsKey hRoot = GetSettings();
	CFarSettingsKey hKey = hRoot.Open(FormatStr(_T("%sPresets"), Name()).c_str());
	if (!hKey.Valid()) return;

	hKey.DeleteAllKeys();

	for (size_t nPreset=0; nPreset<size(); nPreset++)
		at(nPreset)->Save(hKey, nPreset);
}

int CPresetCollection::ShowMenu(bool bExecute, int nDefaultID)
{
	int piBreakKeys[]={VK_INSERT, VK_ADD, VK_DELETE, VK_F4, VK_F5, (PKF_CONTROL<<16)|VK_UP, (PKF_CONTROL<<16)|VK_DOWN, (PKF_CONTROL<<16)|VK_RETURN, 0};
	vector<tstring> arrItems;

	do {
		arrItems.resize(size());
		for (size_t nPreset = 0; nPreset < size(); nPreset++) {
			CPreset *pPreset = at(nPreset);
			arrItems[nPreset] = pPreset->Name();
			if (pPreset->m_nID == nDefaultID) m_nCurrent = nPreset;
		}
		arrItems.push_back(tstring());

		int nBreakKey;
		tstring strTitle = FormatStr(_T("%s presets"), Name());
		int nResult = ChooseMenu(arrItems, strTitle.c_str(), _T("Ins,Del,F4,F5,Ctrl-\x18\x19,Ctrl-Enter"), _T("Presets"), m_nCurrent,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);
		if (nResult >= 0) m_nCurrent = nResult;

		switch (nBreakKey) {
		case -1:
			if (nResult >= (int)size()) break;	//	Enter in empty menu or last item
			if (bExecute && (nResult >= 0)) at(nResult)->Apply();
			return nResult;
		case 0:
		case 1:{		//	VK_INSERT
			CPreset *pPreset = NewPreset();
			if (EditPreset(pPreset)) {
				if (nBreakKey == 1) m_nCurrent = size();
				pPreset->m_nID = FindUnusedID();
				insert(begin() + m_nCurrent, pPreset);
				Save();
			} else {
				delete pPreset;
			}
			break;
			  }
		case 2:			//	VK_DELETE
			if (m_nCurrent < size()) {
				const TCHAR *Lines[]={_T("Delete"), GetMsg(MDeletePresetQuery),
					at(m_nCurrent)->Name().c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(FMSG_WARNING, _T("DeletePreset"), Lines, 5, 2)==0) {
					delete at(m_nCurrent);
					erase(begin() + m_nCurrent);
					Save();
				}
			}
			break;
		case 3:			//	VK_F4
			if (m_nCurrent < size()) {
				if (EditPreset(at(m_nCurrent))) Save();
			}
			break;
		case 4:			//	VK_F5
			if (m_nCurrent < size()) {
				CPreset *pPreset = NewPreset();
				pPreset->CopyFrom(*at(m_nCurrent));
				pPreset->Name() = GetMsg(MCopyOf) + pPreset->Name();
				if (EditPreset(pPreset)) {
					pPreset->m_nID = FindUnusedID();
					insert(begin() + m_nCurrent + 1, pPreset);
					Save();
				}
			}
			break;
		case 5:			//	VK_CTRL_UP
			if (m_nCurrent > 0) {
				CPreset *pPreset = at(m_nCurrent-1);
				at(m_nCurrent-1) = at(m_nCurrent);
				at(m_nCurrent) = pPreset;
				Save();
				m_nCurrent--;
			}
			break;
		case 6:			//	VK_CTRL_DOWN
			if (m_nCurrent < size()-1) {
				CPreset *pPreset = at(m_nCurrent+1);
				at(m_nCurrent+1) = at(m_nCurrent);
				at(m_nCurrent) = pPreset;
				Save();
				m_nCurrent++;
			}
			break;
		case 7:			//	VK_CTRLENTER
			if (m_nCurrent < size()) {
				at(m_nCurrent)->ExecutePreset();
			}
			break;
		}
	} while (true);
}

CPreset *CPresetCollection::operator()(int nID) {
	for (size_t nPreset = 0; nPreset < size(); nPreset++) {
		if (at(nPreset)->m_nID == nID) return at(nPreset);
	}
	return NULL;
}

void CPresetCollection::FillMenuItems(vector<CFarMenuItemEx> &MenuItems) {
	for (size_t nPreset = 0; nPreset < size(); nPreset++) {
		CPreset *pPreset = at(nPreset);
		if (pPreset->m_bAddToMenu) {
			CFarMenuItemEx Item;
			pPreset->FillMenuItem(Item);
			MenuItems.push_back(Item);
		}
	}
}

CPreset *CPresetCollection::FindMenuPreset(int &nIndex) {
	for (size_t nPreset = 0; nPreset < size(); nPreset++) {
		CPreset *pPreset = at(nPreset);
		if (pPreset->m_bAddToMenu) {
			if (nIndex == 0) return pPreset; else nIndex--;
		}
	}
	return NULL;
}

int CPresetCollection::FindUnusedID() {
	int nID = 1;
	for (size_t nPreset = 0; nPreset < size(); nPreset++) {
		int nCurID = at(nPreset)->m_nID;
		if (nCurID >= nID) nID = nCurID+1;
	}
	return nID;
}

void CPresetCollection::ValidateIDs() {
	for (size_t nPreset = 0; nPreset < size(); nPreset++) {
		if (at(nPreset)->m_nID == 0)
			at(nPreset)->m_nID = FindUnusedID();
	}
}

//////////////////////////////////////////////////////////////////////////

const BatchActionIndex NO_BATCH_INDEX(-1, -1);

CBatchType::CBatchType(int nTitle, ...)
: m_nTitle(nTitle)
{
	va_list List;
	va_start(List, nTitle);

	CPresetCollection *pColl;
	while ((pColl = va_arg(List, CPresetCollection *)) != NULL) {
		push_back(pColl);
	}

	va_end(List);
}

CPresetCollection *CBatchType::operator()(int nCollID) {
	for (size_t nColl = 0; nColl < size(); nColl++) {
		if (at(nColl)->ID() == nCollID)
			return at(nColl);
	}
	return NULL;
}

CPreset *CBatchType::operator[](const BatchActionIndex &Pair) {
	for (size_t nColl = 0; nColl < size(); nColl++) {
		if (at(nColl)->ID() == Pair.first) {
			return (*at(nColl))(Pair.second);
		}
	}
	return NULL;
}

BatchActionIndex CBatchType::SelectPreset() {
	vector<tstring> arrItems;

	for (size_t nColl = 0; nColl < size(); nColl++) {
		CPresetCollection *pColl = at(nColl);
		arrItems.push_back(pColl->Title());
	}

	int nResult = 0;
	do {
		nResult = ChooseMenu(arrItems, GetMsg(m_nTitle), NULL, NULL, nResult,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, NULL, NULL);

		if (nResult < 0) return NO_BATCH_INDEX;

		CPresetCollection *pColl = at(nResult);
		int nPreset = pColl->ShowMenu(false);
		if (nPreset >= 0) {
			CPreset *pPreset = pColl->at(nPreset);
			return BatchActionIndex(pColl->ID(), pPreset->m_nID);
		}
	} while (true);
}

//////////////////////////////////////////////////////////////////////////

CBatchAction::CBatchAction(CBatchType &Type)
: m_Type(Type), m_bAddToMenu(false)
{
}

CBatchAction::CBatchAction(CBatchType &Type, tstring strName, CFarSettingsKey &hKey)
: m_Type(Type)
, m_strName(strName)
, m_bAddToMenu(false)
{
	CFarSettingsKey hOwnKey = hKey.Open(strName.c_str(), false);
	if (!hOwnKey.Valid()) return;

	hOwnKey.QueryStringValue(_T("Name"), m_strName);
	hOwnKey.QueryBoolValue(  _T("AddToMenu"), m_bAddToMenu);

	for (size_t nIndex = 0; ; nIndex++) {
		CFarSettingsKey hValue = hOwnKey.Open(FormatStr(_T("%04d"), nIndex).c_str(), false);
		if (!hValue.Valid()) break;

		BatchActionIndex NewIndex = NO_BATCH_INDEX;
		hValue.QueryIntValue(_T("Coll"), NewIndex.first);
		hValue.QueryIntValue(_T("ID"),   NewIndex.second);

		push_back(NewIndex);
	}
}

CBatchActionCollection *CBatchAction::Collection()
{
	if (&m_Type == g_pEditorBatchType)
		return g_pEditorBatches;
	else if (&m_Type == g_pPanelBatchType)
		return g_pPanelBatches;
	else
		return NULL;
}

void CBatchAction::Save(CFarSettingsKey &hKey, int nIndex)
{
	CFarSettingsKey hOwnKey = hKey.Open(FormatStr(_T("%04d"), nIndex).c_str(), true);
	if (!hOwnKey.Valid()) return;

	hOwnKey.SetStringValue(_T("Name"), m_strName);
	hOwnKey.SetBoolValue  (_T("AddToMenu"), m_bAddToMenu);

	hOwnKey.DeleteAllKeys();

	for (size_t nActIndex = 0; nActIndex < size(); nActIndex++) {
		CFarSettingsKey hValue = hOwnKey.Open(FormatStr(_T("%04d"), nActIndex).c_str(), true);
		if (!hValue.Valid()) continue;

		hValue.SetIntValue(_T("Coll"), at(nActIndex).first);
		hValue.SetIntValue(_T("ID"),   at(nActIndex).second);
	}
}

bool CBatchAction::EditProperties() {
	CFarDialog Dialog(60, 12, _T("BatchProperties"));
	Dialog.AddFrame(MBatch);
	Dialog.Add(new CFarTextItem(5, 3, 0, MBatchName));
	Dialog.Add(new CFarEditItem(5, 4, 53, DIF_HISTORY, _T("SearchText"), m_strName));
	Dialog.Add(new CFarCheckBoxItem(5, 6, 0, MAddToMenu, &m_bAddToMenu));
	
	Dialog.AddButtons(MOk, MCancel);
	return Dialog.Display(-1) == 0;
}

int CountPresets(CBatchActionCollection &Coll, CPreset *pPreset)
{
	int nCount = 0;

	for (size_t nAction = 0; nAction < Coll.size(); nAction++) {
		CBatchAction &Action = *Coll[nAction];
		for (size_t nPreset = 0; nPreset < Action.size(); nPreset++) {
			if (Coll.m_Type[Action[nPreset]] == pPreset)
				nCount++;
		}
	}

	return nCount;
}

bool CBatchAction::EditItems()
{
	int piBreakKeys[] = {VK_INSERT, VK_ADD, (PKF_CONTROL<<16)|VK_UP, (PKF_CONTROL<<16)|VK_DOWN, VK_DELETE, VK_F4,
		(PKF_CONTROL<<16)|VK_RETURN, ((PKF_CONTROL|PKF_ALT)<<16)|VK_RETURN, 0};

	//	Clean-up nonexistent presets
	for (size_t nPreset = 0; nPreset < size(); nPreset++) {
		CPreset *pPreset = m_Type[at(nPreset)];
		if (pPreset == NULL) {
			erase(begin()+nPreset);
			nPreset--;
			continue;
		}
	}
	vector<bool> arrExecuted(size());

	do {
		vector<CFarMenuItemEx> arrItems;
		arrItems.resize(size()+1);
		for (size_t nPreset = 0; nPreset < size(); nPreset++)
		{
			arrItems[nPreset].SetText(m_Type[at(nPreset)]->Name().c_str());
			if (arrExecuted[nPreset]) arrItems[nPreset].Check(true);
		}
		if (m_nCurrent < arrItems.size()) arrItems[m_nCurrent].Select(true);

		int nBreakKey;
		tstring strTitle = FormatStr(GetMsg(MBatchCommands), RemoveAmpersand(m_strName).c_str());

		int nResult = StartupInfo.Menu(-1, -1, 0, FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT|FMENU_USEEXT, strTitle.c_str(), _T("Ins,Ctrl-\x18\x19,Ctrl-Enter,Del"), _T("Batch"),
			piBreakKeys, &nBreakKey, (const FarMenuItem *)&arrItems[0], arrItems.size());
		if (nResult >= 0) m_nCurrent = nResult;

		switch (nBreakKey) {
		case -1:
			return size() > 0;
		case 0:
		case 1:{		//	VK_INSERT
			BatchActionIndex NewIndex = m_Type.SelectPreset();
			if (NewIndex != NO_BATCH_INDEX) {
				if (nBreakKey == 1) m_nCurrent = size();
				insert(begin() + m_nCurrent, NewIndex);
				arrExecuted.insert(arrExecuted.begin() + m_nCurrent, false);
			}
			break;
			   }
		case 2:			//	VK_CTRL_UP
			if (m_nCurrent > 0) {
				value_type nSave = at(m_nCurrent-1);
				at(m_nCurrent-1) = at(m_nCurrent);
				at(m_nCurrent) = nSave;
				m_nCurrent--;
			}
			break;
		case 3:			//	VK_CTRL_DOWN
			if (m_nCurrent < size()-1) {
				value_type nSave = at(m_nCurrent+1);
				at(m_nCurrent+1) = at(m_nCurrent);
				at(m_nCurrent) = nSave;
				m_nCurrent++;
			}
			break;
		case 4:			//	VK_DEL
			if (m_nCurrent < size()) {
				erase(begin() + m_nCurrent);
				arrExecuted.erase(arrExecuted.begin() + m_nCurrent);
			}
			break;
		case 5:			//	VK_F4
			if (m_nCurrent < size()) {

				BatchActionIndex Index = at(m_nCurrent);
				CPresetCollection *pColl = m_Type(Index.first);
				CPreset *pPreset = (*pColl)(Index.second);
				if (pPreset == NULL) break;

				if (pPreset->m_bAddToMenu || (CountPresets(*Collection(), pPreset) > 1)) {
					const TCHAR *Lines[] = { GetMsg(MWarning),GetMsg(MUsedPreset),GetMsg(MOk),GetMsg(MCancel) };
					if (StartupInfo.Message(FMSG_WARNING,_T(""),Lines,4,2) != 0) break;
				}

				if (pColl->EditPreset(pPreset))
					pColl->Save();
			}
			break;
		case 6:			//	VK_CTRLENTER
			if (m_nCurrent < size()) {
				BatchActionIndex Index = at(m_nCurrent);
				CPresetCollection *pColl = m_Type(Index.first);
				CPreset *pPreset = (*pColl)(Index.second);
				if (pPreset == NULL) break;

				arrExecuted[m_nCurrent] = true;
				pPreset->ExecutePreset();
			}
			break;
		case 7:			//	VK_CTRLALTENTER
			for (size_t nPreset = 0; (nPreset <= m_nCurrent) && (nPreset < size()); nPreset++) {
				BatchActionIndex Index = at(nPreset);
				CPresetCollection *pColl = m_Type(Index.first);
				CPreset *pPreset = (*pColl)(Index.second);
				if (pPreset == NULL) continue;

				arrExecuted[nPreset] = true;
				pPreset->ExecutePreset();
			}
			break;
		}
	} while (true);
}

void CBatchAction::Execute() {
	for (size_t nAction = 0; nAction < size(); nAction++) {
		CPreset *pPreset = m_Type[at(nAction)];
		if (pPreset == NULL) continue;
		if (pPreset->ExecutePreset() != OR_OK) break;
	}
}

CFarMenuItemEx CBatchAction::GetMenuItem() {
	return CFarMenuItemEx(GetMsg(MMenuBatch) + m_strName);
}

//////////////////////////////////////////////////////////////////////////

CBatchActionCollection::CBatchActionCollection(CBatchType &Type, CFarSettingsKey &hKey)
: m_Type(Type)
, m_nCurrent(0)
{
	hKey.StartEnumKeys();

	do {
		tstring strName;
		if (!hKey.GetNextEnum(strName)) break;
		push_back(new CBatchAction(m_Type, strName.c_str(), hKey));
	} while (TRUE);
}

void CBatchActionCollection::Save(CFarSettingsKey &hKey)
{
	hKey.DeleteAllKeys();

	for (size_t nIndex = 0; nIndex < size(); nIndex++) {
		at(nIndex)->Save(hKey, nIndex);
	}
}

void CBatchActionCollection::ShowMenu()
{
	int piBreakKeys[]={VK_INSERT, VK_ADD, VK_DELETE, VK_F4, VK_F6, (PKF_CONTROL<<16)|VK_UP, (PKF_CONTROL<<16)|VK_DOWN, 0};
	vector<tstring> arrItems;

	do {
		arrItems.resize(size());
		for (size_t nBatch = 0; nBatch < size(); nBatch++)
			arrItems[nBatch] = at(nBatch)->m_strName;
		arrItems.push_back(tstring());

		int nBreakKey;
		int nResult = ChooseMenu(arrItems, GetMsg(m_Type.m_nTitle), _T("Ins,Del,F4,F6,Ctrl-\x18\x19"), _T("Batches"), m_nCurrent,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);
		if (nResult >= 0) m_nCurrent = nResult;

		switch (nBreakKey) {
		case -1:
			if (nResult >= (int)size()) break;

			if (nResult >= 0) {
				CBatchAction *pBatch = at(nResult);
				const TCHAR *Lines[]={_T("Execute"), GetMsg(MExecuteBatchQuery),
					pBatch->m_strName.c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(FMSG_WARNING, _T("ExecuteBatch"), Lines, 5, 2) != 0) break;

				pBatch->Execute();
			}

			return;
		case 0:
		case 1:{		//	VK_INSERT
			CBatchAction *pBatch = new CBatchAction(m_Type);
			pBatch->m_strName = _T("New Batch");
			if (pBatch->EditProperties()) {
				if (nBreakKey == 1) m_nCurrent = size();
				insert(begin()+m_nCurrent, pBatch);
				WriteRegistry();
			} else {
				delete pBatch;
			}
			break;
			   }
		case 2:			//	VK_DELETE
			if (m_nCurrent < size()) {
				const TCHAR *Lines[]={_T("Delete"), GetMsg(MDeleteBatchQuery),
					at(m_nCurrent)->m_strName.c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(FMSG_WARNING, _T("DeleteBatch"), Lines, 5, 2)==0) {
					delete at(m_nCurrent);
					erase(begin() + m_nCurrent);
					WriteRegistry();
				}
			}
			break;
		case 3:			//	VK_F4
			if (m_nCurrent < size()) {
				at(m_nCurrent)->EditItems();
				WriteRegistry();
			}
			break;
		case 4:			//	VK_F6
			if (m_nCurrent < size()) {
				at(m_nCurrent)->EditProperties();
				WriteRegistry();
			}
			break;
		case 5:			//	VK_CTRL_UP
			if (m_nCurrent > 0) {
				CBatchAction *pBatch = at(m_nCurrent-1);
				at(m_nCurrent-1) = at(m_nCurrent);
				at(m_nCurrent) = pBatch;
				WriteRegistry();
				m_nCurrent--;
			}
			break;
		case 6:			//	VK_CTRL_DOWN
			if (m_nCurrent < size()-1) {
				CBatchAction *pBatch = at(m_nCurrent+1);
				at(m_nCurrent+1) = at(m_nCurrent);
				at(m_nCurrent) = pBatch;
				WriteRegistry();
				m_nCurrent++;
			}
			break;
		}
	} while (true);
}

void CBatchActionCollection::FillMenuItems(vector<CFarMenuItemEx> &MenuItems) {
	for (size_t nAction = 0; nAction < size(); nAction++) {
		CBatchAction *pAction = at(nAction);
		if (pAction->m_bAddToMenu) {
			MenuItems.push_back(pAction->GetMenuItem());
		}
	}
}

CBatchAction *CBatchActionCollection::FindMenuAction(int &nIndex) {
	for (size_t nAction = 0; nAction < size(); nAction++) {
		CBatchAction *pAction = at(nAction);
		if (pAction->m_bAddToMenu) {
			if (nIndex == 0) return pAction; else nIndex--;
		}
	}
	return NULL;
}
