#include "StdAfx.h"
#include "..\RESearch.h"
#include "FileOperations.h"

#ifndef UNICODE

bool RunSearchANSI(CFileBackend *pBackend, IFrontend *pFrontend)
{
	shared_ptr<IDecoder> pDecoder;

	eLikeUnicode nDetect = LikeUnicode(pBackend->Buffer(), pBackend->Size());
	switch (nDetect) {
	case UNI_LE:
		pDecoder = new CUnicodeToOEMDecoder();
		if (!pBackend->SetDecoder(pDecoder, 2)) return false;
		return pFrontend->Process(pBackend);
	case UNI_BE:
		pDecoder = new CReverseUnicodeToOEMDecoder();
		if (!pBackend->SetDecoder(pDecoder, 2)) return false;
		return pFrontend->Process(pBackend);
	case UNI_UTF8:
		pDecoder = new CUTF8ToOEMDecoder();
		if (!pBackend->SetDecoder(pDecoder, 3)) return false;
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


bool RunSearch(LPCTSTR szFileName, IFrontend *pFrontend, bool /*bUTF8*/)
{
	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->Init(FBufferSize*1024*1024)) return false;
	if (!pBackend->Open(szFileName, FASearchHead ? FASearchHeadLimit : -1)) return false;

	return RunSearchANSI(pBackend, pFrontend);
}

bool RunReplace(LPCTSTR szInFileName, LPCTSTR szOutFileName, IFrontend *pFrontend, bool /*bUTF8*/)
{
	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->Init(FBufferSize*1024*1024)) return false;
	if (!pBackend->Open(szInFileName, szOutFileName)) return false;

	return RunSearchANSI(pBackend, pFrontend);
}

#else

bool RunSearchUTF8(CFileBackend *pBackend, IFrontend *pFrontend)
{
	shared_ptr<IDecoder> pDecoder;

	eLikeUnicode nDetect = LikeUnicode(pBackend->Buffer(), pBackend->Size());
	switch (nDetect) {
	case UNI_LE:
		pDecoder = new CUnicodeToUTF8Decoder();
		if (!pBackend->SetDecoder(pDecoder, 2)) return false;
		return pFrontend->Process(pBackend);
	case UNI_BE:
		pDecoder = new CReverseUnicodeToUTF8Decoder();
		if (!pBackend->SetDecoder(pDecoder, 2)) return false;
		return pFrontend->Process(pBackend);
	case UNI_UTF8:
		pDecoder = new CPassthroughDecoder();
		if (!pBackend->SetDecoder(pDecoder, 3)) return false;
		return pFrontend->Process(pBackend);
	}

	if (FCanUseDefCP) {
		pDecoder = new CSingleByteToUTF8Decoder(GetDefCP());
		if (!pBackend->SetDecoder(pDecoder)) return false;
		if (pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	if (!FAllCharTables) return false;

	for (cp_set::iterator it = g_setAllCPs.begin(); it != g_setAllCPs.end(); it++) {
		UINT nCP = *it;
		if ((nCP == GetDefCP()) || (nCP == CP_UNICODE) || (nCP == CP_REVERSEBOM) || (nCP == CP_UTF8)) continue;

		pDecoder = new CSingleByteToUTF8Decoder(nCP);
		if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	if (g_setAllCPs.find(CP_UNICODE) != g_setAllCPs.end()) {
		pDecoder = new CUnicodeToUTF8Decoder();
		if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	if (g_setAllCPs.find(CP_UTF8) != g_setAllCPs.end()) {
		pDecoder = new CPassthroughDecoder();
		if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	if (g_setAllCPs.find(CP_REVERSEBOM) != g_setAllCPs.end()) {
		pDecoder = new CReverseUnicodeToUTF8Decoder();
		if (pBackend->SetDecoder(pDecoder) && pFrontend->Process(pBackend)) return true;
		if (Interrupted()) return false;
	}

	return false;
}

bool RunSearchUnicode(CFileBackend *pBackend, IFrontend *pFrontend)
{
	shared_ptr<IDecoder> pDecoder;

	eLikeUnicode nDetect = LikeUnicode(pBackend->Buffer(), pBackend->Size());
	switch (nDetect) {
	case UNI_LE:
		pDecoder = new CPassthroughDecoder();
		if (!pBackend->SetDecoder(pDecoder, 2)) return false;
		return pFrontend->Process(pBackend);
	case UNI_BE:
		pDecoder = new CReverseUnicodeToUnicodeDecoder();
		if (!pBackend->SetDecoder(pDecoder, 2)) return false;
		return pFrontend->Process(pBackend);
	case UNI_UTF8:
		pDecoder = new CUTF8ToUnicodeDecoder();
		if (!pBackend->SetDecoder(pDecoder, 3)) return false;
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

bool RunSearch(LPCTSTR szFileName, IFrontend *pFrontend, bool bUTF8)
{
	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->Init(FBufferSize*1024*1024)) return false;
	if (!pBackend->Open(szFileName, FASearchHead ? FASearchHeadLimit : -1)) return false;

	return (bUTF8) ? RunSearchUTF8(pBackend, pFrontend) : RunSearchUnicode(pBackend, pFrontend);
}


bool RunReplace(LPCTSTR szInFileName, LPCTSTR szOutFileName, IFrontend *pFrontend, bool bUTF8)
{
	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->Init(FBufferSize*1024*1024)) return false;
	if (!pBackend->Open(szInFileName, szOutFileName)) return false;

	return (bUTF8) ? RunSearchUTF8(pBackend, pFrontend) : RunSearchUnicode(pBackend, pFrontend);
}

#endif
