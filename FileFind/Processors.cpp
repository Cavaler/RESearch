#include "StdAfx.h"
#include "..\RESearch.h"
#include "Processors.h"

CSingleByteSplitLineProcessor::CSingleByteSplitLineProcessor(IBackend *pBackend)
: m_pBackend(pBackend)
{
	m_szBuffer = m_pBackend->Buffer();

	m_szEOL = m_szBuffer;

	int nSize = m_pBackend->Size();
	SkipNoCRLF(m_szEOL, &nSize);
}

//	Предполагаем, что в этот момент мы указываем на нормальную строку

bool CSingleByteSplitLineProcessor::GetNextLine()
{
	int nSize = m_pBackend->Size() - (m_szEOL - m_pBackend->Buffer());
	if (nSize < 2) {
		if (!m_pBackend->Last()) {
			if (!m_pBackend->Move(m_szEOL - m_pBackend->Buffer())) return false;
			m_szEOL = m_pBackend->Buffer();
			nSize = m_pBackend->Size();
		} else {
			if (nSize == 0) return false;
		}
	}

	SkipCRLF(m_szEOL, &nSize);
	m_szBuffer = m_szEOL;
	//	Нашли начало строки

	SkipNoCRLF(m_szEOL, &nSize);
	if (nSize == 0) {
		if (!m_pBackend->Last()) {
			if (!m_pBackend->Move(m_szBuffer - m_pBackend->Buffer())) return false;
			m_szBuffer = m_pBackend->Buffer();
			nSize = m_pBackend->Size();

			m_szEOL = m_szBuffer;
			SkipNoCRLF(m_szEOL, &nSize);
		} else {
			if (m_szBuffer == m_szEOL) return false;
		}
	}

	return true;
}

const char *CSingleByteSplitLineProcessor::Buffer()
{
	return m_szBuffer;
}

INT_PTR	CSingleByteSplitLineProcessor::Size()
{
	return m_szEOL - m_szBuffer;
}

//////////////////////////////////////////////////////////////////////////

CSingleByteSeveralLineProcessor::CSingleByteSeveralLineProcessor(IBackend *pBackend, int nLines, INT_PTR nMaxSize)
: m_pBackend(pBackend)
, m_nLines(nLines)
, m_nMaxSize(nMaxSize)
{
	m_szBuffer = m_pBackend->Buffer();
	m_szEOL = m_szBuffer;
	int nSize = m_pBackend->Size();

	for (int nLine = 0; nLine < m_nLines; nLine++) {
		m_arrLines.push_back(m_szEOL);
		SkipNoCRLF(m_szEOL, &nSize);
		if (nLine == m_nLines-1) break;
		SkipCRLF(m_szEOL, &nSize);
		if (nSize == 0) break;
	}

	m_bAtEnd = (nSize == 0) && m_pBackend->Last();
}

bool CSingleByteSeveralLineProcessor::GetNextLine()
{
	if (m_bAtEnd) return GetNextLastLine();

	if (m_arrLines.size() == m_nLines) {
		m_arrLines.erase(m_arrLines.begin());
		m_szBuffer = m_arrLines[0];
	}

	int nSize = m_pBackend->Size() - (m_szEOL - m_pBackend->Buffer());
	if (nSize < 2) {
		if (!m_pBackend->Last()) {
			if (!BackendMove(m_szEOL - m_pBackend->Buffer())) return false;
			nSize = m_pBackend->Size();
		} else {
			if (nSize == 0) { m_bAtEnd = true; return GetNextLastLine(); }
		}
	}

	SkipCRLF(m_szEOL, &nSize);
	const char *szLine = m_szEOL;
	//	Нашли начало строки

	SkipNoCRLF(m_szEOL, &nSize);
	if (nSize == 0) {
		if (!m_pBackend->Last()) {
			INT_PTR nMove = m_szBuffer - m_pBackend->Buffer();
			if (!BackendMove(nMove)) return false;
			szLine -= nMove;
			nSize = m_pBackend->Size();

			SkipNoCRLF(m_szEOL, &nSize);
		} else {
			if (m_szBuffer == m_szEOL) { m_bAtEnd = true; return GetNextLastLine(); }
		}
	}

	m_arrLines.push_back(szLine);

	return true;
}

bool CSingleByteSeveralLineProcessor::BackendMove(INT_PTR nLength)
{
	if (!m_pBackend->Move(nLength)) return false;
	m_szBuffer -= nLength;
	m_szEOL    -= nLength;

	for (size_t nLine = 0; nLine < m_arrLines.size(); nLine++) {
		m_arrLines[nLine] -= nLength;
	}

	return true;
}

bool CSingleByteSeveralLineProcessor::GetNextLastLine()
{
	if (m_arrLines.size() <= 1) return false;

	m_arrLines.erase(m_arrLines.begin());
	m_szBuffer = m_arrLines[0];

	return true;
}

const char *CSingleByteSeveralLineProcessor::Buffer()
{
	return m_szBuffer;
}

INT_PTR	CSingleByteSeveralLineProcessor::Size()
{
	return m_szEOL - m_szBuffer;
}