#ifndef __VIEWFIND_H
#define __VIEWFIND_H

#include "..\RESearch.h"
#include "..\Presets.h"

class CVSPresetCollection : public CPresetCollection {
public:
	CVSPresetCollection(CParameterSet &ParamSet) : CPresetCollection(ParamSet, "ViewFind", MVSPreset) {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
};

extern CParameterSet g_VSParamSet;
EXTERN CVSPresetCollection *VSPresets;

void VReadRegistry(HKEY Key);
void VWriteRegistry(HKEY Key);
BOOL ViewerSearch();
BOOL ViewerSearchAgain();

OperationResult ViewSearchExecutor();

#endif
