#include "StdAfx.h"
#include "Encoders.h"

//////////////////////////////////////////////////////////////////////////
// CEncoder

CEncoder::CEncoder()
: m_szBuffer(NULL)
, m_nSize(0)
{
}

void CEncoder::Clear()
{
	if (m_szBuffer) {
		free(m_szBuffer);
		m_szBuffer = NULL;
	}
	m_nSize = 0;
}

CEncoder::~CEncoder()
{
	Clear();
}

char *CEncoder::Buffer()
{
	return m_szBuffer;
}

INT_PTR	CEncoder::Size()
{
	return m_nSize;
}

//////////////////////////////////////////////////////////////////////////
// CSameWidthEncoder

INT_PTR	CSameWidthEncoder::DecodedOffset (INT_PTR nOffset)
{
	return nOffset;
}

INT_PTR	CSameWidthEncoder::OriginalOffset(INT_PTR nOffset)
{
	return nOffset;
}

//////////////////////////////////////////////////////////////////////////
// CPassthroughEncoder

bool CPassthroughEncoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();

	m_szBuffer = (char *)malloc(nLength);
	if (m_szBuffer == NULL) return false;

	memmove(m_szBuffer, szBuffer, nLength);
	m_nSize = nLength;

	return true;
}

bool CPassthroughEncoder::Encode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();

	m_szBuffer = (char *)malloc(nLength);
	if (m_szBuffer == NULL) return false;

	memmove(m_szBuffer, szBuffer, nLength);
	m_nSize = nLength;

	return true;
}

IEncoder *CPassthroughEncoder::Clone()
{
	return new CPassthroughEncoder();
}

//////////////////////////////////////////////////////////////////////////
// CSingleByteToOEMEncoder

CSingleByteToOEMEncoder::CSingleByteToOEMEncoder(UINT nCP, UINT nOEM )
: m_nCP(nCP)
, m_nOEM(nOEM)
{
}

bool CSingleByteToOEMEncoder::Decode(const char *szBuffer, INT_PTR &nLength)
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

bool CSingleByteToOEMEncoder::Encode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	int nSize = MultiByteToWideChar(m_nOEM, 0, szBuffer, nLength, NULL, 0);
	if (nSize == 0) return false;

	vector<wchar_t> arrWsz(nSize);
	MultiByteToWideChar(m_nOEM, 0, szBuffer, nLength, &arrWsz[0], arrWsz.size());

	nSize = WideCharToMultiByte(m_nCP, 0, &arrWsz[0], arrWsz.size(), NULL, 0, NULL, NULL);
	if (nSize == 0) return false;

	m_szBuffer = (char *)malloc(nSize);
	if (m_szBuffer == NULL) return false;

	m_nSize = WideCharToMultiByte(m_nCP, 0, &arrWsz[0], arrWsz.size(), m_szBuffer, nSize, NULL, NULL);

	return true;
}

IEncoder *CSingleByteToOEMEncoder::Clone()
{
	return new CSingleByteToOEMEncoder(m_nCP, m_nOEM);
}

//////////////////////////////////////////////////////////////////////////
// CTableToOEMEncoder

CTableToOEMEncoder::CTableToOEMEncoder(const char *szTable)
: m_szTable(szTable)
{
}

bool CTableToOEMEncoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	m_szBuffer = (char *)malloc(nLength);
	m_nSize = nLength;

	for (INT_PTR nChar = 0; nChar < nLength; nChar++)
		m_szBuffer[nChar] = m_szTable[(BYTE)szBuffer[nChar]];

	return true;
}

bool CTableToOEMEncoder::Encode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	char szReverseTable[256];
	memset(&szReverseTable, 256, 0);
	for (INT_PTR nChar = 0; nChar < 256; nChar++)
		szReverseTable[m_szTable[nChar]] = nChar;

	m_szBuffer = (char *)malloc(nLength);
	m_nSize = nLength;

	for (INT_PTR nChar = 0; nChar < nLength; nChar++)
		m_szBuffer[nChar] = szReverseTable[(BYTE)szBuffer[nChar]];

	return true;
}

IEncoder *CTableToOEMEncoder::Clone()
{
	return new CTableToOEMEncoder(m_szTable);
}

//////////////////////////////////////////////////////////////////////////
// CUnicodeToOEMEncoder

CUnicodeToOEMEncoder::CUnicodeToOEMEncoder(UINT nOEM)
: m_nOEM(nOEM)
{
}

bool CUnicodeToOEMEncoder::Decode(const char *szBuffer, INT_PTR &nLength)
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

bool CUnicodeToOEMEncoder::Encode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	int nSize = MultiByteToWideChar(m_nOEM, 0, szBuffer, nLength, NULL, 0);
	if (nSize == 0) return false;

	m_szBuffer = (char *)malloc(nSize*2);
	if (m_szBuffer == NULL) return false;

	m_nSize = MultiByteToWideChar(m_nOEM, 0, szBuffer, nLength, (wchar_t *)szBuffer, nSize)*2;

	return true;
}

