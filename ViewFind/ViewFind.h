#ifndef __VIEWFIND_H
#define __VIEWFIND_H

#include "..\RESearch.h"
#include "..\Presets.h"

class CVSPresetCollection:public CPresetCollection {
public:
	CVSPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "ViewFind";}
};

extern CParameterBatch g_VSBatch;
EXTERN CVSPresetCollection *VSPresets;

void VReadRegistry(HKEY Key);
void VWriteRegistry(HKEY Key);
BOOL ViewerSearch();
BOOL ViewerSearchAgain();

#endif
