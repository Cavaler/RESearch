#include <PersistVariables.h>

PERSIST_TYPED_VARIABLE(int, SeveralLines, 32, 1, 65535)
PERSIST_TYPED_VARIABLE(int, SeveralLinesKB, 16, 1, 1024)
PERSIST_BOOL_VARIABLE(DotMatchesNewline, TRUE)
PERSIST_bool_VARIABLE_(g_bUseSeparateThread, _T("UseSeparateThread"), true)
PERSIST_TYPED_VARIABLE_(int, g_nMaxInThreadLength, _T("MaxInThreadLength"), 1024, 0, MAXINT_PTR)
PERSIST_TYPED_VARIABLE_(int, g_nThreadStackMB, _T("ThreadStackMB"), 64, 0, MAXINT_PTR)
PERSIST_bool_VARIABLE_(g_bShowUsageWarnings, _T("ShowUsageWarnings"), true)
PERSIST_bool_VARIABLE(g_bEscapesInPlainText, true)
PERSIST_BOOL_VARIABLE(g_bDefaultOEM, TRUE)

PERSIST_TYPED_VARIABLE(ShowPosition, EShowPosition, SP_CENTER, SP_TOP, SP_BOTTOM)
PERSIST_TYPED_VARIABLE(int, EShowPositionOffset, 0, -1024, 1024)
PERSIST_TYPED_VARIABLE(int, ERightSideOffset, 5, 0, 1024)
PERSIST_bool_VARIABLE(EKeepLineIfVisible, TRUE)
PERSIST_TYPED_VARIABLE(FindTextAtCursor, EFindTextAtCursor, FT_WORD, FT_NONE, FT_ANY)
PERSIST_BOOL_VARIABLE(EFindSelection, TRUE)
PERSIST_bool_VARIABLE(EAutoFindInSelection, true)
PERSIST_bool_VARIABLE(g_bUseRealEOL, false)

#include <PersistVariablesUndef.h>
