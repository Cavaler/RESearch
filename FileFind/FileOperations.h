#pragma once

#include "Backend.h"
#include "Decoders.h"
#include "Processors.h"
#include "Frontends.h"

#ifndef UNICODE
bool RunSearch(LPCTSTR szFileName, IFrontend *pFrontend);
#else
bool RunSearch(LPCTSTR szFileName, IFrontend *pFrontend, bool bUTF8);
#endif
