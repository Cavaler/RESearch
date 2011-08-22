#pragma once

#include "Backend.h"
#include "Encoders.h"
#include "Processors.h"
#include "Frontends.h"
#include "shared_ptr.h"

bool RunSearch(LPCTSTR szFileName, IFrontend *pFrontend)
{
	shared_ptr<IEncoder> pEncoder = new CPassthroughEncoder();

	shared_ptr<CFileBackend> pBackend = new CFileBackend();
	if (!pBackend->Init(1024, pEncoder)) return false;

	if (!pBackend->Open(szFileName)) return false;

	return pFrontend->Process(pBackend);
}
