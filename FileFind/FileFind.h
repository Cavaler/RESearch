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

extern CParameterBatch g_FSBatch;
extern CParameterBatch g_FRBatch;
extern CFSPresetCollection *FSPresets;
extern CFRPresetCollection *FRPresets;
extern CPresetBatchCollection *FRBatch;

extern BOOL FCaseSensitive;
extern SearchAs FSearchAs;
extern SearchIn FSearchIn;
extern string FMask;
extern BOOL FMaskAsRegExp;
extern MaskCase FMaskCase;
extern eReplaceReadonly FRReplaceReadonly;
extern string FText;
extern BOOL FUTF8;

extern BOOL FSInverse;
extern BOOL FAllCharTables;
extern string FRReplace;
extern BOOL FROpenModified;
extern BOOL FRConfirmFile;
extern BOOL FRConfirmLine;
extern BOOL FRSaveOriginal;
extern BOOL FRepeating;

extern BOOL FAdvanced;
extern BOOL FACaseSensitive;
extern BOOL FADirectoryCaseSensitive;
extern BOOL FASearchHead;
extern DWORD FASearchHeadLimit;

extern string FTextUpcase;
extern pcre *FPattern;
extern pcre_extra *FPatternExtra;
extern vector<string> FSWords;

extern string MaskText;
extern string SearchText;
extern string ReplaceText;

extern int  FileConfirmed,FRConfirmFileThisRun,FRConfirmReadonlyThisRun;
extern int  FRConfirmLineThisRun,FRConfirmLineThisFile;

extern PluginPanelItem *PanelItems;
extern int ItemsNumber;

typedef char XLatTable[256];
extern XLatTable *XLatTables;
extern XLatTable *UpCaseXLatTables;
extern int XLatTableCount;

typedef void (*ProcessFileProc)(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber);

OperationResult FileFind(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog);
OperationResult FileReplace(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog);
BOOL AdvancedSettings();
BOOL MaskCaseHere();

OperationResult NoFilesFound();

BOOL MultipleMasksApply(string Masks,char *Filename);
void AddFile(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber);
int  ScanDirectories(PluginPanelItem **PanelItems,int *ItemsNumber,ProcessFileProc ProcessFile);
int  FPreparePattern();

BOOL ConfirmFile(int Title,const char *FileName);
void SkipNoCRLF(char *&Buffer,int *Size);
void SkipCRLF(char *&Buffer,int *Size);
void SkipWholeLine(char *&Buffer,int *Size);

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
