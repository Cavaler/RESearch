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

	} while (!Interrupted());

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
	} while (!Interrupted() && Proc.GetNextLine());

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CSearchSeveralLineRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleByteSeveralLineProcessor Proc(pBackend, SeveralLines, SeveralLinesKB);

	do {
		int nResult = do_pcre_exec(FPattern, FPatternExtra, Proc.Buffer(), Proc.Size(), 0, 0, REParamA.Match(), REParamA.Count());
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

		int nResult = do_pcre_exec(FPattern, FPatternExtra, szBuffer, nSize, 0, 0, REParamA.Match(), REParamA.Count());
		if (nResult >= 0) return true;

		if (pBackend->Last()) break;
		if (!pBackend->Move(nSize > 1024 ? nSize - 1024 : nSize)) break;

	} while (!Interrupted());

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

//////////////////////////////////////////////////////////////////////////

bool CReplacePlainTextFrontend::Process(IBackend *pBackend)
{
	TCHAR *szTable = (FCaseSensitive) ? NULL : UpCaseTable;

	tstring TextUpcase = (FCaseSensitive) ? FText : UpCaseString(FText);
	PrepareBMHSearch(TextUpcase.data(), TextUpcase.length());

	REParam.Clear();

	do {
		const char *szBuffer = pBackend->Buffer();
		INT_PTR nSize = pBackend->Size();

		int nOffset;
		while ((nOffset = BMHSearch(szBuffer, nSize, TextUpcase.data(), TextUpcase.size(), szTable)) >= 0)
		{
			if (!pBackend->CheckWriteReady()) return false;

			REParam.AddSource(szBuffer+nOffset, TextUpcase.size());
			REParam.AddFNumbers(FileNumber, FindNumber, ReplaceNumber);
			string strReplace = CSO::CreateReplaceString(FRReplace.c_str(), "\n", -1, REParam);

			FindNumber++;

			if (ConfirmReplacement(REParam.Original().c_str(), strReplace.c_str(), pBackend->FileName())) {
				if (!pBackend->WriteBack(szBuffer - pBackend->Buffer() + nOffset)) break;
				if (!pBackend->WriteThru(strReplace.data(), strReplace.size(), TextUpcase.size())) break;

				ReplaceNumber++;
			} else {
				if (Interrupted()) break;
			}

			szBuffer += nOffset + TextUpcase.size();
			nSize    -= nOffset + TextUpcase.size();
		}

		if (pBackend->Last()) break;
		if (!pBackend->Move(pBackend->Size()-FText.size())) break;

	} while (!Interrupted());

	return ReplaceNumber > 0;
}

//////////////////////////////////////////////////////////////////////////

bool ReplaceRegExpProcess(IBackend *pBackend, ISplitLineProcessor &Proc)
{
	REParam.Clear();
	REParam.AddRE(FPattern);

	do {
		const char *szBuffer = Proc.Buffer();
		INT_PTR nSize  = Proc.Size();
		INT_PTR nStart = Proc.Start();

		int nResult;
		while ((nResult = do_pcre_exec(FPattern, FPatternExtra, szBuffer, nSize, nStart, 0, REParam.Match(), REParam.Count())) >= 0)
		{
			if (!pBackend->CheckWriteReady()) return false;

			int nOffset, nLength;
			REParam.FillStartLength(&nOffset, &nLength);

			REParam.AddSource(szBuffer, nOffset+nLength);
			REParam.AddFNumbers(FileNumber, FindNumber, ReplaceNumber);
			string strReplace = CSO::CreateReplaceString(FRReplace.c_str(), "\n", -1, REParam);

			FindNumber++;

			if (ConfirmReplacement(REParam.GetParam(0).c_str(), strReplace.c_str(), pBackend->FileName())) {
				if (!Proc.WriteBack(szBuffer - Proc.Buffer() + nOffset)) break;
				if (!Proc.WriteThru(strReplace.data(), strReplace.size(), nLength)) break;

				ReplaceNumber++;
			} else {
				if (Interrupted()) break;
				Proc.SkipTo(szBuffer - Proc.Buffer() + nOffset + nLength);
			}

//			szBuffer += nOffset + nLength;
//			nSize    -= nOffset + nLength;
			nStart    = nOffset + nLength;
		}

	} while (!Interrupted() && Proc.GetNextLine());

	return ReplaceNumber > 0;
}

bool CReplaceRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleByteSplitLineProcessor Proc(pBackend);

	return ReplaceRegExpProcess(pBackend, Proc);
}

//////////////////////////////////////////////////////////////////////////

bool CReplaceSeveralLineRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleByteSeveralLineProcessor Proc(pBackend, SeveralLines, SeveralLinesKB);

	return ReplaceRegExpProcess(pBackend, Proc);
}

#endif
