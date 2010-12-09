#include <PersistVariables.h>

PERSIST_STRING_VARIABLE(EText, _T(""))
PERSIST_STRING_VARIABLE(ERReplace, _T(""))
PERSIST_BOOL_VARIABLE(ERegExp, FALSE)
PERSIST_BOOL_VARIABLE(ESeveralLine, FALSE)
PERSIST_BOOL_VARIABLE(ECaseSensitive, FALSE)

PERSIST_TYPED_VARIABLE(EPositioning, EPositionAt, EP_DIR, EP_BEGIN, EP_END)
PERSIST_bool_VARIABLE(EPositionAtSub, true)
PERSIST_STRING_VARIABLE(EPositionSubName, _T("pos"))

PERSIST_TYPED_VARIABLE(int, EREvaluateScript, 0, 0, 255)

PERSIST_BOOL_VARIABLE(EFLeaveFilter, TRUE)

PERSIST_STRING_VARIABLE(ETSource, _T(""))
PERSIST_STRING_VARIABLE(ETTarget, _T(""))

#include <PersistVariablesUndef.h>
