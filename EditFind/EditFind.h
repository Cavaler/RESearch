/* Copyright (C) 2000,Kostromitinov Pavel */
#ifndef __EDITFIND_H
#define __EDITFIND_H
#include "..\RESearch.h"
#include "..\FileFind\FileFind.h"
#include "..\Presets.h"

class CESPresetCollection:public CStdPresetCollection {
public:
	CESPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("EditFind"), MESPreset) {}
	virtual bool EditPreset(CPreset *pPreset);
	virtual int  ID() { return 3; }
};

class CERPresetCollection:public CStdPresetCollection {
public:
	CERPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("EditReplace"), MERPreset) {}
	virtual bool EditPreset(CPreset *pPreset);
	virtual int  ID() { return 0; }
};

class CEFPresetCollection:public CStdPresetCollection {
public:
	CEFPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("EditFilter"), MEFPreset) {}
	virtual bool EditPreset(CPreset *pPreset);
	virtual int  ID() { return 1; }
};

class CETPresetCollection:public CStdPresetCollection {
public:
	CETPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("EditTransliterate"), METPreset) {}
	virtual bool EditPreset(CPreset *pPreset);
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
EXTERN bool ERRemoveEmpty;
EXTERN bool ERRemoveNoMatch;
EXTERN bool EREvaluate VALUE(false);
EXTERN bool EReverse VALUE(false);
EXTERN bool EIncremental VALUE(false);
EXTERN bool ESearchAgainCalled VALUE(false);
EXTERN bool EInSelection;
EXTERN int  EInSelectionPreset;
EXTERN bool EFromCurrentPosition VALUE(false);

EXTERN tstring ETextUpcase;
EXTERN pcre *EPattern VALUE(NULL);
EXTERN pcre_extra *EPatternExtra VALUE(NULL);
#ifdef UNICODE
EXTERN pcre16 *EPattern16 VALUE(NULL);
EXTERN pcre16_extra *EPattern16Extra VALUE(NULL);
#endif
EXTERN const unsigned char *ECharacterTables VALUE(NULL);
EXTERN int SelStartLine,SelStartPos,SelEndLine,SelEndPos,SelType;

EXTERN vector<TCHAR>	g_LineBuffer;
EXTERN vector<int>		g_LineOffsets;
EXTERN size_t			g_FirstLine;
EXTERN tstring			g_DefEOL;
EXTERN tstring			g_LastEOL;
void SetDefEOL(LPCTSTR szEOL);
void FillLineBuffer(size_t FirstLine, size_t LastLine);
void ClearLineBuffer();

EXTERN EditorInfo EdInfo;
EXTERN WORD BarColor;
#ifdef UNICODE
EXTERN wstring EditorFileName;
#endif
EXTERN EditorInfo StartEdInfo;
bool RefreshEditorInfo();
void RefreshEditorColorInfo();
void EditorFillNamedParameters();

bool EditorSearch();
bool EditorSearchAgain();
bool EditorReplace();
bool EditorReplaceAgain();
bool _EditorReplaceAgain();
bool EditorFilter();
bool EditorFilterAgain();
bool EditorRepeatText();
bool EditorRepeatAgain();
bool EditorTransliterate();
bool EditorTransliterateAgain();
bool EditorListAllAgain();
bool EditorCountAllAgain();

void UpdateESDialog(CFarDialog *pDlg, bool bCheckSel = true);
LONG_PTR WINAPI EditorSearchDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2);

bool EditorListAllHasResults();
bool EditorListAllShowResults(bool bImmediate);

EXTERN bool EListAllFromPreset;
EXTERN bool ECountAllFromPreset;
OperationResult EditorSearchExecutor();
OperationResult EditorReplaceExecutor();
OperationResult EditorFilterExecutor();
OperationResult EditorTransliterateExecutor();
void EditorSeekToBeginEnd();
bool EditorUpdateSelectionPosition();
void EditorUpdatePresetPosition();

void PatchEditorInfo(EditorInfo &EdInfo);

bool SearchInLine(const TCHAR *Line,int Length,int Start,int End,int *MatchStart,int *MatchLength,int nExecOptions);
bool SearchInText(int &FirstLine,int &StartPos,int &LastLine,int &EndPos,bool bSkipClear=false,bool bNotBOL=false);
void AdjustPosition(TCHAR *Lines, int &FirstLine, int &StartPos);
#if defined(FAR3) && defined(_WIN64)
void AdjustPosition(TCHAR *Lines, intptr_t &FirstLine, intptr_t &StartPos);
#endif
void Relative2Absolute(int Line,TCHAR *Lines,int MatchStart,int MatchLength,int &FirstLine,int &StartPos,int &LastLine,int &EndPos);

void EditorSearchOK(int FirstLine,int StartPos,int LastLine,int EndPos);
void DoEditReplace(int FirstLine, int StartPos, int &LastLine, int &EndPos, const tstring &Replace);

int  TopLine(int NeededLine);
int  TopLine(int FirstLine, int NeededLine, int LastLine);
int	 LeftColumn(int AtPosition);
int	 LeftColumn(int LeftPosition, int AtPosition, int RightPosition, int ScreenWidth);
void GetHighlightPosition(EditorSetPosition &Position, int FirstLine,int StartPos,int LastLine,int EndPos);

void SaveSelection();
void RestoreSelection();
void RestorePosition(const EditorInfo &StartEdInfo);
bool EPreparePattern(tstring &SearchText);

void FindIfClockPresent();
void ShowCurrentLine(int CurLine,int TotalLines,int TotalColumns);

tstring PickupSelection();
tstring PickupMultilineSelection();
tstring PickupWord();
tstring PickupText();

#ifndef UNICODE
void EditorToOEM(char *Buffer,int Length);
void EditorToOEM(EditorGetString &String);
void EditorToOEM(string &String);
void OEMToEditor(char *Buffer,int Length);
void OEMToEditor(string &String);
#endif

void SynchronizeWithFile(bool bReplace);
void EReadRegistry (CFarSettingsKey Key);
void EWriteRegistry(CFarSettingsKey Key);
void ECleanup(bool PatternOnly);

extern "C" const unsigned char *far_maketables(struct CharTableSet *pTable);

int  EctlGetString(EditorGetString *String);
tstring EctlGetString(int nLine);
tstring ToString   (const EditorGetString &String, int nAssureLength = -1);
tstring ToStringEOL(const EditorGetString &String, int nAssureLength = -1);
tstring GetEOL     (const EditorGetString &String);
void ToArray   (const EditorGetString &String, vector<TCHAR> &arrBuffer);
void ToArray   (const EditorSetString &String, vector<TCHAR> &arrBuffer);
void EctlSetString(EditorSetString *String);
void EctlSetStringWithWorkarounds(EditorSetString *String);
void EctlSetPosition(EditorSetPosition *Position);
void EctlForceSetPosition(EditorSetPosition *Position);

#endif
