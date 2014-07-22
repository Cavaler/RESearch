#include "StdAfx.h"
#include "..\RESearch.h"
#include "FileOperations.h"

class CClearDecoder
{
public:
	CClearDecoder(CFileBackend *pBackend) : m_pBackend(pBackend) {}
	~CClearDecoder() { if (m_pBackend) m_pBackend->ClearDecoder(); }
protected:
	CFileBackend *m_pBackend;
};

#ifndef UNICODE

bool RunSearchANSI(CFileBackend *pBackend, IFrontend *pFrontend)
{
	shared_ptr<IDecoder> pDecoder;
	CClearDecoder _cd(pBackend);

	int nSkip;
	eLikeUnicode nDetect = LikeUnicode(pBackend->Buffer(), pBackend->Size(), nSkip);
	switch (nDetect) {
	case UNI_LE:
		pDecoder = new CUnicodeToOEMDecoder();
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		return pFrontend->Process(pBackend);
	case UNI_BE:
		pDecoder = new CReverseUnicodeToOEMDecoder();
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		return pFrontend->Process(pBackend);
	case UNI_UTF8:
		pDecoder = new CUTF8ToOEMDecoder();
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		return pFrontend->Process(pBackend);
	}

	pDecoder = new CPassthroughDecoder();
	if (!pBackend->SetDecoder(pDecoder)) return false;
	if (pFrontend->Process(pBackend)) return true;
	if (Interrupted()) return false;

	if (!FAllCharTables) return false;

	for (size_t nTable = 0; nTable < XLatTables.size(); nTable++) {
		const char *szDecodeTable = (const char *)XLatTables[nTable].DecodeTable;
		const char *szEncodeTable = (const char *)XLatTables[nTable].EncodeTable;

		pDecoder = new CTableToOEMDecoder(szDecodeTable, szEncodeTable);
		if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	pDecoder = new CUnicodeToOEMDecoder();
	if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
	if (Interrupted()) return false;

	pDecoder = new CUTF8ToOEMDecoder();
	if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
	if (Interrupted()) return false;

	pDecoder = new CReverseUnicodeToOEMDecoder();
	if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
	if (Interrupted()) return false;

	return false;
}


bool RunSearch(LPCTSTR szFileName, IFrontend *pFrontend)
{
	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->SetBlockSize(FBufferSize*1024*1024)) return false;
	if (!pBackend->Open(szFileName, FAdvanced && FASearchHead ? FASearchHeadLimit : -1)) return false;

	return RunSearchANSI(pBackend, pFrontend);
}

bool RunReplace(LPCTSTR szInFileName, LPCTSTR szOutFileName, IFrontend *pFrontend)
{
	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->SetBlockSize(0)) return false;
	if (!pBackend->Open(szInFileName, szOutFileName)) return false;

	return RunSearchANSI(pBackend, pFrontend);
}

#else

bool RunSearchUnicode(CFileBackend *pBackend, IFrontend *pFrontend)
{
	shared_ptr<IDecoder> pDecoder;
	CClearDecoder _cd(pBackend);

	int nSkip;
	eLikeUnicode nDetect = LikeUnicode(pBackend->Buffer(), pBackend->Size(), nSkip);
	switch (nDetect) {
	case UNI_LE:
		pDecoder = new CPassthroughDecoder();
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		return pFrontend->Process(pBackend);
	case UNI_BE:
		pDecoder = new CReverseUnicodeToUnicodeDecoder();
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		return pFrontend->Process(pBackend);
	case UNI_UTF8:
		pDecoder = new CUTF8ToUnicodeDecoder();
		if (!pBackend->SetDecoder(pDecoder, nSkip)) return false;
		return pFrontend->Process(pBackend);
	}

	if (FCanUseDefCP) {
		pDecoder = new CSingleByteToUnicodeDecoder(GetDefCP());
		if (!pBackend->SetDecoder(pDecoder)) return false;
		if (pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	if (!FAllCharTables) return false;

	for (cp_set::iterator it = g_setAllCPs.begin(); it != g_setAllCPs.end(); it++) {
		UINT nCP = *it;
		if ((nCP == GetDefCP()) || (nCP == CP_UNICODE) || (nCP == CP_REVERSEBOM) || (nCP == CP_UTF8)) continue;

		pDecoder = new CSingleByteToUnicodeDecoder(nCP);
		if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	if (g_setAllCPs.find(CP_UNICODE) != g_setAllCPs.end()) {
		pDecoder = new CPassthroughDecoder();
		if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	if (g_setAllCPs.find(CP_UTF8) != g_setAllCPs.end()) {
		pDecoder = new CUTF8ToUnicodeDecoder();
		if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	if (g_setAllCPs.find(CP_REVERSEBOM) != g_setAllCPs.end()) {
		pDecoder = new CReverseUnicodeToUnicodeDecoder();
		if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	return false;
}

bool RunSearch(LPCTSTR szFileName, IFrontend *pFrontend)
{
	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->SetBlockSize(FBufferSize*1024*1024)) return false;
	if (!pBackend->Open(szFileName, FAdvanced && FASearchHead ? FASearchHeadLimit : -1)) return false;

	return RunSearchUnicode(pBackend, pFrontend);
}

bool DoFinalReplace(IBackend *pBackend)
{
	if (g_bInterrupted || !g_bFinalChecked) return false;

	IReplaceParametersInternalPtr spREInt = g_spREParam;
	if (spREInt == NULL) return false;

	g_bFinalReplace = true;
	TREParameters REBackup = REParam;
	REParam.Clear();
	REParam.AddFNumbers(FilesScanned, FileNumber, FindNumber, ReplaceNumber);
	tstring strReplace = CSO::CreateReplaceString(FRReplace.c_str(), _T("\n"), ScriptEngine(FREvaluate), REParam);
	g_bFinalReplace = false;

	if (!g_bSkipReplace && ! g_bInterrupted)
	{
		pBackend->AppendData((LPCSTR)strReplace.data(), strReplace.size()*sizeof(TCHAR));
		return true;
	}

	REParam = REBackup;
	REParam.RebuildSingleCharParam();

	return false;
}

bool RunReplace(LPCTSTR szInFileName, LPCTSTR szOutFileName, IFrontend *pFrontend)
{
	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->SetBlockSize(0)) return false;
	if (!pBackend->Open(szInFileName, szOutFileName)) return false;

	g_spREParam = NULL;

	bool bResult = RunSearchUnicode(pBackend, pFrontend);
	if (g_bInterrupted) return false;

	bResult |= DoFinalReplace(pBackend);

	return bResult;
}

#endif
