#pragma once
#include "IFileOperations.h"

class CDecoder : public IDecoder
{
public:
	CDecoder();
	~CDecoder();

public:
	virtual char *	Buffer();
	virtual INT_PTR	Size();

protected:
	char	   *m_szBuffer;
	INT_PTR		m_nSize;
	void		Clear();
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
	virtual bool	Encode(const char *szBuffer, INT_PTR &nLength);
	virtual IDecoder *GetDecoder();
};

class CUTF8Traverse {
public:
	void SetString(const char *szBuffer, INT_PTR nLength);

	INT_PTR	ByteToChar(INT_PTR nOffset);
	INT_PTR	CharToByte(INT_PTR nOffset);

protected:
	std::map<INT_PTR, INT_PTR> utf2char;
	std::map<INT_PTR, INT_PTR> char2utf;

	INT_PTR nByte;
	INT_PTR nChar;
};

void CutToValidUTF8(const char *szBuffer, INT_PTR &nLength);

#include "OEMDecoders.h"
#include "UTF8Decoders.h"
#include "UnicodeDecoders.h"
