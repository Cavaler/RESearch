#include "StdAfx.h"
#include "..\RESearch.h"

CParameterSet g_VSParamSet(ViewSearchExecutor,
	"Text", &SearchText, "@Text", &EText, NULL, NULL,
	"IsRegExp", &ERegExp, "CaseSensitive", &ECaseSensitive, "SeveralLine", &ESeveralLine, NULL
);

void VReadRegistry(CFarSettingsKey Key)
{
	VSPresets = new CVSPresetCollection(g_VSParamSet);
}

void VWriteRegistry(CFarSettingsKey Key)
{
}
