#include <PersistVariables.h>

PERSIST_bool_VARIABLE(g_bFRStripCommon, true)
PERSIST_STRING_VARIABLE(g_strStrip, _T("^\\d+\\s*([-.]\\s*)?"))
PERSIST_STRING_VARIABLE(g_strPrefix, _T(""))
PERSIST_STRING_VARIABLE(g_strPostfix, _T(" - "))
PERSIST_TYPED_VARIABLE(int, g_nStartWith, 1, 0, 65535)
PERSIST_TYPED_VARIABLE(int, g_nWidth, 0, 0, 256)

#include <PersistVariablesUndef.h>
