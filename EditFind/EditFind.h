/* Copyright (C) 2000,Kostromitinov Pavel */
#ifndef __EDITFIND_H
#define __EDITFIND_H
#include "..\RESearch.h"
#include "..\FileFind\FileFind.h"
#include "..\Presets.h"

class CESPresetCollection:public CStdPresetCollection {
public:
	CESPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("EditFind"), MESPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 3; }
};

class CERPresetCollection:public CStdPresetCollection {
public:
	CERPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("EditReplace"), MERPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 0; }
};

class CEFPresetCollection:public CStdPresetCollection {
public:
	CEFPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("EditFilter"), MEFPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 1; }
};

class CETPresetCollection:public CStdPresetCollection {
public:
	CETPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("EditTransliterate"), METPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 2; }
};

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

extern CParameterSet g_ESParamSet;
extern CParameterSet g_ERParamSet;
extern CParameterSet g_EFParamSet;
extern CParameterSet g_ETParamSet;
EXTERN CESPresetCollection *ESPresets;
EXTERN CERPresetCollection *ERPresets;
EXTERN CEFPresetCollection *EFPresets;
EXTERN CETPresetCollection *ETPresets;

EXTERN int LastAction VALUE(-1);

EXTERN tstring ERReplace_O2E;
EXTERN BOOL ERRemoveEmpty;
EXTERN BOOL ERRemoveNoMatch;
EXTERN bool EREvaluate VALUE(false);
EXTERN BOOL EReverse VALUE(FALSE);
EXTERN BOOL ESearchAgainCalled VALUE(FALSE);
EXTERN BOOL EInSelection;
EXTERN BOOL EUTF8 VALUE(FALSE);

EXTERN tstring ETextUpcase;
EXTERN pcre *EPattern VALUE(NULL);
EXTERN pcre_extra *EPatternExtra VALUE(NULL);
EXTERN const unsigned char *ECharacterTables VALUE(NULL);
EXTERN int *Match VALUE(NULL);
EXTERN int MatchCount VALUE(0);
EXTERN char *MatchedLine VALUE(NULL);
EXTERN int MatchedLineLength VALUE(0);
EXTERN int SelStartLine,SelStartPos,SelEndLine,SelEndPos,SelType;

EXTERN EditorInfo EdInfo;
EXTERN EditorInfo StartEdInfo;
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
BOOL EditorListAllShowResults(bool bImmediate);

EXTERN BOOL EListAllFromPreset;
OperationResult EditorSearchExecutor();
OperationResult EditorReplaceExecutor();
OperationResult EditorFilterExecutor();
OperationResult EditorTransliterateExecutor();
void EditorSeekToBeginEnd();

void PatchEditorInfo(EditorInfo &EdInfo);

BOOL SearchInLine(const char *Line,int Length,int Start,int End,int *MatchStart,int *MatchLength,BOOL NeedMatch);
BOOL SearchInText(int &FirstLine,int &StartPos,int &LastLine,int &EndPos,BOOL NeedMatch);
int  TopLine(int NeededLine,int ScreenHeight,int TotalLines,int CurrentTopLine);
int	LeftColumn(int RightPosition,int ScreenWidth);
void SaveSelection();
void RestoreSelection();
void RestorePosition(const EditorInfo &StartEdInfo);
BOOL EPreparePattern(tstring &SearchText);

void FindIfClockPresent();
void ShowCurrentLine(int CurLine,int TotalLines,int TotalColumns);
string PickupText();
void EditorToOEM(char *Buffer,int Length);
void EditorToOEM(EditorGetString &String);
void EditorToOEM(string &String);
void OEMToEditor(char *Buffer,int Length);
void OEMToEditor(string &String);

void SynchronizeWithFile(bool bReplace);
void EReadRegistry(HKEY Key);
void EWriteRegistry(HKEY Key);
void DeleteMatchInfo();
void ECleanup(BOOL PatternOnly);

extern "C" const unsigned char *far_maketables(struct CharTableSet *pTable);

void EctlGetString(EditorGetString *String);
string EctlGetString(int nLine);
string ToString(EditorGetString &String);
void EctlSetString(EditorSetString *String);
void EctlSetPosition(EditorSetPosition *Position);
void EctlForceSetPosition(EditorSetPosition *Position);

#endif
