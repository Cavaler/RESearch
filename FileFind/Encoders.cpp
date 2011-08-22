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

INT_PTR	CPassthroughEncoder::DecodedOffset (INT_PTR nOffset)
{
	return nOffset;
}

INT_PTR	CPassthroughEncoder::OriginalOffset(INT_PTR nOffset)
{
	return nOffset;
}

IEncoder *CPassthroughEncoder::Clone()
{
	return new CPassthroughEncoder();
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
