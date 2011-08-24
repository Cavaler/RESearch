#include "StdAfx.h"
#include "Decoders.h"

//////////////////////////////////////////////////////////////////////////
// CSingleByteToUTF8Decoder

CSingleByteToUTF8Decoder::CSingleByteToUTF8Decoder(UINT nCP)
: m_nCP(nCP)
{
}

bool CSingleByteToUTF8Decoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	return false;
}

INT_PTR	CSingleByteToUTF8Decoder::DecodedOffset (INT_PTR nOffset)
{
	return -1;
}

INT_PTR	CSingleByteToUTF8Decoder::OriginalOffset(INT_PTR nOffset)
{
	return -1;
}

IDecoder *CSingleByteToUTF8Decoder::GetDecoder()
{
	return new CUTF8ToOEMDecoder(m_nCP);
}

//////////////////////////////////////////////////////////////////////////
// CUnicodeToUTF8Decoder

bool CUnicodeToUTF8Decoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	return false;
}

INT_PTR	CUnicodeToUTF8Decoder::DecodedOffset (INT_PTR nOffset)
{
	return -1;
}

INT_PTR	CUnicodeToUTF8Decoder::OriginalOffset(INT_PTR nOffset)
{
	return -1;
}

IDecoder *CUnicodeToUTF8Decoder::GetDecoder()
{
	return new CUTF8ToUnicodeDecoder();
}

//////////////////////////////////////////////////////////////////////////
// CReverseUnicodeToUTF8Decoder

bool CReverseUnicodeToUTF8Decoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	return false;
}

INT_PTR	CReverseUnicodeToUTF8Decoder::DecodedOffset (INT_PTR nOffset)
{
	return -1;
}

INT_PTR	CReverseUnicodeToUTF8Decoder::OriginalOffset(INT_PTR nOffset)
{
	return -1;
}

IDecoder *CReverseUnicodeToUTF8Decoder::GetDecoder()
{
//	return new CUTF8ToReverseUnicodeDecoder();
	return NULL;
}
