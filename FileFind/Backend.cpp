#include "StdAfx.h"
#include "..\RESearch.h"

char *CFileBackend::m_szBuffer = NULL;
INT_PTR CFileBackend::m_nBlockSize = 0;

DWORD I64Ceil(__int64 i64)
{
	if (i64 > 0xFFFFFFFF) return 0xFFFFFFFF;
	if (i64 < 0) return 0;
	return (DWORD) i64;
}

#ifdef _WIN64
#define ICeil(i64) I64Ceil(i64)
#else
#define ICeil(i64) ((DWORD)(i64))
#endif

CFileBackend::CFileBackend()
: m_hFile		(INVALID_HANDLE_VALUE)
, m_pDecoder	(NULL)
, m_nSize		(0)
, m_hOutFile	(INVALID_HANDLE_VALUE)
, m_nBuffered   (0)
, m_pEncoder	(NULL)
{
}

CFileBackend::~CFileBackend()
{
	if (g_bInterrupted) Abort(); else Close();
	Done();
}

bool CFileBackend::Init(INT_PTR nBlockSize)
{
	if (m_nBlockSize != nBlockSize) {
		m_nBlockSize = nBlockSize;
		m_szBuffer   = (char *)realloc(m_szBuffer, nBlockSize);
	}

	m_nSize      = 0;

	return m_szBuffer != NULL;
}

void CFileBackend::Done()
{
	if (g_bInterrupted) Abort(); else Close();

	if (m_pEncoder) delete m_pEncoder;

	m_nSize    = 0;
}

void CFileBackend::Free()
{
	if (m_szBuffer) {
		free(m_szBuffer);
		m_szBuffer = NULL;
	}

	m_nBlockSize = 0;
}

bool CFileBackend::Open(LPCTSTR szFileName, INT_PTR nMaxSize)
{
	m_hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE) return false;

	DWORD dwSizeHigh;
	DWORD dwSizeLow = GetFileSize(m_hFile, &dwSizeHigh);
	m_nSizeLimit = dwSizeLow + (((__int64)dwSizeHigh)<<32);
	if ((nMaxSize > 0) && (nMaxSize < m_nSizeLimit)) m_nSizeLimit = nMaxSize;

	m_nOriginalSizeLimit = m_nSizeLimit;

	m_strFileName = szFileName;

	ReadUp(0);

	return true;
}

bool CFileBackend::Open(LPCTSTR szInFileName, LPCTSTR szOutFileName)
{
	m_hFile = CreateFile(szInFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE) return false;

	DWORD dwSizeHigh;
	DWORD dwSizeLow = GetFileSize(m_hFile, &dwSizeHigh);
	m_nSizeLimit = dwSizeLow + (((__int64)dwSizeHigh)<<32);

	m_nOriginalSizeLimit = m_nSizeLimit;

	m_strFileName = szInFileName;
	m_strOutFileName = szOutFileName;

	ReadUp(0);

	return true;
}

bool CFileBackend::SetDecoder(IDecoder *pDecoder, INT_PTR nSkip)
{
	m_pDecoder = pDecoder;
	m_nSkip    = nSkip;

	SetFilePointer(m_hFile, nSkip, NULL, FILE_BEGIN);

	m_bEOF = false;
	m_nSizeLimit = m_nOriginalSizeLimit;

	return ReadUp(0);
}

IDecoder *CFileBackend::GetDecoder()
{
	return m_pDecoder;
}

bool CFileBackend::ResetDecoder(IDecoder *pDecoder)
{
	return SetDecoder(pDecoder, m_nSkip);
}

bool CFileBackend::ReadUp(INT_PTR nRest)
{
	DWORD dwToRead = m_nBlockSize-nRest;
	if (dwToRead > m_nSizeLimit) dwToRead = (DWORD)m_nSizeLimit;

	LARGE_INTEGER liZero = {0, 0};
	LARGE_INTEGER liCurrent;
	SetFilePointerEx(m_hFile, liZero, &liCurrent, FILE_CURRENT);
	m_nBlockOffset = liCurrent.QuadPart;

	DWORD dwRead;
	if (!ReadFile(m_hFile, m_szBuffer+nRest, dwToRead, &dwRead, NULL)) return false;

	m_nSize = dwRead + nRest;
	m_nSizeLimit -= dwRead;

	m_bEOF = (m_nSizeLimit == 0) || (dwRead < dwToRead);

	if (m_pDecoder) {
		INT_PTR nOrigSize = m_nSize;
		if (!m_pDecoder->Decode(m_szBuffer, m_nSize)) return false;
		if (nOrigSize != m_nSize)
			SetFilePointer(m_hFile, m_nSize-nOrigSize, NULL, FILE_CURRENT);
	}

	return true;
}

