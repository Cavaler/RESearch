#pragma once
#include "IFileOperations.h"

//	Handles all single-byte as well as UTF-8
class CSingleByteSplitLineProcessor : ISplitLineProcessor
{
public:
	CSingleByteSplitLineProcessor(IBackend *pBackend);

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();

protected:
	IBackend *m_pBackend;

	const char *m_szBuffer;
	const char *m_szEOL;
};
