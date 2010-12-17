#include "StdAfx.h"
#include "..\RESearch.h"

CParameterSet g_VSParamSet(ViewSearchExecutor, 2, 3,
	"Text", &SearchText, "@Text", &EText,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine
);

void VReadRegistry(HKEY Key) {
	VSPresets = new CVSPresetCollection(g_VSParamSet);
}

void VWriteRegistry(HKEY Key) {
}
