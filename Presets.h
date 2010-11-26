#ifndef __PRESETS_H
#define __PRESETS_H

#pragma warning(disable:327)

typedef OperationResult (*PresetExecutor)();

//	Mapping of parameter names to existing variables
class CParameterSet {
public:
	CParameterSet(PresetExecutor Executor, int nStringCount, int nIntCount, ...);
	map<string, tstring *> m_mapStrings;
	map<string, int *> m_mapInts;
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
	CPreset(CParameterSet &ParamSet, const tstring &strName, HKEY hKey);	// hKey is root key

	OperationResult ExecutePreset();
	virtual void Apply();
	void FillMenuItem(CFarMenuItem &Item);
	void Save(HKEY hKey, int nIndex);

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
	virtual CPreset *LoadPreset(const tstring &strName, HKEY hKey) = 0;
	virtual CPreset *NewPreset() = 0;
	virtual BOOL EditPreset(CPreset *pPreset) = 0;
	virtual int  ID() = 0;	// For batches
	CPreset *operator()(int nID);

	void FillMenuItems(vector<CFarMenuItem> &MenuItems);
	CPreset *FindMenuPreset(int &nIndex);

	const TCHAR *Name()  {return m_strKey.c_str();}
	const TCHAR *Title() {return GetMsg(m_nTitle);}

	CParameterSet &m_ParamSet;
	tstring m_strKey;
	int m_nTitle;

protected:
	int FindUnusedID();
	void ValidateIDs();
};

template<class _Preset>
class CPresetCollectionT : public CPresetCollection
{
public:
	CPresetCollectionT(CParameterSet &ParamSet, const TCHAR *strKey, int nTitle)
		: CPresetCollection(ParamSet, strKey, nTitle) { Load(); } 
	virtual CPreset *LoadPreset(const tstring &strName, HKEY hKey) { return new _Preset(m_ParamSet, strName, hKey); }
	virtual CPreset *NewPreset() { return new _Preset(m_ParamSet); }
};

typedef CPresetCollectionT<CPreset> CStdPresetCollection;

//////////////////////////////////////////////////////////////////////////

typedef pair<int, int> BatchActionIndex;		// Collection and Preset IDs
extern const BatchActionIndex NO_BATCH_INDEX;

class CBatchType : public vector<CPresetCollection *> {
public:
	CBatchType(int nTitle, ...);
	CPreset *operator[](const BatchActionIndex &Pair);

	BatchActionIndex SelectPreset();

public:
	int m_nTitle;
};

class CBatchAction : public vector<BatchActionIndex> {		// Collection index and ID
public:
	CBatchAction(CBatchType &Type);
	CBatchAction(CBatchType &Type, tstring strName, HKEY hKey);
	void Save(HKEY hKey, int nIndex);

	bool Edit();
	void EditItems();
	void Execute();

	CFarMenuItem GetMenuItem();

	bool m_bAddToMenu;
	tstring m_strName;

protected:
	CBatchType &m_Type;
};

class CBatchActionCollection : public vector<CBatchAction *> {
public:
	CBatchActionCollection(CBatchType &Type, HKEY hKey);	// CPresetCollection *
	void Save(HKEY hKey);

	void ShowMenu();

	void FillMenuItems(vector<CFarMenuItem> &MenuItems);
	CBatchAction *FindMenuAction(int &nIndex);
public:
	CBatchType &m_Type;
};

#endif
