#include "StdAfx.h"
#include "Frontends.h"
#include "SingleByteProcessors.h"
#include "UnicodeProcessors.h"
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

		while (nSize > 0)
		{
			int nOffset = BMHSearch(szBuffer, nSize, TextUpcase.data(), TextUpcase.size(), szTable);
			if (nOffset >= 0)
			{
				FindNumber++;
				if (!FShowStatistics) return true;

				szBuffer += nOffset + TextUpcase.size();
				nSize    -= nOffset + TextUpcase.size();
			}
			else
				break;
		}

		if (pBackend->Last()) break;
		if (!pBackend->Move((nSize-FText.size())*2)) break;

	} while (!Interrupted());

	return FindNumber > 0;
}

//////////////////////////////////////////////////////////////////////////

bool SearchRegExpProcess(IBackend *pBackend, ISplitLineProcessor &Proc)
{
	do {
		const wchar_t *szBuffer = Proc.BufferW();
		INT_PTR nSize   = Proc.SizeW();
		INT_PTR nOffset = Proc.StartW();

		while (nOffset < nSize)
		{
			int nResult = do_pcre16_exec(FPattern16, FPatternExtra16, szBuffer, nSize, nOffset, PCRE_NO_UTF8_CHECK, REParam.Match(), REParam.Count());
			if (nResult >= 0)
			{
				if (FindNumber == 0) g_nFoundColumn = REParam.m_arrMatch[0]+1;
				FindNumber++;
				if (!FShowStatistics) return true;

				nOffset = REParam.m_arrMatch[1];
				Proc.SkipToW(nOffset);
			}
			else
				break;
		}

		if (FindNumber == 0) g_nFoundLine++;

	} while (!Interrupted() && Proc.GetNextLine());

	return FindNumber > 0;
}

//////////////////////////////////////////////////////////////////////////

bool CSearchRegExpFrontend::Process(IBackend *pBackend)
{
	CUnicodeSplitLineProcessor Proc(pBackend);

	return SearchRegExpProcess(pBackend, Proc);
}

//////////////////////////////////////////////////////////////////////////

bool CSearchSeveralLineRegExpFrontend::Process(IBackend *pBackend)
{
	CUnicodeSeveralLineProcessor Proc(pBackend, SeveralLines, SeveralLinesKB);

	return SearchRegExpProcess(pBackend, Proc);
}

//////////////////////////////////////////////////////////////////////////

bool CSearchMultiLineRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleBytePassThroughProcessor Proc(pBackend);

	return SearchRegExpProcess(pBackend, Proc);
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
			if (!pBackend->CheckWriteReady()) return false;

			REParam.AddSource(szBuffer+nOffset, TextUpcase.size());
			REParam.AddFNumbers(FilesScanned, FileNumber, FindNumber, ReplaceNumber);
			wstring strReplace = CSO::CreateReplaceString(FRReplace.c_str(), L"\n", ScriptEngine(FREvaluate), REParam);

			FindNumber++;

			if (ConfirmReplacement() || ConfirmReplacement(REParam.Original().c_str(), strReplace.c_str(), pBackend->FileName())) {
				if (!pBackend->WriteBack((szBuffer - pBackend->BufferW() + nOffset)*2)) break;
				if (!pBackend->WriteThru((LPCSTR)strReplace.data(), strReplace.size()*2, TextUpcase.size()*2)) break;

				ReplaceNumber++;
			} else {
				if (Interrupted()) break;
			}

			szBuffer += nOffset + TextUpcase.size();
			nSize    -= nOffset + TextUpcase.size();
		}

		if (pBackend->Last()) break;
		if (!pBackend->Move(pBackend->Size()-FText.size()*2)) break;

	} while (!Interrupted());

	return !g_bInterrupted && (ReplaceNumber > 0);
}

//////////////////////////////////////////////////////////////////////////

bool ReplaceRegExpProcess(IBackend *pBackend, ISplitLineProcessor &Proc)
{
	REParam.Clear();
	REParam.AddRE(FPattern);

	do {
		const wchar_t *szBuffer = Proc.BufferW();
		INT_PTR nSize  = Proc.SizeW();
		INT_PTR nStart = Proc.StartW();

		int nResult;
		while ((nResult = do_pcre16_exec(FPattern16, FPatternExtra16, szBuffer, nSize, nStart, PCRE_NO_UTF8_CHECK, REParam.Match(), REParam.Count())) >= 0)
		{
			if (!pBackend->CheckWriteReady()) return false;

			int nOffset, nLength;
			REParam.FillStartLength(&nOffset, &nLength);

			REParam.AddSource(szBuffer, nOffset+nLength);
			REParam.AddFNumbers(FilesScanned, FileNumber, FindNumber, ReplaceNumber);
			wstring strReplace = CSO::CreateReplaceString(FRReplace.c_str(), L"\n", ScriptEngine(FREvaluate), REParam);

			FindNumber++;

			if (ConfirmReplacement() || ConfirmReplacement(REParam.GetParam(0).c_str(), strReplace.c_str(), pBackend->FileName())) {
				if (!Proc.WriteBack(nOffset*2)) break;
				if (!Proc.WriteThru((const char *)strReplace.data(), strReplace.size()*2, nLength*2)) break;

				ReplaceNumber++;
			} else {
				if (Interrupted()) break;
				Proc.SkipToW(nOffset + nLength);
			}

			nStart = nOffset + (nLength ? nLength : 1);
		}

	} while (!Interrupted() && Proc.GetNextLine());

	return !g_bInterrupted && (ReplaceNumber > 0);
}

//////////////////////////////////////////////////////////////////////////

bool CReplaceRegExpFrontend::Process(IBackend *pBackend)
{
	CUnicodeSplitLineProcessor Proc(pBackend);

	return ReplaceRegExpProcess(pBackend, Proc);
}

bool CReplaceSeveralLineRegExpFrontend::Process(IBackend *pBackend)
{
	CUnicodeSeveralLineProcessor Proc(pBackend, SeveralLines, SeveralLinesKB);

	return ReplaceRegExpProcess(pBackend, Proc);
}

bool CReplaceMultiLineRegExpFrontend::Process(IBackend *pBackend)
{
	CSingleBytePassThroughProcessor Proc(pBackend);

	return ReplaceRegExpProcess(pBackend, Proc);
}

#endif
