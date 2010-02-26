/* Copyright (C) 2000,Kostromitinov Pavel */
#ifndef __FILETOOLS_H
#define __FILETOOLS_H
#include "..\RESearch.h"
#include "..\Presets.h"
#include "..\FileFind\FileFind.h"

class CRnPresetCollection:public CStdPresetCollection {
public:
	CRnPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("FileRename"), MRnPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 1; }
};

class CQRPresetCollection:public CStdPresetCollection {
public:
	CQRPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("FileQuickRename"), MQRPreset) {}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual int  ID() { return 2; }
};

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

extern CParameterSet g_RnParamSet;
extern CParameterSet g_QRParamSet;
EXTERN CRnPresetCollection *RnPresets;
EXTERN CQRPresetCollection *QRPresets;

void FTReadRegistry(HKEY Key);
void FTWriteRegistry(HKEY Key);
void FTCleanup(BOOL PatternOnly);

void ChangeSelection(int How);
OperationResult RenameFiles(panelitem_vector &PanelItems,BOOL ShowDialog);
OperationResult RenameSelectedFiles(panelitem_vector &PanelItems,BOOL ShowDialog);
OperationResult RenumberFiles();
OperationResult UndoRenameFiles();

OperationResult RenameFilesExecutor();
OperationResult QuickRenameFilesExecutor();

#endif __FILETOOLS_H
