/* Copyright (C) 2000,Kostromitinov Pavel */
#ifndef __FILETOOLS_H
#define __FILETOOLS_H
#include "..\RESearch.h"
#include "..\Presets.h"
#include "..\FileFind\FileFind.h"

class CRPresetCollection:public CPresetCollection {
public:
	CRPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "FileRename";}
};

class CQRPresetCollection:public CPresetCollection {
public:
	CQRPresetCollection() {Load();}
	virtual BOOL EditPreset(CPreset *pPreset);
	virtual const char *GetName() {return "FileQuickRename";}
};

extern CParameterBatch g_RBatch;
extern CParameterBatch g_QRBatch;
EXTERN CRPresetCollection *RPresets;
EXTERN CQRPresetCollection *QRPresets;
EXTERN CPresetBatchCollection *RBatch;
EXTERN CPresetBatchCollection *QRBatch;

EXTERN string g_strStrip;
EXTERN string g_strPrefix;
EXTERN int    g_nWidth;
EXTERN int    g_nStartWith;
EXTERN string g_strPostfix;

void FTReadRegistry(HKEY Key);
void FTWriteRegistry(HKEY Key);
void FTCleanup(BOOL PatternOnly);

void ChangeSelection(int How);
OperationResult RenameFiles(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog);
OperationResult RenameSelectedFiles(PluginPanelItem **PanelItems,int *ItemsNumber,BOOL ShowDialog);
OperationResult RenumberFiles();

#endif __FILETOOLS_H
