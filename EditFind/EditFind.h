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

class CELPresetCollection:public CPresetCollection {
public:
	CELPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "EditList";}
};

extern CParameterBatch g_ESBatch;
extern CParameterBatch g_ERBatch;
extern CParameterBatch g_EFBatch;
extern CParameterBatch g_ELBatch;
EXTERN CESPresetCollection *ESPresets;
EXTERN CERPresetCollection *ERPresets;
EXTERN CEFPresetCollection *EFPresets;
EXTERN CELPresetCollection *ELPresets;
EXTERN CPresetBatchCollection *ERBatch;
EXTERN CPresetBatchCollection *EFBatch;

EXTERN int LastAction VALUE(-1);
EXTERN string EText;
EXTERN BOOL ERegExp;
EXTERN BOOL ESeveralLine;
EXTERN BOOL ECaseSensitive;
EXTERN string ERReplace;
EXTERN string ERReplace_O2E;
EXTERN BOOL ERRemoveEmpty;
EXTERN BOOL ERRemoveNoMatch;
EXTERN bool EREvaluate VALUE(false);
EXTERN int EREvaluateScript;
EXTERN BOOL EFLeaveFilter;
EXTERN BOOL EReverse VALUE(FALSE);
EXTERN BOOL ESearchAgainCalled VALUE(FALSE);
EXTERN BOOL EInSelection;
EXTERN BOOL EUTF8 VALUE(FALSE);
EXTERN string ETSource;
EXTERN string ETTarget;

EXTERN string ETextUpcase;
EXTERN pcre *EPattern VALUE(NULL);
EXTERN pcre_extra *EPatternExtra VALUE(NULL);
EXTERN const unsigned char *ECharacterTables VALUE(NULL);
EXTERN int *Match VALUE(NULL);
EXTERN int MatchCount VALUE(0);
EXTERN char *MatchedLine VALUE(NULL);
EXTERN int MatchedLineLength VALUE(0);
EXTERN int SelStartLine,SelStartPos,SelEndLine,SelEndPos,SelType;

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

#endif
