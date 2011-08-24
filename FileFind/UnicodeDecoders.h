#pragma once
#include "IFileOperations.h"

class CSingleByteToUnicodeDecoder : public CDecoder
{
public:
	CSingleByteToUnicodeDecoder(UINT nCP);

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetDecoder();

protected:
	UINT m_nCP;
};

class CUTF8ToUnicodeDecoder : public CDecoder
{
public:
	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual INT_PTR	DecodedOffset (INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual IDecoder *GetDecoder();

protected:
	CUTF8Traverse m_UT;
};

class CReverseUnicodeToUnicodeDecoder : public CSameWidthDecoder
{
public:
	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength);
	virtual IDecoder *GetDecoder();
};

void ReverseUnicode(char *szBuffer, INT_PTR nLength);
void ReverseUnicode(const char *szFrom, INT_PTR nLength, char *szTo);

class CAnyToReverseUnicode : public IDecoder
{
public:
	CAnyToReverseUnicode(IDecoder *pDecoder);
	~CAnyToReverseUnicode();

	virtual bool		Decode(const char *szBuffer, INT_PTR &nLength);
	virtual const char *Buffer() { return m_pDecoder->Buffer(); }
	virtual INT_PTR		Size()   { return m_pDecoder->Size();   }

	virtual INT_PTR		DecodedOffset (INT_PTR nOffset) { return m_pDecoder->DecodedOffset (nOffset); }
	virtual INT_PTR		OriginalOffset(INT_PTR nOffset) { return m_pDecoder->OriginalOffset(nOffset); }

	virtual IDecoder *	GetDecoder();

protected:
	IDecoder * m_pDecoder;
};

