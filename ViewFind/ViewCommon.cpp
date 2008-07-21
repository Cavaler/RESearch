#include "StdAfx.h"
#include "..\RESearch.h"

CParameterSet g_VSParamSet(ViewSearchExecutor, 2, 4,
	"Text", &SearchText, "@Text", &EText,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine, "UTF8", &EUTF8
					 );

void VReadRegistry(HKEY Key) {
	VSPresets = new CVSPresetCollection(g_VSParamSet);
}

void VWriteRegistry(HKEY Key) {
}
