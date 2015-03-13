#ifdef FAR3
#include <PersistVariablesFar3.h>
#else
#include <PersistVariables.h>
#endif

PERSIST_STRING_VARIABLE(EText, _T(""))
PERSIST_STRING_VARIABLE(ERReplace, _T(""))
PERSIST_bool_VARIABLE(ERegExp, false)
PERSIST_bool_VARIABLE(ESeveralLine, false)
PERSIST_bool_VARIABLE(ECaseSensitive, false)

PERSIST_TYPED_VARIABLE(EPositioning, EPositionAt, EP_DIR, EP_BEGIN, EP_END)
PERSIST_bool_VARIABLE(EPositionAtSub, true)
PERSIST_STRING_VARIABLE(EPositionSubName, _T("pos"))

PERSIST_STRING_VARIABLE(EREvaluateScript, _T(""))

PERSIST_bool_VARIABLE(EFLeaveFilter, true)
PERSIST_TYPED_VARIABLE(size_t, ERRepeatCount, 2, 1, 16384)

PERSIST_STRING_VARIABLE(ETSource, _T(""))
PERSIST_STRING_VARIABLE(ETTarget, _T(""))

#include <PersistVariablesUndef.h>
