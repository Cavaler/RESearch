#ifdef FAR3
#include <PersistVariablesFar3.h>
#else
#include <PersistVariables.h>
#endif

PERSIST_TYPED_VARIABLE(int, SeveralLines, 32, 1, 65535)
PERSIST_TYPED_VARIABLE(int, SeveralLinesKB, 16, 1, 1024)
PERSIST_bool_VARIABLE(DotMatchesNewline, true)
PERSIST_bool_VARIABLE_(g_bUseSeparateThread, _T("UseSeparateThread"), true)
PERSIST_TYPED_VARIABLE_(int, g_nMaxInThreadLength, _T("MaxInThreadLength"), 1024, 0, MAXINT_PTR)
PERSIST_TYPED_VARIABLE_(int, g_nThreadStackMB, _T("ThreadStackMB"), 64, 0, MAXINT_PTR)
PERSIST_bool_VARIABLE_(g_bShowUsageWarnings, _T("ShowUsageWarnings"), true)
PERSIST_bool_VARIABLE(g_bEscapesInPlainText, true)
PERSIST_bool_VARIABLE(g_bIgnoreIdentReplace, false)
PERSIST_bool_VARIABLE(g_bReplaceOnShiftIns, false)
PERSIST_bool_VARIABLE(g_bDefaultOEM, true)

PERSIST_TYPED_VARIABLE(ShowPosition, EShowPosition, SP_CENTER, SP_TOP, SP_BOTTOM)
PERSIST_TYPED_VARIABLE(int, EShowPositionOffset, 0, -1024, 1024)
PERSIST_TYPED_VARIABLE(int, ELRSideOffset, 5, 0, 1024)
PERSIST_TYPED_VARIABLE(int, ETDSideOffset, 2, 0, 1024)
PERSIST_bool_VARIABLE(EKeepLineIfVisible, true)
PERSIST_TYPED_VARIABLE(FindTextAtCursor, EFindTextAtCursor, FT_WORD, FT_NONE, FT_ANY)
PERSIST_bool_VARIABLE(EFindSelection, true)
PERSIST_bool_VARIABLE(EAutoFindInSelection, true)
PERSIST_bool_VARIABLE(g_bUseRealEOL, false)

#include <PersistVariablesUndef.h>
