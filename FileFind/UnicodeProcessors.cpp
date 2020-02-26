#include "StdAfx.h"
#include "..\RESearch.h"
#include "UnicodeProcessors.h"

CUnicodeSplitLineProcessor::CUnicodeSplitLineProcessor(IBackend *pBackend)
: m_pBackend(pBackend)
{
	m_szBuffer = m_pBackend->BufferW();

	m_szEOL = m_szBuffer;

	int nSize = m_pBackend->SizeW();
	SkipNoCRLF(m_szEOL, &nSize);
}

bool CUnicodeSplitLineProcessor::GetNextLine()
{
	int nSize = m_pBackend->SizeW() - (m_szEOL - m_pBackend->BufferW());
	if (nSize < 2) {
		if (!m_pBackend->Last()) {
			if (!m_pBackend->Move((m_szEOL - m_pBackend->BufferW())*2)) return false;
			m_szEOL = m_pBackend->BufferW();
			nSize = m_pBackend->SizeW();
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
			if (!m_pBackend->Move((m_szBuffer - m_pBackend->BufferW())*2)) return false;
			m_szBuffer = m_pBackend->BufferW();
			nSize = m_pBackend->SizeW();

			m_szEOL = m_szBuffer;
			SkipNoCRLF(m_szEOL, &nSize);
		} else {
			if (m_szBuffer == m_szEOL) return false;
		}
	}

	return true;
}

const char *CUnicodeSplitLineProcessor::Buffer()
{
	return (const char *)m_szBuffer;
}

INT_PTR	CUnicodeSplitLineProcessor::Size()
{
	return (m_szEOL - m_szBuffer)*2;
}

INT_PTR	CUnicodeSplitLineProcessor::Start()
{
	return 0;
}

bool CUnicodeSplitLineProcessor::WriteBack(INT_PTR nOffset)
{
	return m_pBackend->WriteBack((m_szBuffer - m_pBackend->BufferW())*2 + nOffset);
}

bool CUnicodeSplitLineProcessor::WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength)
{
	return m_pBackend->WriteThru(szBuffer, nLength, nSkipLength);
}

void CUnicodeSplitLineProcessor::SkipTo(INT_PTR nOffset)
{
}

//////////////////////////////////////////////////////////////////////////

CUnicodeSeveralLineProcessor::CUnicodeSeveralLineProcessor(IBackend *pBackend, int nLines, INT_PTR nMaxSize)
: m_pBackend(pBackend)
, m_nLines(nLines)
, m_nMaxSize(nMaxSize)
{
	CFileBackend *pFileBackend = dynamic_cast<CFileBackend *>(m_pBackend);
	m_pOwnDecoder.reset(new CUnicodeCRLFDecoder(pFileBackend->GetDecoder()));
	pFileBackend->ResetDecoder(m_pOwnDecoder.get());

	m_szBuffer = m_pBackend->BufferW();
	m_szEOL    = m_szBuffer;
	int nSize  = m_pBackend->SizeW();

	for (size_t nLine = 0; nLine < m_nLines; nLine++) {
		m_arrLines.push_back(m_szEOL);
		SkipNoCRLF(m_szEOL, &nSize);
		if (nLine == m_nLines-1) break;
		SkipCRLF(m_szEOL, &nSize);
		if (nSize == 0) break;

		if (Overflow()) break;
	}

	m_nSkipOffset  = 0;
	m_nStartOffset = 0;

	m_bAtEnd = (nSize == 0) && m_pBackend->Last();
}

CUnicodeSeveralLineProcessor::~CUnicodeSeveralLineProcessor()
{
	CFileBackend *pFileBackend = dynamic_cast<CFileBackend *>(m_pBackend);
	pFileBackend->ResetDecoder(NULL);
}

bool CUnicodeSeveralLineProcessor::Overflow()
{
	return Size() >= SeveralLinesKB*1024;
}

