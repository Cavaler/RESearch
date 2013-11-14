#include "StdAfx.h"
#include "..\RESearch.h"

::CHandle g_hOutput;
int     g_nLines;

void AddGrepLine(const TCHAR *szLine, bool bEOL = true)
{
	DWORD dwWritten;
	WriteFile(g_hOutput, szLine, _tcslen(szLine)*sizeof(TCHAR), &dwWritten, NULL);
	if (bEOL) WriteFile(g_hOutput, _T("\r\n"), 2*sizeof(TCHAR), &dwWritten, NULL);
}

void AddGrepLine(const tstring &strLine, bool bEOL = true)
{
	DWORD dwWritten;
	WriteFile(g_hOutput, strLine.data(), strLine.size()*sizeof(TCHAR), &dwWritten, NULL);
	if (bEOL) WriteFile(g_hOutput, _T("\r\n"), 2*sizeof(TCHAR), &dwWritten, NULL);
}

void AddGrepFileName(const tstring &strFileName)
{
	AddGrepLine(FGFileNamePrepend, false);
	AddGrepLine(strFileName, FGFileNameAppend.empty());
	AddGrepLine(FGFileNameAppend, false);
}

void AddGrepResultLine(const string &strLine, int nLineNumber)
{
	if (FGAddLineNumbers) {
		AddGrepLine(FormatStr(_T("%d:"), nLineNumber), false);
	}
#ifdef UNICODE
	if (FSearchAs == SA_REGEXP) {
		AddGrepLine(UTF8ToUnicode(strLine));
	} else {
		DWORD dwWritten;
		WriteFile(g_hOutput, strLine.data(), strLine.size(), &dwWritten, NULL);
		AddGrepLine(L"");
	}
#else
	AddGrepLine(strLine);
#endif
}

bool GrepLineFound(const string &strBuf, string &strMatch)
{
	BOOL bResult;

#ifdef UNICODE
	if (FSearchAs == SA_REGEXP) {
		bResult = do_pcre_execA(FPattern, FPatternExtra, strBuf.data(), strBuf.length(), 0, 0, REParamA.Match(), REParamA.Count()) >= 0;
		if (bResult) {
			REParamA.AddSource(strBuf.data(), strBuf.length());
			strMatch = REParamA.GetParam(0);
		}
	} else {
		TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;
		bResult = BMHSearch((WCHAR *)strBuf.data(), strBuf.length()/2, FTextUpcase.data(), FTextUpcase.size(), szTable) >= 0;
	}
#else
	if (FSearchAs == SA_REGEXP) {
		bResult = do_pcre_exec(FPattern, FPatternExtra, strBuf.data(), strBuf.length(), 0, 0, REParamA.Match(), REParamA.Count()) >= 0;
		if (bResult) {
			REParamA.AddSource(strBuf.data(), strBuf.length());
			strMatch = REParamA.GetParam(0);
		}
	} else {
		TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;
		bResult = BMHSearch(strBuf.data(), strBuf.length(), FTextUpcase.data(), FTextUpcase.size(), szTable) >= 0;
	}
#endif

	return (bResult != 0) != (FSInverse != 0);
}

class CGrepFrontend : public IFrontend
{
public:
	CGrepFrontend(bool bUTF8 = true) : m_pProc(NULL), m_bUTF8(bUTF8) {}
	~CGrepFrontend() { if (m_pProc) delete m_pProc; }

	virtual bool	Process(IBackend *pBackend);

protected:
	ISplitLineProcessor *m_pProc;
	bool m_bUTF8;
};

