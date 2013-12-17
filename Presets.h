#ifndef __PRESETS_H
#define __PRESETS_H

#pragma warning(disable:327)

typedef OperationResult (*PresetExecutor)();

//	Mapping of parameter names to existing variables
class CParameterSet {
public:
	CParameterSet(PresetExecutor Executor, ...);
	map<string, tstring *> m_mapStrings;
	map<string, int *> m_mapInts;
	map<string, bool *> m_mapBools;
	PresetExecutor m_Executor;
};

class CParameterBackup {
public:
	CParameterBackup(CParameterSet &Set, bool bAutoRestore = true);
	void Restore();
	~CParameterBackup();

	map<string, tstring> m_mapStrings;
	map<string, int> m_mapInts;

	CParameterSet &m_Set;
	bool m_bAutoRestore;
};

//	Saveable mapping of parameter names to values
class CPreset {
public:
	CPreset(CParameterSet &ParamSet);
	CPreset(CParameterSet &ParamSet, const tstring &strName, CFarSettingsKey &hKey);	// hKey is root key
	void FillDefaults();
	void CopyFrom(const CPreset &Preset);

	OperationResult ExecutePreset();
	virtual void Apply();
	void FillMenuItem(CFarMenuItemEx &Item);
	void Save(CFarSettingsKey &hKey, int nIndex);

	tstring &Name() {return m_mapStrings[""];}

public:
	int m_nID;
	bool m_bAddToMenu;
	map<string, tstring> m_mapStrings;
	map<string, int> m_mapInts;

	CParameterSet &m_ParamSet;
};

class CPresetCollection : public vector<CPreset *> {
public:
	CPresetCollection(CParameterSet &ParamSet, const TCHAR *strKey, int nTitle);
	void Load();
	virtual ~CPresetCollection();

	void Save();
	int ShowMenu(bool bExecute, int nDefaultID = 0);
	virtual CPreset *LoadPreset(const tstring &strName, CFarSettingsKey &hKey) = 0;
	virtual CPreset *NewPreset() = 0;
	virtual bool EditPreset(CPreset *pPreset) = 0;
	virtual int  ID() = 0;	// For batches
	CPreset *operator()(int nID);

	void FillMenuItems(vector<CFarMenuItemEx> &MenuItems);
	CPreset *FindMenuPreset(int &nIndex);

	const TCHAR *Name()  {return m_strKey.c_str();}
	const TCHAR *Title() {return GetMsg(m_nTitle);}

	CParameterSet &m_ParamSet;
	tstring m_strKey;
	int m_nTitle;

protected:
	int  FindUnusedID();
	void ValidateIDs();

	size_t m_nCurrent;
};

template<class _Preset>
class CPresetCollectionT : public CPresetCollection
{
public:
	CPresetCollectionT(CParameterSet &ParamSet, const TCHAR *strKey, int nTitle)
		: CPresetCollection(ParamSet, strKey, nTitle) { Load(); } 
	virtual CPreset *LoadPreset(const tstring &strName, CFarSettingsKey &hKey) { return new _Preset(m_ParamSet, strName, hKey); }
	virtual CPreset *NewPreset() { return new _Preset(m_ParamSet); }
};

typedef CPresetCollectionT<CPreset> CStdPresetCollection;

//////////////////////////////////////////////////////////////////////////

typedef pair<int, int> BatchActionIndex;		// Collection and Preset IDs
extern const BatchActionIndex NO_BATCH_INDEX;

class CBatchType : public vector<CPresetCollection *> {
public:
	CBatchType(int nTitle, ...);
	CPresetCollection *operator()(int nCollID);
	CPreset *operator[](const BatchActionIndex &Pair);

	BatchActionIndex SelectPreset();

public:
	int m_nTitle;
};

class CBatchAction : public vector<BatchActionIndex> {		// Collection index and ID
public:
	CBatchAction(CBatchType &Type);
	CBatchAction(CBatchType &Type, tstring strName, CFarSettingsKey &hKey);
	void Save(CFarSettingsKey &hKey, int nIndex);

	bool EditProperties();
	bool EditItems();
	void Execute();

	CFarMenuItemEx GetMenuItem();

	bool m_bAddToMenu;
	tstring m_strName;

protected:
	CBatchType &m_Type;

	class CBatchActionCollection *Collection();

	size_t m_nCurrent;
};

class CBatchActionCollection : public vector<CBatchAction *> {
public:
	CBatchActionCollection(CBatchType &Type, CFarSettingsKey &hKey);	// CPresetCollection *
	void Save(CFarSettingsKey &hKey);

	void ShowMenu();

	void FillMenuItems(vector<CFarMenuItemEx> &MenuItems);
	CBatchAction *FindMenuAction(int &nIndex);

public:
	CBatchType &m_Type;

protected:
	size_t m_nCurrent;
};

#endif
