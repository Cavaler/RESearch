#include "StdAfx.h"
#include "Frontends.h"
#include "..\RESearch.h"

CSearchPlainTextFrontend::CSearchPlainTextFrontend(const tstring &strText)
: m_strText(strText)
{
}

#ifndef UNICODE

bool CSearchPlainTextFrontend::Process(IBackend *pBackend)
{
	TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring TextUpcase = (FCaseSensitive) ? m_strText : UpCaseString(m_strText);
	PrepareBMHSearch(TextUpcase.data(), TextUpcase.length());

	do {
		char *szBuffer = pBackend->Buffer();
		INT_PTR nSize  = pBackend->Size();

		if (BMHSearch(szBuffer, nSize, TextUpcase.data(), TextUpcase.size(), szTable) >= 0) return true;

		if (pBackend->Last()) break;

		if (!pBackend->Move(nSize-m_strText.size())) break;
	} while (true);

	return false;
}

#else

bool CSearchPlainTextFrontend::Process(IBackend *pBackend)
{
	WCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring TextUpcase = (FCaseSensitive) ? m_strText : UpCaseString(m_strText);
	PrepareBMHSearch(TextUpcase.data(), TextUpcase.length());

	do {
		char *szBuffer = pBackend->Buffer();
		INT_PTR nSize  = pBackend->Size();

		if (BMHSearch((WCHAR *)szBuffer, nSize/2, TextUpcase.data(), TextUpcase.size(), szTable) >= 0) return true;

		if (pBackend->Last()) break;

		if (!pBackend->Move(nSize-m_strText.size()*2)) break;
	} while (true);

	return false;
}

#endif
