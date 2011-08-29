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

//////////////////////////////////////////////////////////////////////////

bool PrepareAndFind(IBackend *pBackend, const string &strText)
{
	TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring strTextUpcase = (FCaseSensitive) ? strText : UpCaseString(strText);
	PrepareBMHSearch(strTextUpcase.data(), strTextUpcase.length());

	const char *szBuffer = pBackend->Buffer();
	INT_PTR nSize  = pBackend->Size();

	return BMHSearch(szBuffer, nSize, strTextUpcase.data(), strTextUpcase.size(), szTable) >= 0;
}

#else

bool PrepareAndFind(IBackend *pBackend, const wstring &strText)
{
	TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring strTextUpcase = (FCaseSensitive) ? strText : UpCaseString(strText);
	PrepareBMHSearch(strTextUpcase.data(), strTextUpcase.length());

	const wchar_t *szBuffer = pBackend->BufferW();
	INT_PTR nSize  = pBackend->SizeW();

	return BMHSearch(szBuffer, nSize, strTextUpcase.data(), strTextUpcase.size(), szTable) >= 0;
}

#endif

bool CSearchMultiTextFrontend::Process(IBackend *pBackend)
{
	vector<tstring> arrMaybe;
	vector<tstring> arrPlus;
	vector<tstring> arrMinus;
	size_t nMaxSize = 0;

	tstring strWhat = FText;
	tstring strWord;
	do {
		GetStripWord(strWhat, strWord);
		if (strWord.empty()) break;

		if (strWord[0] == '+') arrPlus .push_back(strWord.substr(1)); else
		if (strWord[0] == '-') arrMinus.push_back(strWord.substr(1)); else arrMaybe.push_back(strWord);

		if (nMaxSize < strWord.size()) nMaxSize = strWord.size();
	} while (true);

	bool bAnyMaybesFound = arrMaybe.empty();

	do {
		for (size_t nMinus = 0; nMinus < arrMinus.size(); nMinus++) {
			if (PrepareAndFind(pBackend, arrMinus[nMinus])) return false;
		}

		for (size_t nPlus = 0; nPlus < arrPlus.size(); ) {
			if (PrepareAndFind(pBackend, arrPlus[nPlus])) {
				arrPlus.erase(arrPlus.begin()+nPlus);
			} else
				nPlus++;
		}

		if (!bAnyMaybesFound) {
			for (size_t nMaybe = 0; nMaybe < arrMaybe.size(); nMaybe++) {
				if (PrepareAndFind(pBackend, arrMaybe[nMaybe])) {
					bAnyMaybesFound = true;
					break;
				}
			}
		}

		if (arrMinus.empty() && arrPlus.empty() && bAnyMaybesFound) return true;

		if (pBackend->Last()) break;
		if (!pBackend->Move(pBackend->Size() - nMaxSize*sizeof(TCHAR))) break;

	} while (true);

	return arrPlus.empty() && bAnyMaybesFound;
}
