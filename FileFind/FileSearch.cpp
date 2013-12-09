#include "StdAfx.h"
#include "..\RESearch.h"

#include "FileOperations.h"

void SearchFile(WIN32_FIND_DATA *FindData, panelitem_vector &PanelItems) {
	BOOL IsFound;

	InitFoundPosition();

	if (FindData->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
		if (FText.empty()) AddFile(FindData, PanelItems);
		return;
	}

	REParam.Clear();
	if (FPattern) REParam.AddRE(FPattern);
#ifdef UNICODE
	REParamA.Clear();
	if (FPattern) REParamA.AddRE(FPattern);
#endif

	if (FText.empty()) IsFound=TRUE; else 
	if (FindData->nFileSizeLow==0) IsFound=FALSE; else 
	switch (FSearchAs) {
	case SA_PLAINTEXT:{
		CSearchPlainTextFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend, false);
		break;
					  }
	case SA_REGEXP:{
		CSearchRegExpFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend, true);
		break;
				   }
	case SA_SEVERALLINE:{
		CSearchSeveralLineRegExpFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend, true);
		break;
						}
	case SA_MULTILINE:{
		CSearchMultiLineRegExpFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend, true);
		break;
					  }
	case SA_MULTITEXT:{
		CSearchMultiTextFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend, false);
		break;
					  }
	default:IsFound=FALSE;
	}

	// This is exclusive 'OR'...
	if ((IsFound) ? !FSInverse : FSInverse) AddFile(FindData, PanelItems, true);
}

bool PrepareFileSearchPattern() {
	if (!FPreparePattern(true)) return false;
	if (FAdvanced) {
		if (!CompileAdvancedSettings()) return false;
	}
	return true;
}

void UpdateFSDialog(CFarDialog *pDlg) {
	bool bRegExp = !pDlg->IsDlgItemChecked(MPlainText);

	if (pDlg->HasItem(MMultiPlainText))
		bRegExp = bRegExp && !pDlg->IsDlgItemChecked(MMultiPlainText);

	if (pDlg->HasItem(MQuoteSearch))
		pDlg->EnableDlgItem(MQuoteSearch, bRegExp);

	if (pDlg->HasItem(MQuoteReplace))
		pDlg->EnableDlgItem(MQuoteReplace, bRegExp || g_bEscapesInPlainText);
}

LONG_PTR WINAPI FileSearchDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2)
{
	int nCtlID = pDlg->GetID(nParam1);

	switch (nMsg) {
	case DN_INITDIALOG:
		UpdateFSDialog(pDlg);
		HighlightREError(pDlg);
		break;
	case DN_BTNCLICK:
		switch (nCtlID) {
		case MPlainText:
		case MRegExp:
		case MSeveralLineRegExp:
		case MMultiLineRegExp:
		case MMultiPlainText:
		case MMultiRegExp:
			UpdateFSDialog(pDlg);
			break;
		}
		break;
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

int SearchPrompt(BOOL Plugin)
{
	CFarDialog Dialog(76,23,_T("FileSearchDlg"));
	Dialog.SetWindowProc(FileSearchDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

	Dialog.AddFrame(MRESearch);

	Dialog.Add(new CFarCheckBoxItem(35,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("Masks"), MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), SearchText));
	Dialog.Add(new CFarButtonItem(67,5,0,0,MQuoteSearch));

	Dialog.Add(new CFarTextItem(5,6,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarRadioButtonItem(5,7,DIF_GROUP,MPlainText,(int *)&FSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,8,0,MRegExp,			(int *)&FSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,9,0,MSeveralLineRegExp,(int *)&FSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,MMultiLineRegExp,	(int *)&FSearchAs,SA_MULTILINE));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MMultiPlainText,	(int *)&FSearchAs,SA_MULTITEXT));

	Dialog.Add(new CFarCheckBoxItem(5,13,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,14,0,MInverseSearch,&FSInverse));
	Dialog.Add(new CFarCheckBoxItem(5,15,0,MAllCharTables,&FAllCharTables));

	Dialog.Add(new CFarCheckBoxItem(56,8,0,_T(""),&FAdvanced));
	Dialog.Add(new CFarButtonItem  (60,8,0,0,MBtnAdvanced));

	Dialog.Add(new CFarTextItem(5,17,0,MSearchIn));
	if (Plugin) {
		if (FSearchIn<SI_FROMCURRENT) FSearchIn=SI_FROMCURRENT;
		Dialog.Add(new CFarComboBoxItem(15,17,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearchPlugin, false),(int *)&FSearchIn,NULL,3));
	} else {
		Dialog.Add(new CFarComboBoxItem(15,17,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));
	}

	Dialog.AddButtons(MOk,MCancel,MBtnClose);
	Dialog.Add(new CFarButtonItem(60,19,0,0,MBtnPresets));

	Dialog.SetFocus(MMask, 1);
	FACaseSensitive=FADirectoryCaseSensitive=MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(-1)) {
		case MOk:
		case MBtnClose:
			FMask=MaskText;
			FText=SearchText;
			break;
		case MQuoteSearch:
			CSO::QuoteRegExpString(SearchText);
			break;
		case MBtnPresets:
			FSPresets->ShowMenu(true);
			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
			break;
		case MBtnAdvanced:
			if (AdvancedSettings()) FAdvanced=TRUE;
			break;
		default:
			return FALSE;
		}
	} while (!IsOKClose(ExitCode) || !PrepareFileSearchPattern());

	return (ExitCode == MOk);
}

