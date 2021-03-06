#include "StdAfx.h"
#include "..\RESearch.h"

#include "FileOperations.h"

void SearchFile(const FIND_DATA *FindData, panelitem_vector &PanelItems)
{
	bool IsFound;

	InitFoundPosition();

	if (FindData->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
		if (FText.empty()) AddFile(FindData, PanelItems);
		return;
	}

	REParam.Clear();
	if (FPattern) REParam.AddRE(FPattern);

	if (FText.empty()) IsFound=true; else 
	if (FindData->nFileSize == 0) IsFound=false; else 
	switch (FSearchAs) {
	case SA_PLAINTEXT:{
		CSearchPlainTextFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend);
		break;
					  }
	case SA_REGEXP:{
		CSearchRegExpFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend);
		break;
				   }
	case SA_SEVERALLINE:{
		CSearchSeveralLineRegExpFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend);
		break;
						}
	case SA_MULTILINE:{
		CSearchMultiLineRegExpFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend);
		break;
					  }
	case SA_MULTITEXT:{
		CSearchMultiTextFrontend Frontend;
		IsFound = RunSearch(FindData->cFileName, &Frontend);
		break;
					  }
	default:IsFound=false;
	}

	// This is exclusive 'OR'...
	if ((IsFound) ? !FSInverse : FSInverse) AddFile(FindData, PanelItems, true);
}

bool PrepareFileSearchPattern()
{
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
	ReplaceEOLDialogProc(nMsg, lParam2);

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
		case MQuoteSearch:
			QuoteRegExpString(pDlg, MText);
			return TRUE;
		}
		break;
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

bool SearchPrompt(bool Plugin)
{
	CFarDialog Dialog(76, 24, _T("FileSearchDlg"));
	Dialog.SetWindowProc(FileSearchDialogProc, 0);
	Dialog.SetUseID(true);

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
	Dialog.Add(new CFarCheckBoxItem(5,16,0,MShowStatistics,&FShowStatistics));

	Dialog.Add(new CFarTextItem(5,18,0,MSearchIn));
	if (Plugin) {
		if (FSearchIn<SI_FROMCURRENT) FSearchIn=SI_FROMCURRENT;
		Dialog.Add(new CFarComboBoxItem(15,18,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearchPlugin, false),(int *)&FSearchIn,NULL,3));
	} else {
		Dialog.Add(new CFarComboBoxItem(15,18,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));
	}

	Dialog.Add(new CFarCheckBoxItem(56,8,0,MLeftBracket,&FAdvanced));
	Dialog.Add(new CFarButtonItem  (62,8,DIF_NOBRACKETS,0,MBtnAdvanced));
	Dialog.Add(new CFarButtonItem(_tcslen(GetMsg(MSeveralLineRegExp))+10,9,0,0,MEllipsis));

	Dialog.AddButtons(MOk,MCancel,MBtnClose);
	Dialog.Add(new CFarButtonItem(60,20,0,0,MBtnPresets));

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
		case MBtnPresets:
			FSPresets->ShowMenu(true);
			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
			break;
		case MBtnAdvanced:
			if (AdvancedSettings()) FAdvanced=true;
			break;
		case MEllipsis:
			ConfigureSeveralLines();
			break;
		default:
			return false;
		}
	} while (!IsOKClose(ExitCode) || !PrepareFileSearchPattern());

	return (ExitCode == MOk);
}

OperationResult FileFind(panelitem_vector &PanelItems, bool ShowDialog, bool bSilent)
{
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.PanelType != PTYPE_FILEPANEL) return OR_FAILED;

	if (PInfo.Plugin)
	{
		if ((PInfo.Flags & PFLAGS_REALNAMES) == 0) return OR_FAILED;
		if (FSearchIn < SI_FROMCURRENT) FSearchIn = SI_FROMCURRENT;
	}

	if (ShowDialog) {
		if (!SearchPrompt(PInfo.Plugin)) return OR_CANCEL;
	} else {
		if (!PrepareFileSearchPattern()) return OR_CANCEL;
	}

	CDebugTimer tm(_T("FileFind() took %d ms"));
	bool bResult = ScanDirectories(PanelItems, SearchFile);
	tm.Stop();

	if (!bResult) return OR_FAILED;

	if (FShowStatistics)
		ShowStatistics(false, PanelItems);

	return !PanelItems.empty() ? OR_PANEL :
		(bSilent || FShowStatistics) ? OR_OK : NoFilesFound();
}

OperationResult FileSearchExecutor()
{
	FMask = MaskText;
	FText = SearchText;
	return FileFind(g_PanelItems, false, true);
}

bool CFSPresetCollection::EditPreset(CPreset *pPreset)
{
	CFarDialog Dialog(76, 28, _T("FSPresetDlg"));
	Dialog.SetWindowProc(FileSearchDialogProc, 0);
	Dialog.SetUseID(true);

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

	Dialog.Add(new CFarCheckBoxItem(5,15,0,MCaseSensitive, &pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(5,16,0,MInverseSearch, &pPreset->m_mapInts["Inverse"]));
	Dialog.Add(new CFarCheckBoxItem(5,17,0,MAllCharTables, &pPreset->m_mapInts["AllCharTables"]));
	Dialog.Add(new CFarCheckBoxItem(5,18,0,MShowStatistics,&pPreset->m_mapInts["ShowStatistics"]));

	Dialog.Add(new CFarTextItem(5,20,0,MSearchIn));
	Dialog.Add(new CFarComboBoxItem(15,20,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),&pPreset->m_mapInts["SearchIn"]));

	Dialog.Add(new CFarCheckBoxItem(56,9,0,MLeftBracket,&bFAdvanced));
	Dialog.Add(new CFarButtonItem  (62,9,DIF_NOBRACKETS,0,MBtnAdvanced));
	Dialog.Add(new CFarButtonItem(_tcslen(GetMsg(MSeveralLineRegExp))+10,11,0,0,MEllipsis));

	Dialog.Add(new CFarCheckBoxItem(5,22,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display()) {
		case MOk:
			pPreset->m_mapInts["AdvancedID"] = bFAdvanced ? nAdvancedID : 0;
			return true;
		case MBtnAdvanced:
			SelectAdvancedPreset(nAdvancedID, bFAdvanced);
			break;
		case MEllipsis:
			ConfigureSeveralLines();
			break;
		default:
			return false;
		}
	} while (true);
}
