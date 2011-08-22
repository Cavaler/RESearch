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

class CPassthroughEncoder : public CEncoder
{
public:
	virtual bool	Decode(char *szBuffer, INT_PTR nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IEncoder *Clone();
	virtual bool	Encode(char *szBuffer, INT_PTR nLength);
};
