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

extern CParameterBatch g_ESBatch;
extern CParameterBatch g_ERBatch;
extern CParameterBatch g_EFBatch;
extern CESPresetCollection *ESPresets;
extern CERPresetCollection *ERPresets;
extern CEFPresetCollection *EFPresets;
extern CPresetBatchCollection *ERBatch;
extern CPresetBatchCollection *EFBatch;

extern int LastAction;
extern string EText;
extern BOOL ERegExp;
extern BOOL ESeveralLine;
extern BOOL ECaseSensitive;
extern string ERReplace;
extern BOOL ERRemoveEmpty;
extern BOOL ERRemoveNoMatch;
extern BOOL EFLeaveFilter;
extern BOOL EReverse;
extern BOOL EInSelection;
extern BOOL EUTF8;

extern pcre *EPattern;
extern pcre_extra *EPatternExtra;
extern int *Match;
extern int MatchCount;
extern char *MatchedLine;
extern int MatchedLineLength;
extern int SelStartLine,SelStartPos,SelEndLine,SelEndPos,SelType;

BOOL EditorSearch();
BOOL EditorSearchAgain();
BOOL EditorReplace();
BOOL EditorReplaceAgain();
BOOL _EditorReplaceAgain();
BOOL EditorFilter();
BOOL EditorFilterAgain();

BOOL SearchInLine(const char *Line,int Length,int Start,int End,int *MatchStart,int *MatchLength,BOOL NeedMatch);
BOOL SearchInText(int &FirstLine,int &StartPos,int &LastLine,int &EndPos,BOOL NeedMatch);
int  TopLine(int Line,int ScreenHeight,int TotalLines);
int	 LeftColumn(int RightPosition,int ScreenWidth);
void SaveSelection();
void RestoreSelection();
BOOL EPreparePattern(string &SearchText);

void FindIfClockPresent();
void ShowCurrentLine(int CurLine,int TotalLines,int TotalColumns);
string PickupText();
void EditorToOEM(char *Buffer,int Length);
void EditorToOEM(EditorGetString &String);
void OEMToEditor(char *Buffer,int Length);

void SynchronizeWithFile(BOOL Replace);
void EReadRegistry(HKEY Key);
void EWriteRegistry(HKEY Key);
void DeleteMatchInfo();
void ECleanup(BOOL PatternOnly);

#endif
