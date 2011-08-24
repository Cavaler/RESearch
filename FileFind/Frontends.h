#pragma once
#include "IFileOperations.h"

class CSearchPlainTextFrontend : public IFrontend
{
public:
	virtual bool	Process(IBackend *pBackend);
};

class CSearchRegExpFrontend : public IFrontend
{
public:
	virtual bool	Process(IBackend *pBackend);
};
