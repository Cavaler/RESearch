#ifndef __RESEARCH_H
#define __RESEARCH_H

#undef _FAR_NO_NAMELESS_UNIONS
#include <FAR.h>
#include <FarDlg.h>

#ifdef UNICODE
#define DIF_VAREDIT 0
#endif

#define arrsizeof(arr) (sizeof(arr)/sizeof(arr[0]))

enum OperationResult {OR_CANCEL,OR_FAILED,OR_OK,OR_PANEL};

#include "Presets.h"

#ifdef DEFINE_VARS
#define EXTERN
#define VALUE(n) = n
#define CONSTRUCT(n) n
#else
#define EXTERN extern
#define VALUE(n)
#define CONSTRUCT(n)
#endif

enum   ShowPosition {SP_TOP,SP_CENTER,SP_BOTTOM};
enum   FindTextAtCursor {FT_NONE,FT_WORD,FT_ANY};

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

EXTERN bool g_bFromCmdLine;

struct sActiveScript {
	tstring m_strName;
	CLSID  m_clsid;
};
EXTERN vector<sActiveScript> m_arrEngines;
EXTERN CFarListData m_lstEngines CONSTRUCT((NULL, 0));

BOOL SystemToLocalTime(FILETIME &ft);
BOOL LocalToSystemTime(FILETIME &ft);

enum eStringTable {

	MOk,
	MCancel,
	MBtnPresets,
	MBtnAdvanced,
	MBtnBatch,
	MError,
	MWarning,

	MSearchResults,

	MF7,

	MTempUpdate,
	MTempSendFiles,

	MMenuHeader,
	MMenuSearch,
	MMenuReplace,
	MMenuGrep,
	MMenuRename,
	MMenuRenameSelected,
	MMenuRenumber,
	MMenuSelect,
	MMenuUnselect,
	MMenuFlipSelection,
	MMenuSearchAgain,
	MMenuSearchAgainRev,
	MMenuSearchReplaceAgain,
	MMenuSearchReplaceAgainRev,
	MMenuFilterText,
	MMenuTransliterate,
	MMenuUTF8Converter,
	MMenuShowLastResults,
	MMenuBatches,
	MMenuPreset,
	MMenuBatch,

	MRESearch,
	MREReplace,
	MMask,
	MAsRegExp,
	MText,
	MCaseSensitive,
	MPlainText,
	MRegExp,
	MSeveralLineRegExp,
	MMultiLineRegExp,
	MMultiPlainText,
	MMultiRegExp,
	MSearchIn,
	MAllDrives,
	MAllLocalDrives,
	MFromRoot,
	MFromCurrent,
	MCurrentOnly,
	MSelected,
	MInverseSearch,
	MAllCharTables,
	MFilesScanned,
	MFilesFound,
	MNoFilesFound,
	MConsoleTitle,

	MREGrep,
	MGrepNames,
	MGrepNamesCount,
	MGrepLines,
	MGrepNamesLines,
	MGrepAddLineNumbers,
	MGrepAdd,
	MGrepContext,
	MGrepOutput,
	MGrepEditor,

	MAdvancedOptions,
	MFullFileNameMatch,
	MDirectoryMatch,
	MInverse,
	MRecursionLevel,
	MDateBefore,
	MDateAfter,
	MCurrent,
	MCreationDate,
	MModificationDate,
	MSizeLess,
	MSizeGreater,
	MSearchHead,
	MAttributes,
	MDirectory,
	MReadOnly,
	MArchive,
	MHidden,
	MSystem,
	MCompressed,
	MEncrypted,

	MViewModified,
	MConfirmFile,
	MConfirmLine,
	MSaveOriginal,
	MOverwriteBackup,
	MConfirmRequest,
	MInFile,
	MTheFile,
	MModifyReadonlyRequest,

	MFilterLines,
	MLeaveMatching,
	MRemoveMatching,

	MListAllLines,

	MRenumber,

	MUTF8Converter,
	MConverterANSI,
	MConverterUTF8,
	MConverterHex,
	MConvert,
	MUp,
	MDown,

	MFileOpenError,
	MFileCreateError,
	MFileBackupError,
	MFileReadError,
	MFileWriteError,
	MWarnMacrosInPlainText,
	MWarnRNInSeveralLine,
	MWarnContinue,

	MCommonSettings,
	MSeveralLinesIs,
	MLinesOr,
	MDotMatchesNewline,
	MUseSeparateThread,
	MMaxInThreadLength,
	MThreadStackMB,
	MShowUsageWarnings,

	MFileSearchSettings,
	MReadAtOnceLimit,
	MKB,
	MMaskDelimiter,
	MMaskNegation,
	MAddAsterisk,
	MDefaultMaskCase,
	MMaskSensitive,
	MMaskInsensitive,
	MMaskVolumeDependent,
	MReplaceReadonly,
	MNever,
	MAsk,
	MAlways,
	MRenumberOptions,
	MStripFromBeginning,
	MPrefix,
	MPostfix,
	MStartFrom,
	MWidth,
	MSkipSystemFolders,

