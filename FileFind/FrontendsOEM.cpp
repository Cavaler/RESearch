#include "StdAfx.h"
#include "Frontends.h"
#include "Processors.h"
#include "..\RESearch.h"

#ifndef UNICODE

bool CSearchPlainTextFrontend::Process(IBackend *pBackend)
{
	TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring TextUpcase = (FCaseSensitive) ? FText : UpCaseString(FText);
	PrepareBMHSearch(TextUpcase.data(), TextUpcase.length());

	do {
		const char *szBuffer = pBackend->Buffer();
		INT_PTR nSize  = pBackend->Size();

		if (BMHSearch(szBuffer, nSize, TextUpcase.data(), TextUpcase.size(), szTable) >= 0) return true;

		if (pBackend->Last()) break;
		if (!pBackend->Move(nSize-FText.size())) break;

	} while (true);

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CSearchRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleByteSplitLineProcessor Proc(pBackend);

	do {
		int nResult = do_pcre_exec(FPattern, FPatternExtra, Proc.Buffer(), Proc.Size(), 0, 0, REParamA.Match(), REParamA.Count());
		if (nResult >= 0) return true;
		g_nFoundLine++;
	} while (Proc.GetNextLine());

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CSearchSeveralLineRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleByteSeveralLineProcessor Proc(pBackend, SeveralLines, SeveralLinesKB);

	do {
		string strSeveral(Proc.Buffer(), Proc.Size());
		strSeveral = ANSIFromUnicode(OEMToUnicode(strSeveral));

		int nResult = do_pcre_exec(FPattern, FPatternExtra, Proc.Buffer(), Proc.Size(), 0, 0, REParamA.Match(), REParamA.Count());
		if (nResult >= 0) return true;
		g_nFoundLine++;
	} while (Proc.GetNextLine());

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CSearchMultiLineRegExpFrontend::Process(IBackend *pBackend)
{
	do {
		const char *szBuffer = pBackend->Buffer();
		INT_PTR nSize  = pBackend->Size();

		int nResult = do_pcre_exec(FPattern, FPatternExtra, szBuffer, nSize, 0, 0, REParamA.Match(), REParamA.Count());
		if (nResult >= 0) return true;

		if (pBackend->Last()) break;
		if (!pBackend->Move(nSize > 1024 ? nSize - 1024 : nSize)) break;

	} while (true);

	return false;
}

#endif
