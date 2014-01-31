#ifndef __RESEARCH_H
#define __RESEARCH_H

#undef _FAR_NO_NAMELESS_UNIONS
#include <FAR.h>
#include <FarDlg.h>
#include <FarCombinedKeys.h>

#include "RESearch_msg.h"

#ifdef UNICODE
#define DIF_VAREDIT 0
#define _tmemchr wmemchr
#else
#define _tmemchr memchr
#define CP_UNICODE 1200
#define CP_REVERSEBOM 1201
#endif

#define arrsizeof(arr) (sizeof(arr)/sizeof(arr[0]))

enum OperationResult {OR_CANCEL,OR_FAILED,OR_OK,OR_PANEL};

//	#define TRY_ENCODINGS_WITH_BOM

#include "Presets.h"

enum   ShowPosition {SP_TOP,SP_CENTER,SP_BOTTOM};
enum   FindTextAtCursor {FT_NONE,FT_WORD,FT_ANY};

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

EXTERN HANDLE g_hInstance;

#ifdef UNICODE
typedef set<UINT> cp_set;
EXTERN cp_set	g_setAllCPs;
#endif

EXTERN bool g_bFromCmdLine;

struct sActiveScript {
	tstring m_strName;
	CLSID  m_clsid;
};
EXTERN vector<sActiveScript> m_arrEngines;
EXTERN CFarListData m_lstEngines CONSTRUCT((NULL, 0));

bool SystemToLocalTime(FILETIME &ft);
bool LocalToSystemTime(FILETIME &ft);

EXTERN CFarSettingsKey Settings;
CFarSettingsKey GetSettings();
void ReadRegistry();
void WriteRegistry();

void ReadActiveScripts();
void EnumActiveScripts();
void SaveActiveScripts();

LPCTSTR ScriptEngine(bool bEnabled);
int  EngineIndex(const tstring &strValue);
void SetEngineIndex(int nIndex, tstring &strValue);
void SanitateEngine();

class CFarEngineStorage : public CFarIntegerStorage
{
public:
	CFarEngineStorage(tstring &strValue);

	virtual void Get(TCHAR *pszBuffer, int nSize) const;
	virtual void Put(int nValue);
protected:
	mutable int	m_nEngine;
	tstring &m_strEngine;
};

void ConfigureCommon();
void ConfigureFile();
void ConfigureGrep();
void ConfigureRenumbering(bool bRuntime);
void ConfigureEditor();
int  ConfigureSeveralLines();

EXTERN ECaseConvert CaseConvert;
EXTERN ECaseConvert OneCaseConvert;
#ifdef UNICODE
TCHAR ConvertCase_OEM(TCHAR C);
#endif
TCHAR ConvertCase(TCHAR C);
#include "StringTemplates.h"

typedef CREParameters<TCHAR> TREParameters;
EXTERN TREParameters REParam;
void MatchDone();

EXTERN int FileNumber;
EXTERN int FindNumber;
EXTERN int ReplaceNumber;
EXTERN DWORD LastTickCount VALUE(0);

EXTERN int REErrorField  VALUE(-1);
EXTERN int REErrorOffset VALUE(-1);

bool CheckUsage(const tstring &strText, bool bRegExp, bool bSeveralLine);
bool PreparePattern(pcre **Pattern,pcre_extra **PatternExtra,const tstring &Text,int CaseSensitive,const unsigned char *pTables=NULL);
void HighlightREError(CFarDialog *pDlg);

void FillDefaultNamedParameters(const TCHAR *szFileName);
void ClearVariables();

template<class CHAR>
basic_string<CHAR> EvaluateReplaceString(CREParameters<CHAR> &Param, const CHAR *Replace, const CHAR *EOL, LPCTSTR szEngine);

#ifdef UNICODE
bool PreparePattern(pcre **Pattern,pcre_extra **PatternExtra,const string &Text,int CaseSensitive,const unsigned char *pTables=NULL);
bool PreparePattern(pcre16 **Pattern,pcre16_extra **PatternExtra,const wstring &Text,int CaseSensitive);
#endif

