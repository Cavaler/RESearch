#include <PersistVariables.h>

PERSIST_STRING_VARIABLE(g_strStrip, "^\\d+\\s*([-.]\\s*)?")
PERSIST_STRING_VARIABLE(g_strPrefix, "")
PERSIST_STRING_VARIABLE(g_strPostfix, " - ")
PERSIST_TYPED_VARIABLE(int, g_nStartWith, 1, 0, 65535)
PERSIST_TYPED_VARIABLE(int, g_nWidth, 0, 0, 256)

#include <PersistVariablesUndef.h>
