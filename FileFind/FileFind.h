/* Copyright (C) 2000,Kostromitinov Pavel */
#ifndef __FILEFIND_H
#define __FILEFIND_H
#include "..\RESearch.h"

enum SearchAs {SA_PLAINTEXT,SA_REGEXP,SA_SEVERALLINE,SA_MULTILINE,SA_MULTITEXT,SA_MULTIREGEXP};
enum SearchIn {SI_ALLDRIVES,SI_ALLLOCAL,SI_FROMROOT,SI_FROMCURRENT,SI_CURRENTONLY,SI_SELECTED};
enum MaskCase {MC_SENSITIVE,MC_INSENSITIVE,MC_VOLUME};
enum eReplaceReadonly {RR_NEVER, RR_ASK, RR_ALWAYS};
enum GrepWhat {GREP_NAMES, GREP_NAMES_COUNT, GREP_LINES, GREP_NAMES_LINES};

class CFPreset : public CPreset {
public:
	CFPreset(CParameterSet &ParamSet);
	CFPreset(CParameterSet &ParamSet, const tstring &strName, HKEY hKey);
	virtual void Apply();
};

typedef CPresetCollectionT<CFPreset> CFPresetCollection;

class CFSPresetCollection:public CFPresetCollection {
public:
	CFSPresetCollection(CParameterSet &ParamSet) : CFPresetCollection(ParamSet, _T("FileFind"), MFSPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 3; }
};

class CFRPresetCollection:public CFPresetCollection {
public:
	CFRPresetCollection(CParameterSet &ParamSet) : CFPresetCollection(ParamSet, _T("FileReplace"), MFRPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 0; }
};

class CFGPresetCollection:public CFPresetCollection {
public:
	CFGPresetCollection(CParameterSet &ParamSet) : CFPresetCollection(ParamSet, _T("FileGrep"), MFGPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 4; }
};

class CFAPresetCollection:public CStdPresetCollection {
public:
	CFAPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("FileAdvanced"), MFAPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 5; }
};

extern CParameterSet g_FSParamSet;
extern CParameterSet g_FRParamSet;
extern CParameterSet g_FGParamSet;
extern CParameterSet g_FAParamSet;
EXTERN CFSPresetCollection *FSPresets;
EXTERN CFRPresetCollection *FRPresets;
EXTERN CFGPresetCollection *FGPresets;
EXTERN CFAPresetCollection *FAPresets;

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

EXTERN BOOL FUTF8 VALUE(FALSE);
EXTERN BOOL FAdvanced VALUE(FALSE);
EXTERN int  FAdvancedID VALUE(0);

// Internals

EXTERN tstring FTextUpcase;
EXTERN pcre *FPattern;
EXTERN pcre_extra *FPatternExtra;
EXTERN vector<string> FSWords;

EXTERN pcre *FMaskPattern VALUE(NULL);
EXTERN pcre_extra *FMaskPatternExtra VALUE(NULL);
EXTERN CFarMaskSet *FMaskSet VALUE(NULL);

EXTERN pcre *FAFullFileNamePattern VALUE(NULL);
EXTERN pcre_extra *FAFullFileNamePatternExtra VALUE(NULL);
EXTERN pcre *FADirectoryPattern VALUE(NULL);
EXTERN pcre_extra *FADirectoryPatternExtra VALUE(NULL);
EXTERN DWORD CurrentRecursionLevel;
EXTERN int  FilesScanned;

EXTERN CFarMaskSet *FASystemFoldersMask VALUE(NULL);

EXTERN tstring MaskText;
EXTERN tstring SearchText;
EXTERN tstring ReplaceText;

EXTERN CFarListData g_WhereToSearch;
EXTERN CFarListData g_WhereToSearchPlugin;

EXTERN int  FileConfirmed,FRConfirmFileThisRun,FRConfirmReadonlyThisRun;
EXTERN int  FRConfirmLineThisRun,FRConfirmLineThisFile;

EXTERN PluginPanelItem *PanelItems;
EXTERN int ItemsNumber;
EXTERN int g_nFoundLine;
EXTERN int g_nFoundColumn;

#ifndef UNICODE
struct CharTableSet2 : public CharTableSet {
  unsigned char UpperDecodeTable[256];
};

EXTERN vector<CharTableSet2> XLatTables;
#endif

typedef void (*ProcessFileProc)(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber);

OperationResult FileFind(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog,BOOL bSilent=FALSE);
OperationResult FileReplace(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog,BOOL bSilent=FALSE);
OperationResult FileGrep(BOOL ShowDialog);
BOOL AdvancedSettings();
BOOL CompileAdvancedSettings();
void SelectAdvancedPreset(int &nID, bool &bSel);
void ApplyAdvancedPreset();

BOOL MaskCaseHere();
bool LocalFileTime(char cDrive);

OperationResult FileSearchExecutor();
OperationResult FileReplaceExecutor();
OperationResult FileGrepExecutor();

OperationResult NoFilesFound();
void InitFoundPosition();

BOOL MultipleMasksApply(const string &Masks, const char *Filename);
void AddFile(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber);
int  ScanDirectories(PluginPanelItem **PanelItems,int *ItemsNumber,ProcessFileProc ProcessFile);
int  FPrepareMaskPattern();
int  FPreparePattern(bool bAcceptEmpty);

BOOL ConfirmFile(int Title,const TCHAR *FileName);

void FReadRegistry(HKEY Key);
void FWriteRegistry(HKEY Key);
void FCleanup(BOOL PatternOnly);

enum eLikeUnicode {UNI_NONE, UNI_LE, UNI_BE, UNI_UTF8};

void SkipNoCRLF(const char *&Buffer,int *Size);
void SkipCRLF(const char *&Buffer,int *Size);
void SkipWholeLine(const char *&Buffer,int *Size);

void SkipNoCRLF(const char *&Buffer,int *Size, eLikeUnicode nUni);
void SkipCRLF(const char *&Buffer,int *Size, eLikeUnicode nUni);
void SkipWholeLine(const char *&Buffer,int *Size, eLikeUnicode nUni);


struct TempUserData {
	TempUserData() : FoundLine(0), FoundColumn(1), ToBeDeleted(false) {}
	TempUserData(int nLine, int nColumn) : FoundLine(nLine), FoundColumn(nColumn), ToBeDeleted(false) {}

	int		FoundLine;
	int		FoundColumn;
	bool	ToBeDeleted;
};

class CTemporaryPanel {
public:
	CTemporaryPanel(PluginPanelItem *NewItems,int NewCount,TCHAR *CalledFolder);
	~CTemporaryPanel();

	void GetOpenPluginInfo(OpenPluginInfo *Info);
	int  GetFindData(PluginPanelItem **PanelItem,int *ItemsNumber,int OpMode);
	int _SetDirectory(TCHAR *Name,int OpMode);
	int PutFiles(PluginPanelItem *AddItems,int AddNumber,int Move,int OpMode);
	int ProcessKey(int Key,unsigned int ControlState);
	void ClosePlugin();

	bool m_bActive;
private:
	PluginPanelItem *Items;
	int Count;
	TCHAR *Folder;
	KeyBarTitles KeyBar;

	void UpdateList();
};
EXTERN CTemporaryPanel *LastTempPanel VALUE(NULL);

#endif
