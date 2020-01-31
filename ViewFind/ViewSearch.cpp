#include "StdAfx.h"
#include "..\RESearch.h"

struct ViewerSearchInfo
{
	tstring m_strFileName;
	__int64  CurPos;	// Line offset in BYTES from file start
};

map<int, ViewerSearchInfo> g_ViewerInfo;

void SetViewerSelection(__int64 nStart, int nLength)
{
	ViewerSelect VSelect = {ITEM_SS(ViewerSelect) nStart, nLength};
	StartupInfo.ViewerControl(VCTL_SELECT, &VSelect);

	//	After select since select changes position to that line on top
	ViewerSetPosition VPos = {ITEM_SS(ViewerSetPosition) 0, (nStart > 512) ? nStart-512 : 0, 0};
	StartupInfo.ViewerControl(VCTL_SETPOSITION, &VPos);

	StartupInfo.ViewerControl(VCTL_REDRAW, NULL);

	g_bInterrupted = true;
}

bool ViewerSearchAgain()
{
	RefreshViewerInfo();
	g_bInterrupted = false;

	CDebugTimer tm(_T("ViewSearch() took %d ms"));

	tstring strFileName;
#ifdef FAR3
	wchar_t szFileName[MAX_PATH];
	StartupInfo.ViewerControl(VCTL_GETFILENAME, szFileName);
	strFileName = szFileName;
	CFileMapping mapInput(szFileName);
#else
	strFileName = VInfo.FileName;
	CFileMapping mapInput(VInfo.FileName);
#endif

	map<int, ViewerSearchInfo>::iterator it = g_ViewerInfo.find(VInfo.ViewerID);
	if ((it == g_ViewerInfo.end()) || (it->second.m_strFileName != strFileName))
	{
		ViewerSearchInfo Info;
		Info.m_strFileName = strFileName;
#ifdef UNICODE
		Info.CurPos = VInfo.FilePos + VInfo.LeftPos;
#else
		Info.CurPos = VInfo.FilePos.i64 + VInfo.LeftPos;
#endif
		g_ViewerInfo[VInfo.ViewerID] = Info;
	}
	ViewerSearchInfo &Info = g_ViewerInfo[VInfo.ViewerID];

	unique_ptr<CFileBackend> pBackend(new CFileBackend());
	if (!pBackend->SetBlockSize(FBufferSize*1024*1024)) return false;
	if (!pBackend->Open(strFileName.c_str(), -1)) return false;

	int nSkip;
	eLikeUnicode nDetect = LikeUnicode(pBackend->Buffer(), pBackend->Size(), nSkip);
	if (Info.CurPos > 0) nSkip = (int)Info.CurPos;

	unique_ptr<IDecoder> pDecoder;
	CClearDecoder _cd(pBackend.get());

	int nSizeDivisor = 1;
#ifdef UNICODE
	switch (VInfo.CurMode.CodePage)
	{
	case 1200:
		pDecoder.reset(new CPassthroughDecoder());
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		break;
	case 1201:
		pDecoder.reset(new CReverseUnicodeToUnicodeDecoder());
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		break;
	case 65001:
		pDecoder.reset(new CUTF8ToUnicodeDecoder());
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		break;
	default:
		pDecoder.reset(new CSingleByteToUnicodeDecoder(VInfo.CurMode.CodePage));
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		break;
	}
#else
	if (VInfo.CurMode.Unicode)
	{
		nSizeDivisor = 2;
		if (nDetect == UNI_BE)
			pDecoder.reset(new CReverseUnicodeToOEMDecoder());
		else
			pDecoder.reset(new CUnicodeToOEMDecoder());
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
	}
	else
	{
		if (VInfo.CurMode.UseDecodeTable && (VInfo.CurMode.TableNum >= 0) && (VInfo.CurMode.TableNum < (int)XLatTables.size()))
		{
			const char *szDecodeTable = (const char *)XLatTables[VInfo.CurMode.TableNum].DecodeTable;
			const char *szEncodeTable = (const char *)XLatTables[VInfo.CurMode.TableNum].EncodeTable;

			pDecoder.reset(new CTableToOEMDecoder(szDecodeTable, szEncodeTable));
			if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		}
		else if (VInfo.CurMode.AnsiMode)
		{
			pDecoder.reset(new CSingleByteToOEMDecoder(CP_ACP));
			if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		}
		else
		{
			pDecoder.reset(new CPassthroughDecoder());
			if (!pBackend->SetDecoder(pDecoder, 0)) return false;
		}
	}
#endif

	tstring _FText = FText;
	bool _FCaseSensitive = FCaseSensitive;
	bool _FShowStatistics = FShowStatistics;
	SearchAs _FSearchAs = FSearchAs;
 
	FText           = EText;
	FCaseSensitive  = ECaseSensitive;
	FShowStatistics = false;

	FSearchAs = ERegExp ? (ESeveralLine ? SA_SEVERALLINE : SA_REGEXP) : SA_PLAINTEXT;
	if (!FPreparePattern(false)) return false;

	unique_ptr<IFrontend> pFrontend = NULL;

	if (ERegExp)
	{
		if (ESeveralLine)
			pFrontend.reset(new CSearchSeveralLineRegExpFrontend());
		else
			pFrontend.reset(new CSearchRegExpFrontend());
	}
	else
	{
		pFrontend.reset(new CSearchPlainTextFrontend());
	}

	bool bResult = pFrontend->Process(pBackend.get());

	FText = _FText;
	FCaseSensitive = _FCaseSensitive;
	FShowStatistics = _FShowStatistics;
	FSearchAs = _FSearchAs;

	tm.Stop();

	if (bResult)
	{
		SetViewerSelection((nSkip + pFrontend->GetOffset())/nSizeDivisor, (int)pFrontend->GetLength()/nSizeDivisor);
		Info.CurPos  = nSkip + pFrontend->GetOffset() + pFrontend->GetLength();
	}
	else if (!g_bInterrupted)
	{
		const TCHAR *Lines[] = {GetMsg(MRESearch), GetMsg(MCannotFind), EText.c_str(), GetMsg(MOk)};
		StartupInfo.Message(FMSG_WARNING,_T("VCannotFind"),Lines,4,1);
	}

	return true;
}

