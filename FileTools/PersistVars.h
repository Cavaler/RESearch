#ifdef FAR3
#include <PersistVariablesFar3.h>
#else
#include <PersistVariables.h>
#endif

PERSIST_bool_VARIABLE(g_bStripRegExp, true)
PERSIST_bool_VARIABLE(g_bStripCommon, true)
PERSIST_STRING_VARIABLE(g_strStrip, _T("^\\d+\\s*([-.]\\s*)?"))
PERSIST_STRING_VARIABLE(g_strPrefix, _T(""))
PERSIST_STRING_VARIABLE(g_strPostfix, _T(" - "))
PERSIST_TYPED_VARIABLE(int, g_nStartWith, 1, 0, 65535)
PERSIST_TYPED_VARIABLE(int, g_nWidth, 0, 0, 256)

#include <PersistVariablesUndef.h>
