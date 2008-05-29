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

CPreset::CPreset(CParameterSet &Batch) : m_nID(0) {
	if (!&Batch) return;

	m_mapStrings[""] = "New Preset";
	map<string, string *>::iterator it1 = Batch.m_mapStrings.begin();
	while (it1 != Batch.m_mapStrings.end()) {
		m_mapStrings[it1->first] = *(it1->second);
		it1++;
	}

	map<string, int *>::iterator it2 = Batch.m_mapInts.begin();
	while (it2 != Batch.m_mapInts.end()) {
		m_mapInts[it2->first] = *(it2->second);
		it2++;
	}
}

CPreset::CPreset(string strName, HKEY hKey) {
	HKEY hOwnKey;
	if (RegCreateKeyEx(hKey, strName.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hOwnKey, NULL) != ERROR_SUCCESS) {
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
	RegCloseKey(hOwnKey);
}

void CPreset::Apply(CParameterSet &Batch) {
	map<string, string *>::iterator it1 = Batch.m_mapStrings.begin();
	while (it1 != Batch.m_mapStrings.end()) {
		map<string, string>::iterator it = m_mapStrings.find(it1->first);
		if (it != m_mapStrings.end()) 
			*(it1->second) = it->second;
		it1++;
	}

	map<string, int *>::iterator it2 = Batch.m_mapInts.begin();
	while (it2 != Batch.m_mapInts.end()) {
		map<string, int>::iterator it = m_mapInts.find(it2->first);
		if (it != m_mapInts.end()) 
			*(it2->second) = it->second;
		it2++;
	}
}

void CPreset::Save(HKEY hKey) {
	HKEY hOwnKey;
	char szName[16];
	sprintf(szName, "%04d", m_nID);

	if (RegCreateKeyEx(hKey, szName, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hOwnKey, NULL) != ERROR_SUCCESS)
		return;

	SetRegBoolValue(hOwnKey, "AddToMenu", m_bAddToMenu);

	map<string, string>::iterator it1 = m_mapStrings.begin();
	while (it1 != m_mapStrings.end()) {
		SetRegStringValue(hOwnKey, it1->first.c_str(), it1->second);
		it1++;
	}

	map<string, int>::iterator it2 = m_mapInts.begin();
	while (it2 != m_mapInts.end()) {
		SetRegIntValue(hOwnKey, it2->first.c_str(), it2->second);
		it2++;
	}

	RegCloseKey(hOwnKey);
}

// ---------------

CPresetCollection::CPresetCollection() {
}

CPresetCollection::~CPresetCollection() {
	for (size_t nPreset=0; nPreset < size(); nPreset++) delete at(nPreset);
}

void CPresetCollection::Load() {
	HKEY hKey;
	char szCurrentKey[256];
	sprintf(szCurrentKey, "%s\\RESearch\\%sPresets", StartupInfo.RootKey, GetName());

	if (RegCreateKeyEx(HKEY_CURRENT_USER, szCurrentKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL)!=ERROR_SUCCESS) return;

	DWORD dwIndex = 0;
	do {
		FILETIME ftTime;
		DWORD dwcbCurrentKey = sizeof(szCurrentKey);

		if (RegEnumKeyEx(hKey, dwIndex, szCurrentKey, &dwcbCurrentKey, NULL, NULL, NULL, &ftTime) != ERROR_SUCCESS) break;
		push_back(new CPreset(szCurrentKey, hKey));
		dwIndex++;

	} while (TRUE);
	RegCloseKey(hKey);

	ValidateIDs();
}

void CPresetCollection::Save() {
	HKEY hKey;
	char szCurrentKey[256];
	sprintf(szCurrentKey, "%s\\RESearch\\%sPresets", StartupInfo.RootKey, GetName());

	if (RegCreateKeyEx(HKEY_CURRENT_USER, szCurrentKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) return;
	do {
		FILETIME ftTime;
		DWORD dwcbCurrentKey = sizeof(szCurrentKey);
		if (RegEnumKeyEx(hKey, 0, szCurrentKey, &dwcbCurrentKey, NULL, NULL, NULL, &ftTime) != ERROR_SUCCESS) break;
		RegDeleteKey(hKey,szCurrentKey);
	} while (TRUE);

	for (size_t nPreset=0; nPreset<size(); nPreset++)
		at(nPreset)->Save(hKey);

	RegCloseKey(hKey);
}

int CPresetCollection::ShowMenu(CParameterSet &Batch) {
	int piBreakKeys[]={VK_INSERT, VK_DELETE, VK_F4, 0};
	vector<string> arrItems;
	do {
		arrItems.resize(size());
		for (size_t nPreset = 0; nPreset < size(); nPreset++)
			arrItems[nPreset] = at(nPreset)->Name();

		int nBreakKey;
		char szTitle[128];
		sprintf(szTitle, "%s presets", GetName());
		int nResult = ChooseMenu(arrItems, szTitle, "Ins,Del,F4", "Presets", 0,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			if (&Batch && (nResult >= 0) && (nResult < (int)size())) at(nResult)->Apply(Batch);
			return nResult;
		case 0:{
			CPreset *pPreset = new CPreset(Batch);
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

CPresetBatch::CPresetBatch(CPresetCollection *pCollection) : m_pCollection(pCollection), m_strName("New Batch") {
}

CPresetBatch::CPresetBatch(CPresetCollection *pCollection, string strName, HKEY hKey) : m_pCollection(pCollection) {
	HKEY hOwnKey;
	if (RegCreateKeyEx(hKey, strName.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hOwnKey, NULL) != ERROR_SUCCESS)
		return;

	QueryRegStringValue(hOwnKey, NULL, m_strName, "Batch");

	int nCount;
	QueryRegIntValue(hOwnKey, "Count", &nCount, 0);
	resize(nCount);

	char szKeyName[16];
	for (int nIndex = 0; nIndex < nCount; nIndex++) {
		sprintf(szKeyName, "%08X", nIndex);
		QueryRegIntValue(hOwnKey, szKeyName, &at(nIndex), 0);
	}
	RegCloseKey(hOwnKey);
}

void CPresetBatch::Save(int nID, HKEY hKey) {
	HKEY hOwnKey;
	char szName[16];
	sprintf(szName, "%04d", nID);

	if (RegCreateKeyEx(hKey, szName, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hOwnKey, NULL) != ERROR_SUCCESS)
		return;

	SetRegStringValue(hOwnKey, NULL, m_strName);
	SetRegIntValue(hOwnKey, "Count", size());

	char szKeyName[16];
	for (size_t nIndex = 0; nIndex < size(); nIndex++) {
		sprintf(szKeyName, "%08X", nIndex);
		SetRegIntValue(hOwnKey, szKeyName, at(nIndex));
	}
	RegCloseKey(hOwnKey);
}

CPresetBatch::~CPresetBatch() {
}

CPreset *CPresetBatch::operator()(size_type nIndex) {
	return (nIndex < size()) ? (*m_pCollection)(at(nIndex)) : NULL;
}

int CPresetBatch::ShowMenu() {
	int piBreakKeys[]={VK_INSERT, (PKF_CONTROL<<16)|VK_UP, (PKF_CONTROL<<16)|VK_DOWN, VK_DELETE, VK_F6, 0};
	vector<string> arrItems;
	int nResult = 0;

	do {
		arrItems.resize(size());
		for (size_t nPreset = 0; nPreset < size(); nPreset++)
			arrItems[nPreset] = (*this)(nPreset)->Name();

		int nBreakKey;
		nResult = ChooseMenu(arrItems, m_strName.c_str(), "Ins,Ctrl-\x18\x19,Del,F6", "Batch", nResult,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			return nResult;
		case 0:{
			int nNewIndex = m_pCollection->ShowMenu();
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
		case 4:{
			CFarDialog Dialog(60, 9, "PresetName");
			Dialog.AddFrame(MBatchName);
			Dialog.Add(new CFarEditItem(5, 3, 53, DIF_HISTORY,"SearchText", m_strName));
			Dialog.AddButtons(MOk, MCancel);
			Dialog.Display(-1);
			break;
			  }
		}
	} while (true);
}

// ---------------

CPresetBatchCollection::CPresetBatchCollection(CPresetCollection *pCollection) :
m_pCollection(pCollection)
{
	HKEY hKey;
	char szCurrentKey[256];
	sprintf(szCurrentKey, "%s\\RESearch\\%sBatch", StartupInfo.RootKey, m_pCollection->GetName());

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
	sprintf(szCurrentKey, "%s\\RESearch\\%sBatch", StartupInfo.RootKey, m_pCollection->GetName());

	if (RegCreateKeyEx(HKEY_CURRENT_USER, szCurrentKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) return;
	do {
		FILETIME ftTime;
		DWORD dwcbCurrentKey = sizeof(szCurrentKey);
		if (RegEnumKeyEx(hKey, 0, szCurrentKey, &dwcbCurrentKey, NULL, NULL, NULL, &ftTime) != ERROR_SUCCESS) break;
		RegDeleteKey(hKey, szCurrentKey);
	} while (TRUE);

	for (size_t nBatch=0; nBatch<size(); nBatch++)
		at(nBatch)->Save(nBatch, hKey);

	RegCloseKey(hKey);
}

int CPresetBatchCollection::ShowMenu(CParameterSet &Batch) {
	int piBreakKeys[]={VK_INSERT, VK_DELETE, VK_F4, 0};
	vector<string> arrItems;

	do {
		arrItems.resize(size());
		for (size_t nBatch = 0; nBatch < size(); nBatch++)
			arrItems[nBatch] = at(nBatch)->m_strName;

		int nBreakKey;
		char szTitle[128];
		sprintf(szTitle, "%s batches", m_pCollection->GetName());
		int nResult = ChooseMenu(arrItems, szTitle, "Ins,Del,F4", "Batches", 0,
			FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, piBreakKeys, &nBreakKey);

		switch (nBreakKey) {
		case -1:
			if (&Batch && Batch.m_Executor && (nResult >= 0) && (nResult < (int)size())) {
				CPresetBatch *pBatch = at(nResult);
				const char *Lines[]={"Execute", GetMsg(MExecuteBatchQuery),
					pBatch->m_strName.c_str(), GetMsg(MOk), GetMsg(MCancel)};
				if (StartupInfo.Message(StartupInfo.ModuleNumber, FMSG_WARNING, "ExecuteBatch", Lines, 5, 2) != 0) break;

				for (size_t nPreset = 0; nPreset < pBatch->size(); nPreset++) {
					CPreset *pPreset = (*pBatch)(nPreset);
					if (pPreset) {
						pPreset->Apply(Batch);
						if (!Batch.m_Executor()) break;
					}
				}
			}

			return nResult;
		case 0:{
			CPresetBatch *pBatch = new CPresetBatch(m_pCollection);
			pBatch->ShowMenu();
			if (!pBatch->empty()) {
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
				at(nResult)->ShowMenu();
				Save();
			}
			break;
		}
	} while (true);
}
