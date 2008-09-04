#ifndef __VIEWFIND_H
#define __VIEWFIND_H

#include "..\RESearch.h"
#include "..\Presets.h"

class CVSPresetCollection : public CPresetCollection {
public:
	CVSPresetCollection(CParameterSet &ParamSet) : CPresetCollection(ParamSet, "ViewFind", MVSPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 0; }
};

extern CParameterSet g_VSParamSet;
EXTERN CVSPresetCollection *VSPresets;

void VReadRegistry(HKEY Key);
void VWriteRegistry(HKEY Key);
BOOL ViewerSearch();
BOOL ViewerSearchAgain();

OperationResult ViewSearchExecutor();

#endif
