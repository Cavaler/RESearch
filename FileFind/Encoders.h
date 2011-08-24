#pragma once
#include "IFileOperations.h"

class CEncoder : public IEncoder
{
public:
	CEncoder();
	~CEncoder();

public:
	virtual char *	Buffer();
	virtual INT_PTR	Size();

protected:
	char	   *m_szBuffer;
	INT_PTR		m_nSize;
	void		Clear();
};

class CSameWidthEncoder : public CEncoder
{
public:
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
};

class CPassthroughEncoder : public CSameWidthEncoder
{
public:
	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual bool	Encode(const char *szBuffer, INT_PTR &nLength);
	virtual IEncoder *Clone();
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

#include "OEMEncoders.h"