bool CUnicodeSeveralLineProcessor::GetNextLine()
{
	bool bAnySkipped = false;
	while ((m_arrLines.size() > 1) && (m_arrLines[1] < m_szBuffer+m_nSkipOffset)) {
		m_arrLines.erase(m_arrLines.begin());
		bAnySkipped = true;
	}
	if (bAnySkipped) {
		m_nStartOffset = m_szBuffer+m_nSkipOffset - m_arrLines[0];
		m_nSkipOffset  = 0;
		m_szBuffer     = m_arrLines[0];
		return true;
	}

	m_nSkipOffset  = 0;
	m_nStartOffset = 0;
	if (m_bAtEnd) return GetNextLastLine();

	m_arrLines.erase(m_arrLines.begin());
	m_szBuffer = m_arrLines.empty() ? m_szEOL : m_arrLines[0];

	if (Overflow()) return true;

	if (!GetNextInLine()) return false;

	while ((m_arrLines.size() < m_nLines) && !Overflow() && !m_bAtEnd) GetNextInLine();

	return true;
}

bool CUnicodeSeveralLineProcessor::GetNextInLine()
{
	int nSize = m_pBackend->SizeW() - (m_szEOL - m_pBackend->BufferW());
	if (nSize < 2) {
		if (!m_pBackend->Last()) {
			if (!BackendMove(m_szEOL - m_pBackend->BufferW())) return false;
			nSize = m_pBackend->SizeW();
		} else {
			if (nSize == 0) { m_bAtEnd = true; return GetNextLastLine(); }
		}
	}

	SkipCRLF(m_szEOL, &nSize);
	const wchar_t *szLine = m_szEOL;
	//	Нашли начало строки

	SkipNoCRLF(m_szEOL, &nSize);
	if (nSize == 0) {
		if (!m_pBackend->Last()) {
			INT_PTR nMove = m_szBuffer - m_pBackend->BufferW();
			if (!BackendMove(nMove)) return false;
			szLine -= nMove;
			nSize = m_pBackend->SizeW();

			SkipNoCRLF(m_szEOL, &nSize);
		} else {
			if (m_szBuffer == m_szEOL) { m_bAtEnd = true; return GetNextLastLine(); }
		}
	}

	m_arrLines.push_back(szLine);

	return true;
}

bool CUnicodeSeveralLineProcessor::BackendMove(INT_PTR nLength)
{
	if (!m_pBackend->Move(nLength*2)) return false;
	m_szBuffer -= nLength;
	m_szEOL    -= nLength;

	for (size_t nLine = 0; nLine < m_arrLines.size(); nLine++) {
		m_arrLines[nLine] -= nLength;
	}

	return true;
}

bool CUnicodeSeveralLineProcessor::GetNextLastLine()
{
	if (m_arrLines.size() <= 1) return false;

	m_arrLines.erase(m_arrLines.begin());
	m_szBuffer = m_arrLines[0];

	return true;
}

const char *CUnicodeSeveralLineProcessor::Buffer()
{
	return (const char *)m_szBuffer;
}

INT_PTR	CUnicodeSeveralLineProcessor::Size()
{
	return (m_szEOL - m_szBuffer)*2;
}

const wchar_t *CUnicodeSeveralLineProcessor::BufferW()
{
	return m_szBuffer;
}

INT_PTR	CUnicodeSeveralLineProcessor::SizeW()
{
	return m_szEOL - m_szBuffer;
}

INT_PTR	CUnicodeSeveralLineProcessor::Start()
{
	return m_nStartOffset*2;
}

bool CUnicodeSeveralLineProcessor::WriteBack(INT_PTR nOffset)
{
	m_nSkipOffset = nOffset/2;
	return m_pBackend->WriteBack((m_szBuffer - m_pBackend->BufferW())*2 + nOffset);
}

bool CUnicodeSeveralLineProcessor::WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength)
{
	m_nSkipOffset += nSkipLength/2;
	return m_pBackend->WriteThru(szBuffer, nLength, nSkipLength);
}

void CUnicodeSeveralLineProcessor::SkipTo(INT_PTR nOffset)
{
	m_nSkipOffset = nOffset/2;
}