INT_PTR CUnicodeToOEMEncoder::DecodedOffset (INT_PTR nOffset)
{
	if (nOffset & 1) return -1;
	return nOffset / 2;
}

INT_PTR CUnicodeToOEMEncoder::OriginalOffset(INT_PTR nOffset)
{
	return nOffset * 2;
}

IEncoder *CUnicodeToOEMEncoder::Clone()
{
	return new CUnicodeToOEMEncoder(m_nOEM);
}

//////////////////////////////////////////////////////////////////////////
// CReverseUnicodeToOEMEncoder

CReverseUnicodeToOEMEncoder::CReverseUnicodeToOEMEncoder(UINT nOEM)
: m_nOEM(nOEM)
{
}

bool CReverseUnicodeToOEMEncoder::Decode(const char *szBuffer, INT_PTR &nLength)
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

bool CReverseUnicodeToOEMEncoder::Encode(const char *szBuffer, INT_PTR &nLength)
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

INT_PTR CReverseUnicodeToOEMEncoder::DecodedOffset (INT_PTR nOffset)
{
	if (nOffset & 1) return -1;
	return nOffset / 2;
}

INT_PTR CReverseUnicodeToOEMEncoder::OriginalOffset(INT_PTR nOffset)
{
	return nOffset * 2;
}

IEncoder *CReverseUnicodeToOEMEncoder::Clone()
{
	return new CReverseUnicodeToOEMEncoder(m_nOEM);
}

//////////////////////////////////////////////////////////////////////////
// CUTF8Traverse
void CUTF8Traverse::SetString(const char *szBuffer, INT_PTR nLength)
{
	nByte = 0;
	nChar = 0;

	for (; nByte < nLength; ) {
		utf2char[nByte] = nChar;
		char2utf[nChar] = nByte;
		char c = szBuffer[nByte];
		if		((c & 0x80) == 0x00) nByte += 1;		//	0xxxxxxx
		else if ((c & 0xE0) == 0xC0) nByte += 2;		//	110xxxxx
		else if ((c & 0xF0) == 0xE0) nByte += 3;		//	1110xxxx
		else if ((c & 0xF8) == 0xF0) nByte += 4;		//	11110xxx
		else nByte += 1;
		nChar++;
	}

	utf2char[nByte] = nChar;
	char2utf[nChar] = nByte;
}

INT_PTR	CUTF8Traverse::ByteToChar(INT_PTR nOffset)
{
	return (nOffset <= nByte) ? utf2char[nOffset] : -1;
}

INT_PTR	CUTF8Traverse::CharToByte(INT_PTR nOffset)
{
	return (nOffset <= nChar) ? char2utf[nOffset] : -1;
}

//////////////////////////////////////////////////////////////////////////
// CUTF8ToOEMEncoder

CUTF8ToOEMEncoder::CUTF8ToOEMEncoder(UINT nOEM)
: m_nOEM(nOEM)
{
}

void CutToValidUTF8(const char *szBuffer, INT_PTR &nLength)
{
	while (nLength > 0) {
		if ((szBuffer[nLength-1] & 0x80) == 0x00) break;		// 1-byte
		if ((szBuffer[nLength-2] & 0xE0) == 0xC0) break;		// 2-byte
		if ((szBuffer[nLength-3] & 0xF0) == 0xE0) break;		// 3-byte
		if ((szBuffer[nLength-4] & 0xF8) == 0xF0) break;		// 4-byte

		nLength--;
	}
}

bool CUTF8ToOEMEncoder::Decode(const char *szBuffer, INT_PTR &nLength)
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

bool CUTF8ToOEMEncoder::Encode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	int nSize = MultiByteToWideChar(m_nOEM, 0, szBuffer, nLength, NULL, 0);
	if (nSize == 0) return false;

	vector<wchar_t> arrWsz(nSize);
	MultiByteToWideChar(m_nOEM, 0, szBuffer, nLength, &arrWsz[0], arrWsz.size());

	nSize = WideCharToMultiByte(CP_UTF8, 0, &arrWsz[0], arrWsz.size(), NULL, 0, NULL, NULL);
	if (nSize == 0) return false;

	m_szBuffer = (char *)malloc(nSize);
	if (m_szBuffer == NULL) return false;

	m_nSize = WideCharToMultiByte(CP_UTF8, 0, &arrWsz[0], arrWsz.size(), m_szBuffer, nSize, NULL, NULL);

	return true;
}

INT_PTR CUTF8ToOEMEncoder::DecodedOffset (INT_PTR nOffset)
{
	return m_UT.ByteToChar(nOffset);
}

INT_PTR CUTF8ToOEMEncoder::OriginalOffset(INT_PTR nOffset)
{
	return m_UT.CharToByte(nOffset);
}

IEncoder *CUTF8ToOEMEncoder::Clone()
{
	return new CUTF8ToOEMEncoder(m_nOEM);
}
