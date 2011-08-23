#include "StdAfx.h"
#include "Backend.h"

CFileBackend::CFileBackend()
: m_hFile		(INVALID_HANDLE_VALUE)
, m_nBlockSize	(0)
, m_pEncoder	(NULL)
, m_szBuffer	(NULL)
, m_nSize		(0)
{
}

CFileBackend::~CFileBackend()
{
	Close();
	Done();
}

bool CFileBackend::Init(INT_PTR nBlockSize, IEncoder *pEncoder)
{
	m_nBlockSize = nBlockSize;
	m_pEncoder   = pEncoder;

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
	m_bEOF = false;

	ReadUp(0);

	return true;
}

bool CFileBackend::ReadUp(INT_PTR nRest)
{
	DWORD dwRead;
	ReadFile(m_hFile, m_szBuffer+nRest, m_nBlockSize-nRest, &dwRead, NULL);
	m_nSize = dwRead + nRest;

	m_bEOF = m_nSize < m_nBlockSize;

	INT_PTR nOrigSize = m_nSize;
	m_pEncoder->Decode(m_szBuffer, m_nSize);
	if (nOrigSize != m_nSize)
		SetFilePointer(m_hFile, m_nSize-nOrigSize, NULL, SEEK_CUR);

	return m_bEOF;
}

void CFileBackend::Close()
{
	if (m_hFile == INVALID_HANDLE_VALUE) return;

	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
}

char *CFileBackend::Buffer()
{
	return m_pEncoder->Buffer();
}

INT_PTR	CFileBackend::Size()
{
	return  m_pEncoder->Size();
}

bool CFileBackend::Last()
{
	return m_bEOF;
}

bool CFileBackend::Move(INT_PTR nLength)
{
	if (nLength < 0) nLength = m_pEncoder->Size() - nLength;

	nLength = m_pEncoder->OriginalOffset(nLength);

	if (nLength > m_nSize) return false;
	int nRest = m_nSize - nLength;

	memmove(m_szBuffer, m_szBuffer + nLength, nRest);

	ReadUp(nRest);

	return true;
}

bool CFileBackend::WriteBack(INT_PTR nLength)
{
	return false;
}

bool CFileBackend::WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength)
{
	return false;
}
