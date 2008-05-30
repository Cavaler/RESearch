/* Copyright (C) 2000,Kostromitinov Pavel */
#ifndef __FILETOOLS_H
#define __FILETOOLS_H
#include "..\RESearch.h"
#include "..\Presets.h"
#include "..\FileFind\FileFind.h"

class CRnPresetCollection:public CPresetCollection {
public:
	CRnPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "FileRename";}
};

class CQRPresetCollection:public CPresetCollection {
public:
	CQRPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "FileQuickRename";}
};

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

extern CParameterSet g_RnParamSet;
extern CParameterSet g_QRParamSet;
EXTERN CRnPresetCollection *RnPresets;
EXTERN CQRPresetCollection *QRPresets;
EXTERN CPresetBatchCollection *RnBatch;
EXTERN CPresetBatchCollection *QRBatch;

void FTReadRegistry(HKEY Key);
void FTWriteRegistry(HKEY Key);
void FTCleanup(BOOL PatternOnly);

void ChangeSelection(int How);
OperationResult RenameFiles(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog);
OperationResult RenameSelectedFiles(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog);
OperationResult RenumberFiles();

OperationResult RenameFilesExecutor();
OperationResult QuickRenameFilesExecutor();

#endif __FILETOOLS_H
