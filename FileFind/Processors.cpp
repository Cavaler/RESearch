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
