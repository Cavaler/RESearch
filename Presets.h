#ifndef __PRESETS_H
#define __PRESETS_H

#pragma warning(disable:327)

class CParameterBatch {
public:
	CParameterBatch(int nStringCount, int nIntCount, ...);
	map<string, string *> m_mapStrings;
	map<string, int *> m_mapInts;
};

class CPreset {
public:
	CPreset(CParameterBatch &Batch);
	CPreset(string strName, HKEY hKey);	// hKey is root key
	void Apply(CParameterBatch &Batch);
	void Save(HKEY hKey);

	string &Name() {return m_mapStrings[""];}
	int m_nID;
	map<string, string> m_mapStrings;
	map<string, int> m_mapInts;
};

class CPresetCollection : public vector<CPreset *> {
public:
	CPresetCollection();
	virtual ~CPresetCollection();
	void Load();
	void Save();
	int ShowMenu(CParameterBatch &Batch = *((CParameterBatch *)NULL));
	virtual BOOL EditPreset(CPreset *pPreset) = 0;
	CPreset *operator()(int nID);
	virtual const char *GetName()=0;

protected:
	int FindUnusedID();
	void ValidateIDs();
};

class CPresetBatch : public vector<int> {
public:
	CPresetBatch(CPresetCollection *pCollection);
	CPresetBatch(CPresetCollection *pCollection, string strName, HKEY hKey);
	void Save(int nID, HKEY hKey);
	~CPresetBatch();

	string m_strName;
	CPreset *operator()(size_type nIndex);
	int ShowMenu();
protected:
	CPresetCollection *m_pCollection;
};

class CPresetBatchCollection : public vector<CPresetBatch *> {
public:
	CPresetBatchCollection(CPresetCollection *pCollection);
	void Save();
	~CPresetBatchCollection();

	typedef BOOL (*BatchExecutor)(CParameterBatch &Batch);
	int ShowMenu(BatchExecutor Executor = NULL, CParameterBatch &Batch = *((CParameterBatch *)NULL));
protected:
	CPresetCollection *m_pCollection;
};

#endif
