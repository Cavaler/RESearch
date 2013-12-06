#pragma once

#include "FileOperations.h"

enum SearchAs {SA_PLAINTEXT,SA_REGEXP,SA_SEVERALLINE,SA_MULTILINE,SA_MULTITEXT,SA_MULTIREGEXP};
enum SearchIn {SI_ALLDRIVES,SI_ALLLOCAL,SI_FROMROOT,SI_FROMCURRENT,SI_CURRENTONLY,SI_SELECTED};
enum MaskCase {MC_SENSITIVE,MC_INSENSITIVE,MC_VOLUME};
enum eReplaceReadonly {RR_NEVER, RR_ASK, RR_ALWAYS};

class CFPreset : public CPreset {
public:
	CFPreset(CParameterSet &ParamSet);
	CFPreset(CParameterSet &ParamSet, const tstring &strName, CFarSettingsKey &hKey);
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

EXTERN BOOL FAdvanced VALUE(FALSE);
EXTERN int  FAdvancedID VALUE(0);
EXTERN bool FRegExp;
EXTERN bool FREvaluate VALUE(false);

// Internals

EXTERN tstring FTextUpcase;
EXTERN pcre *FPattern;
EXTERN pcre_extra *FPatternExtra;
EXTERN vector<tstring> FSWords;
#ifdef UNICODE
EXTERN string FOEMTextUpcase;
EXTERN string FOEMReplace;
EXTERN bool FCanUseDefCP;
EXTERN pcre *FPatternA;
EXTERN pcre_extra *FPatternExtraA;
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
EXTERN int  ScanProgressX;

EXTERN CFarMaskSet *FASystemFoldersMask VALUE(NULL);

EXTERN tstring MaskText;
EXTERN tstring SearchText;
EXTERN tstring ReplaceText;

EXTERN CFarListData g_WhereToSearch;
EXTERN CFarListData g_WhereToSearchPlugin;

EXTERN bool FileConfirmed,FRConfirmFileThisRun,FRConfirmReadonlyThisRun;
EXTERN bool FRConfirmLineThisRun, FRConfirmLineThisFile, FRSkipThisFile;

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

LONG_PTR WINAPI FileSearchDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2);

BOOL MaskCaseHere();
bool LocalFileTime(TCHAR cDrive);

OperationResult FileSearchExecutor();
OperationResult FileReplaceExecutor();
OperationResult FileGrepExecutor();

OperationResult NoFilesFound();
void InitFoundPosition();

bool MultipleMasksApply(const TCHAR *Filename);
void FileFillNamedParameters(const TCHAR *szFileName);
void AddFile(WIN32_FIND_DATA *FindData, panelitem_vector &PanelItems, bool bSearch = false);
void AddFile(const TCHAR *szFileName, panelitem_vector &PanelItems, bool bSearch = false);
int  ScanDirectories(panelitem_vector &PanelItems,ProcessFileProc ProcessFile);
int  FPrepareMaskPattern();
int  FPreparePattern(bool bAcceptEmpty);

BOOL ConfirmFile(int Title,const TCHAR *FileName);
bool ConfirmFileReadonly(const TCHAR *FileName);
bool ConfirmReplacement();
bool ConfirmReplacement(const TCHAR *Found, const TCHAR *Replaced, const TCHAR *FileName);

void FReadRegistry (CFarSettingsKey Key);
void FWriteRegistry(CFarSettingsKey Key);
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

bool FromUTF8(const char *Buffer, int Size, vector<char> &arrData);
bool FromUTF8(const char *Buffer, int Size, vector<wchar_t> &arrData);

//	All sizes are in characters

template<class CHAR> void SkipNoCRLF(const CHAR *&Buffer,int *Size);
template<class CHAR> void SkipCRLF(const CHAR *&Buffer,int *Size);
template<class CHAR> void SkipWholeLine(const CHAR *&Buffer,int *Size);

void SkipNoCRLF(const char *&Buffer,int *Size, eLikeUnicode nUni);
void SkipCRLF(const char *&Buffer,int *Size, eLikeUnicode nUni);
void SkipWholeLine(const char *&Buffer,int *Size, eLikeUnicode nUni);

struct TempUserData {
	TempUserData()
		: FoundLine(0), FoundColumn(1), NeedSearch(false), ToBeDeleted(false) {}
	TempUserData(int nLine, int nColumn, bool bSearch)
		: FoundLine(nLine), FoundColumn(nColumn), NeedSearch(bSearch), ToBeDeleted(false) {}

	int		FoundLine;
	int		FoundColumn;
	bool	NeedSearch;
	bool	ToBeDeleted;
};

class CTemporaryPanel {
public:
	CTemporaryPanel(panelitem_vector &PanelItems,TCHAR *CalledFolder);
	~CTemporaryPanel();
#ifdef FAR3
	void GetOpenPanelInfo(OpenPanelInfo *Info);
	int ProcessPanelInput(const INPUT_RECORD *pInput);
#else
	void GetOpenPluginInfo(OpenPluginInfo *Info);
#endif
	int  GetFindData(PluginPanelItem **PanelItem,int *ItemsNumber,int OpMode);
	int _SetDirectory(const TCHAR *Name,int OpMode);
	int PutFiles(PluginPanelItem *AddItems,int AddNumber,int Move,int OpMode);
	int ProcessKey(int Key,unsigned int ControlState);
	void ClosePlugin();

	bool m_bActive;
private:
	vector<CPluginPanelItem> m_arrItems;
	tstring m_strBaseFolder;
	KeyBarTitles KeyBar;
#ifdef FAR3
	KeyBarLabel KeyBarLabels[1];
#endif

	void UpdateList();
};
EXTERN CTemporaryPanel *LastTempPanel VALUE(NULL);
