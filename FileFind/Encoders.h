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

class CSingleByteToOEMEncoder : public CSameWidthEncoder
{
public:
	CSingleByteToOEMEncoder(UINT nCP, UINT nOEM = CP_OEMCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual bool	Encode(const char *szBuffer, INT_PTR &nLength);
	virtual IEncoder *Clone();

protected:
	UINT m_nCP;
	UINT m_nOEM;
};

class CTableToOEMEncoder : public CSameWidthEncoder
{
public:
	CTableToOEMEncoder(const char *szTable);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual bool	Encode(const char *szBuffer, INT_PTR &nLength);
	virtual IEncoder *Clone();

protected:
	const char *m_szTable;
};

class CUnicodeToOEMEncoder : public CEncoder
{
public:
	CUnicodeToOEMEncoder(UINT nOEM = CP_OEMCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual bool	Encode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IEncoder *Clone();

protected:
	UINT m_nOEM;
};

class CReverseUnicodeToOEMEncoder : public CEncoder
{
public:
	CReverseUnicodeToOEMEncoder(UINT nOEM = CP_OEMCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual bool	Encode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IEncoder *Clone();

protected:
	UINT m_nOEM;
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

class CUTF8ToOEMEncoder : public CEncoder
{
public:
	CUTF8ToOEMEncoder(UINT nOEM = CP_OEMCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual bool	Encode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IEncoder *Clone();

protected:
	UINT m_nOEM;
	CUTF8Traverse m_UT;
};
