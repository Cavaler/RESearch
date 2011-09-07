#pragma once
#include "IFileOperations.h"
#include "shared_ptr.h"

//	Handles all single-byte as well as UTF-8
class CSingleByteSplitLineProcessor : public ISplitLineProcessor
{
public:
	CSingleByteSplitLineProcessor(IBackend *pBackend);

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();

	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);

protected:
	IBackend *m_pBackend;

	const char *m_szBuffer;
	const char *m_szEOL;
};

class CSingleByteSeveralLineProcessor : public ISplitLineProcessor
{
public:
	CSingleByteSeveralLineProcessor(IBackend *pBackend, int nLines, INT_PTR nMaxSize);

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();

	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);

protected:
	IBackend *m_pBackend;
	int m_nLines;
	INT_PTR m_nMaxSize;

	shared_ptr<IDecoder> m_pOwnDecoder;

	const char *m_szBuffer;
	const char *m_szEOL;

	bool m_bAtEnd;
	vector<const char *> m_arrLines;

	bool GetNextLastLine();
	bool BackendMove(INT_PTR nLength);
};

//	Needed only for Unicode Plain-text Grep
class CUnicodeSplitLineProcessor : public ISplitLineProcessor
{
public:
	CUnicodeSplitLineProcessor(IBackend *pBackend);

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();

	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);

protected:
	IBackend *m_pBackend;

	const wchar_t *m_szBuffer;
	const wchar_t *m_szEOL;
};
