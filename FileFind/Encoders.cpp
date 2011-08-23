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

bool CPassthroughEncoder::Decode(char *szBuffer, INT_PTR nLength)
{
	Clear();

	m_szBuffer = (char *)malloc(nLength);
	if (m_szBuffer == NULL) return false;

	memmove(m_szBuffer, szBuffer, nLength);
	m_nSize = nLength;

	return true;
}

bool CPassthroughEncoder::Encode(char *szBuffer, INT_PTR nLength)
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

bool CSingleByteToOEMEncoder::Decode(char *szBuffer, INT_PTR nLength)
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

bool CSingleByteToOEMEncoder::Encode(char *szBuffer, INT_PTR nLength)
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
// CUnicodeToOEMEncoder

CUnicodeToOEMEncoder::CUnicodeToOEMEncoder(UINT nOEM)
: m_nOEM(nOEM)
{
}

bool CUnicodeToOEMEncoder::Decode(char *szBuffer, INT_PTR nLength)
{
	Clear();
	if (nLength == 0) return true;

	int nSize = WideCharToMultiByte(m_nOEM, 0, (wchar_t *)szBuffer, nLength/2, NULL, 0, NULL, NULL);
	if (nSize == 0) return false;

	m_szBuffer = (char *)malloc(nSize);
	if (m_szBuffer == NULL) return false;

	m_nSize = WideCharToMultiByte(m_nOEM, 0, (wchar_t *)szBuffer, nLength/2, m_szBuffer, nSize, NULL, NULL);

	return true;
}

bool CUnicodeToOEMEncoder::Encode(char *szBuffer, INT_PTR nLength)
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
