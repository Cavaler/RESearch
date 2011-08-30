#pragma once
#include "IFileOperations.h"

class CSingleByteToUTF8Decoder : public CDecoder
{
public:
	CSingleByteToUTF8Decoder(UINT nCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetEncoder();

protected:
	UINT m_nCP;
	CUTF8Traverse m_UT;
};

class CUnicodeToUTF8Decoder : public CDecoder
{
public:
	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetEncoder();

protected:
	CUTF8Traverse m_UT;
};

class CReverseUnicodeToUTF8Decoder : public CDecoder
{
public:
	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetEncoder();

protected:
	CUTF8Traverse m_UT;
};
