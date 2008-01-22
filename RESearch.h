#ifndef __RESEARCH_H
#define __RESEARCH_H

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

// Common
EXTERN int  SeveralLines;
EXTERN int  SeveralLinesKB;
EXTERN BOOL AllowEmptyMatch;
EXTERN BOOL DotMatchesNewline;
EXTERN bool g_bUseSeparateThread;
EXTERN int  g_nMaxInThreadLength;
EXTERN int  g_nThreadStackMB;

enum   ShowPosition {SP_TOP,SP_CENTER,SP_BOTTOM};
EXTERN ShowPosition EShowPosition;
EXTERN int  EShowPositionOffset;
EXTERN int ERightSideOffset;
enum   FindTextAtCursor {FT_NONE,FT_WORD,FT_ANY};
EXTERN FindTextAtCursor EFindTextAtCursor;
EXTERN BOOL EFindSelection;

EXTERN bool g_bFromCmdLine;

struct sActiveScript {
	string m_strName;
	CLSID  m_clsid;
};
EXTERN vector<sActiveScript> m_arrEngines;
EXTERN CFarListData m_lstEngines CONSTRUCT((NULL, 0));

enum OperationResult {OR_CANCEL,OR_FAILED,OR_OK,OR_PANEL};

enum eStringTable {

	MOk,
	MCancel,
	MPresets,
	MAdvanced,
	MError,
	MBatch,

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

	MCommonSettings,
	MSeveralLinesIs,
	MLinesOr,
	MAllowEmptyMatch,
	MDotMatchesNewline,
	MUseSeparateThread,
	MMaxInThreadLength,
	MThreadStackMB,

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
	MFSPreset,
	MFRPreset,
	MFAPreset,
	MVSPreset,
	MPresetName,
	MDeletePresetQuery,
	MBatchName,
	MDeleteBatchQuery,
	MExecuteBatchQuery,
};

void ReadRegistry();
void WriteRegistry();
void EnumActiveScripts();
int  ConfigureSeveralLines();

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

#include "Presets.h"
#include "ViewFind\ViewFind.h"
#include "EditFind\EditFind.h"
#include "FileFind\FileFind.h"
#include "FileTools\FileTools.h"

#endif