	MEditorSearchSettings,
	MShowPositionOffset,
	Mfrom,
	MTop,
	MCenter,
	MBottom,
	MKeepLineIfVisible,
	MRightSideOffset,
	MFindTextAtCursor,
	MNone,
	MWordOnly,
	MAnyText,
	MFindSelection,
	MAutoFindInSelection,

	MInvalidNumber,

	MRepeating,

	MSearchFor,
	MReplaceWith,
	MReverseSearch,
	MSeveralLine,
	MUTF8,
	MInSelection,
	MRemoveEmpty,
	MRemoveNoMatch,
	MEvaluateAsScript,
	MRunEditor,
	MCannotFind,
	MErrorCreatingEngine,
	MErrorParsingText,
	MErrorLoadingTypeLib,
	MErrorExecutingScript,
	MListAllFromPreset,

	MTransliterate,
	MTransSource,
	MTransTarget,

	MAskReplace,
	MAskWith,
	MSearch,
	MReplace,
	MAll,
	MAllFiles,
	MSkip,
	MShowAll,

	MAskRename,
	MAskTo,
	MRenameError,
	MFile,
	MAskOverwrite,
	MAskCreatePath,

	MInvalidCmdLine,

	MESPreset,
	MERPreset,
	MEFPreset,
	METPreset,
	MFSPreset,
	MFRPreset,
	MFGPreset,
	MFAPreset,
	MRnPreset,
	MQRPreset,
	MVSPreset,
	MPresetName,
	MAddToMenu,
	MDeletePresetQuery,

	MPanelBatches,
	MEditorBatches,
	MBatch,
	MBatchName,
	MBtnCommands,
	MBatchCommands,

	MDeleteBatchQuery,
	MExecuteBatchQuery,

	MRegExpError,
};

HKEY OpenRegistry(const TCHAR *szSubKey=NULL, bool bCreate=true);
void ReadRegistry();
void WriteRegistry();
void ReadActiveScripts();
int  ConfigureSeveralLines();

bool CheckUsage(const tstring &strText, bool bRegExp, bool bSeveralLine);
void PrepareBMHSearch(const TCHAR *String,int StringLength,size_t nPattern = 0);
BOOL PreparePattern(pcre **Pattern,pcre_extra **PatternExtra,const tstring &Text,int CaseSensitive,BOOL bUTF8=FALSE,const unsigned char *pTables=NULL);
BOOL PreparePattern(CRegExp &reObject, const string &Text, int CaseSensitive, BOOL bUTF8=FALSE, const unsigned char *pTables=NULL);
tstring CreateReplaceString(const TCHAR *Matched,int *Match,int Count,const TCHAR *Replace,const TCHAR *EOL,int *Numbers,int Engine);
tstring EvaluateReplaceString(const TCHAR *Matched,int *Match,int Count,const TCHAR *Replace,const TCHAR *EOL,int *Numbers,int Engine);

BOOL LoadPresets(char *Which,char **StringNames,int StringCount,char **IntNames,int IntCount,void **PresetData,int *PresetCount);
BOOL SavePresets(char *Which,char **StringNames,int StringCount,char **IntNames,int IntCount,void *PresetData,int PresetCount);

void ShowErrorMsg(const TCHAR *sz1, const TCHAR *sz2 = NULL, const TCHAR *szHelp = NULL);
void ShowHResultError(int nError, HRESULT hResult, const TCHAR *szHelp = NULL);

#ifdef UNICODE
EXTERN TCHAR UpCaseTable[65536];
#else
EXTERN char UpCaseTable[256];
EXTERN CharTableSet *m_pReplaceTable;
#endif

void PrepareLocaleStuff();
tstring UpCaseString(const tstring &strText);
void UTF8Converter(tstring strInit = _T(""));

int BMHSearch(const TCHAR *Buffer,int BufferLength,const TCHAR *String,int StringLength,TCHAR *XLatTable,int nPattern = 0);
int ReverseBMHSearch(const TCHAR *Buffer,int BufferLength,const TCHAR *String,int StringLength,TCHAR *XLatTable,int nPattern = 0);

void QuoteRegExpString(tstring &strText);
void QuoteReplaceString(tstring &strText);

void StartREThread();
void StopREThread();
int do_pcre_exec(const pcre *external_re, const pcre_extra *extra_data,
	const TCHAR *subject, int length, int start_offset, int options, int *offsets,
	int offsetcount);
#ifdef UNICODE
int do_pcre_execA(const pcre *external_re, const pcre_extra *extra_data,
	const char *subject, int length, int start_offset, int options, int *offsets,
	int offsetcount);
#endif

void RunExternalEditor(tstring &strText);

inline bool Interrupted256(int nValue) {
	return (((nValue & 0xFF) == 0) && Interrupted());
}

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

#endif
