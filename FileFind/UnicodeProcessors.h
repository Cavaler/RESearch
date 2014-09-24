#pragma once
#include "IFileOperations.h"
#include "shared_ptr.h"

class CUnicodeSplitLineProcessor : public ISplitLineProcessor
{
public:
	CUnicodeSplitLineProcessor(IBackend *pBackend);

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();
	virtual INT_PTR		    Start();

	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);
	virtual void	SkipTo   (INT_PTR nOffset);

protected:
	IBackend *m_pBackend;

	const wchar_t *m_szBuffer;
	const wchar_t *m_szEOL;
};

class CUnicodeSeveralLineProcessor : public ISplitLineProcessor
{
public:
	CUnicodeSeveralLineProcessor(IBackend *pBackend, int nLines, INT_PTR nMaxSize);
	~CUnicodeSeveralLineProcessor();

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();
	virtual INT_PTR		    Start();

	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);
	virtual void	SkipTo   (INT_PTR nOffset);

	const wchar_t *	BufferW();
	INT_PTR			SizeW();

protected:
	IBackend *m_pBackend;
	size_t  m_nLines;
	INT_PTR m_nMaxSize;

	shared_ptr<IDecoder> m_pOwnDecoder;

	const wchar_t *m_szBuffer;
	const wchar_t *m_szEOL;

	bool m_bAtEnd;
	vector<const wchar_t *> m_arrLines;
	INT_PTR m_nSkipOffset;
	INT_PTR m_nStartOffset;

	bool GetNextInLine();
	bool GetNextLastLine();
	bool BackendMove(INT_PTR nLength);

	bool Overflow();
};