OperationResult FileFind(panelitem_vector &PanelItems, BOOL ShowDialog, BOOL bSilent) {
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.PanelType!=PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin&&((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;

	if (ShowDialog) {
		if (!SearchPrompt(PInfo.Plugin)) return OR_CANCEL;
	} else {
		if (!PrepareFileSearchPattern()) return OR_CANCEL;
	}

	CDebugTimer tm(_T("FileFind() took %d ms"));
	int nResult = ScanDirectories(PanelItems, SearchFile);
	tm.Stop();

	if (nResult) {
		return (PanelItems.empty()) ? (bSilent ? OR_OK : NoFilesFound()) : OR_PANEL;
	} else return OR_FAILED;
}

OperationResult FileSearchExecutor() {
	FMask = MaskText;
	FText = SearchText;
	return FileFind(g_PanelItems, FALSE, TRUE);
}

BOOL CFSPresetCollection::EditPreset(CPreset *pPreset)
{
	CFarDialog Dialog(76,24,_T("FSPresetDlg"));
	Dialog.AddFrame(MFSPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"), pPreset->Name()));

	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));
	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("Masks"), pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));

	int *pSearchAs = &pPreset->m_mapInts["SearchAs"];
	int  nAdvancedID = pPreset->m_mapInts["AdvancedID"];
	bool bFAdvanced = nAdvancedID > 0;

	Dialog.Add(new CFarRadioButtonItem(5, 9,DIF_GROUP,MPlainText,pSearchAs,SA_PLAINTEXT));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,		MRegExp,	 pSearchAs,SA_REGEXP));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,		MSeveralLineRegExp,	pSearchAs,SA_SEVERALLINE));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,		MMultiLineRegExp,	pSearchAs,SA_MULTILINE));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,		MMultiPlainText,	pSearchAs,SA_MULTITEXT));

	Dialog.Add(new CFarCheckBoxItem(5,15,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(5,16,0,MInverseSearch,&pPreset->m_mapInts["Inverse"]));
	Dialog.Add(new CFarCheckBoxItem(56,9,0,_T(""),&bFAdvanced));
	Dialog.Add(new CFarButtonItem(60,9,0,0,MBtnAdvanced));

	Dialog.Add(new CFarCheckBoxItem(5,18,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(2, -2, -4)) {
		case 0:
			pPreset->m_mapInts["AdvancedID"] = bFAdvanced ? nAdvancedID : 0;
			return TRUE;
		case 1:
			SelectAdvancedPreset(nAdvancedID, bFAdvanced);
			break;
		default:
			return FALSE;
		}
	} while (true);
}
