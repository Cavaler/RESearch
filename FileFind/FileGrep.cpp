#include "StdAfx.h"
#include "..\RESearch.h"

::CHandle		g_hOutput;

bool			g_bCacheOutput;
vector<BYTE>	g_arrOutput;

void WriteGrepOutput(LPCVOID lpBuffer, DWORD dwSize)
{
	if (g_bCacheOutput) {
		g_arrOutput.insert(g_arrOutput.end(), (BYTE *)lpBuffer, (BYTE *)lpBuffer + dwSize);
	} else {
		DWORD dwWritten;
		WriteFile(g_hOutput, lpBuffer, dwSize, &dwWritten, NULL);
	}
}

void FlushGrepOutput()
{
	if (g_arrOutput.empty()) return;

	DWORD dwWritten;
	WriteFile(g_hOutput, &g_arrOutput[0], g_arrOutput.size(), &dwWritten, NULL);
	g_arrOutput.clear();
}

void AddGrepLine(const TCHAR *szLine, bool bEOL = true)
{
	WriteGrepOutput(szLine, _tcslen(szLine)*sizeof(TCHAR));
	if (bEOL) WriteGrepOutput(_T("\r\n"), 2*sizeof(TCHAR));
}

void AddGrepLine(const tstring &strLine, bool bEOL = true)
{
	WriteGrepOutput(strLine.data(), strLine.size()*sizeof(TCHAR));
	if (bEOL) WriteGrepOutput(_T("\r\n"), 2*sizeof(TCHAR));
}

void AddGrepFileName(const tstring &strFileName)
{
	TREParameters NoParam;
	tstring strPrepend = CSO::CreateReplaceString(FGFileNamePrepend.c_str(), _T("\n"), NULL, NoParam);
	tstring strAppend  = CSO::CreateReplaceString(FGFileNameAppend.c_str(),  _T("\n"), NULL, NoParam);

	AddGrepLine(strPrepend, false);
	AddGrepLine(strFileName, strAppend.empty());
	AddGrepLine(strAppend, false);
}

void AddGrepResultLine(const tstring &strLine, int nLineNumber)
{
	if (FGAddLineNumbers) {
		AddGrepLine(FormatStr(_T("%d:"), nLineNumber), false);
	}
	AddGrepLine(strLine);
}

bool GrepLineFound(const tstring &strBuf, tstring &strMatch)
{
	bool bResult;

#ifdef UNICODE
	if (FSearchAs == SA_REGEXP) {
		bResult = do_pcre16_exec(FPattern16, FPatternExtra16, strBuf.data(), strBuf.length(), 0, 0, REParam.Match(), REParam.Count()) >= 0;
		if (bResult) {
			REParam.AddSource(strBuf.data(), strBuf.length());
			strMatch = REParam.GetParam(0);
		}
	} else {
		TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;
		bResult = BMHSearch(strBuf.data(), strBuf.length(), FTextUpcase.data(), FTextUpcase.size(), szTable) >= 0;
	}
#else
	if (FSearchAs == SA_REGEXP) {
		bResult = do_pcre_exec(FPattern, FPatternExtra, strBuf.data(), strBuf.length(), 0, 0, REParam.Match(), REParam.Count()) >= 0;
		if (bResult) {
			REParam.AddSource(strBuf.data(), strBuf.length());
			strMatch = REParam.GetParam(0);
		}
	} else {
		TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;
		bResult = BMHSearch(strBuf.data(), strBuf.length(), FTextUpcase.data(), FTextUpcase.size(), szTable) >= 0;
	}
#endif

	return bResult != (FSInverse != 0);
}

bool GrepMatchesFound(const tstring &strBuf, vector<tstring> &arrMatch)
{
	REParam.AddSource(strBuf.data(), strBuf.length());

	size_t nStart = 0;
	while (nStart < strBuf.size())  {
#ifdef UNICODE
		if (do_pcre16_exec(FPattern16, FPatternExtra16, strBuf.data(), strBuf.length(), nStart, 0, REParam.Match(), REParam.Count()) < 0) break;
		arrMatch.push_back(REParam.GetParam(0));
		nStart = REParam.m_arrMatch[1];
#else
		if (do_pcre_exec(FPattern, FPatternExtra, strBuf.data(), strBuf.length(), nStart, 0, REParam.Match(), REParam.Count()) < 0) break;
		arrMatch.push_back(REParam.GetParam(0));
		nStart = REParam.m_arrMatch[1];
#endif
	}

	return !arrMatch.empty();
}

