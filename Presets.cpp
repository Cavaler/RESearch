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
	while (nStringCount--) {
		const char *szName = va_arg(List, const char *);
		m_mapStrings[szName] = va_arg(List, tstring *);
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
	m_mapStrings[""] = _T("New Preset");
	m_bAddToMenu = false;

	map<string, tstring *>::iterator it1 = ParamSet.m_mapStrings.begin();
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

CPreset::CPreset(CParameterSet &ParamSet, const tstring &strName, HKEY hKey)
: m_ParamSet(ParamSet)
{
	CHKey hOwnKey = RegOpenSubkey(hKey, strName.c_str());
	if (!hOwnKey) {
		m_nID = 0;
		return;
	}
	m_nID = _ttoi(strName.c_str());

	QueryRegBoolValue(hOwnKey, _T("AddToMenu"), &m_bAddToMenu, false);

	DWORD dwIndex = 0;
	TCHAR szName[256];
	DWORD dwcbName = arrsizeof(szName), dwType;

	while (RegEnumValue(hOwnKey, dwIndex, szName, &dwcbName, NULL, &dwType, NULL, 0) == ERROR_SUCCESS) {
		if (dwType == REG_DWORD) {
			int nValue;
			QueryRegIntValue(hOwnKey, szName, &nValue, 0);
			m_mapInts[NameKey(szName)] = nValue;
		} else if (dwType == REG_SZ) {
			tstring strValue;
			QueryRegStringValue(hOwnKey, szName, strValue, _T(""));
			m_mapStrings[NameKey(szName)] = strValue;
		}
		dwcbName = arrsizeof(szName);
		dwIndex++;
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

void CPreset::FillMenuItem(CFarMenuItem &Item) {
#ifdef UNICODE
	Item.Text = _tcsdup(Name().c_str());
#else
	strcat(strcpy(Item.Text, GetMsg(MMenuPreset)), Name().c_str());
	Item.Selected = Item.Checked = Item.Separator = FALSE;
#endif
}

void CPreset::Save(HKEY hKey) {
	CHKey hOwnKey = RegCreateSubkey(hKey, FormatStr(_T("%04d"), m_nID).c_str());
	if (!hOwnKey) return;

	map<string, tstring>::iterator it1 = m_mapStrings.begin();
	while (it1 != m_mapStrings.end()) {
		if (it1->first[0] != '@') SetRegStringValue(hOwnKey, KeyName(it1->first).c_str(), it1->second);
		it1++;
	}

	map<string, int>::iterator it2 = m_mapInts.begin();
	while (it2 != m_mapInts.end()) {
		if (it2->first[0] != '@') SetRegIntValue(hOwnKey, KeyName(it2->first).c_str(), it2->second);
		it2++;
	}

	SetRegBoolValue(hOwnKey, _T("AddToMenu"), m_bAddToMenu);
}

//////////////////////////////////////////////////////////////////////////

CPresetCollection::CPresetCollection(CParameterSet &ParamSet, const TCHAR *strKey, int nTitle)
: m_ParamSet(ParamSet), m_strKey(strKey), m_nTitle(nTitle) {}

void CPresetCollection::Load()
{
	TCHAR szCurrentKey[256];
	_stprintf_s(szCurrentKey, 256, _T("%s\\RESearch\\%sPresets"), StartupInfo.RootKey, Name());

	CHKey hKey = RegCreateSubkey(HKEY_CURRENT_USER, szCurrentKey);
	if (!hKey) return;

	DWORD dwIndex = 0;
	do {
		FILETIME ftTime;
		DWORD dwcbCurrentKey = arrsizeof(szCurrentKey);

		if (RegEnumKeyEx(hKey, dwIndex, szCurrentKey, &dwcbCurrentKey, NULL, NULL, NULL, &ftTime) != ERROR_SUCCESS) break;
		push_back(LoadPreset(szCurrentKey, hKey));
		dwIndex++;
	} while (TRUE);

	ValidateIDs();
}

CPresetCollection::~CPresetCollection() {
	for (size_t nPreset=0; nPreset < size(); nPreset++) delete at(nPreset);
}

void CPresetCollection::Save() {
	TCHAR szCurrentKey[256];
	_stprintf_s(szCurrentKey, 256, _T("%s\\RESearch\\%sPresets"), StartupInfo.RootKey, Name());

	CHKey hKey = RegCreateSubkey(HKEY_CURRENT_USER, szCurrentKey);
	if (!hKey) return;

	do {
		FILETIME ftTime;
		DWORD dwcbCurrentKey = arrsizeof(szCurrentKey);
		if (RegEnumKeyEx(hKey, 0, szCurrentKey, &dwcbCurrentKey, NULL, NULL, NULL, &ftTime) != ERROR_SUCCESS) break;
		RegDeleteKey(hKey,szCurrentKey);
	} while (TRUE);

	for (size_t nPreset=0; nPreset<size(); nPreset++)
		at(nPreset)->Save(hKey);
}

int CPresetCollection::ShowMenu(bool bExecute, int nDefaultID) {
	int piBreakKeys[]={VK_INSERT, VK_DELETE, VK_F4, 0};
	vector<tstring> arrItems;

	do {
		int nDefault = 0;
		arrItems.resize(size());
		for (size_t nPreset = 0; nPreset < size(); nPreset++) {
			CPreset *pPreset = at(nPreset);
			arrItems[nPreset] = pPreset->Name();
			if (pPreset->m_nID == nDefaultID) nDefault = nPreset;
		}

		int nBreakKey;
		tstring strTitle = FormatStr(_T("%s presets"), Name());
		int nResult = ChooseMenu(arrItems, strTitle.c_str(), _T("Ins,Del,F4"), _T("Presets"), nDefault,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			if (bExecute && (nResult >= 0) && (nResult < (int)size())) at(nResult)->Apply();
			return nResult;
		case 0:{
			CPreset *pPreset = NewPreset();
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
				const TCHAR *Lines[]={_T("Delete"), GetMsg(MDeletePresetQuery),
					at(nResult)->Name().c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber, FMSG_WARNING, _T("DeletePreset"), Lines, 5, 2)==0) {
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

void CPresetCollection::FillMenuItems(vector<CFarMenuItem> &MenuItems) {
	for (size_t nPreset = 0; nPreset < size(); nPreset++) {
		CPreset *pPreset = at(nPreset);
		if (pPreset->m_bAddToMenu) {
			CFarMenuItem Item;
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

CBatchAction::CBatchAction(CBatchType &Type, tstring strName, HKEY hKey)
: m_Type(Type), m_bAddToMenu(false), m_strName(strName)
{
	CHKey hOwnKey = RegOpenSubkey(hKey, m_strName.c_str());
	if (!hOwnKey) return;

	QueryRegBoolValue(hOwnKey, _T("AddToMenu"), &m_bAddToMenu, false);

	for (size_t nIndex = 0; ; nIndex++) {
		CHKey hValue = RegOpenSubkey(hOwnKey, FormatStr(_T("%04d"), nIndex).c_str());
		if (!hValue) break;

		BatchActionIndex NewIndex;
		QueryRegIntValue(hValue, _T("Coll"), &NewIndex.first, NO_BATCH_INDEX.first);
		QueryRegIntValue(hValue, _T("ID"), &NewIndex.second, NO_BATCH_INDEX.second);

		push_back(NewIndex);
	}
}

void CBatchAction::Save(HKEY hKey) {
	CHKey hOwnKey = RegCreateSubkey(hKey, m_strName.c_str());
	if (!hOwnKey) return;

	SetRegBoolValue(hOwnKey, _T("AddToMenu"), m_bAddToMenu);

	RegDeleteAllSubkeys(hOwnKey);

	for (size_t nIndex = 0; nIndex < size(); nIndex++) {
		CHKey hValue = RegCreateSubkey(hOwnKey, FormatStr(_T("%04d"), nIndex).c_str());
		if (!hValue) continue;

		SetRegIntValue(hValue, _T("Coll"), at(nIndex).first);
		SetRegIntValue(hValue, _T("ID"), at(nIndex).second);
	}
}

bool CBatchAction::Edit() {
	CFarDialog Dialog(60, 12, _T("BatchProperties"));
	Dialog.AddFrame(MBatch);
	Dialog.Add(new CFarTextItem(5, 3, 0, MBatchName));
	Dialog.Add(new CFarEditItem(5, 4, 53, DIF_HISTORY, _T("SearchText"), m_strName));
	Dialog.Add(new CFarCheckBoxItem(5, 6, 0, MAddToMenu, &m_bAddToMenu));
	Dialog.Add(new CFarButtonItem(34, 6, 0, 0, MBtnCommands));
	Dialog.AddButtons(MOk, MCancel);
	do {
		switch (Dialog.Display(2, -2, -3)) {
		case 0:
			return true;
		case 1:
			EditItems();
			break;
		default:
			return false;
		}
	} while (true);
}

void CBatchAction::EditItems() {
	int piBreakKeys[]={VK_INSERT, (PKF_CONTROL<<16)|VK_UP, (PKF_CONTROL<<16)|VK_DOWN, VK_DELETE, 0};
	vector<tstring> arrItems;
	int nResult = 0;

	do {
		arrItems.resize(size());
		for (size_t nPreset = 0; nPreset < size(); nPreset++) {
			arrItems[nPreset] = m_Type[at(nPreset)]->Name();
		}

		int nBreakKey;
		nResult = ChooseMenu(arrItems, GetMsg(MBatchCommands), _T("Ins,Ctrl-\x18\x19,Del"), _T("Batch"), nResult,
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

CFarMenuItem CBatchAction::GetMenuItem() {
	return CFarMenuItem(m_strName);
}

//////////////////////////////////////////////////////////////////////////

CBatchActionCollection::CBatchActionCollection(CBatchType &Type, HKEY hKey)
: m_Type(Type)
{
	TCHAR szKeyName[256];
	DWORD dwIndex = 0;
	do {
		DWORD dwcbKeyName = arrsizeof(szKeyName);

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
	vector<tstring> arrItems;

	do {
		arrItems.resize(size());
		for (size_t nBatch = 0; nBatch < size(); nBatch++)
			arrItems[nBatch] = at(nBatch)->m_strName;

		int nBreakKey;
		int nResult = ChooseMenu(arrItems, GetMsg(m_Type.m_nTitle), _T("Ins,Del,F4"), _T("Batches"), 0,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			if ((nResult >= 0) && (nResult < (int)size())) {
				CBatchAction *pBatch = at(nResult);
				const TCHAR *Lines[]={_T("Execute"), GetMsg(MExecuteBatchQuery),
					pBatch->m_strName.c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber, FMSG_WARNING, _T("ExecuteBatch"), Lines, 5, 2) != 0) break;

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
				const TCHAR *Lines[]={_T("Delete"), GetMsg(MDeleteBatchQuery),
					at(nResult)->m_strName.c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber, FMSG_WARNING, _T("DeleteBatch"), Lines, 5, 2)==0) {
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


void CBatchActionCollection::FillMenuItems(vector<CFarMenuItem> &MenuItems) {
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
