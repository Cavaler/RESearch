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

enum EPositioning {EP_BEGIN, EP_DIR, EP_END};

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

#ifndef UNICODE
EXTERN tstring ERReplace_O2E;
#endif
EXTERN BOOL ERRemoveEmpty;
EXTERN BOOL ERRemoveNoMatch;
EXTERN bool EREvaluate VALUE(false);
EXTERN BOOL EReverse VALUE(FALSE);
EXTERN BOOL ESearchAgainCalled VALUE(FALSE);
EXTERN BOOL EInSelection;

EXTERN tstring ETextUpcase;
EXTERN pcre *EPattern VALUE(NULL);
EXTERN pcre_extra *EPatternExtra VALUE(NULL);
EXTERN const unsigned char *ECharacterTables VALUE(NULL);
EXTERN int SelStartLine,SelStartPos,SelEndLine,SelEndPos,SelType;

EXTERN vector<TCHAR>	g_LineBuffer;
EXTERN vector<int>		g_LineOffsets;
EXTERN size_t			g_FirstLine;
EXTERN tstring			g_DefEOL;

EXTERN EditorInfo EdInfo;
#ifdef UNICODE
EXTERN wstring EditorFileName;
#endif
EXTERN EditorInfo StartEdInfo;
void RefreshEditorInfo();
void EditorFillNamedParameters();

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

void UpdateESDialog(CFarDialog *pDlg, bool bCheckSel = true);
LONG_PTR WINAPI EditorSearchDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2);

BOOL EditorListAllHasResults();
BOOL EditorListAllShowResults(bool bImmediate);

EXTERN BOOL EListAllFromPreset;
OperationResult EditorSearchExecutor();
OperationResult EditorReplaceExecutor();
OperationResult EditorFilterExecutor();
OperationResult EditorTransliterateExecutor();
void EditorSeekToBeginEnd();

void PatchEditorInfo(EditorInfo &EdInfo);

BOOL SearchInLine(const TCHAR *Line,int Length,int Start,int End,int *MatchStart,int *MatchLength);
BOOL SearchInText(int &FirstLine,int &StartPos,int &LastLine,int &EndPos,bool bSkipClear=false);
void AdjustPosition(TCHAR *Lines, int &FirstLine,int &StartPos);
void Relative2Absolute(int Line,TCHAR *Lines,int MatchStart,int MatchLength,int &FirstLine,int &StartPos,int &LastLine,int &EndPos);

void EditorSearchOK(int FirstLine,int StartPos,int LastLine,int EndPos);

int  TopLine(int NeededLine);
int  TopLine(int FirstLine, int NeededLine, int LastLine);
int	 LeftColumn(int AtPosition);
int	 LeftColumn(int LeftPosition, int AtPosition, int RightPosition, int ScreenWidth);
void GetHighlightPosition(EditorSetPosition &Position, int FirstLine,int StartPos,int LastLine,int EndPos);

void SaveSelection();
void RestoreSelection();
void RestorePosition(const EditorInfo &StartEdInfo);
BOOL EPreparePattern(tstring &SearchText);

void FindIfClockPresent();
void ShowCurrentLine(int CurLine,int TotalLines,int TotalColumns);
tstring PickupText();

#ifndef UNICODE
void EditorToOEM(char *Buffer,int Length);
void EditorToOEM(EditorGetString &String);
void EditorToOEM(string &String);
void OEMToEditor(char *Buffer,int Length);
void OEMToEditor(string &String);
#endif

void SynchronizeWithFile(bool bReplace);
void EReadRegistry(HKEY Key);
void EWriteRegistry(HKEY Key);
void ECleanup(BOOL PatternOnly);

extern "C" const unsigned char *far_maketables(struct CharTableSet *pTable);

void EctlGetString(EditorGetString *String);
tstring EctlGetString(int nLine);
tstring ToString(EditorGetString &String);
void EctlSetString(EditorSetString *String);
void EctlSetPosition(EditorSetPosition *Position);
void EctlForceSetPosition(EditorSetPosition *Position);

#endif
