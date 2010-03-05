#include "StdAfx.h"
#include "..\RESearch.h"
#include "EncodedFile.h"

CDelayedDecoder::CDelayedDecoder(const char *szData, DWORD dwSize, CDecoder &pDecoder) : m_szData(szData), m_dwSize(dwSize), m_pDecoder(pDecoder) {
	GetSystemInfo(&m_Info);

	m_dwBufferSize = (DWORD)(m_dwSize*m_pDecoder.SizeIncr());
	m_szBuffer = (char *)VirtualAlloc(NULL, m_dwBufferSize, MEM_RESERVE, PAGE_READWRITE);

	if (m_dwBufferSize >= BLOCK_SIZE)
		ResizeBuffer(BLOCK_SIZE-1);
	else if (m_dwBufferSize > 0)
		ResizeBuffer(m_dwBufferSize-1);

	m_dwCommitRead = 0;
	m_dwCommitWrite = 0;
}

CDelayedDecoder::~CDelayedDecoder() {
	VirtualFree(m_szBuffer, 0, MEM_RELEASE);
}

CDelayedDecoder::operator const char * () {
	return m_szBuffer;
}

DWORD CDelayedDecoder::Size() {
	return m_dwBufferSize;
}

int CDelayedDecoder::ExcFilter(const _EXCEPTION_POINTERS *pExcInfo) {
	if (pExcInfo->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
		return EXCEPTION_EXECUTE_HANDLER;

	const char *szAddress = (const char *)pExcInfo->ExceptionRecord->ExceptionInformation[1];
	if ((szAddress < m_szBuffer+m_dwCommitWrite) || (szAddress >= m_szBuffer+m_dwBufferSize))
		return EXCEPTION_EXECUTE_HANDLER;

	DWORD dwCommitSize = szAddress-m_szBuffer+1;
	ResizeBuffer(dwCommitSize);

	return EXCEPTION_CONTINUE_EXECUTION;
}

bool CDelayedDecoder::ResizeBuffer(DWORD dwCommitSize) {
	//	Round up to page size
	dwCommitSize = ((dwCommitSize+BLOCK_SIZE-1)/BLOCK_SIZE)*BLOCK_SIZE;
	if (dwCommitSize > m_dwBufferSize) dwCommitSize = m_dwBufferSize;
	dwCommitSize = ((dwCommitSize+m_Info.dwPageSize-1)/m_Info.dwPageSize)*m_Info.dwPageSize;

	//	Allocate
	if (VirtualAlloc(m_szBuffer, dwCommitSize, MEM_COMMIT, PAGE_READWRITE) ==  NULL)
		return false;

	//	Encode
	DWORD dwEncodeSize = dwCommitSize;
	if (dwEncodeSize > m_dwBufferSize) dwEncodeSize = m_dwBufferSize;
	m_pDecoder.Encode(
		m_szData+m_dwCommitRead,
		(DWORD)(dwEncodeSize/m_pDecoder.SizeIncr()-m_dwCommitRead),
		m_szBuffer+m_dwCommitWrite);
	m_dwCommitRead  = (DWORD)(dwEncodeSize/m_pDecoder.SizeIncr());
	m_dwCommitWrite = dwEncodeSize;

	return true;
}

//////////////////////////////////////////////////////////////////////////

float CNoDecoder::SizeIncr() {
	return 1;
}

void CNoDecoder::Encode(const char *szSource, DWORD dwSize, char *szTarget) {
	memmove(szTarget, szSource, dwSize);
}

//////////////////////////////////////////////////////////////////////////

CToUnicodeDecoder::CToUnicodeDecoder(UINT nCP)
: m_nCP(nCP)
{
}

float CToUnicodeDecoder::SizeIncr() {
	return 2;
}

void CToUnicodeDecoder::Encode(const char *szSource, DWORD dwSize, char *szTarget) {
	MultiByteToWideChar(m_nCP, 0, szSource, dwSize, (LPWSTR)szTarget, dwSize);
}

//////////////////////////////////////////////////////////////////////////

CFromUnicodeDecoder::CFromUnicodeDecoder(UINT nCP)
: m_nCP(nCP)
{
}

float CFromUnicodeDecoder::SizeIncr() {
	return 1/2;
}

void CFromUnicodeDecoder::Encode(const char *szSource, DWORD dwSize, char *szTarget) {
	WideCharToMultiByte(m_nCP, 0, (LPCWSTR)szSource, dwSize, szTarget, dwSize, NULL, NULL);
}

//////////////////////////////////////////////////////////////////////////

CTableDecoder::CTableDecoder(TCHAR *szTable)
: m_szTable(szTable)
{
}

float CTableDecoder::SizeIncr() {
	return 1;
}

void CTableDecoder::Encode(const char *szSource, DWORD dwSize, char *szTarget)
{
#ifdef UNICODE
	const wchar_t *wszSource = (const wchar_t *)szSource;
	wchar_t *wszTarget = (wchar_t *)szTarget;
	for (DWORD nChar = 0; nChar < dwSize/2; nChar++) {
		*(wszTarget++) = m_szTable[(USHORT)wszSource[nChar]);
	}
#else
	for (DWORD nChar = 0; nChar < dwSize; nChar++)
		*(szTarget++) = m_szTable[(BYTE)szSource[nChar]];
#endif
}

//////////////////////////////////////////////////////////////////////////

template<class ret>
ret RunDecoder(CDelayedDecoder &Decoder, ret (*Processor)(const TCHAR *, DWORD)) {
	__try {
		return Processor((const TCHAR *)(const char *)Decoder, Decoder.Size()/sizeof(TCHAR));
	} __except (Decoder.ExcFilter(GetExceptionInformation())) {
		return false;
	}
}
