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

	if (!AllocBuffer(nSize*2)) return false;

	m_nSize = MultiByteToWideChar(m_nCP, 0, szBuffer, nLength, (wchar_t *)m_szBuffer, nSize)*2;

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

IDecoder *CSingleByteToUnicodeDecoder::GetEncoder()
{
	return new CUnicodeToOEMDecoder(m_nCP);
}

//////////////////////////////////////////////////////////////////////////
// CUTF8ToUnicodeDecoder

bool CUTF8ToUnicodeDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	if (nLength == 0) return true;

	CutToValidUTF8(szBuffer, nLength);
	m_UT.SetString(szBuffer, nLength);

	m_nSize = MultiByteToWideChar(CP_UTF8, 0, szBuffer, nLength, NULL, 0);
	if (m_nSize == 0) return false;
	if (m_UT.CharToByte(m_nSize) != nLength) return false;

	m_nSize *= 2;

	m_szBuffer = (char *)malloc(m_nSize);
	MultiByteToWideChar(CP_UTF8, 0, szBuffer, nLength, (WCHAR *)m_szBuffer, m_nSize/2);

	return true;
}

INT_PTR	CUTF8ToUnicodeDecoder::DecodedOffset (INT_PTR nOffset)
{
	return m_UT.ByteToChar(nOffset)*2;
}

INT_PTR	CUTF8ToUnicodeDecoder::OriginalOffset(INT_PTR nOffset)
{
	if (nOffset & 1) return -1;
	return m_UT.CharToByte(nOffset/2);
}

IDecoder *CUTF8ToUnicodeDecoder::GetEncoder()
{
	return new CUnicodeToUTF8Decoder();
}

void ReverseUnicode(char *szBuffer, INT_PTR nLength)
{
	for (INT_PTR nChar = 0; nChar < nLength-1; nChar+=2) {
		swap(szBuffer[nChar], szBuffer[nChar+1]);
	}
}

void ReverseUnicode(const char *szFrom, INT_PTR nLength, char *szTo)
{
	if (szFrom == szTo) {
		ReverseUnicode(szTo, nLength);
		return;
	}

	for (INT_PTR nChar = 0; nChar < nLength-1; nChar+=2) {
		szTo[nChar  ] = szFrom[nChar+1];
		szTo[nChar+1] = szFrom[nChar  ];
	}
}

//////////////////////////////////////////////////////////////////////////
// CReverseUnicodeToUnicodeDecoder

bool CReverseUnicodeToUnicodeDecoder::Decode(const char *szBuffer, INT_PTR &nLength)
{
	Clear();
	nLength &= ~1;	// Even only
	if (nLength == 0) return true;

	m_nSize = nLength;
	if (!AllocBuffer(m_nSize)) return false;

	ReverseUnicode(szBuffer, nLength, m_szBuffer);

	return true;
}

IDecoder *CReverseUnicodeToUnicodeDecoder::GetEncoder()
{
	return new CReverseUnicodeToUnicodeDecoder();
}

CAnyToReverseUnicode::CAnyToReverseUnicode(IDecoder *pDecoder)
: m_pDecoder(pDecoder)
{
}

CAnyToReverseUnicode::~CAnyToReverseUnicode()
{
	delete m_pDecoder;
}

bool CAnyToReverseUnicode::Decode(const char *szBuffer, INT_PTR &nLength)
{
	if (!m_pDecoder->Decode(szBuffer, nLength)) return false;

	ReverseUnicode((char *)m_pDecoder->Buffer(), m_pDecoder->Size());

	return true;
}

IDecoder *CAnyToReverseUnicode::GetEncoder()
{
	return NULL;
}
