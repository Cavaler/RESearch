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
EXTERN vector<tstring> FSWords;
#ifdef UNICODE
EXTERN bool FCanUseDefCP;
#endif

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

EXTERN panelitem_vector g_PanelItems;
EXTERN int g_nFoundLine;
EXTERN int g_nFoundColumn;

#ifndef UNICODE
struct CharTableSet2 : public CharTableSet {
  unsigned char UpperDecodeTable[256];
};

EXTERN vector<CharTableSet2> XLatTables;
#endif

typedef void (*ProcessFileProc)(WIN32_FIND_DATA *FindData, panelitem_vector &PanelItems);

OperationResult FileFind(panelitem_vector &PanelItems,BOOL ShowDialog,BOOL bSilent=FALSE);
OperationResult FileReplace(panelitem_vector &PanelItems,BOOL ShowDialog,BOOL bSilent=FALSE);
OperationResult FileGrep(BOOL ShowDialog);
BOOL AdvancedSettings();
BOOL CompileAdvancedSettings();
void SelectAdvancedPreset(int &nID, bool &bSel);
void ApplyAdvancedPreset();

BOOL MaskCaseHere();
bool LocalFileTime(TCHAR cDrive);

OperationResult FileSearchExecutor();
OperationResult FileReplaceExecutor();
OperationResult FileGrepExecutor();

OperationResult NoFilesFound();
void InitFoundPosition();

BOOL MultipleMasksApply(const TCHAR *Filename);
void AddFile(WIN32_FIND_DATA *FindData,panelitem_vector &PanelItems);
int  ScanDirectories(panelitem_vector &PanelItems,ProcessFileProc ProcessFile);
int  FPrepareMaskPattern();
int  FPreparePattern(bool bAcceptEmpty);

BOOL ConfirmFile(int Title,const TCHAR *FileName);

void FReadRegistry(HKEY Key);
void FWriteRegistry(HKEY Key);
void FCleanup(BOOL PatternOnly);

#ifndef UNICODE
void XLatBuffer(BYTE *Buffer,int Length,int Table);
#endif
enum eLikeUnicode {UNI_NONE, UNI_LE, UNI_BE, UNI_UTF8};
eLikeUnicode LikeUnicode(const char *Buffer, int Size);
bool FromUnicodeDetect(const char *Buffer, int Size, vector<TCHAR> &arrData, eLikeUnicode nDetect);
bool FromUnicodeSkipDetect(const char *Buffer, int Size, vector<TCHAR> &arrData, eLikeUnicode nDetect);
bool FromUnicodeLE(const char *Buffer, int Size, vector<TCHAR> &arrData);
bool FromUnicodeBE(const char *Buffer, int Size, vector<TCHAR> &arrData);
bool FromUTF8(const char *Buffer, int Size, vector<TCHAR> &arrData);

template<class CHAR> void SkipNoCRLF(const CHAR *&Buffer,int *Size);
template<class CHAR> void SkipCRLF(const CHAR *&Buffer,int *Size);
template<class CHAR> void SkipWholeLine(const CHAR *&Buffer,int *Size);

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
	CTemporaryPanel(panelitem_vector &PanelItems,TCHAR *CalledFolder);
	~CTemporaryPanel();

	void GetOpenPluginInfo(OpenPluginInfo *Info);
	int  GetFindData(PluginPanelItem **PanelItem,int *ItemsNumber,int OpMode);
	int _SetDirectory(TCHAR *Name,int OpMode);
	int PutFiles(PluginPanelItem *AddItems,int AddNumber,int Move,int OpMode);
	int ProcessKey(int Key,unsigned int ControlState);
	void ClosePlugin();

	bool m_bActive;
private:
	vector<CPluginPanelItem> m_arrItems;
	tstring m_strFolder;
	KeyBarTitles KeyBar;

	void UpdateList();
};
EXTERN CTemporaryPanel *LastTempPanel VALUE(NULL);

#endif
