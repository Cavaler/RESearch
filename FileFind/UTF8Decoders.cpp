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
	Clear();
	if (nLength == 0) return true;

	int nSize = MultiByteToWideChar(m_nCP, 0, szBuffer, nLength, NULL, 0);
	if (nSize == 0) return false;

	vector<wchar_t> arrWsz(nSize);
	MultiByteToWideChar(m_nCP, 0, szBuffer, nLength, &arrWsz[0], arrWsz.size());

	nSize = WideCharToMultiByte(CP_UTF8, 0, &arrWsz[0], arrWsz.size(), NULL, 0, NULL, NULL);
	if (nSize == 0) return false;

	m_szBuffer = (char *)malloc(nSize);
	if (m_szBuffer == NULL) return false;

	m_nSize = WideCharToMultiByte(CP_UTF8, 0, &arrWsz[0], arrWsz.size(), m_szBuffer, nSize, NULL, NULL);

	m_UT.SetString(m_szBuffer, m_nSize);

	return true;
}

INT_PTR	CSingleByteToUTF8Decoder::DecodedOffset (INT_PTR nOffset)
{
	return m_UT.CharToByte(nOffset);
}

INT_PTR	CSingleByteToUTF8Decoder::OriginalOffset(INT_PTR nOffset)
{
	return m_UT.ByteToChar(nOffset);
}

IDecoder *CSingleByteToUTF8Decoder::GetDecoder()
{
	return new CUTF8ToOEMDecoder(m_nCP);
}

//////////////////////////////////////////////////////////////////////////
// CUnicodeToUTF8Decoder

bool CUnicodeToUTF8Decoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	nLength &= ~1;	// Even only
	if (nLength == 0) return true;

	m_nSize = WideCharToMultiByte(CP_UTF8, 0, (WCHAR *)szBuffer, nLength/2, NULL, 0, NULL, NULL);
	if (m_nSize == 0) return false;

	m_szBuffer = (char *)malloc(m_nSize);
	if (m_szBuffer == NULL) return false;

	m_nSize = WideCharToMultiByte(CP_UTF8, 0, (WCHAR *)szBuffer, nLength/2, m_szBuffer, m_nSize, NULL, NULL);

	m_UT.SetString(m_szBuffer, m_nSize);

	return true;
}

INT_PTR	CUnicodeToUTF8Decoder::DecodedOffset (INT_PTR nOffset)
{
	return m_UT.CharToByte(nOffset)*2;
}

INT_PTR	CUnicodeToUTF8Decoder::OriginalOffset(INT_PTR nOffset)
{
	if (nOffset & 1) return -1;
	return m_UT.ByteToChar(nOffset/2);
}

IDecoder *CUnicodeToUTF8Decoder::GetDecoder()
{
	return new CUTF8ToUnicodeDecoder();
}

//////////////////////////////////////////////////////////////////////////
// CReverseUnicodeToUTF8Decoder

bool CReverseUnicodeToUTF8Decoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	nLength &= ~1;	// Even only
	if (nLength == 0) return true;

	vector<char> szLE(nLength);
	ReverseUnicode(szBuffer, nLength, &szLE[0]);

	m_nSize = WideCharToMultiByte(CP_UTF8, 0, (WCHAR *)&szLE[0], nLength/2, NULL, 0, NULL, NULL);
	if (m_nSize == 0) return false;

	m_szBuffer = (char *)malloc(m_nSize);
	if (m_szBuffer == NULL) return false;

	m_nSize = WideCharToMultiByte(CP_UTF8, 0, (WCHAR *)&szLE[0], nLength/2, m_szBuffer, m_nSize, NULL, NULL);

	m_UT.SetString(m_szBuffer, m_nSize);

	return true;
}

INT_PTR	CReverseUnicodeToUTF8Decoder::DecodedOffset (INT_PTR nOffset)
{
	return m_UT.CharToByte(nOffset)*2;
}

INT_PTR	CReverseUnicodeToUTF8Decoder::OriginalOffset(INT_PTR nOffset)
{
	if (nOffset & 1) return -1;
	return m_UT.ByteToChar(nOffset/2);
}

IDecoder *CReverseUnicodeToUTF8Decoder::GetDecoder()
{
	return new CAnyToReverseUnicode(new CUTF8ToUnicodeDecoder());
}
