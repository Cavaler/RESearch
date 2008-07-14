#include <PersistVariables.h>

PERSIST_TYPED_VARIABLE(int, SeveralLines, 32, 1, 65535)
PERSIST_TYPED_VARIABLE(int, SeveralLinesKB, 16, 1, 1024)
PERSIST_BOOL_VARIABLE(DotMatchesNewline, TRUE)
PERSIST_bool_VARIABLE_(g_bUseSeparateThread, "UseSeparateThread", true)
PERSIST_TYPED_VARIABLE_(int, g_nMaxInThreadLength, "MaxInThreadLength", 1024, 0, MAXINT_PTR)
PERSIST_TYPED_VARIABLE_(int, g_nThreadStackMB, "ThreadStackMB", 64, 0, MAXINT_PTR)
PERSIST_bool_VARIABLE_(g_bShowUsageWarnings, "ShowUsageWarnings", true)

PERSIST_TYPED_VARIABLE(ShowPosition, EShowPosition, SP_CENTER, SP_TOP, SP_BOTTOM)
PERSIST_TYPED_VARIABLE(int, EShowPositionOffset, 0, -1024, 1024)
PERSIST_TYPED_VARIABLE(int, ERightSideOffset, 5, 0, 1024)
PERSIST_TYPED_VARIABLE(FindTextAtCursor, EFindTextAtCursor, FT_WORD, FT_NONE, FT_ANY)
PERSIST_BOOL_VARIABLE(EFindSelection, TRUE)

#include <PersistVariablesUndef.h>
