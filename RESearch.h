#ifndef __RESEARCH_H
#define __RESEARCH_H

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
	string m_strName;
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
	MRightSideOffset,
	MFindTextAtCursor,
	MNone,
	MWordOnly,
	MAnyText,
	MFindSelection,

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
	MFAPreset,
	MVSPreset,
	MPresetName,
	MAddToMenu,
	MDeletePresetQuery,

	MBatch,
	MBatchName,
	MBtnCommands,
	MBatchCommands,

	MDeleteBatchQuery,
	MExecuteBatchQuery,
};

HKEY OpenRegistry(const char *szSubKey=NULL, bool bCreate=true);
void ReadRegistry();
void WriteRegistry();
void ReadActiveScripts();
int  ConfigureSeveralLines();

bool CheckUsage(const string &strText, bool bRegExp, bool bSeveralLine);
void PrepareBMHSearch(const char *String,int StringLength,size_t nPattern = 0);
BOOL PreparePattern(pcre **Pattern,pcre_extra **PatternExtra,const string &Text,int CaseSensitive,BOOL bUTF8=FALSE,const unsigned char *pTables=NULL);
BOOL PreparePattern(CRegExp &reObject, const string &Text, int CaseSensitive, BOOL bUTF8=FALSE, const unsigned char *pTables=NULL);
string CreateReplaceString(const char *Matched,int *Match,int Count,const char *Replace,const char *EOL,int *Numbers,int Engine);
string EvaluateReplaceString(const char *Matched,int *Match,int Count,const char *Replace,const char *EOL,int *Numbers,int Engine);

BOOL LoadPresets(char *Which,char **StringNames,int StringCount,char **IntNames,int IntCount,void **PresetData,int *PresetCount);
BOOL SavePresets(char *Which,char **StringNames,int StringCount,char **IntNames,int IntCount,void *PresetData,int PresetCount);

void ShowErrorMsg(const char *sz1, const char *sz2 = NULL, const char *szHelp = NULL);
void ShowHResultError(int nError, HRESULT hResult, const char *szHelp = NULL);

EXTERN char UpCaseTable[256];
EXTERN CharTableSet *m_pReplaceTable;
void SetANSILocale();

int BMHSearch(const char *Buffer,int BufferLength,const char *String,int StringLength,char *XLatTable,int nPattern = 0);
int ReverseBMHSearch(const char *Buffer,int BufferLength,const char *String,int StringLength,char *XLatTable,int nPattern = 0);
void UTF8Converter(string strInit = "");
void QuoteRegExpString(string &strText);
void QuoteReplaceString(string &strText);

void StartREThread();
void StopREThread();
int do_pcre_exec(const pcre *external_re, const pcre_extra *extra_data,
	const char *subject, int length, int start_offset, int options, int *offsets,
	int offsetcount);

bool do_pcre_exec(CRegExp &reObject, const char *subject, int length, int start_offset, int options,
				  int *offsets, int offsetcount);

inline bool Interrupted256(int nValue) {
	return (((nValue & 0xFF) == 0) && Interrupted());
}

#include "Presets.h"
#include "ViewFind\ViewFind.h"
#include "EditFind\EditFind.h"
#include "FileFind\FileFind.h"
#include "FileTools\FileTools.h"

#endif
