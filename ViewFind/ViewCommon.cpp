#include "StdAfx.h"
#include "..\RESearch.h"

CParameterSet g_VSBatch(ViewSearchExecutor, 1, 4,
	"Text", &SearchText,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine, "UTF8", &EUTF8
					 );

void VReadRegistry(HKEY Key) {
	VSPresets=new CVSPresetCollection();
}

void VWriteRegistry(HKEY Key) {
}
