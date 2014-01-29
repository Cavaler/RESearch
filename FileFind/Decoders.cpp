#include "StdAfx.h"
#include "Decoders.h"

//////////////////////////////////////////////////////////////////////////
// CDecoder

CDecoder::CDecoder()
: m_szBuffer(NULL)
, m_nAllocSize(0)
, m_nSize(0)
{
}

void CDecoder::Clear()
{
	m_nSize = 0;
}

bool CDecoder::AllocBuffer(INT_PTR nSize)
{
	if ((m_szBuffer != NULL) && (nSize <= m_nAllocSize)) return true;

	//	realloc() involves memory copy, we don't need it
	free(m_szBuffer);
	m_szBuffer = (char *)malloc(nSize);
	if (m_szBuffer == NULL) {
		m_nAllocSize = 0;
		return false;
	}

	m_nAllocSize = nSize;
	return true;
}

CDecoder::~CDecoder()
{
	if (m_szBuffer) free(m_szBuffer);
}

const char *CDecoder::Buffer()
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

	if (!AllocBuffer(nLength)) return false;

	memmove(m_szBuffer, szBuffer, nLength);
	m_nSize = nLength;

	return true;
}

IDecoder *CPassthroughDecoder::GetEncoder()
{
	return new CPassthroughDecoder();
}

//////////////////////////////////////////////////////////////////////////
// CSingleByteCRLFDecoder

CSingleByteCRLFDecoder::CSingleByteCRLFDecoder(IDecoder *pBackDecoder)
: m_pBackDecoder(pBackDecoder)
{
}

bool CSingleByteCRLFDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	if (!m_pBackDecoder->Decode(szBuffer, nLength)) return false;

	Clear();

	if (!AllocBuffer(m_pBackDecoder->Size())) return false;

	m_mapSkipped.clear();
	m_mapSkipped.reserve(m_pBackDecoder->Size() / 32);
	m_nSize = 0;

	const char *szDecBuffer = m_pBackDecoder->Buffer();
	INT_PTR nDecSize = m_pBackDecoder->Size();
	char *szOurBuffer = m_szBuffer;

	const char *szCR;
	while (szCR = (const char *)memchr(szDecBuffer, '\r', nDecSize)) {
		INT_PTR nSkip = szCR-szDecBuffer;

		memmove(szOurBuffer, szDecBuffer, nSkip);
		szOurBuffer += nSkip;
		m_nSize     += nSkip;

		if (m_mapSkipped.empty() || (m_mapSkipped.back().first != szOurBuffer-m_szBuffer))
			m_mapSkipped.push_back(skip_element(szOurBuffer-m_szBuffer, 1));
		else
			m_mapSkipped.back().second++;

		szDecBuffer += nSkip+1;
		nDecSize    -= nSkip+1;
	}

	memmove(szOurBuffer, szDecBuffer, nDecSize);
	m_nSize += nDecSize;
	m_nDOIn = -1;
	m_nOOIn = -1;

	return true;
}

INT_PTR	CSingleByteCRLFDecoder::DecodedOffset(INT_PTR nOffset)
{
	nOffset = m_pBackDecoder->DecodedOffset(nOffset);
	if (nOffset < 0) return nOffset;

	INT_PTR nNewOffset = nOffset;
	size_t nSkipped = 0;
	if ((m_nDOIn >= 0) && (nOffset >= m_nDOIn)) {
		nSkipped    = m_nDOSkip;
		nNewOffset -= m_nDOOut;
	}

	for (; nSkipped < m_mapSkipped.size(); nSkipped++)
	{
		const skip_element &Skip = m_mapSkipped[nSkipped];
		if (nNewOffset <= Skip.first)
			break;
		else if (nNewOffset >= Skip.first+Skip.second)
			nNewOffset -= Skip.second;
		else {	// Weird case of \r\r\n
			nNewOffset = Skip.first;
			break;
		}
	}

	m_nDOIn   = nOffset;
	m_nDOSkip = nSkipped;
	m_nDOOut  = nOffset - nNewOffset;

	return nNewOffset;
}

INT_PTR	CSingleByteCRLFDecoder::OriginalOffset(INT_PTR nOffset)
{
	INT_PTR nNewOffset = nOffset;
	size_t nSkipped = 0;

	if ((m_nOOIn >= 0) && (nOffset >= m_nOOIn)) {
		nSkipped    = m_nOOSkip;
		nNewOffset += m_nOOOut;
	}

	for (; nSkipped < m_mapSkipped.size(); nSkipped++)
	{
		const skip_element &Skip = m_mapSkipped[nSkipped];
		if (Skip.first < nOffset)
			nNewOffset += Skip.second;
		else
			break;
	}

	m_nOOIn   = nOffset;
	m_nOOSkip = nSkipped;
	m_nOOOut  = nNewOffset - nOffset;

	return m_pBackDecoder->OriginalOffset(nNewOffset);
}

IDecoder *CSingleByteCRLFDecoder::GetEncoder()
{
	return m_pBackDecoder->GetEncoder();
}

//////////////////////////////////////////////////////////////////////////
// CUTF8Traverse

template<class Vector> void resize_vector(Vector &vec, size_t nLength)
{
	if ((vec.size() < nLength) || (vec.size() > nLength*16))
		vec.resize(nLength);
}

CUTF8Traverse::CUTF8Traverse()
{
	SetString("", 0);
}

void CUTF8Traverse::SetString(const char *szBuffer, INT_PTR nLength)
{
	nByte = 0;
	nChar = 0;

	resize_vector(utf2char, nLength+2);
	resize_vector(char2utf, nLength+2);

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