bool CGrepFrontend::Process(IBackend *pBackend)
{
#ifdef UNICODE
	if (m_bUTF8)
		m_pProc = new CSingleByteSplitLineProcessor(pBackend);
	else
		m_pProc = new CUnicodeSplitLineProcessor(pBackend);
#else
	m_pProc = new CSingleByteSplitLineProcessor(pBackend);
#endif

	deque<string> arrStringBuffer;
	int nFoundCount = 0;
	int nFirstBufferLine = 0;
	int nLastMatched = -1;

	REParamA.Clear();
	REParamA.AddRE(FPattern);

	do {
		const char *szBuffer = m_pProc->Buffer();
		INT_PTR nSize  = m_pProc->Size();

		arrStringBuffer.push_back(string(szBuffer, szBuffer+nSize));

		string strMatch;
		if (GrepLineFound(arrStringBuffer.back(), strMatch)) {
			nFoundCount++;

			switch (FGrepWhat) {
			case GREP_NAMES:
				AddGrepFileName(pBackend->FileName());
				return true;
			case GREP_NAMES_LINES:
				if (nFoundCount == 1) AddGrepFileName(pBackend->FileName());
				break;
			}

			if (FGAddContext) {
				nLastMatched = arrStringBuffer.size()-1;
			} else if ((FGrepWhat == GREP_LINES) ||(FGrepWhat == GREP_NAMES_LINES) ) {
				AddGrepResultLine(FGMatchingLinePart ? strMatch : arrStringBuffer.back(), nFirstBufferLine + 1);
			}
		} else {
			if (nLastMatched >= 0) {
				if (nLastMatched < (int)(arrStringBuffer.size()-FGContextLines*2-1)) {
					switch (FGrepWhat) {
					case GREP_NAMES_LINES:
					case GREP_LINES:
						AddGrepLine(_T(">>>"));
						for (size_t nLine = 0; nLine <= nLastMatched+FGContextLines; nLine++) {
							AddGrepResultLine(arrStringBuffer[nLine], nLine + nFirstBufferLine + 1);
						}
						AddGrepLine(_T("<<<"));
						break;
					}
					while ((int)arrStringBuffer.size() > FGContextLines) {
						arrStringBuffer.pop_front();
						nFirstBufferLine++;
					}
					nLastMatched = -1;
				} // else not yet time for output
			} else if (FGAddContext) {
				while ((int)arrStringBuffer.size() > FGContextLines) {
					arrStringBuffer.pop_front();
					nFirstBufferLine++;
				}
			}
		}

		if (!FGAddContext) {
			arrStringBuffer.clear();
			nFirstBufferLine++;
		}

	} while (m_pProc->GetNextLine());

	switch (FGrepWhat) {
	case GREP_NAMES_COUNT:
		if (nFoundCount > 0) AddGrepFileName(FormatStr(_T("%s:%d"), pBackend->FileName(), nFoundCount).c_str());
		break;
	case GREP_NAMES_LINES:
	case GREP_LINES:
		if (FGAddContext && (nLastMatched >= 0)) {
			AddGrepLine(_T(">>>"));
			for (size_t nLine = 0; (nLine < arrStringBuffer.size()) && (nLine <= nLastMatched+FGContextLines); nLine++) {
				AddGrepResultLine(arrStringBuffer[nLine], nLine + nFirstBufferLine + 1);
			}
			AddGrepLine(_T("<<<"));
		}
		break;
	}

	return nFoundCount > 0;
}

void GrepFile(WIN32_FIND_DATA *FindData, panelitem_vector &PanelItems)
{
	if (FText.empty()) {
		AddFile(FindData, PanelItems);
		AddGrepLine(FindData->cFileName);
		return;
	}

	bool bAnyFound;
	if (FSearchAs == SA_REGEXP) {
		CGrepFrontend Frontend(true);
		bAnyFound = RunSearch(FindData->cFileName, &Frontend, true);
	} else {
		CGrepFrontend Frontend(false);
		bAnyFound = RunSearch(FindData->cFileName, &Frontend, false);
	}

	if (bAnyFound) AddFile(FindData, PanelItems, true);
}

bool PrepareFileGrepPattern()
{
	if (!FPreparePattern(true)) return false;
	if (FAdvanced) {
		if (!CompileAdvancedSettings()) return false;
	}
	return true;
}

void UpdateFGDialog(CFarDialog *pDlg)
{
	bool bRegExp  = pDlg->IsDlgItemChecked(MRegExp);
	bool bContext = pDlg->IsDlgItemChecked(MGrepAdd);
	bool bLines   = pDlg->IsDlgItemChecked(MGrepLines) || pDlg->IsDlgItemChecked(MGrepNamesLines);

	pDlg->EnableDlgItem(MGrepAdd, bLines);
	pDlg->EnableDlgItem(MGrepAddLineNumbers, bLines);
	pDlg->EnableDlgItem(MQuoteSearch, bRegExp);
	pDlg->EnableDlgItem(MGrepMatchedLinePart, bLines && bRegExp && !bContext);
	if (!bRegExp || bContext) pDlg->CheckDlgItem(MGrepMatchedLinePart, false);

	bool bMatchedPart = pDlg->IsDlgItemChecked(MGrepMatchedLinePart);
	pDlg->EnableDlgItem(MGrepAdd, !bMatchedPart);
	pDlg->EnableDlgItem(MGrepAdd, !bMatchedPart, 1);
	if (bMatchedPart) pDlg->CheckDlgItem(MGrepAdd, false);
}

