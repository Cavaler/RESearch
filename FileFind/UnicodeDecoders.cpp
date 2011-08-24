#include "StdAfx.h"
#include "Decoders.h"

//////////////////////////////////////////////////////////////////////////
// CSingleByteToUnicodeDecoder

CSingleByteToUnicodeDecoder::CSingleByteToUnicodeDecoder(UINT nCP)
: m_nCP(nCP)
{
}

bool CSingleByteToUnicodeDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	int nSize = MultiByteToWideChar(m_nCP, 0, szBuffer, nLength, NULL, 0);
	if (nSize == 0) return false;

	m_szBuffer = (char *)malloc(nSize*2);
	if (m_szBuffer == NULL) return false;

	m_nSize = MultiByteToWideChar(m_nCP, 0, szBuffer, nLength, (wchar_t *)szBuffer, nSize)*2;

	return true;
}

INT_PTR	CSingleByteToUnicodeDecoder::DecodedOffset (INT_PTR nOffset)
{
	return nOffset * 2;
}

INT_PTR	CSingleByteToUnicodeDecoder::OriginalOffset(INT_PTR nOffset)
{
	if (nOffset & 1) return -1;
	return nOffset / 2;
}

IDecoder *CSingleByteToUnicodeDecoder::GetDecoder()
{
	return new CUnicodeToOEMDecoder(m_nCP);
}

//////////////////////////////////////////////////////////////////////////
// CUTF8ToUnicodeDecoder

bool CUTF8ToUnicodeDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	return false;
}

INT_PTR	CUTF8ToUnicodeDecoder::DecodedOffset (INT_PTR nOffset)
{
	return m_UT.ByteToChar(nOffset);
}

INT_PTR	CUTF8ToUnicodeDecoder::OriginalOffset(INT_PTR nOffset)
{
	return m_UT.CharToByte(nOffset);
}

IDecoder *CUTF8ToUnicodeDecoder::GetDecoder()
{
	return new CUnicodeToUTF8Decoder();
}

//////////////////////////////////////////////////////////////////////////
// CReverseUnicodeToUnicodeDecoder

bool CReverseUnicodeToUnicodeDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	return false;
}

IDecoder *CReverseUnicodeToUnicodeDecoder::GetDecoder()
{
	return new CReverseUnicodeToUnicodeDecoder();
}
