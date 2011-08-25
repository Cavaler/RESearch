#include <PersistVariables.h>

PERSIST_BOOL_VARIABLE(FCaseSensitive, FALSE)
PERSIST_BOOL_VARIABLE(FMaskAsRegExp, FALSE)
PERSIST_STRING_VARIABLE(FMask, _T("*.*"))
PERSIST_STRING_VARIABLE(FText, _T(""))
PERSIST_TYPED_VARIABLE(SearchAs, FSearchAs, SA_PLAINTEXT, SA_PLAINTEXT, SA_MULTIREGEXP)
PERSIST_TYPED_VARIABLE(SearchIn, FSearchIn, SI_FROMCURRENT, SI_ALLDRIVES, SI_SELECTED)
PERSIST_TYPED_VARIABLE(MaskCase, FMaskCase, MC_VOLUME, MC_SENSITIVE, MC_VOLUME)
PERSIST_TYPED_VARIABLE(eReplaceReadonly, FRReplaceReadonly, RR_ASK, RR_NEVER, RR_ALWAYS)

PERSIST_TYPED_VARIABLE(int, FBufferSize, 16, 1, 1024)

PERSIST_BOOL_VARIABLE(FSInverse, FALSE)
PERSIST_BOOL_VARIABLE(FAllCharTables, FALSE)
PERSIST_STRING_VARIABLE(FRReplace, _T(""))
PERSIST_BOOL_VARIABLE(FROpenModified, FALSE)
PERSIST_BOOL_VARIABLE(FRConfirmFile, TRUE)
PERSIST_BOOL_VARIABLE(FRConfirmLine, TRUE)
PERSIST_BOOL_VARIABLE(FRSaveOriginal, FALSE)
PERSIST_BOOL_VARIABLE(FROverwriteBackup, TRUE)
PERSIST_BOOL_VARIABLE(FRepeating, FALSE)
PERSIST_BOOL_VARIABLE(FRLeaveSelection, FALSE)
PERSIST_BOOL_VARIABLE(FRPreviewRename, FALSE)
PERSIST_BOOL_VARIABLE(FSEditSrchAfterFile, FALSE)
PERSIST_BOOL_VARIABLE(FSUseSingleCR, FALSE)

PERSIST_TYPED_VARIABLE(GrepWhat, FGrepWhat, GREP_NAMES_LINES, GREP_NAMES, GREP_NAMES_LINES)
PERSIST_BOOL_VARIABLE(FGAddLineNumbers, FALSE)
PERSIST_BOOL_VARIABLE(FGAddContext, FALSE)
PERSIST_TYPED_VARIABLE(DWORD, FGContextLines, 1, 0, 1024)
PERSIST_BOOL_VARIABLE(FGOutputToFile, FALSE)
PERSIST_STRING_VARIABLE(FGOutputFile, _T(""))
PERSIST_BOOL_VARIABLE(FGOpenInEditor, TRUE)

PERSIST_BOOL_VARIABLE(FACaseSensitive, FALSE)
PERSIST_BOOL_VARIABLE(FADirectoryCaseSensitive, FALSE)
PERSIST_BOOL_VARIABLE(FAFullFileNameMatch, FALSE)
PERSIST_BOOL_VARIABLE(FAFullFileNameInverse, FALSE)
PERSIST_STRING_VARIABLE(FAFullFileName, _T(""))

PERSIST_BOOL_VARIABLE(FADirectoryMatch, FALSE)
PERSIST_BOOL_VARIABLE(FADirectoryInverse, FALSE)
PERSIST_STRING_VARIABLE(FADirectoryName, _T(""))
PERSIST_TYPED_VARIABLE(DWORD, FARecursionLevel, 0, 0, 256)
PERSIST_BOOL_VARIABLE(FASkipSystemFolders, FALSE)
PERSIST_STRING_VARIABLE(FASystemFolders, _T("CVS.;.svn;_svn."))

PERSIST_BOOL_VARIABLE(FADateBefore, FALSE)
PERSIST_FILETIME_VARIABLE(FADateBeforeThis)
PERSIST_BOOL_VARIABLE(FADateAfter, FALSE)
PERSIST_FILETIME_VARIABLE(FADateAfterThis)
PERSIST_BOOL_VARIABLE(FAModificationDate, FALSE)

PERSIST_BOOL_VARIABLE(FASizeLess, FALSE)
PERSIST_TYPED_VARIABLE(DWORD, FASizeLessLimit, 0, 0, 0x7FFFFFFF)
PERSIST_BOOL_VARIABLE(FASizeGreater, FALSE)
PERSIST_TYPED_VARIABLE(DWORD, FASizeGreaterLimit, 0, 0, 0x7FFFFFFF)

PERSIST_TYPED_VARIABLE(DWORD, FAAttributesSet, 0, 0, 0x7FFFFFFF)
PERSIST_TYPED_VARIABLE(DWORD, FAAttributesCleared, 0, 0, 0x7FFFFFFF)

PERSIST_BOOL_VARIABLE(FASearchHead, FALSE)
PERSIST_TYPED_VARIABLE(DWORD, FASearchHeadLimit, 0, 0, 0x7FFFFFFF)

PERSIST_TYPED_VARIABLE(int, TPPanelMode, 4, 0, 9)
PERSIST_TYPED_VARIABLE(int, TPSortMode, SM_DEFAULT, 0, 20)
PERSIST_TYPED_VARIABLE(int, TPSortOrder, 0, 0, 1)

#include <PersistVariablesUndef.h>
