/* Copyright (C) 2000,Kostromitinov Pavel */
#ifndef __EDITFIND_H
#define __EDITFIND_H
#include "..\RESearch.h"
#include "..\FileFind\FileFind.h"
#include "..\Presets.h"

class CESPresetCollection:public CPresetCollection {
public:
	CESPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "EditFind";}
};

class CERPresetCollection:public CPresetCollection {
public:
	CERPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "EditReplace";}
};

class CEFPresetCollection:public CPresetCollection {
public:
	CEFPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "EditFilter";}
};

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

extern CParameterSet g_ESParamSet;
extern CParameterSet g_ERParamSet;
extern CParameterSet g_EFParamSet;
extern CParameterSet g_ELParamSet;
EXTERN CESPresetCollection *ESPresets;
EXTERN CERPresetCollection *ERPresets;
EXTERN CEFPresetCollection *EFPresets;
EXTERN CPresetBatchCollection *ERBatch;
EXTERN CPresetBatchCollection *EFBatch;

EXTERN int LastAction VALUE(-1);

EXTERN string ERReplace_O2E;
EXTERN BOOL ERRemoveEmpty;
EXTERN BOOL ERRemoveNoMatch;
EXTERN bool EREvaluate VALUE(false);
EXTERN BOOL EReverse VALUE(FALSE);
EXTERN BOOL ESearchAgainCalled VALUE(FALSE);
EXTERN BOOL EInSelection;
EXTERN BOOL EUTF8 VALUE(FALSE);

EXTERN string ETextUpcase;
EXTERN pcre *EPattern VALUE(NULL);
EXTERN pcre_extra *EPatternExtra VALUE(NULL);
EXTERN const unsigned char *ECharacterTables VALUE(NULL);
EXTERN int *Match VALUE(NULL);
EXTERN int MatchCount VALUE(0);
EXTERN char *MatchedLine VALUE(NULL);
EXTERN int MatchedLineLength VALUE(0);
EXTERN int SelStartLine,SelStartPos,SelEndLine,SelEndPos,SelType;

EXTERN EditorInfo EdInfo;
void RefreshEditorInfo();

BOOL EditorSearch();
BOOL EditorSearchAgain();
BOOL EditorReplace();
BOOL EditorReplaceAgain();
BOOL _EditorReplaceAgain();
BOOL EditorFilter();
BOOL EditorFilterAgain();
BOOL EditorTransliterate();
BOOL EditorTransliterateAgain();
BOOL EditorListAll();
BOOL EditorListAllAgain();
BOOL EditorListAllShowResults();

OperationResult EditorSearchExecutor();
OperationResult EditorReplaceExecutor();
OperationResult EditorFilterExecutor();
OperationResult EditorListAllExecutor();

void PatchEditorInfo(EditorInfo &EdInfo);

BOOL SearchInLine(const char *Line,int Length,int Start,int End,int *MatchStart,int *MatchLength,BOOL NeedMatch);
BOOL SearchInText(int &FirstLine,int &StartPos,int &LastLine,int &EndPos,BOOL NeedMatch);
int  TopLine(int Line,int ScreenHeight,int TotalLines);
int	LeftColumn(int RightPosition,int ScreenWidth);
void SaveSelection();
void RestoreSelection();
BOOL EPreparePattern(string &SearchText);

void FindIfClockPresent();
void ShowCurrentLine(int CurLine,int TotalLines,int TotalColumns);
string PickupText();
void EditorToOEM(char *Buffer,int Length);
void EditorToOEM(EditorGetString &String);
void EditorToOEM(string &String);
void OEMToEditor(char *Buffer,int Length);
void OEMToEditor(string &String);

void SynchronizeWithFile(BOOL Replace);
void EReadRegistry(HKEY Key);
void EWriteRegistry(HKEY Key);
void DeleteMatchInfo();
void ECleanup(BOOL PatternOnly);

extern "C" const unsigned char *far_maketables(struct CharTableSet *pTable);

void EctlGetString(EditorGetString *String);
void EctlSetString(EditorSetString *String);
void EctlSetPosition(EditorSetPosition *Position);
void EctlForceSetPosition(EditorSetPosition *Position);

#endif
