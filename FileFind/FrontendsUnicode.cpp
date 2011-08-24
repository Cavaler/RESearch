#include "StdAfx.h"
#include "Frontends.h"
#include "..\RESearch.h"

#ifdef UNICODE

bool CSearchPlainTextFrontend::Process(IBackend *pBackend)
{
	WCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring TextUpcase = (FCaseSensitive) ? FText : UpCaseString(FText);
	PrepareBMHSearch(TextUpcase.data(), TextUpcase.length());

	do {
		const char *szBuffer = pBackend->Buffer();
		INT_PTR nSize  = pBackend->Size();

		if (BMHSearch((WCHAR *)szBuffer, nSize/2, TextUpcase.data(), TextUpcase.size(), szTable) >= 0) return true;

		if (pBackend->Last()) break;

		if (!pBackend->Move(nSize-FText.size()*2)) break;
	} while (true);

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CSearchRegExpFrontend::Process(IBackend *pBackend)
{
	return false;
}

#endif