class CGrepFrontend : public IFrontend
{
public:
	CGrepFrontend() : m_pProc(NULL) {}
	~CGrepFrontend() { if (m_pProc) delete m_pProc; }

	virtual bool	Process(IBackend *pBackend);

protected:
	ISplitLineProcessor *m_pProc;
};

bool CGrepFrontend::Process(IBackend *pBackend)
{
#ifdef UNICODE
	m_pProc = new CUnicodeSplitLineProcessor(pBackend);
#else
	m_pProc = new CSingleByteSplitLineProcessor(pBackend);
#endif

	deque<tstring> arrStringBuffer;
	int nFoundLineCount = 0;
	int nFoundMatchCount = 0;
	int nFirstBufferLine = 0;
	int nLastMatched = -1;

	REParam.Clear();
	REParam.AddRE(FPattern);

	g_bCacheOutput = true;
	g_arrOutput.clear();

	do {
#ifdef UNICODE
		const wchar_t *szBuffer = m_pProc->BufferW();
		INT_PTR nSize  = m_pProc->SizeW();
#else
		const char *szBuffer = m_pProc->Buffer();
		INT_PTR nSize  = m_pProc->Size();
#endif

		arrStringBuffer.push_back(tstring(szBuffer, szBuffer+nSize));

		vector<tstring> arrMatch;
		tstring strMatch;

		bool bFound = (FGOutputLines && FGMatchingLinePart) || (FGOutputNames && FGAddMatchCount)
			? GrepMatchesFound(arrStringBuffer.back(), arrMatch)
			: GrepLineFound(arrStringBuffer.back(), strMatch);

		if (bFound) {
			nFoundLineCount++;
			nFoundMatchCount += arrMatch.size();

			if (FGOutputNames && !FGAddLineCount && !FGAddMatchCount) {
				if (FGOutputLines) {
					if (nFoundLineCount == 1) AddGrepFileName(pBackend->FileName());
				} else {
					AddGrepFileName(pBackend->FileName());
					FlushGrepOutput();
					return true;
				}
			}

			if (FGAddContext) {
				nLastMatched = arrStringBuffer.size()-1;
			} else if (FGOutputLines) {
				if (FGMatchingLinePart) {
					for each (const tstring &strFound in arrMatch)
						AddGrepResultLine(strFound, nFirstBufferLine + 1);
				} else {
					AddGrepResultLine(/*FGMatchingLinePart ? strMatch : */arrStringBuffer.back(), nFirstBufferLine + 1);
				}
			}
		} else {
			if (nLastMatched >= 0) {
				if (nLastMatched < (int)(arrStringBuffer.size()-FGContextLines*2-1)) {
					if (FGOutputLines) {
						AddGrepLine(_T(">>>"));
						for (size_t nLine = 0; nLine <= nLastMatched+FGContextLines; nLine++) {
							AddGrepResultLine(arrStringBuffer[nLine], nLine + nFirstBufferLine + 1);
						}
						AddGrepLine(_T("<<<"));
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

	g_bCacheOutput = false;

	if (FGOutputNames && (FGAddLineCount || FGAddMatchCount) && (nFoundLineCount > 0)) {
		tstring strFileName = pBackend->FileName();
		if (FGAddLineCount ) strFileName += FormatStr(_T(":%d"), nFoundLineCount );
		if (FGAddMatchCount) strFileName += FormatStr(_T(";%d"), nFoundMatchCount);
		AddGrepFileName(strFileName);
	}

	FlushGrepOutput();

	if (FGOutputLines && (FGAddContext && (nLastMatched >= 0))) {
		AddGrepLine(_T(">>>"));
		for (size_t nLine = 0; (nLine < arrStringBuffer.size()) && (nLine <= nLastMatched+FGContextLines); nLine++) {
			AddGrepResultLine(arrStringBuffer[nLine], nLine + nFirstBufferLine + 1);
		}
		AddGrepLine(_T("<<<"));
	}

	return nFoundLineCount > 0;
}

void GrepFile(const FIND_DATA *FindData, panelitem_vector &PanelItems)
{
	if (FText.empty()) {
		AddFile(FindData, PanelItems);
		AddGrepLine(FindData->cFileName);
		return;
	}

	CGrepFrontend Frontend;
	bool bAnyFound = RunSearch(FindData->cFileName, &Frontend);

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
	bool bInverse = pDlg->IsDlgItemChecked(MInverseSearch);
	pDlg->EnableDlgItem(MQuoteSearch, bRegExp);

	bool bNames   = pDlg->IsDlgItemChecked(MGrepOutputNames);
	pDlg->EnableCheckDlgItem(MGrepOutputLCount, bNames);
	pDlg->EnableCheckDlgItem(MGrepOutputMCount, bNames && bRegExp && !bInverse);

	bool bLines   = pDlg->IsDlgItemChecked(MGrepOutputLines);
	bool bContext = pDlg->IsDlgItemChecked(MGrepAndContext);
	pDlg->EnableCheckDlgItem(MGrepOutputLNumbers, bLines);
	pDlg->EnableCheckDlgItem(MGrepMatchedLinePart, bLines && bRegExp && !bContext && !bInverse);

	bool bMatchedPart = pDlg->IsDlgItemChecked(MGrepMatchedLinePart);
	pDlg->EnableCheckDlgItem(MGrepAndContext, bLines && !bMatchedPart);

	bContext = pDlg->IsDlgItemChecked(MGrepAndContext);
	pDlg->EnableDlgItem(MGrepAndContext, bContext, 1);
	pDlg->EnableDlgItem(MGrepAndContext, bContext, 2);
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

bool GrepPrompt(bool bPlugin)
{
	bool AsRegExp = (FSearchAs == SA_REGEXP) || (FSearchAs == SA_SEVERALLINE) || (FSearchAs == SA_MULTILINE) || (FSearchAs == SA_MULTIREGEXP);

	CFarDialog Dialog(77, 27, _T("FileGrepDlg"));
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

	Dialog.Add(new CFarCheckBoxItem(5,10,0,MGrepOutputNames,&FGOutputNames));
		Dialog.Add(new CFarCheckBoxItem(7,11,0,MGrepOutputLCount,&FGAddLineCount));
		Dialog.Add(new CFarCheckBoxItem(7,12,0,MGrepOutputMCount,&FGAddMatchCount));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MGrepOutputLines,&FGOutputLines));
		Dialog.Add(new CFarCheckBoxItem(7,14,0,MGrepOutputLNumbers,&FGAddLineNumbers));
		Dialog.Add(new CFarCheckBoxItem(7,15,0,MGrepAndContext,&FGAddContext));
		Dialog.Add(new CFarEditItem(15,15,19,0,NULL,(int &)FGContextLines,new CFarIntegerRangeValidator(0,1024)));
		Dialog.Add(new CFarTextItem(21,15,0,MGrepContextLines));
		Dialog.Add(new CFarCheckBoxItem(7,16,0,MGrepMatchedLinePart, &FGMatchingLinePart));

	Dialog.Add(new CFarCheckBoxItem(5,18,0,MGrepOutputTo,&FGOutputToFile));
	Dialog.Add(new CFarEditItem(20,18,45,DIF_HISTORY,_T("RESearch.GrepOutput"), FGOutputFile));
	Dialog.Add(new CFarCheckBoxItem(5,19,0,MGrepEditor,&FGOpenInEditor));

	Dialog.Add(new CFarButtonItem(60,11,0,0,MBtnAdvanced));
	Dialog.Add(new CFarButtonItem(60,14,0,0,MBtnSettings));

	Dialog.Add(new CFarTextItem(5,21,0,MSearchIn));
	Dialog.Add(new CFarComboBoxItem(15,21,60,DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND,new CFarListData(g_WhereToSearch, false),(int *)&FSearchIn));

	Dialog.AddButtons(MOk,MCancel,MBtnClose);
	Dialog.Add(new CFarButtonItem(60,24,0,0,MBtnPresets));
	Dialog.Add(new CFarCheckBoxItem(56,11,0,_T(""),&FAdvanced));
	Dialog.SetFocus(MMask, 1);
	FACaseSensitive = FADirectoryCaseSensitive = MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display()) {
		case MOk:
		case MBtnClose:
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
			if (AdvancedSettings()) FAdvanced=true;
			break;
		case MBtnSettings:
			ConfigureGrep();
			break;
		default:
			return false;
		}
	} while (!IsOKClose(ExitCode) || !PrepareFileGrepPattern() || (!FGOutputToFile && !FGOpenInEditor));

	return (ExitCode == MOk);
}

OperationResult FileGrep(bool ShowDialog)
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
		GetTempFileName(szBuffer, _T("grep"), 0, szName);
		strFileName = szName;
	}

	g_hOutput = CreateFile(FullExtendedFileName(strFileName).c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
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

	return FileGrep(false);
}

bool CFGPresetCollection::EditPreset(CPreset *pPreset)
{
	SearchAs FSA = (SearchAs)pPreset->m_mapInts["SearchAs"];
	bool AsRegExp = (FSA == SA_REGEXP) || (FSA == SA_SEVERALLINE) || (FSA == SA_MULTILINE) || (FSA == SA_MULTIREGEXP);

	CFarDialog Dialog(76,26,_T("FGPresetDlg"));
	Dialog.SetWindowProc(FileGrepDialogProc, 0);
	Dialog.SetUseID(true);
	Dialog.SetCancelID(MCancel);

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

	Dialog.Add(new CFarCheckBoxItem(5,12,0,MGrepOutputNames,&pPreset->m_mapInts["OutputNames"]));
		Dialog.Add(new CFarCheckBoxItem(7,13,0,MGrepOutputLCount,&pPreset->m_mapInts["AddLineCount"]));
		Dialog.Add(new CFarCheckBoxItem(7,14,0,MGrepOutputMCount,&pPreset->m_mapInts["AddMatchCount"]));
	Dialog.Add(new CFarCheckBoxItem(5,15,0,MGrepOutputLines,&pPreset->m_mapInts["OutputLines"]));
		Dialog.Add(new CFarCheckBoxItem(7,16,0,MGrepOutputLNumbers,&pPreset->m_mapInts["AddLineNumbers"]));
		Dialog.Add(new CFarCheckBoxItem(7,17,0,MGrepAndContext,&pPreset->m_mapInts["AddContext"]));
		Dialog.Add(new CFarEditItem(15,17,19,0,NULL,&pPreset->m_mapInts["ContextLines"],new CFarIntegerRangeValidator(0,1024)));
		Dialog.Add(new CFarTextItem(21,17,0,MGrepContextLines));
		Dialog.Add(new CFarCheckBoxItem(7,18,0,MGrepMatchedLinePart, &pPreset->m_mapInts["MatchingLinePart"]));

	int  nAdvancedID = pPreset->m_mapInts["AdvancedID"];
	bool bFAdvanced = nAdvancedID > 0;

	Dialog.Add(new CFarCheckBoxItem(56,12,0,_T(""),&bFAdvanced));
	Dialog.Add(new CFarButtonItem(60,12,0,0,MBtnAdvanced));
	Dialog.Add(new CFarCheckBoxItem(5,20,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(-1)) {
		case MOk:
			pPreset->m_mapInts["SearchAs"] = AsRegExp ? SA_REGEXP : SA_PLAINTEXT;
			pPreset->m_mapInts["AdvancedID"] = bFAdvanced ? nAdvancedID : 0;
			return true;
		case MBtnAdvanced:
			SelectAdvancedPreset(nAdvancedID, bFAdvanced);
			break;
		default:
			return false;
		}
	} while (true);

	return false;
}
