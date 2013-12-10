#ifndef __VIEWFIND_H
#define __VIEWFIND_H

#include "..\RESearch.h"
#include "..\Presets.h"

class CVSPresetCollection : public CStdPresetCollection {
public:
	CVSPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("ViewFind"), MVSPreset) {}
	virtual bool EditPreset(CPreset *pPreset);
	virtual int  ID() { return 0; }
};

extern CParameterSet g_VSParamSet;
EXTERN CVSPresetCollection *VSPresets;

EXTERN ViewerInfo VInfo;
bool RefreshViewerInfo();

void VReadRegistry (CFarSettingsKey Key);
void VWriteRegistry(CFarSettingsKey Key);
bool ViewerSearch();
bool ViewerSearchAgain();

OperationResult ViewSearchExecutor();

#endif
