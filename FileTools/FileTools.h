/* Copyright (C) 2000,Kostromitinov Pavel */
#ifndef __FILETOOLS_H
#define __FILETOOLS_H
#include "..\RESearch.h"
#include "..\Presets.h"
#include "..\FileFind\FileFind.h"

class CRnPresetCollection:public CFPresetCollection {
public:
	CRnPresetCollection(CParameterSet &ParamSet) : CFPresetCollection(ParamSet, _T("FileRename"), MRnPreset) {}
	virtual bool EditPreset(CPreset *pPreset);
	virtual int  ID() { return 1; }
};

class CQRPresetCollection:public CStdPresetCollection {
public:
	CQRPresetCollection(CParameterSet &ParamSet) : CStdPresetCollection(ParamSet, _T("FileQuickRename"), MQRPreset) {}
	virtual bool EditPreset(CPreset *pPreset);
	virtual int  ID() { return 2; }
};

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

extern CParameterSet g_RnParamSet;
extern CParameterSet g_QRParamSet;
EXTERN CRnPresetCollection *RnPresets;
EXTERN CQRPresetCollection *QRPresets;

typedef pair<tstring, tstring> rename_pair;
typedef vector<rename_pair> rename_vector;
//	Not a map to enforce reverse rename sequence
EXTERN rename_vector m_arrLastRename;

void FTReadRegistry (CFarSettingsKey Key);
void FTWriteRegistry(CFarSettingsKey Key);
void FTCleanup(bool PatternOnly);

void ChangeSelection(int How);
OperationResult RenameFiles(panelitem_vector &PanelItems,bool ShowDialog);
OperationResult RenameSelectedFiles(panelitem_vector &PanelItems,bool ShowDialog);
OperationResult RenumberFiles();
OperationResult UndoRenameFiles();

OperationResult RenameFilesExecutor();
OperationResult QuickRenameFilesExecutor();

#endif __FILETOOLS_H
