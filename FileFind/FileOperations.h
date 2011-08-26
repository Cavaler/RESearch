#pragma once

#include "Backend.h"
#include "Decoders.h"
#include "Processors.h"
#include "Frontends.h"

bool RunSearch(LPCTSTR szFileName, IFrontend *pFrontend, bool bUTF8);
