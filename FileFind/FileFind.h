/* Copyright (C) 2000,Kostromitinov Pavel */
#ifndef __FILEFIND_H
#define __FILEFIND_H
#include "..\RESearch.h"

enum SearchAs {SA_PLAINTEXT,SA_REGEXP,SA_SEVERALLINE,SA_MULTILINE,SA_MULTITEXT,SA_MULTIREGEXP};
enum SearchIn {SI_ALLDRIVES,SI_ALLLOCAL,SI_FROMROOT,SI_FROMCURRENT,SI_CURRENTONLY,SI_SELECTED};
enum MaskCase {MC_SENSITIVE,MC_INSENSITIVE,MC_VOLUME};
enum eReplaceReadonly {RR_NEVER, RR_ASK, RR_ALWAYS};

class CFSPresetCollection:public CPresetCollection {
public:
	CFSPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "FileFind";}
};

class CFRPresetCollection:public CPresetCollection {
public:
	CFRPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "FileReplace";}
};

class CFAPresetCollection:public CPresetCollection {
public:
	CFAPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "FileAdvanced";}
};

extern CParameterBatch g_FSBatch;
extern CParameterBatch g_FRBatch;
extern CParameterBatch g_FABatch;
EXTERN CFSPresetCollection *FSPresets;
EXTERN CFRPresetCollection *FRPresets;
EXTERN CFAPresetCollection *FAPresets;
EXTERN CPresetBatchCollection *FRBatch;

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

EXTERN BOOL FUTF8 VALUE(FALSE);
EXTERN BOOL FAdvanced VALUE(FALSE);

// Internals

EXTERN string FTextUpcase;
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

EXTERN string MaskText;
EXTERN string SearchText;
EXTERN string ReplaceText;

EXTERN int  FileConfirmed,FRConfirmFileThisRun,FRConfirmReadonlyThisRun;
EXTERN int  FRConfirmLineThisRun,FRConfirmLineThisFile;

EXTERN PluginPanelItem *PanelItems;
EXTERN int ItemsNumber;

struct CharTableSet2 : public CharTableSet {
  unsigned char UpperDecodeTable[256];
};

EXTERN vector<CharTableSet2> XLatTables;

typedef void (*ProcessFileProc)(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber);

OperationResult FileFind(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog);
OperationResult FileReplace(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog);
BOOL AdvancedSettings();
BOOL MaskCaseHere();

OperationResult NoFilesFound();

BOOL MultipleMasksApply(const string &Masks, const char *Filename);
void AddFile(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber);
int  ScanDirectories(PluginPanelItem **PanelItems,int *ItemsNumber,ProcessFileProc ProcessFile);
int  FPreparePattern();

BOOL ConfirmFile(int Title,const char *FileName);
void SkipNoCRLF(const char *&Buffer,int *Size);
void SkipCRLF(const char *&Buffer,int *Size);
void SkipWholeLine(const char *&Buffer,int *Size);

void FReadRegistry(HKEY Key);
void FWriteRegistry(HKEY Key);
void FCleanup(BOOL PatternOnly);

class CTemporaryPanel {
public:
	CTemporaryPanel(PluginPanelItem *NewItems,int NewCount,char *CalledFolder);
	~CTemporaryPanel();

	void GetOpenPluginInfo(OpenPluginInfo *Info);
	int GetFindData(PluginPanelItem **PanelItem,int *ItemsNumber,int OpMode);
	int _SetDirectory(char *Name,int OpMode);
	int PutFiles(PluginPanelItem *AddItems,int AddNumber,int Move,int OpMode);
	int ProcessKey(int Key,unsigned int ControlState);
	void ClosePlugin();
private:
	PluginPanelItem *Items;
	int Count;
	char *Folder;
	KeyBarTitles KeyBar;

	void UpdateList();
};

#endif
