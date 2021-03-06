#include "StdAfx.h"
#include "..\RESearch.h"
#include "SingleByteProcessors.h"

CSingleBytePassThroughProcessor::CSingleBytePassThroughProcessor(IBackend *pBackend)
: m_pBackend(pBackend)
{
}

bool CSingleBytePassThroughProcessor::GetNextLine()
{
	if (m_pBackend->Last()) return false;
	if (!m_pBackend->Move(m_pBackend->Size())) return false;

	return true;
}

const char *CSingleBytePassThroughProcessor::Buffer()
{
	return m_pBackend->Buffer();
}

INT_PTR CSingleBytePassThroughProcessor::Size()
{
	return m_pBackend->Size();
}

INT_PTR CSingleBytePassThroughProcessor::Start()
{
	return 0;
}

bool CSingleBytePassThroughProcessor::WriteBack(INT_PTR nOffset)
{
	return m_pBackend->WriteBack(nOffset);
}

bool CSingleBytePassThroughProcessor::WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength)
{
	return m_pBackend->WriteThru(szBuffer, nLength, nSkipLength);
}

void CSingleBytePassThroughProcessor::SkipTo(INT_PTR nOffset)
{
}

//////////////////////////////////////////////////////////////////////////

CSingleByteSplitLineProcessor::CSingleByteSplitLineProcessor(IBackend *pBackend)
: m_pBackend(pBackend)
{
	m_szBuffer = m_pBackend->Buffer();

	m_szEOL = m_szBuffer;

	int nSize = m_pBackend->Size();
	SkipNoCRLF(m_szEOL, &nSize);
}

//	������������, ��� � ���� ������ �� ��������� �� ���������� ������

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
	//	����� ������ ������

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

INT_PTR	CSingleByteSplitLineProcessor::Start()
{
	return 0;
}

bool CSingleByteSplitLineProcessor::WriteBack(INT_PTR nOffset)
{
	return m_pBackend->WriteBack((m_szBuffer - m_pBackend->Buffer()) + nOffset);
}

bool CSingleByteSplitLineProcessor::WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength)
{
	return m_pBackend->WriteThru(szBuffer, nLength, nSkipLength);
}

void CSingleByteSplitLineProcessor::SkipTo(INT_PTR nOffset)
{
}

//////////////////////////////////////////////////////////////////////////

CSingleByteSeveralLineProcessor::CSingleByteSeveralLineProcessor(IBackend *pBackend, int nLines, INT_PTR nMaxSize)
: m_pBackend(pBackend)
, m_nLines(nLines)
, m_nMaxSize(nMaxSize)
{
	CFileBackend *pFileBackend = dynamic_cast<CFileBackend *>(m_pBackend);
	m_pOwnDecoder.reset(new CSingleByteCRLFDecoder(pFileBackend->GetDecoder()));
	pFileBackend->ResetDecoder(m_pOwnDecoder.get());

	m_szBuffer = m_pBackend->Buffer();
	m_szEOL    = m_szBuffer;
	int nSize  = m_pBackend->Size();

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

CSingleByteSeveralLineProcessor::~CSingleByteSeveralLineProcessor()
{
	CFileBackend *pFileBackend = dynamic_cast<CFileBackend *>(m_pBackend);
	pFileBackend->ResetDecoder(NULL);
}

bool CSingleByteSeveralLineProcessor::Overflow()
{
	return Size() >= SeveralLinesKB*1024;
}

bool CSingleByteSeveralLineProcessor::GetNextLine()
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

bool CSingleByteSeveralLineProcessor::GetNextInLine()
{
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
	//	����� ������ ������

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

INT_PTR	CSingleByteSeveralLineProcessor::Start()
{
	return m_nStartOffset;
}

bool CSingleByteSeveralLineProcessor::WriteBack(INT_PTR nOffset)
{
	m_nSkipOffset = nOffset;
	return m_pBackend->WriteBack((m_szBuffer - m_pBackend->Buffer()) + nOffset);
}

bool CSingleByteSeveralLineProcessor::WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength)
{
	m_nSkipOffset += nSkipLength;
	return m_pBackend->WriteThru(szBuffer, nLength, nSkipLength);
}

void CSingleByteSeveralLineProcessor::SkipTo(INT_PTR nOffset)
{
	m_nSkipOffset = nOffset;
}
