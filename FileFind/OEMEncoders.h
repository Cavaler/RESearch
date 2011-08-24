#pragma once
#include "IFileOperations.h"

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
