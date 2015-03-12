#pragma once

#include "Backend.h"
#include "Decoders.h"
#include "SingleByteProcessors.h"
#include "UnicodeProcessors.h"
#include "Frontends.h"

class CClearDecoder
{
public:
	CClearDecoder(CFileBackend *pBackend) : m_pBackend(pBackend) {}
	~CClearDecoder() { if (m_pBackend) m_pBackend->ClearDecoder(); }
protected:
	CFileBackend *m_pBackend;
};

bool RunSearch (LPCTSTR szFileName, IFrontend *pFrontend);
bool RunReplace(LPCTSTR szInFileName, LPCTSTR szOutFileName, IFrontend *pFrontend);

#ifdef UNICODE
bool RunSearchANSI   (CFileBackend *pBackend, IFrontend *pFrontend);
#else
bool RunSearchUTF8   (CFileBackend *pBackend, IFrontend *pFrontend);
bool RunSearchUnicode(CFileBackend *pBackend, IFrontend *pFrontend);
#endif
