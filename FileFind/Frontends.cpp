#include "StdAfx.h"
#include "Frontends.h"
#include "..\RESearch.h"

CSearchPlainTextFrontend::CSearchPlainTextFrontend(const tstring &strText)
: m_strText(strText)
{
}

bool CSearchPlainTextFrontend::Process(IBackend *pBackend)
{
	TCHAR *Table = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring TextUpcase = (FCaseSensitive) ? m_strText : UpCaseString(m_strText);
	PrepareBMHSearch(TextUpcase.data(), TextUpcase.length());

	do {
		char *szBuffer = pBackend->Buffer();
		INT_PTR nSize  = pBackend->Size();

		if (BMHSearch(szBuffer, nSize, TextUpcase.data(), TextUpcase.size(), Table) >= 0) return true;

		if (pBackend->Last()) break;

		if (!pBackend->Move(nSize-m_strText.size())) break;
	} while (true);

	return false;
}
