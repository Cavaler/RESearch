#include "StdAfx.h"
#include "Frontends.h"
#include "Processors.h"
#include "..\RESearch.h"

#ifdef UNICODE

bool CSearchPlainTextFrontend::Process(IBackend *pBackend)
{
	WCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring TextUpcase = (FCaseSensitive) ? FText : UpCaseString(FText);
	PrepareBMHSearch(TextUpcase.data(), TextUpcase.length());

	do {
		const wchar_t *szBuffer = pBackend->BufferW();
		INT_PTR nSize = pBackend->SizeW();

		if (BMHSearch(szBuffer, nSize, TextUpcase.data(), TextUpcase.size(), szTable) >= 0) return true;

		if (pBackend->Last()) break;
		if (!pBackend->Move((nSize-FText.size())*2)) break;

	} while (!Interrupted());

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CSearchRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleByteSplitLineProcessor Proc(pBackend);

	do {
		int nResult = do_pcre_execA(FPattern, FPatternExtra, Proc.Buffer(), Proc.Size(), 0, 0, REParamA.Match(), REParamA.Count());
		if (nResult >= 0) return true;
		g_nFoundLine++;
	} while (!Interrupted() && Proc.GetNextLine());

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CSearchSeveralLineRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleByteSplitLineProcessor Proc(pBackend);

	do {
		int nResult = do_pcre_execA(FPattern, FPatternExtra, Proc.Buffer(), Proc.Size(), 0, 0, REParamA.Match(), REParamA.Count());
		if (nResult >= 0) return true;
		g_nFoundLine++;
	} while (!Interrupted() && Proc.GetNextLine());

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CSearchMultiLineRegExpFrontend::Process(IBackend *pBackend)
{
	do {
		const char *szBuffer = pBackend->Buffer();
		INT_PTR nSize  = pBackend->Size();

		int nResult = do_pcre_execA(FPattern, FPatternExtra, szBuffer, nSize, 0, 0, REParamA.Match(), REParamA.Count());
		if (nResult >= 0) return true;

		if (pBackend->Last()) break;
		if (!pBackend->Move(nSize > 1024 ? nSize - 1024 : nSize)) break;

	} while (!Interrupted());

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool PrepareAndFind(IBackend *pBackend, const wstring &strText)
{
	TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring strTextUpcase = (FCaseSensitive) ? strText : UpCaseString(strText);
	PrepareBMHSearch(strTextUpcase.data(), strTextUpcase.length());

	const wchar_t *szBuffer = pBackend->BufferW();
	INT_PTR nSize  = pBackend->SizeW();

	return BMHSearch(szBuffer, nSize, strTextUpcase.data(), strTextUpcase.size(), szTable) >= 0;
}

//////////////////////////////////////////////////////////////////////////

bool CReplacePlainTextFrontend::Process(IBackend *pBackend)
{
	TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring TextUpcase = (FCaseSensitive) ? FText : UpCaseString(FText);
	PrepareBMHSearch(TextUpcase.data(), TextUpcase.length());

	REParam.Clear();

	do {
		const wchar_t *szBuffer = pBackend->BufferW();
		INT_PTR nSize = pBackend->SizeW();

		int nOffset;
		while ((nOffset = BMHSearch(szBuffer, nSize, TextUpcase.data(), TextUpcase.size(), szTable)) >= 0)
		{
			if (!pBackend->WriteBack((szBuffer - pBackend->BufferW() + nOffset)*2)) break;

			REParam.AddSource(szBuffer+nOffset, TextUpcase.size());
			REParam.AddFNumbers(FileNumber, FindNumber, ReplaceNumber);
			wstring strReplace = CSO::CreateReplaceString(FRReplace.c_str(), L"\n", -1, REParam);

			if (!pBackend->WriteThru((LPCSTR)strReplace.data(), strReplace.size()*2, TextUpcase.size()*2)) break;

			szBuffer += nOffset + TextUpcase.size();
			nSize    -= nOffset + TextUpcase.size();
			FindNumber++;
			ReplaceNumber++;
		}

		if (pBackend->Last()) break;
		if (!pBackend->Move(pBackend->Size()-FText.size()*2)) break;

	} while (!Interrupted());

	return FindNumber > 0;
}

//////////////////////////////////////////////////////////////////////////

bool ReplaceRegExpProcess(ISplitLineProcessor &Proc)
{
	REParamA.Clear();
	REParamA.AddRE(FPattern);

	do {
		const char *szBuffer = Proc.Buffer();
		INT_PTR nSize = Proc.Size();

		int nResult;
		while ((nResult = do_pcre_execA(FPattern, FPatternExtra, szBuffer, nSize, 0, 0, REParamA.Match(), REParamA.Count())) >= 0)
		{
			int nOffset, nLength;
			REParamA.FillStartLength(&nOffset, &nLength);

			if (!Proc.WriteBack(szBuffer - Proc.Buffer() + nOffset)) break;

			REParamA.AddSource(szBuffer, nOffset+nLength);
			REParamA.AddFNumbers(FileNumber, FindNumber, ReplaceNumber);
			string strReplace = CSOA::CreateReplaceString(UTF8FromUnicode(FRReplace).c_str(), "\n", -1, REParamA);

			if (!Proc.WriteThru(strReplace.data(), strReplace.size(), nLength)) break;

			szBuffer += nOffset + nLength;
			nSize    -= nOffset + nLength;
			FindNumber++;
			ReplaceNumber++;
		}

	} while (!Interrupted() && Proc.GetNextLine());

	return FindNumber > 0;
}

bool CReplaceRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleByteSplitLineProcessor Proc(pBackend);

	return ReplaceRegExpProcess(Proc);
}

//////////////////////////////////////////////////////////////////////////

bool CReplaceSeveralLineRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleByteSeveralLineProcessor Proc(pBackend, SeveralLines, SeveralLinesKB);

	return ReplaceRegExpProcess(Proc);
}

#endif
