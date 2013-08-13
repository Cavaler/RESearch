#ifdef FAR3
#include <PersistVariablesFar3.h>
#else
#include <PersistVariables.h>
#endif

PERSIST_STRING_VARIABLE(EText, _T(""))
PERSIST_STRING_VARIABLE(ERReplace, _T(""))
PERSIST_BOOL_VARIABLE(ERegExp, FALSE)
PERSIST_BOOL_VARIABLE(ESeveralLine, FALSE)
PERSIST_BOOL_VARIABLE(ECaseSensitive, FALSE)

PERSIST_TYPED_VARIABLE(EPositioning, EPositionAt, EP_DIR, EP_BEGIN, EP_END)
PERSIST_bool_VARIABLE(EPositionAtSub, true)
PERSIST_STRING_VARIABLE(EPositionSubName, _T("pos"))

PERSIST_STRING_VARIABLE(EREvaluateScript, _T(""))

PERSIST_BOOL_VARIABLE(EFLeaveFilter, TRUE)

PERSIST_STRING_VARIABLE(ETSource, _T(""))
PERSIST_STRING_VARIABLE(ETTarget, _T(""))

#include <PersistVariablesUndef.h>
