#ifndef __RESEARCH_H
#define __RESEARCH_H

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma warning(disable:4786)
#include <vector>
#include <string>
#include <map>
using namespace std;

#include <EasyReg.h>
#include <StringEx.h>

#undef _FAR_NO_NAMELESS_UNIONS
#include <FAR.h>
#include <FarDlg.h>
#include <FarKeys.hpp>
#include <pcre\pcre.h>
#include <CRegExp.h>
#include <CMapping.h>

#include "Presets.h"

#ifdef DEFINE_VARS
#define EXTERN
#define VALUE(n) = n
#else
#define EXTERN extern
#define VALUE(n)
#endif

// Common
EXTERN int  SeveralLines;
EXTERN BOOL AllowEmptyMatch;
EXTERN BOOL DotMatchesNewline;
EXTERN bool g_bUseSeparateThread;
EXTERN int  g_nMaxInThreadLength;
EXTERN int  g_nThreadStackMB;

EXTERN int  ReadAtOnceLimit;
EXTERN char MaskDelimiter;
EXTERN char MaskNegation;
EXTERN BOOL AutoappendAsterisk;
enum   ShowPosition {SP_TOP,SP_CENTER,SP_BOTTOM};
EXTERN ShowPosition EShowPosition;
EXTERN int  EShowPositionOffset;
EXTERN int ERightSideOffset;
enum   FindTextAtCursor {FT_NONE,FT_WORD,FT_ANY};
EXTERN FindTextAtCursor EFindTextAtCursor;
EXTERN BOOL EFindSelection;

EXTERN BOOL Interrupt;

enum OperationResult {OR_CANCEL,OR_FAILED,OR_OK,OR_PANEL};

enum {
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
	MMenuRename,
	MMenuRenameSelected,
	MMenuRenumber,
	MMenuSelect,
	MMenuUnselect,
	MMenuFlipSelection,
	MMenuSearchAgain,
	MMenuSearchReplaceAgain,
	MMenuFilterText,
	MMenuUTF8Converter,

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
	MConfirmRequest,
	MInFile,
	MTheFile,
	MModifyReadonlyRequest,

	MFilterLines,
	MLeaveMatching,
	MRemoveMatching,
	
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
	MCannotFind,

	MAskReplace,
	MAskWith,
	MSearch,
	MReplace,
	MAll,
	MAllFiles,
	MSkip,

	MAskRename,
	MAskTo,
	MRenameError,
	MFile,
	MAskOverwrite,

	MInvalidCmdLine,

	MESPreset,
	MERPreset,
	MEFPreset,
	MFSPreset,
	MFRPreset,
	MVSPreset,
	MPresetName,
	MDeletePresetQuery,
	MBatchName,
	MDeleteBatchQuery,
	MExecuteBatchQuery
};

void ReadRegistry();
void WriteRegistry();

void PrepareBMHSearch(const char *String,int StringLength,int nPattern = 0);
BOOL PreparePattern(pcre **Pattern,pcre_extra **PatternExtra,string &Text,int CaseSensitive,BOOL bUTF8=FALSE);
char *CreateReplaceString(const char *Matched,int *Match,int Count,const char *Replace,const char *EOL,int *Numbers,int &ResultLength);

BOOL LoadPresets(char *Which,char **StringNames,int StringCount,char **IntNames,int IntCount,void **PresetData,int *PresetCount);
BOOL SavePresets(char *Which,char **StringNames,int StringCount,char **IntNames,int IntCount,void *PresetData,int PresetCount);
BOOL Interrupted();

EXTERN char UpCaseTable[256];
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
#endif