LONG_PTR WINAPI FileGrepDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2)
{
	int nCtlID = pDlg->GetID(nParam1);

	switch (nMsg) {
	case DN_INITDIALOG:
		UpdateFGDialog(pDlg);
		HighlightREError(pDlg);
		break;
	case DN_BTNCLICK:
		UpdateFGDialog(pDlg);
		break;
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

bool GrepPrompt(BOOL bPlugin) {
	BOOL AsRegExp = (FSearchAs == SA_REGEXP) || (FSearchAs == SA_SEVERALLINE) || (FSearchAs == SA_MULTILINE) || (FSearchAs == SA_MULTIREGEXP);

	CFarDialog Dialog(77, 26, _T("FileGrepDlg"));
	Dialog.SetWindowProc(FileGrepDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

	Dialog.AddFrame(MREGrep);

	Dialog.Add(new CFarCheckBoxItem(35,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("Masks"), MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), SearchText));
	Dialog.Add(new CFarButtonItem(67,5,0,0,MQuoteSearch));

	Dialog.Add(new CFarTextItem(5,6,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&AsRegExp));
	Dialog.Add(new CFarCheckBoxItem(35,7,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MInverseSearch,&FSInverse));
	Dialog.Add(new CFarCheckBoxItem(35,8,0,MAllCharTables,&FAllCharTables));
	Dialog.Add(new CFarTextItem(5,9,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));

	Dialog.Add(new CFarRadioButtonItem(5,10,DIF_GROUP,MGrepNames,		(int *)&FGrepWhat,GREP_NAMES));
	Dialog.Add(new CFarRadioButtonItem(5,11,0,MGrepNamesCount,	(int *)&FGrepWhat,GREP_NAMES_COUNT));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,MGrepLines,		(int *)&FGrepWhat,GREP_LINES));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,MGrepNamesLines,	(int *)&FGrepWhat,GREP_NAMES_LINES));

	Dialog.Add(new CFarCheckBoxItem(5,14,0,MGrepAdd,&FGAddContext));
	Dialog.Add(new CFarEditItem(15,14,20,0,NULL,(int &)FGContextLines,new CFarIntegerRangeValidator(0,1024)));
	Dialog.Add(new CFarTextItem(22,14,0,MGrepContext));
	Dialog.Add(new CFarCheckBoxItem(5,15,0,MGrepAddLineNumbers, &FGAddLineNumbers));
	Dialog.Add(new CFarCheckBoxItem(5,16,0,MGrepMatchedLinePart, &FGMatchingLinePart));

	Dialog.Add(new CFarCheckBoxItem(5,17,0,MGrepOutput,&FGOutputToFile));
	Dialog.Add(new CFarEditItem(20,17,45,DIF_HISTORY,_T("RESearch.GrepOutput"), FGOutputFile));
	Dialog.Add(new CFarCheckBoxItem(5,18,0,MGrepEditor,&FGOpenInEditor));

	Dialog.Add(new CFarTextItem(5,20,0,MSearchIn));
	Dialog.Add(new CFarComboBoxItem(15,20,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,10,0,0,MBtnPresets));
	Dialog.Add(new CFarCheckBoxItem(56,11,0,_T(""),&FAdvanced));
	Dialog.Add(new CFarButtonItem(60,11,0,0,MBtnAdvanced));
	Dialog.Add(new CFarButtonItem(60,14,0,0,MBtnSettings));
	Dialog.SetFocus(MMask, 1);
	FACaseSensitive = FADirectoryCaseSensitive = MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(-1)) {
		case MOk:
			FSearchAs = AsRegExp ? SA_REGEXP : SA_PLAINTEXT;
			FMask=MaskText;
			FText=SearchText;
			break;
		case MQuoteSearch:
			if (AsRegExp) CSO::QuoteRegExpString(SearchText);
			break;
		case MBtnPresets:
			FGPresets->ShowMenu(true);
			break;
		case MBtnAdvanced:
			if (AdvancedSettings()) FAdvanced=TRUE;
			break;
		case MBtnSettings:
			ConfigureGrep();
			break;
		default:
			return false;
		}
	} while ((ExitCode != MOk) || !PrepareFileGrepPattern() || (!FGOutputToFile && !FGOpenInEditor));

	return true;
}

