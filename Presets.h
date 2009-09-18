#ifndef __PRESETS_H
#define __PRESETS_H

#pragma warning(disable:327)

typedef OperationResult (*PresetExecutor)();

//	Mapping of parameter names to existing variables
class CParameterSet {
public:
	CParameterSet(PresetExecutor Executor, int nStringCount, int nIntCount, ...);
	map<string, string *> m_mapStrings;
	map<string, int *> m_mapInts;
	PresetExecutor m_Executor;
};

class CParameterBackup {
public:
	CParameterBackup(CParameterSet &Set, bool bAutoRestore = true);
	void Restore();
	~CParameterBackup();

	map<string, string> m_mapStrings;
	map<string, int> m_mapInts;

	CParameterSet &m_Set;
	bool m_bAutoRestore;
};

//	Saveable mapping of parameter names to values
class CPreset {
public:
	CPreset(CParameterSet &ParamSet);
	CPreset(CParameterSet &ParamSet, const string &strName, HKEY hKey);	// hKey is root key

	OperationResult ExecutePreset();
	virtual void Apply();
	void FillMenuItem(FarMenuItem &Item);
	void Save(HKEY hKey);

	string &Name() {return m_mapStrings[""];}

public:
	int m_nID;
	bool m_bAddToMenu;
	map<string, string> m_mapStrings;
	map<string, int> m_mapInts;

	CParameterSet &m_ParamSet;
};

class CPresetCollection : public vector<CPreset *> {
public:
	CPresetCollection(CParameterSet &ParamSet, const char *strKey, int nTitle);
	virtual ~CPresetCollection();
	void Save();
	int ShowMenu(bool bExecute);
	virtual CPreset *LoadPreset(const string &strName, HKEY hKey);
	virtual CPreset *NewPreset();
	virtual BOOL EditPreset(CPreset *pPreset) = 0;
	virtual int  ID() = 0;	// For batches
	CPreset *operator()(int nID);

	void FillMenuItems(vector<FarMenuItem> &MenuItems);
	CPreset *FindMenuPreset(int &nIndex);

	const char *Name()  {return m_strKey.c_str();}
	const char *Title() {return GetMsg(m_nTitle);}

	CParameterSet &m_ParamSet;
	string m_strKey;
	int m_nTitle;

protected:
	int FindUnusedID();
	void ValidateIDs();
};

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
	CBatchAction(CBatchType &Type, string strName, HKEY hKey);
	void Save(HKEY hKey);

	bool Edit();
	void EditItems();
	void Execute();

	FarMenuItem GetMenuItem();

	bool m_bAddToMenu;
	string m_strName;

protected:
	CBatchType &m_Type;
};

class CBatchActionCollection : public vector<CBatchAction *> {
public:
	CBatchActionCollection(CBatchType &Type, HKEY hKey);	// CPresetCollection *
	void Save(HKEY hKey);

	void ShowMenu();

	void FillMenuItems(vector<FarMenuItem> &MenuItems);
	CBatchAction *FindMenuAction(int &nIndex);
public:
	CBatchType &m_Type;
};

#endif
