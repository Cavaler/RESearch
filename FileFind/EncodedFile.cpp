#include "StdAfx.h"
#include "..\RESearch.h"
#include "EncodedFile.h"

CDelayedEncoder::CDelayedEncoder(const char *szData, DWORD dwSize, CEncoder &pEncoder) : m_szData(szData), m_dwSize(dwSize), m_pEncoder(pEncoder) {
	GetSystemInfo(&m_Info);

	m_dwBufferSize = (DWORD)(m_dwSize*m_pEncoder.SizeIncr());
	m_szBuffer = (char *)VirtualAlloc(NULL, m_dwBufferSize, MEM_RESERVE, PAGE_READWRITE);

	if (m_dwBufferSize >= BLOCK_SIZE)
		ResizeBuffer(BLOCK_SIZE-1);
	else if (m_dwBufferSize > 0)
		ResizeBuffer(m_dwBufferSize-1);

	m_dwCommitRead = 0;
	m_dwCommitWrite = 0;
}

CDelayedEncoder::~CDelayedEncoder() {
	VirtualFree(m_szBuffer, 0, MEM_RELEASE);
}

CDelayedEncoder::operator const char * () {
	return m_szBuffer;
}

DWORD CDelayedEncoder::Size() {
	return m_dwBufferSize;
}

int CDelayedEncoder::ExcFilter(const _EXCEPTION_POINTERS *pExcInfo) {
	if (pExcInfo->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
		return EXCEPTION_EXECUTE_HANDLER;

	const char *szAddress = (const char *)pExcInfo->ExceptionRecord->ExceptionInformation[1];
	if ((szAddress < m_szBuffer+m_dwCommitWrite) || (szAddress >= m_szBuffer+m_dwBufferSize))
		return EXCEPTION_EXECUTE_HANDLER;

	DWORD dwCommitSize = szAddress-m_szBuffer+1;
	ResizeBuffer(dwCommitSize);

	return EXCEPTION_CONTINUE_EXECUTION;
}

bool CDelayedEncoder::ResizeBuffer(DWORD dwCommitSize) {
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
	m_pEncoder.Encode(
		m_szData+m_dwCommitRead,
		(DWORD)(dwEncodeSize/m_pEncoder.SizeIncr()-m_dwCommitRead),
		m_szBuffer+m_dwCommitWrite);
	m_dwCommitRead  = (DWORD)(dwEncodeSize/m_pEncoder.SizeIncr());
	m_dwCommitWrite = dwEncodeSize;

	return true;
}

//////////////////////////////////////////////////////////////////////////

float CNoEncoder::SizeIncr() {
	return 1;
}

void CNoEncoder::Encode(const char *szSource, DWORD dwSize, char *szTarget) {
	memmove(szTarget, szSource, dwSize);
}

//////////////////////////////////////////////////////////////////////////

CToUnicodeEncoder::CToUnicodeEncoder(UINT nCP)
: m_nCP(nCP)
{
}

float CToUnicodeEncoder::SizeIncr() {
	return 2;
}

void CToUnicodeEncoder::Encode(const char *szSource, DWORD dwSize, char *szTarget) {
	MultiByteToWideChar(m_nCP, 0, szSource, dwSize, (LPWSTR)szTarget, dwSize);
}

//////////////////////////////////////////////////////////////////////////

CFromUnicodeEncoder::CFromUnicodeEncoder(UINT nCP)
: m_nCP(nCP)
{
}

float CFromUnicodeEncoder::SizeIncr() {
	return 1/2;
}

void CFromUnicodeEncoder::Encode(const char *szSource, DWORD dwSize, char *szTarget) {
	WideCharToMultiByte(m_nCP, 0, (LPCWSTR)szSource, dwSize, szTarget, dwSize, NULL, NULL);
}

bool RunGrepBuffer(WIN32_FIND_DATA *FindData, panelitem_vector &PanelItems, CDelayedEncoder &Encoder) {
	__try {
//		return GrepBuffer(FindData, PanelItems, (const TCHAR *)(const char *)Encoder, Encoder.Size()/sizeof(TCHAR));
	} __except (Encoder.ExcFilter(GetExceptionInformation())) {
		return false;
	}
}