OperationResult FileGrep(BOOL ShowDialog)
{
	CPanelInfo PInfo;
	PInfo.GetInfo(false);
	if (PInfo.PanelType!=PTYPE_FILEPANEL) return OR_FAILED;
	if (PInfo.Plugin&&((PInfo.Flags&PFLAGS_REALNAMES)==0)) return OR_FAILED;

	if (ShowDialog) {
		if (!GrepPrompt(PInfo.Plugin)) return OR_CANCEL;
	} else {
		if (!PrepareFileGrepPattern()) return OR_CANCEL;
	}

	tstring strFileName;
	if (FGOutputToFile && !FGOutputFile.empty()) {
		strFileName = FGOutputFile;
	} else {
		TCHAR szBuffer[MAX_PATH], szName[MAX_PATH];
		GetTempPath(MAX_PATH, szBuffer);
		GetTempFileName(szBuffer, _T("re"), 0, szName);
		strFileName = szName;
	}

	g_hOutput = CreateFile(strFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
		CREATE_ALWAYS, 0, NULL);
	if (!g_hOutput) {
		ShowLastError(GetMsg(MFileCreateError), strFileName.c_str());
		return OR_FAILED;
	}

#ifdef UNICODE
	AddGrepLine(L"\xFEFF", false);
#endif

	CDebugTimer tm(_T("FileGrep() took %d ms"));
	int nResult = ScanDirectories(g_PanelItems, GrepFile);
	tm.Stop();

	if (nResult) {
		g_hOutput.Close();
		if (FGOpenInEditor) {
			StartupInfo.Editor(strFileName.c_str(), NULL, 0, 0, -1, -1,
				EF_NONMODAL|EF_IMMEDIATERETURN|EF_ENABLE_F6| (FGOutputToFile ? 0 : EF_DELETEONLYFILEONCLOSE), 0, 1
#ifdef UNICODE
				, CP_UNICODE
#endif
				);
		}
		return OR_OK;
	} else return OR_FAILED;
}

OperationResult FileGrepExecutor() {
	FMask = MaskText;
	FText = SearchText;

	return FileGrep(FALSE);
}

BOOL CFGPresetCollection::EditPreset(CPreset *pPreset) {
	SearchAs FSA = (SearchAs)pPreset->m_mapInts["SearchAs"];
	BOOL AsRegExp = (FSA == SA_REGEXP) || (FSA == SA_SEVERALLINE) || (FSA == SA_MULTILINE) || (FSA == SA_MULTIREGEXP);

	CFarDialog Dialog(76,25,_T("FGPresetDlg"));
	Dialog.AddFrame(MREGrep);

	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"), pPreset->Name()));
	Dialog.Add(new CFarCheckBoxItem(35,4,0,MAsRegExp,&pPreset->m_mapInts["MaskAsRegExp"]));
	Dialog.Add(new CFarTextItem(5,4,0,MMask));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,_T("Masks"), pPreset->m_mapStrings["Mask"]));

	Dialog.Add(new CFarTextItem(5,6,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,8,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarCheckBoxItem(5,9,0,MRegExp,&AsRegExp));
	Dialog.Add(new CFarCheckBoxItem(35,9,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(5,10,0,MInverseSearch,&pPreset->m_mapInts["Inverse"]));

	Dialog.Add(new CFarTextItem(5,11,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));

	Dialog.Add(new CFarRadioButtonItem(5,12,DIF_GROUP,MGrepNames,	&pPreset->m_mapInts["GrepWhat"],GREP_NAMES));
	Dialog.Add(new CFarRadioButtonItem(5,13,0,MGrepNamesCount,		&pPreset->m_mapInts["GrepWhat"],GREP_NAMES_COUNT));
	Dialog.Add(new CFarRadioButtonItem(5,14,0,MGrepLines,			&pPreset->m_mapInts["GrepWhat"],GREP_LINES));
	Dialog.Add(new CFarRadioButtonItem(5,15,0,MGrepNamesLines,		&pPreset->m_mapInts["GrepWhat"],GREP_NAMES_LINES));

	Dialog.Add(new CFarCheckBoxItem(5,16,0,MGrepAdd,&pPreset->m_mapInts["AddContext"]));
	Dialog.Add(new CFarEditItem(15,16,20,0,NULL,&pPreset->m_mapInts["ContextLines"],new CFarIntegerRangeValidator(0,1024)));
	Dialog.Add(new CFarTextItem(22,16,0,MGrepContext));
	Dialog.Add(new CFarCheckBoxItem(5,17,0,MGrepAddLineNumbers, &pPreset->m_mapInts["AddLineNumbers"]));
	Dialog.Add(new CFarCheckBoxItem(5,18,0,MGrepMatchedLinePart, &pPreset->m_mapInts["MatchingLinePart"]));

	int  nAdvancedID = pPreset->m_mapInts["AdvancedID"];
	bool bFAdvanced = nAdvancedID > 0;

	Dialog.Add(new CFarCheckBoxItem(56,12,0,_T(""),&bFAdvanced));
	Dialog.Add(new CFarButtonItem(60,12,0,0,MBtnAdvanced));
	Dialog.Add(new CFarCheckBoxItem(5,19,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(2, MOk, MBtnAdvanced)) {
		case MOk:
			pPreset->m_mapInts["SearchAs"] = AsRegExp ? SA_REGEXP : SA_PLAINTEXT;
			pPreset->m_mapInts["AdvancedID"] = bFAdvanced ? nAdvancedID : 0;
			return TRUE;
		case MBtnAdvanced:
			SelectAdvancedPreset(nAdvancedID, bFAdvanced);
			break;
		default:
			return FALSE;
		}
	} while (true);
}