void CFileBackend::Close()
{
	if (m_hOutFile != INVALID_HANDLE_VALUE) {

		LARGE_INTEGER liZero = {0, 0};
		LARGE_INTEGER liCurrent;
		SetFilePointerEx(m_hFile, liZero, &liCurrent, FILE_CURRENT);
		m_nBlockOffset = liCurrent.QuadPart;

		CatchUpOutput();
		FlushBuffer();

		CloseHandle(m_hOutFile);
		m_hOutFile = INVALID_HANDLE_VALUE;
	}

	if (m_hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	m_pDecoder = NULL;
}

void CFileBackend::Abort()
{
	if (m_hOutFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hOutFile);
		m_hOutFile = INVALID_HANDLE_VALUE;

		DeleteFile(m_strOutFileName.c_str());
	}

	if (m_hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	m_pDecoder = NULL;
}

const char *CFileBackend::Buffer()
{
	return m_pDecoder ? m_pDecoder->Buffer() : m_szBuffer;
}

INT_PTR	CFileBackend::Size()
{
	return m_pDecoder ? m_pDecoder->Size() : m_nSize;
}

const wchar_t *CFileBackend::BufferW()
{
	return (const wchar_t *)Buffer();
}

INT_PTR	CFileBackend::SizeW()
{
	return Size()/2;
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

bool CFileBackend::CheckWriteReady()
{
	if (!OpenOutput()) return false;
	if (!CatchUpOutput()) return false;

	return true;
}

bool CFileBackend::WriteBack(INT_PTR nOffset)
{
	if (!OpenOutput()) return false;
	if (!CatchUpOutput()) return false;

	if (m_pDecoder) {
		nOffset = m_pDecoder->OriginalOffset(nOffset);
		if (nOffset < 0) return false;
	}

	const char *szStart = m_szBuffer;
	szStart +=           m_nBackedUp-m_nBlockOffset;
	nOffset -= (INT_PTR)(m_nBackedUp-m_nBlockOffset);

	if (!BufferOutput(szStart, nOffset)) return false;

	m_nBackedUp += nOffset;

	return true;
}

bool CFileBackend::WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength)
{
	if (!OpenOutput()) return false;
	if (!CatchUpOutput()) return false;

	if (m_pDecoder) {
		INT_PTR nBackedUp = m_pDecoder->DecodedOffset ((INT_PTR)(m_nBackedUp - m_nBlockOffset));
		if (nBackedUp < 0) return false;
		INT_PTR nSkipEnd  = m_pDecoder->OriginalOffset((INT_PTR)(nBackedUp + nSkipLength));
		if (nSkipEnd < 0) return false;

		nSkipLength = nSkipEnd - (INT_PTR)(m_nBackedUp - m_nBlockOffset);
	}

	if ((m_pEncoder == NULL) && (m_pDecoder != NULL))
		m_pEncoder = m_pDecoder->GetEncoder();

	if (m_pEncoder != NULL) {
		m_pEncoder->Decode(szBuffer, nLength);
		szBuffer = m_pEncoder->Buffer();
		nLength  = m_pEncoder->Size();
	}

	if (!BufferOutput(szBuffer, nLength)) return false;

	m_nBackedUp += nSkipLength;

	return true;
}

bool CFileBackend::CatchUpOutput()
{
	if (m_nBackedUp >= m_nBlockOffset) return true;

	LARGE_INTEGER liZero = {0, 0};
	LARGE_INTEGER liCurrent;
	SetFilePointerEx(m_hFile, liZero, &liCurrent, FILE_CURRENT);

	LARGE_INTEGER liStart;
	liStart.QuadPart = m_nBackedUp;
	SetFilePointerEx(m_hFile, liStart, NULL, FILE_BEGIN);

	vector<BYTE> arrBuffer(m_nBlockSize);
	do {
		DWORD dwToRead = I64Ceil(m_nBlockOffset - m_nBackedUp);
		if (dwToRead > ICeil(m_nBlockSize)) dwToRead = ICeil(m_nBlockSize);

		DWORD dwRead;
		if (!ReadFile(m_hFile, &arrBuffer[0], dwToRead, &dwRead, NULL)) break;

		DWORD dwWritten;
		if (!WriteFile(m_hOutFile, &arrBuffer[0], dwRead, &dwWritten, NULL)) break;

		m_nBackedUp += dwToRead;

	} while (m_nBackedUp < m_nBlockOffset);

	SetFilePointerEx(m_hFile, liCurrent, NULL, FILE_BEGIN);

	return true;
}

bool CFileBackend::OpenOutput()
{
	if (m_hOutFile != INVALID_HANDLE_VALUE) return true;

	if (!ConfirmFile(MREReplace, m_strFileName.c_str())) return false;

	m_hOutFile = CreateFile(m_strOutFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	if (m_hOutFile == INVALID_HANDLE_VALUE) return false;

	m_nBackedUp = 0;

	m_arrWriteBuffer.resize(1024*1024);
	m_nBuffered = 0;

	return true;
}

BOOL CFileBackend::BufferOutput(LPCVOID lpBuffer, DWORD dwWrite)
{
	if (m_nBuffered + dwWrite <= m_arrWriteBuffer.size()) {
		memmove(&m_arrWriteBuffer[m_nBuffered], lpBuffer, dwWrite);
		m_nBuffered += dwWrite;
		return TRUE;
	}

	if (!FlushBuffer()) return FALSE;

	if (dwWrite >= m_arrWriteBuffer.size()/16) {
		DWORD dwWritten;
		if (!WriteFile(m_hOutFile, lpBuffer, dwWrite, &dwWritten, NULL)) return FALSE;
		if (dwWritten != dwWrite) return FALSE;
		return TRUE;
	} else {
		return BufferOutput(lpBuffer, dwWrite);
	}
}

BOOL CFileBackend::FlushBuffer()
{
	DWORD dwWritten;

	if (!WriteFile(m_hOutFile, &m_arrWriteBuffer[0], m_nBuffered, &dwWritten, NULL)) return FALSE;
	if (dwWritten != m_nBuffered) return FALSE;
	m_nBuffered = 0;

	return TRUE;
}

LPCTSTR CFileBackend::FileName()
{
	return m_strFileName.c_str();
}