bool RefreshViewerInfo()
{
	VInfo.StructSize=sizeof(VInfo);
	if (StartupInfo.ViewerControl(VCTL_GETINFO, &VInfo) == 0) return false;

	return true;
}

bool ViewerSearch()
{
	RefreshViewerInfo();
	g_ViewerInfo.erase(VInfo.ViewerID);
#ifndef UNICODE
	EdInfo.FileName = VInfo.FileName;		//	For FillDefaultNamedParameters
#endif

	CFarDialog Dialog(76,13,_T("SearchDlg"));
	Dialog.SetWindowProc(EditorSearchDialogProc, 0);
	Dialog.SetUseID(true);

	Dialog.AddFrame(MRESearch);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"),SearchText));
	Dialog.Add(new CFarButtonItem(67,3,0,0,MQuoteSearch));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,_T("")));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(30,5,0,MSeveralLine,&ESeveralLine));
	Dialog.Add(new CFarButtonItem  (53,5,0,0,MEllipsis));
	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,7,DIF_DISABLE,MReverseSearch,&EReverse));
	Dialog.AddButtons(MOk,MCancel,MBtnClose);
	Dialog.Add(new CFarButtonItem(60,9,0,0,MBtnPresets));

	SearchText = EText;
	ESeveralLine = false;
	EReverse = false;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display()) {
		case MOk:
		case MBtnClose:
			break;
		case MQuoteSearch:
			if (ERegExp) CSO::QuoteRegExpString(SearchText);
			break;
		case MBtnPresets:
			VSPresets->ShowMenu(true);
			break;
		case MEllipsis:
			ConfigureSeveralLines();
			break;
		default:
			return false;
		}
	} while (!IsOKClose(ExitCode) || !EPreparePattern(SearchText));

	EText=SearchText;
	if ((ExitCode == MOk) && !EText.empty()) ViewerSearchAgain();

	return true;
}

bool CVSPresetCollection::EditPreset(CPreset *pPreset)
{
	CFarDialog Dialog(76,16,_T("VSPresetDlg"));
	Dialog.AddFrame(MVSPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,_T("RESearch.PresetName"),pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY|DIF_VAREDIT,_T("SearchText"), pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(30,7,0,MSeveralLine,&pPreset->m_mapInts["SeveralLine"]));
	Dialog.Add(new CFarCheckBoxItem(5,10,0,MAddToMenu,&pPreset->m_bAddToMenu));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(1, -2)) {
		case 0:
			return true;
		default:
			return false;
		}
	} while (true);
}

OperationResult ViewSearchExecutor()
{
	RefreshViewerInfo();
	g_ViewerInfo.erase(VInfo.ViewerID);
#ifndef UNICODE
	EdInfo.FileName = VInfo.FileName;		//	For FillDefaultNamedParameters
#endif

	if (!EPreparePattern(SearchText)) return OR_FAILED;
	EText = SearchText;

	return ViewerSearchAgain() ? OR_OK : OR_CANCEL;
}
