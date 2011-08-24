#pragma once

#include "Backend.h"
#include "Encoders.h"
#include "Processors.h"
#include "Frontends.h"
#include "shared_ptr.h"

bool RunSearch(LPCTSTR szFileName, IFrontend *pFrontend)
{
	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->Init(66)) return false;
	if (!pBackend->Open(szFileName)) return false;

	shared_ptr<IEncoder> pEncoder;

	eLikeUnicode nDetect = LikeUnicode(pBackend->Buffer(), pBackend->Size());
	switch (nDetect) {
	case UNI_LE:
		pEncoder = new CUnicodeToOEMEncoder();
		if (!pBackend->SetEncoder(pEncoder, 2)) return false;
		return pFrontend->Process(pBackend);
	case UNI_BE:
		pEncoder = new CReverseUnicodeToOEMEncoder();
		if (!pBackend->SetEncoder(pEncoder, 2)) return false;
		return pFrontend->Process(pBackend);
	case UNI_UTF8:
		pEncoder = new CUTF8ToOEMEncoder();
		if (!pBackend->SetEncoder(pEncoder, 3)) return false;
		return pFrontend->Process(pBackend);
	}

	pEncoder = new CPassthroughEncoder();
	if (!pBackend->SetEncoder(pEncoder)) return false;
	if (pFrontend->Process(pBackend)) return true;

	if (!FAllCharTables) return false;

	pEncoder = new CUnicodeToOEMEncoder();
	if (pBackend->SetEncoder(pEncoder) && pFrontend->Process(pBackend)) return true;

	pEncoder = new CUTF8ToOEMEncoder();
	if (pBackend->SetEncoder(pEncoder) && pFrontend->Process(pBackend)) return true;

	pEncoder = new CReverseUnicodeToOEMEncoder();
	if (pBackend->SetEncoder(pEncoder) && pFrontend->Process(pBackend)) return true;

	return false;
}
