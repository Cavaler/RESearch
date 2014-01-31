#pragma once
#include "IFileOperations.h"

class CDecoder : public IDecoder
{
public:
	CDecoder();
	~CDecoder();

public:
	virtual const char *Buffer();
	virtual INT_PTR		Size();

protected:
	char	   *m_szBuffer;
	INT_PTR		m_nAllocSize;
	INT_PTR		m_nSize;

	void		Clear();
	bool		AllocBuffer(INT_PTR nSize);
};

class CSameWidthDecoder : public CDecoder
{
public:
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
};

class CPassthroughDecoder : public CSameWidthDecoder
{
public:
	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual IDecoder *GetEncoder();
};

class CSingleByteCRLFDecoder : public CDecoder
{
public:
	CSingleByteCRLFDecoder(IDecoder *pBackDecoder);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetEncoder();

protected:
	IDecoder *m_pBackDecoder;

	typedef pair<INT_PTR, int> skip_element;
	typedef vector<skip_element> skip_map;
	skip_map m_mapSkipped;

	INT_PTR		m_nDOIn;
	size_t		m_nDOSkip;
	INT_PTR		m_nDOOut;

	INT_PTR		m_nOOIn;
	size_t		m_nOOSkip;
	INT_PTR		m_nOOOut;
};

class CUnicodeCRLFDecoder : public CDecoder
{
public:
	CUnicodeCRLFDecoder(IDecoder *pBackDecoder);

	virtual INT_PTR	Size();
	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetEncoder();

protected:
	IDecoder *m_pBackDecoder;

	typedef pair<INT_PTR, int> skip_element;
	typedef vector<skip_element> skip_map;
	skip_map m_mapSkipped;

	INT_PTR		m_nDOIn;
	size_t		m_nDOSkip;
	INT_PTR		m_nDOOut;

	INT_PTR		m_nOOIn;
	size_t		m_nOOSkip;
	INT_PTR		m_nOOOut;
};

class CUTF8Traverse {
public:
	CUTF8Traverse();

	void SetString(const char *szBuffer, INT_PTR nLength);

	INT_PTR	ByteToChar(INT_PTR nOffset);
	INT_PTR	CharToByte(INT_PTR nOffset);

protected:
	std::vector<INT_PTR> utf2char;
	std::vector<INT_PTR> char2utf;

	INT_PTR nByte;
	INT_PTR nChar;
};

void CutToValidUTF8(const char *szBuffer, INT_PTR &nLength);

#include "OEMDecoders.h"
#include "UTF8Decoders.h"
#include "UnicodeDecoders.h"
