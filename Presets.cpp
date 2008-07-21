#include "StdAfx.h"
#include "RESearch.h"

CParameterSet::CParameterSet(PresetExecutor Executor, int nStringCount, int nIntCount, ...)
: m_Executor(Executor)
{
	va_list List;
	va_start(List, nIntCount);
	while (nStringCount--) {
		const char *szName = va_arg(List, const char *);
		m_mapStrings[szName] = va_arg(List, string *);
	}
	while (nIntCount--) {
		const char *szName = va_arg(List, const char *);
		m_mapInts[szName] = va_arg(List, int *);
	}
	va_end(List);
}

CParameterBackup::CParameterBackup(CParameterSet &Set, bool bAutoRestore)
: m_Set(Set), m_bAutoRestore(bAutoRestore)
{
	for (map<string, string *>::iterator its = m_Set.m_mapStrings.begin(); its != m_Set.m_mapStrings.end(); its++) {
		m_mapStrings[its->first] = *(its->second);
	}

	for (map<string, int *>::iterator iti = m_Set.m_mapInts.begin(); iti != m_Set.m_mapInts.end(); iti++) {
		m_mapInts[iti->first] = *(iti->second);
	}
}

void CParameterBackup::Restore() {
	for (map<string, string *>::iterator its = m_Set.m_mapStrings.begin(); its != m_Set.m_mapStrings.end(); its++) {
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

CPreset::CPreset(CParameterSet &ParamSet)
: m_ParamSet(ParamSet), m_nID(0)
{
	m_mapStrings[""] = "New Preset";
	m_bAddToMenu = false;

	map<string, string *>::iterator it1 = ParamSet.m_mapStrings.begin();
	while (it1 != ParamSet.m_mapStrings.end()) {
		m_mapStrings[it1->first] = *(it1->second);
		it1++;
	}

	map<string, int *>::iterator it2 = ParamSet.m_mapInts.begin();
	while (it2 != ParamSet.m_mapInts.end()) {
		m_mapInts[it2->first] = *(it2->second);
		it2++;
	}
}

CPreset::CPreset(CParameterSet &ParamSet, string strName, HKEY hKey)
: m_ParamSet(ParamSet)
{
	CHKey hOwnKey = RegOpenSubkey(hKey, strName.c_str());
	if (!hOwnKey) {
		m_nID = 0;
		return;
	}
	m_nID = atoi(strName.c_str());

	QueryRegBoolValue(hOwnKey, "AddToMenu", &m_bAddToMenu, false);

	DWORD dwIndex = 0;
	char szName[256];
	DWORD dwcbName = sizeof(szName), dwType;

	while (RegEnumValue(hOwnKey, dwIndex, szName, &dwcbName, NULL, &dwType, NULL, 0) == ERROR_SUCCESS) {
		if (dwType == REG_DWORD) {
			int nValue;
			QueryRegIntValue(hOwnKey, szName, &nValue, 0);
			m_mapInts[szName] = nValue;
		} else if (dwType == REG_SZ) {
			string strValue;
			QueryRegStringValue(hOwnKey, szName, strValue, "");
			m_mapStrings[szName] = strValue;
		}
		dwcbName = sizeof(szName);
		dwIndex++;
	}
}

OperationResult CPreset::ExecutePreset() {
	CParameterBackup _Backup(m_ParamSet);
	Apply();
	return m_ParamSet.m_Executor();
}

void CPreset::Apply() {
	map<string, string *>::iterator it1 = m_ParamSet.m_mapStrings.begin();
	while (it1 != m_ParamSet.m_mapStrings.end()) {
		if (it1->first[0] != '@') {
			map<string, string>::iterator it = m_mapStrings.find(it1->first);
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

void CPreset::FillMenuItem(FarMenuItem &Item) {
	strcat(strcpy(Item.Text, GetMsg(MMenuPreset)), Name().c_str());
	Item.Selected = Item.Checked = Item.Separator = FALSE;
}

void CPreset::Save(HKEY hKey) {
	CHKey hOwnKey = RegCreateSubkey(hKey, FormatStr("%04d", m_nID).c_str());
	if (!hOwnKey) return;

	SetRegBoolValue(hOwnKey, "AddToMenu", m_bAddToMenu);

	map<string, string>::iterator it1 = m_mapStrings.begin();
	while (it1 != m_mapStrings.end()) {
		if (it1->first[0] != '@') SetRegStringValue(hOwnKey, it1->first.c_str(), it1->second);
		it1++;
	}

	map<string, int>::iterator it2 = m_mapInts.begin();
	while (it2 != m_mapInts.end()) {
		if (it2->first[0] != '@') SetRegIntValue(hOwnKey, it2->first.c_str(), it2->second);
		it2++;
	}
}

// ---------------

CPresetCollection::CPresetCollection(CParameterSet &ParamSet, const char *strKey, int nTitle)
: m_ParamSet(ParamSet), m_strKey(strKey), m_nTitle(nTitle)
{
	char szCurrentKey[256];
	sprintf(szCurrentKey, "%s\\RESearch\\%sPresets", StartupInfo.RootKey, Name());

	CHKey hKey = RegCreateSubkey(HKEY_CURRENT_USER, szCurrentKey);
	if (!hKey) return;

	DWORD dwIndex = 0;
	do {
		FILETIME ftTime;
		DWORD dwcbCurrentKey = sizeof(szCurrentKey);

		if (RegEnumKeyEx(hKey, dwIndex, szCurrentKey, &dwcbCurrentKey, NULL, NULL, NULL, &ftTime) != ERROR_SUCCESS) break;
		push_back(new CPreset(m_ParamSet, szCurrentKey, hKey));
		dwIndex++;
	} while (TRUE);

	ValidateIDs();
}

CPresetCollection::~CPresetCollection() {
	for (size_t nPreset=0; nPreset < size(); nPreset++) delete at(nPreset);
}

void CPresetCollection::Save() {
	char szCurrentKey[256];
	sprintf(szCurrentKey, "%s\\RESearch\\%sPresets", StartupInfo.RootKey, Name());

	CHKey hKey = RegCreateSubkey(HKEY_CURRENT_USER, szCurrentKey);
	if (!hKey) return;

	do {
		FILETIME ftTime;
		DWORD dwcbCurrentKey = sizeof(szCurrentKey);
		if (RegEnumKeyEx(hKey, 0, szCurrentKey, &dwcbCurrentKey, NULL, NULL, NULL, &ftTime) != ERROR_SUCCESS) break;
		RegDeleteKey(hKey,szCurrentKey);
	} while (TRUE);

	for (size_t nPreset=0; nPreset<size(); nPreset++)
		at(nPreset)->Save(hKey);
}

int CPresetCollection::ShowMenu(bool bExecute) {
	int piBreakKeys[]={VK_INSERT, VK_DELETE, VK_F4, 0};
	vector<string> arrItems;
	do {
		arrItems.resize(size());
		for (size_t nPreset = 0; nPreset < size(); nPreset++)
			arrItems[nPreset] = at(nPreset)->Name();

		int nBreakKey;
		char szTitle[128];
		sprintf(szTitle, "%s presets", Name());
		int nResult = ChooseMenu(arrItems, szTitle, "Ins,Del,F4", "Presets", 0,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			if (bExecute && (nResult >= 0) && (nResult < (int)size())) at(nResult)->Apply();
			return nResult;
		case 0:{
			CPreset *pPreset = new CPreset(m_ParamSet);
			if (EditPreset(pPreset)) {
				pPreset->m_nID = FindUnusedID();
				push_back(pPreset);
				Save();
			} else {
				delete pPreset;
			}
			break;
			  }
		case 1:
			if (nResult < (int)size()) {
				const char *Lines[]={"Delete", GetMsg(MDeletePresetQuery),
					at(nResult)->Name().c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber, FMSG_WARNING, "DeletePreset", Lines, 5, 2)==0) {
					delete at(nResult);
					erase(begin() + nResult);
					Save();
				}
			}
			break;
		case 2:
			if (nResult < (int)size()) {
				if (EditPreset(at(nResult))) Save();
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

void CPresetCollection::FillMenuItems(vector<FarMenuItem> &MenuItems) {
	for (size_t nPreset = 0; nPreset < size(); nPreset++) {
		CPreset *pPreset = at(nPreset);
		if (pPreset->m_bAddToMenu) {
			FarMenuItem Item;
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

// ---------------

CPresetBatch::CPresetBatch(CPresetCollection *pCollection)
: m_pCollection(pCollection)
, m_strName("New Batch")
, m_bAddToMenu(false)
{
}

CPresetBatch::CPresetBatch(CPresetCollection *pCollection, string strName, HKEY hKey) : m_pCollection(pCollection) {
	CHKey hOwnKey = RegCreateSubkey(hKey, strName.c_str());
	if (!hOwnKey) return;

	QueryRegStringValue(hOwnKey, NULL, m_strName, "Batch");
	QueryRegBoolValue(hOwnKey, "AddToMenu", &m_bAddToMenu, false);

	int nCount;
	QueryRegIntValue(hOwnKey, "Count", &nCount, 0);
	resize(nCount);

	char szKeyName[16];
	for (int nIndex = 0; nIndex < nCount; nIndex++) {
		sprintf(szKeyName, "%08X", nIndex);
		QueryRegIntValue(hOwnKey, szKeyName, &at(nIndex), 0);
	}
}

void CPresetBatch::Save(int nID, HKEY hKey) {
	CHKey hOwnKey = RegCreateSubkey(hKey, FormatStr("%04d", nID).c_str());
	if (!hOwnKey) return;

	SetRegStringValue(hOwnKey, NULL, m_strName);
	SetRegBoolValue(hOwnKey, "AddToMenu", m_bAddToMenu);
	SetRegIntValue(hOwnKey, "Count", size());

	char szKeyName[16];
	for (size_t nIndex = 0; nIndex < size(); nIndex++) {
		sprintf(szKeyName, "%08X", nIndex);
		SetRegIntValue(hOwnKey, szKeyName, at(nIndex));
	}
}

void CPresetBatch::FillMenuItem(FarMenuItem &Item) {
	strcat(strcpy(Item.Text, GetMsg(MMenuBatch)), m_strName.c_str());
	Item.Selected = Item.Checked = Item.Separator = FALSE;
}

void CPresetBatch::Execute(CParameterSet &ParamSet) {
	CParameterBackup Backup(ParamSet);

	for (size_t nPreset = 0; nPreset < size(); nPreset++) {
		CPreset *pPreset = (*this)(nPreset);
		if (pPreset) {
			if (!pPreset->ExecutePreset()) break;
		}
	}
}

CPresetBatch::~CPresetBatch() {
}

CPreset *CPresetBatch::operator()(size_type nIndex) {
	return (nIndex < size()) ? (*m_pCollection)(at(nIndex)) : NULL;
}

bool CPresetBatch::Edit() {
	CFarDialog Dialog(60, 12, "BatchProperties");
	Dialog.AddFrame(MBatch);
	Dialog.Add(new CFarTextItem(5, 3, 0, MBatchName));
	Dialog.Add(new CFarEditItem(5, 4, 53, DIF_HISTORY, "SearchText", m_strName));
	Dialog.Add(new CFarCheckBoxItem(5, 6, 0, MAddToMenu, &m_bAddToMenu));
	Dialog.Add(new CFarButtonItem(34, 6, 0, 0, MBtnCommands));
	Dialog.AddButtons(MOk, MCancel);
	do {
		switch (Dialog.Display(2, -2, -3)) {
		case 0:
			return true;
		case 1:
			ShowMenu();
			break;
		default:
			return false;
		}
	} while (true);
}

int CPresetBatch::ShowMenu() {
	int piBreakKeys[]={VK_INSERT, (PKF_CONTROL<<16)|VK_UP, (PKF_CONTROL<<16)|VK_DOWN, VK_DELETE, 0};
	vector<string> arrItems;
	int nResult = 0;

	do {
		arrItems.resize(size());
		for (size_t nPreset = 0; nPreset < size(); nPreset++)
			arrItems[nPreset] = (*this)(nPreset)->Name();

		int nBreakKey;
		nResult = ChooseMenu(arrItems, GetMsg(MBatchCommands), "Ins,Ctrl-\x18\x19,Del", "Batch", nResult,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			return nResult;
		case 0:{
			int nNewIndex = m_pCollection->ShowMenu(false);
			if (nNewIndex >= 0) {
				push_back(m_pCollection->at(nNewIndex)->m_nID);
				nResult = size()-1;
			}
			break;
			  }
		case 1:
			if (nResult > 0) {
				int nSave = at(nResult-1);
				at(nResult-1) = at(nResult);
				at(nResult) = nSave;
				nResult--;
			}
			break;
		case 2:
			if ((nResult >= 0) && (nResult < (int)size()-1)) {
				int nSave = at(nResult+1);
				at(nResult+1) = at(nResult);
				at(nResult) = nSave;
				nResult++;
			}
			break;
		case 3:
			if (nResult < (int)size()) {
				erase(begin() + nResult);
			}
			break;
		}
	} while (true);
}

// ---------------

CPresetBatchCollection::CPresetBatchCollection(CPresetCollection *pCollection) :
m_pCollection(pCollection)
{
	HKEY hKey;
	char szCurrentKey[256];
	sprintf(szCurrentKey, "%s\\RESearch\\%sBatch", StartupInfo.RootKey, m_pCollection->Name());

	if (RegCreateKeyEx(HKEY_CURRENT_USER, szCurrentKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL)!=ERROR_SUCCESS) return;

	DWORD dwIndex = 0;
	do {
		FILETIME ftTime;
		DWORD dwcbCurrentKey = sizeof(szCurrentKey);

		if (RegEnumKeyEx(hKey, dwIndex, szCurrentKey, &dwcbCurrentKey, NULL, NULL, NULL, &ftTime) != ERROR_SUCCESS) break;
		push_back(new CPresetBatch(m_pCollection, szCurrentKey, hKey));
		dwIndex++;

	} while (TRUE);
	RegCloseKey(hKey);
}

CPresetBatchCollection::~CPresetBatchCollection() {
	for (size_t nBatch=0; nBatch < size(); nBatch++) delete at(nBatch);
}

void CPresetBatchCollection::Save() {
	HKEY hKey;
	char szCurrentKey[256];
	sprintf(szCurrentKey, "%s\\RESearch\\%sBatch", StartupInfo.RootKey, m_pCollection->Name());

	if (RegCreateKeyEx(HKEY_CURRENT_USER, szCurrentKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) return;
	RegDeleteAllSubkeys(hKey);

	for (size_t nBatch=0; nBatch<size(); nBatch++)
		at(nBatch)->Save(nBatch, hKey);

	RegCloseKey(hKey);
}

int CPresetBatchCollection::ShowMenu(CParameterSet &ParamSet) {
	int piBreakKeys[]={VK_INSERT, VK_DELETE, VK_F4, 0};
	vector<string> arrItems;

	do {
		arrItems.resize(size());
		for (size_t nBatch = 0; nBatch < size(); nBatch++)
			arrItems[nBatch] = at(nBatch)->m_strName;

		int nBreakKey;
		char szTitle[128];
		sprintf(szTitle, "%s batches", m_pCollection->Name());
		int nResult = ChooseMenu(arrItems, szTitle, "Ins,Del,F4", "Batches", 0,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			if (&ParamSet && ParamSet.m_Executor && (nResult >= 0) && (nResult < (int)size())) {
				CPresetBatch *pBatch = at(nResult);
				const char *Lines[]={"Execute", GetMsg(MExecuteBatchQuery),
					pBatch->m_strName.c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber, FMSG_WARNING, "ExecuteBatch", Lines, 5, 2) != 0) break;

				pBatch->Execute(ParamSet);
			}

			return nResult;
		case 0:{
			CPresetBatch *pBatch = new CPresetBatch(m_pCollection);
			if (pBatch->Edit()) {
				push_back(pBatch);
				Save();
			} else {
				delete pBatch;
			}
			break;
			  }
		case 1:
			if (nResult < (int)size()) {
				const char *Lines[]={"Delete", GetMsg(MDeleteBatchQuery),
					at(nResult)->m_strName.c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber, FMSG_WARNING, "DeleteBatch", Lines, 5, 2)==0) {
					delete at(nResult);
					erase(begin() + nResult);
					Save();
				}
			}
			break;
		case 2:
			if (nResult < (int)size()) {
				at(nResult)->Edit();
				Save();
			}
			break;
		}
	} while (true);
}

void CPresetBatchCollection::FillMenuItems(vector<FarMenuItem> &MenuItems) {
	for (size_t nBatch = 0; nBatch < size(); nBatch++) {
		CPresetBatch *pBatch = at(nBatch);
		if (pBatch->m_bAddToMenu) {
			FarMenuItem Item;
			pBatch->FillMenuItem(Item);
			MenuItems.push_back(Item);
		}
	}
}

CPresetBatch *CPresetBatchCollection::FindMenuBatch(int &nIndex) {
	for (size_t nBatch = 0; nBatch < size(); nBatch++) {
		CPresetBatch *pBatch = at(nBatch);
		if (pBatch->m_bAddToMenu) {
			if (nIndex == 0) return pBatch; else nIndex--;
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

CBatchAction::CBatchAction(CBatchType &Type)
: m_Type(Type), m_bAddToMenu(false)
{
}

CBatchAction::CBatchAction(CBatchType &Type, string strName, HKEY hKey)
: m_Type(Type), m_bAddToMenu(false), m_strName(strName)
{
	CHKey hOwnKey = RegOpenSubkey(hKey, m_strName.c_str());
	if (!hOwnKey) return;

	QueryRegBoolValue(hOwnKey, "AddToMenu", &m_bAddToMenu, false);

	for (size_t nIndex = 0; ; nIndex++) {
		CHKey hValue = RegOpenSubkey(hOwnKey, FormatStr("%04d", nIndex).c_str());
		if (!hValue) break;

		BatchActionIndex NewIndex;
		QueryRegIntValue(hValue, "Coll", &NewIndex.first, NO_BATCH_INDEX.first);
		QueryRegIntValue(hValue, "ID", &NewIndex.second, NO_BATCH_INDEX.second);

		push_back(NewIndex);
	}
}

void CBatchAction::Save(HKEY hKey) {
	CHKey hOwnKey = RegCreateSubkey(hKey, m_strName.c_str());
	if (!hOwnKey) return;

	SetRegBoolValue(hOwnKey, "AddToMenu", m_bAddToMenu);

	RegDeleteAllSubkeys(hOwnKey);

	for (size_t nIndex = 0; nIndex < size(); nIndex++) {
		CHKey hValue = RegCreateSubkey(hOwnKey, FormatStr("%04d", nIndex).c_str());
		if (!hValue) continue;

		SetRegIntValue(hValue, "Coll", at(nIndex).first);
		SetRegIntValue(hValue, "ID", at(nIndex).second);
	}
}

bool CBatchAction::Edit() {
	CFarDialog Dialog(60, 12, "BatchProperties");
	Dialog.AddFrame(MBatch);
	Dialog.Add(new CFarTextItem(5, 3, 0, MBatchName));
	Dialog.Add(new CFarEditItem(5, 4, 53, DIF_HISTORY, "SearchText", m_strName));
	Dialog.Add(new CFarCheckBoxItem(5, 6, 0, MAddToMenu, &m_bAddToMenu));
	Dialog.Add(new CFarButtonItem(34, 6, 0, 0, MBtnCommands));
	Dialog.AddButtons(MOk, MCancel);
	do {
		switch (Dialog.Display(2, -2, -3)) {
		case 0:
			return true;
		case 1:
			ShowMenu();
			break;
		default:
			return false;
		}
	} while (true);
}

void CBatchAction::ShowMenu() {
	int piBreakKeys[]={VK_INSERT, (PKF_CONTROL<<16)|VK_UP, (PKF_CONTROL<<16)|VK_DOWN, VK_DELETE, 0};
	vector<string> arrItems;
	int nResult = 0;

	do {
		arrItems.resize(size());
		for (size_t nPreset = 0; nPreset < size(); nPreset++) {
			arrItems[nPreset] = m_Type[at(nPreset)]->Name();
		}

		int nBreakKey;
		nResult = ChooseMenu(arrItems, GetMsg(MBatchCommands), "Ins,Ctrl-\x18\x19,Del", "Batch", nResult,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			return;
		case 0:{
			BatchActionIndex NewIndex = m_Type.SelectPreset();
			if (NewIndex != NO_BATCH_INDEX) {
				push_back(NewIndex);
				nResult = size()-1;
			}
			break;
			   }
		case 1:
			if (nResult > 0) {
				value_type nSave = at(nResult-1);
				at(nResult-1) = at(nResult);
				at(nResult) = nSave;
				nResult--;
			}
			break;
		case 2:
			if ((nResult >= 0) && (nResult < (int)size()-1)) {
				value_type nSave = at(nResult+1);
				at(nResult+1) = at(nResult);
				at(nResult) = nSave;
				nResult++;
			}
			break;
		case 3:
			if (nResult < (int)size()) {
				erase(begin() + nResult);
			}
			break;
		}
	} while (true);
}

void CBatchAction::Execute() {
	for (size_t nAction = 0; nAction < size(); nAction++) {
		CPreset *pPreset = m_Type[at(nAction)];
		if (pPreset->ExecutePreset() != OR_OK) break;
	}
}

//////////////////////////////////////////////////////////////////////////
const BatchActionIndex NO_BATCH_INDEX(0, -1);

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

CPreset *CBatchType::operator[](const BatchActionIndex &Pair) {
	if ((Pair.first < 0) || (Pair.first >= size())) return NULL;
	return (*at(Pair.first))(Pair.second);
}

BatchActionIndex CBatchType::SelectPreset() {
	vector<string> arrItems;

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
			return BatchActionIndex(nResult, pPreset->m_nID);
		}
	} while (true);
}

//////////////////////////////////////////////////////////////////////////

CBatchActionCollection::CBatchActionCollection(CBatchType &Type, HKEY hKey)
: m_Type(Type)
{
	char szKeyName[256];
	DWORD dwIndex = 0;
	do {
		DWORD dwcbKeyName = sizeof(szKeyName);

		if (RegEnumKeyEx(hKey, dwIndex, szKeyName, &dwcbKeyName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
		push_back(new CBatchAction(m_Type, szKeyName, hKey));
		dwIndex++;
	} while (TRUE);
}

void CBatchActionCollection::Save(HKEY hKey) {
	RegDeleteAllSubkeys(hKey);

	for (size_t nIndex = 0; nIndex < size(); nIndex++) {
		at(nIndex)->Save(hKey);
	}
}

void CBatchActionCollection::ShowMenu() {
	int piBreakKeys[]={VK_INSERT, VK_DELETE, VK_F4, 0};
	vector<string> arrItems;

	do {
		arrItems.resize(size());
		for (size_t nBatch = 0; nBatch < size(); nBatch++)
			arrItems[nBatch] = at(nBatch)->m_strName;

		int nBreakKey;
		int nResult = ChooseMenu(arrItems, GetMsg(m_Type.m_nTitle), "Ins,Del,F4", "Batches", 0,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			if ((nResult >= 0) && (nResult < (int)size())) {
				CBatchAction *pBatch = at(nResult);
				const char *Lines[]={"Execute", GetMsg(MExecuteBatchQuery),
					pBatch->m_strName.c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber, FMSG_WARNING, "ExecuteBatch", Lines, 5, 2) != 0) break;

				pBatch->Execute();
			}

			return;
		case 0:{
			CBatchAction *pBatch = new CBatchAction(m_Type);
			if (pBatch->Edit()) {
				push_back(pBatch);
				WriteRegistry();
			} else {
				delete pBatch;
			}
			break;
			   }
		case 1:
			if (nResult < (int)size()) {
				const char *Lines[]={"Delete", GetMsg(MDeleteBatchQuery),
					at(nResult)->m_strName.c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber, FMSG_WARNING, "DeleteBatch", Lines, 5, 2)==0) {
					delete at(nResult);
					erase(begin() + nResult);
					WriteRegistry();
				}
			}
			break;
		case 2:
			if (nResult < (int)size()) {
				at(nResult)->Edit();
				WriteRegistry();
			}
			break;
		}
	} while (true);
}
