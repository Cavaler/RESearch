#pragma once
#include "IFileOperations.h"
#include "shared_ptr.h"

class CSingleBytePassThroughProcessor : public ISplitLineProcessor
{
public:
	CSingleBytePassThroughProcessor(IBackend *pBackend);

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();
	virtual INT_PTR		    Start();

	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);
	virtual void	SkipTo   (INT_PTR nOffset);

protected:
	IBackend *m_pBackend;
};

//	Handles all single-byte as well as UTF-8
class CSingleByteSplitLineProcessor : public ISplitLineProcessor
{
public:
	CSingleByteSplitLineProcessor(IBackend *pBackend);

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();
	virtual INT_PTR		    Start();

	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);
	virtual void	SkipTo   (INT_PTR nOffset);

protected:
	IBackend *m_pBackend;

	const char *m_szBuffer;
	const char *m_szEOL;
};

class CSingleByteSeveralLineProcessor : public ISplitLineProcessor
{
public:
	CSingleByteSeveralLineProcessor(IBackend *pBackend, int nLines, INT_PTR nMaxSize);
	~CSingleByteSeveralLineProcessor();

	virtual bool			GetNextLine();
	virtual const char *	Buffer();
	virtual INT_PTR			Size();
	virtual INT_PTR		    Start();

	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);
	virtual void	SkipTo   (INT_PTR nOffset);

protected:
	IBackend *m_pBackend;
	size_t  m_nLines;
	INT_PTR m_nMaxSize;

	shared_ptr<IDecoder> m_pOwnDecoder;

	const char *m_szBuffer;
	const char *m_szEOL;

	bool m_bAtEnd;
	vector<const char *> m_arrLines;
	INT_PTR m_nSkipOffset;
	INT_PTR m_nStartOffset;

	bool GetNextInLine();
	bool GetNextLastLine();
	bool BackendMove(INT_PTR nLength);

	bool Overflow();
};
