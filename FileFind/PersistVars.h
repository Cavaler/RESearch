#ifdef FAR3
#include <PersistVariablesFar3.h>
#else
#include <PersistVariables.h>
#endif

PERSIST_bool_VARIABLE(FCaseSensitive, false)
PERSIST_bool_VARIABLE(FMaskAsRegExp, false)
PERSIST_bool_VARIABLE(FUseShortFilenames, false)
PERSIST_STRING_VARIABLE(FMask, _T("*.*"))
PERSIST_STRING_VARIABLE(FText, _T(""))
PERSIST_TYPED_VARIABLE(SearchAs, FSearchAs, SA_PLAINTEXT, SA_PLAINTEXT, SA_MULTIREGEXP)
PERSIST_TYPED_VARIABLE(SearchIn, FSearchIn, SI_FROMCURRENT, SI_ALLDRIVES, SI_SELECTED)
PERSIST_TYPED_VARIABLE(MaskCase, FMaskCase, MC_VOLUME, MC_SENSITIVE, MC_VOLUME)
PERSIST_TYPED_VARIABLE(eReplaceReadonly, FRReplaceReadonly, RR_ASK, RR_NEVER, RR_ALWAYS)

PERSIST_TYPED_VARIABLE(int, FBufferSize, 16, 1, 1024)

PERSIST_bool_VARIABLE(FSInverse, false)
PERSIST_bool_VARIABLE(FAllCharTables, false)
PERSIST_STRING_VARIABLE(FRReplace, _T(""))
PERSIST_bool_VARIABLE(FROpenModified, false)
PERSIST_bool_VARIABLE(FRShowStatistics, false)
PERSIST_bool_VARIABLE(FRConfirmFile, true)
PERSIST_bool_VARIABLE(FRConfirmLine, true)
PERSIST_bool_VARIABLE(FRSaveOriginal, false)
PERSIST_bool_VARIABLE(FROverwriteBackup, true)
PERSIST_bool_VARIABLE(FRReplaceToNew, false)
PERSIST_bool_VARIABLE(FRepeating, false)
PERSIST_bool_VARIABLE(FRLeaveSelection, false)
PERSIST_bool_VARIABLE(FRPreviewRename, false)
PERSIST_bool_VARIABLE(FSEditSrchAfterFile, false)
PERSIST_bool_VARIABLE(FSUseSingleCR, false)

PERSIST_bool_VARIABLE(FGOutputNames, true)
PERSIST_bool_VARIABLE(FGAddLineCount, false)
PERSIST_bool_VARIABLE(FGAddMatchCount, false)
PERSIST_bool_VARIABLE(FGOutputLines, true)
PERSIST_bool_VARIABLE(FGAddLineNumbers, false)
PERSIST_bool_VARIABLE(FGAddContext, false)
PERSIST_TYPED_VARIABLE(DWORD, FGContextLines, 1, 0, 1024)
PERSIST_bool_VARIABLE(FGMatchingLinePart, false)
PERSIST_STRING_VARIABLE(FGFileNamePrepend, _T(""))
PERSIST_STRING_VARIABLE(FGFileNameAppend, _T(""))
PERSIST_bool_VARIABLE(FGOutputToFile, false)
PERSIST_STRING_VARIABLE(FGOutputFile, _T(""))
PERSIST_bool_VARIABLE(FGOpenInEditor, true)
PERSIST_bool_VARIABLE(FGOpenPanel, false)

PERSIST_bool_VARIABLE(FACaseSensitive, false)
PERSIST_bool_VARIABLE(FADirectoryCaseSensitive, false)
PERSIST_bool_VARIABLE(FAFullFileNameMatch, false)
PERSIST_bool_VARIABLE(FAFullFileNameInverse, false)
PERSIST_STRING_VARIABLE(FAFullFileName, _T(""))

PERSIST_bool_VARIABLE(FADirectoryMatch, false)
PERSIST_bool_VARIABLE(FADirectoryInverse, false)
PERSIST_STRING_VARIABLE(FADirectoryName, _T(""))
PERSIST_TYPED_VARIABLE(DWORD, FARecursionLevel, 0, 0, 256)
PERSIST_bool_VARIABLE(FASkipSystemFolders, false)
PERSIST_STRING_VARIABLE(FASystemFolders, _T("CVS.;.svn;_svn."))

PERSIST_bool_VARIABLE(FADateBefore, false)
PERSIST_FILETIME_VARIABLE(FADateBeforeThis)
PERSIST_bool_VARIABLE(FADateAfter, false)
PERSIST_FILETIME_VARIABLE(FADateAfterThis)
PERSIST_bool_VARIABLE(FAModificationDate, false)

PERSIST_bool_VARIABLE(FASizeLess, false)
PERSIST_TYPED_VARIABLE(DWORD, FASizeLessLimit, 0, 0, 0x7FFFFFFF)
PERSIST_bool_VARIABLE(FASizeGreater, false)
PERSIST_TYPED_VARIABLE(DWORD, FASizeGreaterLimit, 0, 0, 0x7FFFFFFF)

PERSIST_TYPED_VARIABLE(DWORD, FAAttributesSet, 0, 0, 0x7FFFFFFF)
PERSIST_TYPED_VARIABLE(DWORD, FAAttributesCleared, 0, 0, 0x7FFFFFFF)

PERSIST_bool_VARIABLE(FASearchHead, false)
PERSIST_TYPED_VARIABLE(DWORD, FASearchHeadLimit, 0, 0, 0x7FFFFFFF)

PERSIST_TYPED_VARIABLE(int, TPPanelMode, 4, 0, 9)
PERSIST_TYPED_VARIABLE(int, TPSortMode, SM_DEFAULT, 0, 20)
PERSIST_TYPED_VARIABLE(int, TPSortOrder, 0, 0, 1)

PERSIST_bool_VARIABLE(FAUseStreams, false)

#include <PersistVariablesUndef.h>
