#include "StdAfx.h"
#include "Decoders.h"

//////////////////////////////////////////////////////////////////////////
// CSingleByteToOEMDecoder

CSingleByteToOEMDecoder::CSingleByteToOEMDecoder(UINT nCP, UINT nOEM )
: m_nCP(nCP)
, m_nOEM(nOEM)
{
}

bool CSingleByteToOEMDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	int nSize = MultiByteToWideChar(m_nCP, 0, szBuffer, nLength, NULL, 0);
	if (nSize == 0) return false;

	vector<wchar_t> arrWsz(nSize);
	MultiByteToWideChar(m_nCP, 0, szBuffer, nLength, &arrWsz[0], arrWsz.size());

	nSize = WideCharToMultiByte(m_nOEM, 0, &arrWsz[0], arrWsz.size(), NULL, 0, NULL, NULL);
	if (nSize == 0) return false;

	m_szBuffer = (char *)malloc(nSize);
	if (m_szBuffer == NULL) return false;

	m_nSize = WideCharToMultiByte(m_nOEM, 0, &arrWsz[0], arrWsz.size(), m_szBuffer, nSize, NULL, NULL);

	return true;
}

IDecoder *CSingleByteToOEMDecoder::GetDecoder()
{
	return new CSingleByteToOEMDecoder(m_nOEM, m_nCP);
}

//////////////////////////////////////////////////////////////////////////
// CTableToOEMDecoder

CTableToOEMDecoder::CTableToOEMDecoder(const char *szDecodeTable, const char *szEncodeTable)
: m_szDecodeTable(szDecodeTable)
, m_szEncodeTable(szEncodeTable)
{
}

bool CTableToOEMDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	m_szBuffer = (char *)malloc(nLength);
	m_nSize = nLength;

	for (INT_PTR nChar = 0; nChar < nLength; nChar++)
		m_szBuffer[nChar] = m_szDecodeTable[(BYTE)szBuffer[nChar]];

	return true;
}

IDecoder *CTableToOEMDecoder::GetDecoder()
{
	return new CTableToOEMDecoder(m_szEncodeTable, m_szDecodeTable);
}

//////////////////////////////////////////////////////////////////////////
// CUnicodeToOEMDecoder

CUnicodeToOEMDecoder::CUnicodeToOEMDecoder(UINT nOEM)
: m_nOEM(nOEM)
{
}

bool CUnicodeToOEMDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	nLength &= ~1;	// Even only
	if (nLength == 0) return true;

	int nSize = WideCharToMultiByte(m_nOEM, 0, (wchar_t *)szBuffer, nLength/2, NULL, 0, NULL, NULL);
	if (nSize == 0) return false;

	m_szBuffer = (char *)malloc(nSize);
	if (m_szBuffer == NULL) return false;

	m_nSize = WideCharToMultiByte(m_nOEM, 0, (wchar_t *)szBuffer, nLength/2, m_szBuffer, nSize, NULL, NULL);

	return true;
}

INT_PTR CUnicodeToOEMDecoder::DecodedOffset (INT_PTR nOffset)
{
	if (nOffset & 1) return -1;
	return nOffset / 2;
}

INT_PTR CUnicodeToOEMDecoder::OriginalOffset(INT_PTR nOffset)
{
	return nOffset * 2;
}

IDecoder *CUnicodeToOEMDecoder::GetDecoder()
{
	return new CSingleByteToUnicodeDecoder(m_nOEM);
}

//////////////////////////////////////////////////////////////////////////
// CReverseUnicodeToOEMDecoder

CReverseUnicodeToOEMDecoder::CReverseUnicodeToOEMDecoder(UINT nOEM)
: m_nOEM(nOEM)
{
}

bool CReverseUnicodeToOEMDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	nLength &= ~1;	// Even only
	if (nLength == 0) return true;

	m_nSize = nLength/2;

	m_szBuffer = (char *)malloc(m_nSize);
	if (m_szBuffer == NULL) return false;

	for (INT_PTR nChar = 0; nChar < m_nSize; nChar++) {
		wchar_t wcSingle = (szBuffer[nChar*2]<<8) + szBuffer[nChar*2+1];
		WideCharToMultiByte(m_nOEM, 0, &wcSingle, 1, m_szBuffer+nChar, 1, NULL, NULL);
	}

	return true;
}

bool CReverseUnicodeToOEMDecoder::Encode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	m_nSize = nLength*2;
	m_szBuffer = (char *)malloc(m_nSize);
	if (m_szBuffer == NULL) return false;

	for (INT_PTR nChar = 0; nChar < m_nSize/2; nChar++) {
		wchar_t wcSingle;
		MultiByteToWideChar(m_nOEM, 0, szBuffer+nChar, 1, &wcSingle, 1);
		m_szBuffer[nChar*2  ] = (wcSingle >> 8) & 0xFF;
		m_szBuffer[nChar*2+1] = wcSingle & 0xFF;
	}

	return true;
}

INT_PTR CReverseUnicodeToOEMDecoder::DecodedOffset (INT_PTR nOffset)
{
	if (nOffset & 1) return -1;
	return nOffset / 2;
}

INT_PTR CReverseUnicodeToOEMDecoder::OriginalOffset(INT_PTR nOffset)
{
	return nOffset * 2;
}

IDecoder *CReverseUnicodeToOEMDecoder::GetDecoder()
{
	return new CAnyToReverseUnicode(new CSingleByteToUnicodeDecoder(m_nOEM));
}

//////////////////////////////////////////////////////////////////////////
// CUTF8ToOEMDecoder

CUTF8ToOEMDecoder::CUTF8ToOEMDecoder(UINT nOEM)
: m_nOEM(nOEM)
{
}

bool CUTF8ToOEMDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	CutToValidUTF8(szBuffer, nLength);
	m_UT.SetString(szBuffer, nLength);

	int nSize = MultiByteToWideChar(CP_UTF8, 0, szBuffer, nLength, NULL, 0);
	if (nSize == 0) return false;
	if (m_UT.CharToByte(nSize) != nLength) return false;

	vector<wchar_t> arrWsz(nSize);
	MultiByteToWideChar(CP_UTF8, 0, szBuffer, nLength, &arrWsz[0], arrWsz.size());

	nSize = WideCharToMultiByte(m_nOEM, 0, &arrWsz[0], arrWsz.size(), NULL, 0, NULL, NULL);
	if (nSize == 0) return false;

	m_szBuffer = (char *)malloc(nSize);
	if (m_szBuffer == NULL) return false;

	m_nSize = WideCharToMultiByte(m_nOEM, 0, &arrWsz[0], arrWsz.size(), m_szBuffer, nSize, NULL, NULL);

	return true;
}

INT_PTR CUTF8ToOEMDecoder::DecodedOffset (INT_PTR nOffset)
{
	return m_UT.ByteToChar(nOffset);
}

INT_PTR CUTF8ToOEMDecoder::OriginalOffset(INT_PTR nOffset)
{
	return m_UT.CharToByte(nOffset);
}

IDecoder *CUTF8ToOEMDecoder::GetDecoder()
{
	return new CSingleByteToUTF8Decoder(m_nOEM);
}
