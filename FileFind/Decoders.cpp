#include "StdAfx.h"
#include "Decoders.h"

//////////////////////////////////////////////////////////////////////////
// CDecoder

CDecoder::CDecoder()
: m_szBuffer(NULL)
, m_nSize(0)
{
}

void CDecoder::Clear()
{
	if (m_szBuffer) {
		free(m_szBuffer);
		m_szBuffer = NULL;
	}
	m_nSize = 0;
}

CDecoder::~CDecoder()
{
	Clear();
}

char *CDecoder::Buffer()
{
	return m_szBuffer;
}

INT_PTR	CDecoder::Size()
{
	return m_nSize;
}

//////////////////////////////////////////////////////////////////////////
// CSameWidthDecoder

INT_PTR	CSameWidthDecoder::DecodedOffset (INT_PTR nOffset)
{
	return nOffset;
}

INT_PTR	CSameWidthDecoder::OriginalOffset(INT_PTR nOffset)
{
	return nOffset;
}

//////////////////////////////////////////////////////////////////////////
// CPassthroughDecoder

bool CPassthroughDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();

	m_szBuffer = (char *)malloc(nLength);
	if (m_szBuffer == NULL) return false;

	memmove(m_szBuffer, szBuffer, nLength);
	m_nSize = nLength;

	return true;
}

bool CPassthroughDecoder::Encode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();

	m_szBuffer = (char *)malloc(nLength);
	if (m_szBuffer == NULL) return false;

	memmove(m_szBuffer, szBuffer, nLength);
	m_nSize = nLength;

	return true;
}

IDecoder *CPassthroughDecoder::GetDecoder()
{
	return new CPassthroughDecoder();
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
