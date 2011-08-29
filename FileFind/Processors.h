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

class CSingleByteSeveralLineProcessor : ISplitLineProcessor
{
public:
	CSingleByteSeveralLineProcessor(IBackend *pBackend, int nLines, INT_PTR nMaxSize);

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();

protected:
	IBackend *m_pBackend;
	int m_nLines;
	INT_PTR m_nMaxSize;

	const char *m_szBuffer;
	const char *m_szEOL;

	bool m_bAtEnd;
	vector<const char *> m_arrLines;

	bool GetNextLastLine();
	bool BackendMove(INT_PTR nLength);
};

//	Needed only for Unicode PLain-text Grep
class CUnicodeSplitLineProcessor : ISplitLineProcessor
{
public:
	CUnicodeSplitLineProcessor(IBackend *pBackend);

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();

protected:
	IBackend *m_pBackend;

	const wchar_t *m_szBuffer;
	const wchar_t *m_szEOL;
};
