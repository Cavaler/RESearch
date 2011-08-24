#include "StdAfx.h"
#include "Backend.h"

CFileBackend::CFileBackend()
: m_hFile		(INVALID_HANDLE_VALUE)
, m_nBlockSize	(0)
, m_pDecoder	(NULL)
, m_szBuffer	(NULL)
, m_nSize		(0)
{
}

CFileBackend::~CFileBackend()
{
	Close();
	Done();
}

bool CFileBackend::Init(INT_PTR nBlockSize)
{
	m_nBlockSize = nBlockSize;

	m_szBuffer   = (char *)malloc(nBlockSize);
	m_nSize      = 0;

	return m_szBuffer != NULL;
}

void CFileBackend::Done()
{
	Close();

	if (m_szBuffer) {
		free(m_szBuffer);
		m_szBuffer = NULL;
	}

	m_nSize    = 0;
}

bool CFileBackend::Open(LPCTSTR szFileName)
{
	m_hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE) return false;

	ReadUp(0);

	return true;
}

bool CFileBackend::SetDecoder(IDecoder *pDecoder, INT_PTR nSkip)
{
	m_pDecoder = pDecoder;

	SetFilePointer(m_hFile, nSkip, NULL, SEEK_SET);

	m_bEOF = false;

	return ReadUp(0);
}

bool CFileBackend::ReadUp(INT_PTR nRest)
{
	DWORD dwRead;
	ReadFile(m_hFile, m_szBuffer+nRest, m_nBlockSize-nRest, &dwRead, NULL);
	m_nSize = dwRead + nRest;

	m_bEOF = m_nSize < m_nBlockSize;

	if (m_pDecoder) {
		INT_PTR nOrigSize = m_nSize;
		if (!m_pDecoder->Decode(m_szBuffer, m_nSize)) return false;
		if (nOrigSize != m_nSize)
			SetFilePointer(m_hFile, m_nSize-nOrigSize, NULL, SEEK_CUR);
	}

	return true;
}

void CFileBackend::Close()
{
	if (m_hFile == INVALID_HANDLE_VALUE) return;

	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;

	m_pDecoder = NULL;
}

char *CFileBackend::Buffer()
{
	return m_pDecoder ? m_pDecoder->Buffer() : m_szBuffer;
}

INT_PTR	CFileBackend::Size()
{
	return m_pDecoder ? m_pDecoder->Size() : m_nSize;
}

bool CFileBackend::Last()
{
	return m_bEOF;
}

bool CFileBackend::Move(INT_PTR nLength)
{
	if (nLength < 0) nLength = Size() - nLength;

	nLength = m_pDecoder->OriginalOffset(nLength);
	if (nLength < 0) return false;

	if (nLength > m_nSize) return false;
	int nRest = m_nSize - nLength;

	memmove(m_szBuffer, m_szBuffer + nLength, nRest);

	return ReadUp(nRest);
}

bool CFileBackend::WriteBack(INT_PTR nLength)
{
	return false;
}

bool CFileBackend::WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength)
{
	return false;
}