bool LoadPresets(char *Which,char **StringNames,int StringCount,char **IntNames,int IntCount,void **PresetData,int *PresetCount);
bool SavePresets(char *Which,char **StringNames,int StringCount,char **IntNames,int IntCount,void *PresetData,int PresetCount);

void ShowErrorMsg(const TCHAR *sz1, const TCHAR *sz2 = NULL, const TCHAR *szHelp = NULL);
void ShowHResultError(int nError, HRESULT hResult, const TCHAR *szHelp = NULL);

EXTERN CONSOLE_SCREEN_BUFFER_INFO ConInfo;
void RefreshConsoleInfo();

void   QuoteString (const TCHAR *Source, int Length, vector<tstring> &arrQuoted, int MaxWidth);
size_t QuoteStrings(const TCHAR *Source, vector<tstring> &arrQuoted, int MaxWidth, int nMaxHeight = -1);

size_t MakeSameWidth(vector<tstring> &arrQuoted);

bool IsOKClose(int ExitCode);

#ifdef UNICODE
EXTERN TCHAR UpCaseTable[65537];
void BuildUpCaseTable(UINT nCP, char *pTable);
char *GetUpCaseTable(int nCP);
typedef map<UINT, char *> upcase_map;
EXTERN upcase_map UpCaseTables;
#else
EXTERN char UpCaseTable[256];
EXTERN CharTableSet *m_pReplaceTable;
#endif

void PrepareLocaleStuff();
tstring UpCaseString(const tstring &strText);
tstring RemoveAmpersand(const tstring &strText);

UINT GetDefCP();
UINT IsDefCP(UINT nCP);
#ifdef UNICODE
wstring DefToUnicode(const string &strDef);
string  DefFromUnicode(const wstring &strUnicode);
const unsigned char *DefCharTables();
bool    CanUseCP(UINT nCP, const wstring &strUnicode);
#endif

void PrepareBMHSearch(const TCHAR *String,int StringLength,size_t nPattern = 0);
int BMHSearch(const TCHAR *Buffer,int BufferLength,const TCHAR *String,int StringLength,TCHAR *XLatTable,int nPattern = 0);
int ReverseBMHSearch(const TCHAR *Buffer,int BufferLength,const TCHAR *String,int StringLength,TCHAR *XLatTable,int nPattern = 0);

#ifdef UNICODE
void PrepareBMHSearchA(const char *String,int StringLength);
int BMHSearchA(const char *Buffer,int BufferLength,const char *String,int StringLength,char *XLatTable);
#endif

void EditorStartUndo();
void EditorEndUndo();

void StartREThread();
void StopREThread();
int do_pcre_exec(const pcre *external_re, const pcre_extra *extra_data,
	const TCHAR *subject, int length, int start_offset, int options, int *offsets,
	int offsetcount);
#ifdef UNICODE
int do_pcre16_exec(const pcre16 *external_re, const pcre16_extra *extra_data,
	const wchar_t *subject, int length, int start_offset, int options, int *offsets,
	int offsetcount);
#else
#define FPatternA FPattern
#define FPatternExtraA FPatternExtra
#endif

bool RunREBuilder(tstring &strSearch, tstring &strReplace);
void RunExternalEditor(tstring &strText);
void RunExternalViewer(tstring &strText);

#include "Presets.h"
#include "ViewFind\ViewFind.h"
#include "EditFind\EditFind.h"
#include "FileFind\FileFind.h"
#include "FileTools\FileTools.h"

EXTERN CBatchType	*g_pEditorBatchType;
EXTERN CBatchType	*g_pPanelBatchType;

EXTERN CBatchActionCollection	*g_pEditorBatches;
EXTERN CBatchActionCollection	*g_pPanelBatches;

EXTERN const unsigned char *OEMCharTables VALUE(NULL);
EXTERN const unsigned char *ANSICharTables VALUE(NULL);

class hack_string : public string {
public:
	hack_string(const char *szData, size_t nSize);
	~hack_string();
};

class hack_wstring : public wstring {
public:
	hack_wstring(const wchar_t *szData, size_t nSize);
	~hack_wstring();
};

#ifdef UNICODE
typedef hack_wstring hack_tstring;
#else
typedef hack_string hack_tstring;
#endif

#endif
