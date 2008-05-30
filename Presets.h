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
	CPreset(string strName, HKEY hKey);	// hKey is root key
	void Apply(CParameterSet &ParamSet);
	void FillMenuItem(FarMenuItem &Item);
	void Save(HKEY hKey);

	string &Name() {return m_mapStrings[""];}
	int m_nID;
	bool m_bAddToMenu;
	map<string, string> m_mapStrings;
	map<string, int> m_mapInts;
};

class CPresetCollection : public vector<CPreset *> {
public:
	CPresetCollection();
	virtual ~CPresetCollection();
	void Load();
	void Save();
	int ShowMenu(CParameterSet &ParamSet = *((CParameterSet *)NULL));
	virtual BOOL EditPreset(CPreset *pPreset) = 0;
	CPreset *operator()(int nID);
	virtual const char *GetName()=0;

	void FillMenuItems(vector<FarMenuItem> &MenuItems);
	CPreset *FindMenuPreset(int &nIndex);

protected:
	int FindUnusedID();
	void ValidateIDs();
};

class CPresetBatch : public vector<int> {
public:
	CPresetBatch(CPresetCollection *pCollection);
	CPresetBatch(CPresetCollection *pCollection, string strName, HKEY hKey);
	void Save(int nID, HKEY hKey);
	void FillMenuItem(FarMenuItem &Item);
	void Execute(CParameterSet &ParamSet);
	bool Edit();
	~CPresetBatch();

	bool m_bAddToMenu;
	string m_strName;
	CPreset *operator()(size_type nIndex);
protected:
	CPresetCollection *m_pCollection;
	int ShowMenu();
};

class CPresetBatchCollection : public vector<CPresetBatch *> {
public:
	CPresetBatchCollection(CPresetCollection *pCollection);
	void Save();
	~CPresetBatchCollection();

	int ShowMenu(CParameterSet &ParamSet = *((CParameterSet *)NULL));

	void FillMenuItems(vector<FarMenuItem> &MenuItems);
	CPresetBatch *FindMenuBatch(int &nIndex);
protected:
	CPresetCollection *m_pCollection;
};

#endif
