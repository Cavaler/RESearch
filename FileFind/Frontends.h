#pragma once
#include "IFileOperations.h"

class CSearchPlainTextFrontend : public IFrontend
{
public:
	CSearchPlainTextFrontend(const tstring &strText);

	virtual bool	Process(IBackend *pBackend);

protected:
	tstring m_strText;

};
