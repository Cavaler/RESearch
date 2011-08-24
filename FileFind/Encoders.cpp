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
