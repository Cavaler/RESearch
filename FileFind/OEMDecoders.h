#pragma once
#include "IFileOperations.h"

class CSingleByteToOEMDecoder : public CSameWidthDecoder
{
public:
	CSingleByteToOEMDecoder(UINT nCP, UINT nOEM = CP_OEMCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual IDecoder *GetDecoder();

protected:
	UINT m_nCP;
	UINT m_nOEM;
};

class CTableToOEMDecoder : public CSameWidthDecoder
{
public:
	CTableToOEMDecoder(const char *szDecodeTable, const char *szEncodeTable);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual IDecoder *GetDecoder();

protected:
	const char *m_szDecodeTable;
	const char *m_szEncodeTable;
};

class CUnicodeToOEMDecoder : public CDecoder
{
public:
	CUnicodeToOEMDecoder(UINT nOEM = CP_OEMCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetDecoder();

protected:
	UINT m_nOEM;
};

class CReverseUnicodeToOEMDecoder : public CDecoder
{
public:
	CReverseUnicodeToOEMDecoder(UINT nOEM = CP_OEMCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual bool	Encode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetDecoder();

protected:
	UINT m_nOEM;
};

class CUTF8ToOEMDecoder : public CDecoder
{
public:
	CUTF8ToOEMDecoder(UINT nOEM = CP_OEMCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetDecoder();

protected:
	UINT m_nOEM;
	CUTF8Traverse m_UT;
};
