#include "StdAfx.h"
#include "..\RESearch.h"

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

CFromUnicodeDecoder::CFromUnicodeDecoder(UINT nCP, bool bLE)
: m_nCP(nCP), m_bLE(bLE)
{
}

float CFromUnicodeDecoder::SizeIncr() {
	return 1/2;
}

void CFromUnicodeDecoder::Encode(const char *szSource, DWORD dwSize, char *szTarget) {
	dwSize /= 2;	// bytes to chars

	if (m_bLE) {
		WideCharToMultiByte(m_nCP, 0, (LPCWSTR)szSource, dwSize, szTarget, dwSize, NULL, NULL);
	} else {
		for (size_t nChar = 0; nChar < dwSize; nChar++) {
			WCHAR wsz = (((BYTE)szSource[nChar*2]) << 8) + (BYTE)szSource[nChar*2+1];
			WideCharToMultiByte(m_nCP, 0, &wsz, 1, szTarget+nChar, 1, NULL, NULL);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

float CByteSwapDecoder::SizeIncr() {
	return 1;
}

void CByteSwapDecoder::Encode(const char *szSource, DWORD dwSize, char *szTarget) {
	dwSize /= 2;	// bytes to chars

	for (size_t nChar = 0; nChar < dwSize; nChar++) {
		szTarget[nChar*2  ] = szSource[nChar*2+1];
		szTarget[nChar*2+1] = szSource[nChar*2  ];
	}
}


//////////////////////////////////////////////////////////////////////////

CToOEMDecoder::CToOEMDecoder(UINT nCP)
: m_nCP(nCP)
{
}

float CToOEMDecoder::SizeIncr() {
	return 1;
}

void CToOEMDecoder::Encode(const char *szSource, DWORD dwSize, char *szTarget) {
	vector<wchar_t> arrTemp(dwSize);
	MultiByteToWideChar(m_nCP, 0, szSource, dwSize, &arrTemp[0], dwSize);
	WideCharToMultiByte(CP_OEMCP, 0, &arrTemp[0], dwSize, szTarget, dwSize, NULL, NULL);
}

//////////////////////////////////////////////////////////////////////////

#ifndef UNICODE

CTableDecoder::CTableDecoder(BYTE *szTable)
: m_szTable(szTable)
{
}

float CTableDecoder::SizeIncr() {
	return 1;
}

void CTableDecoder::Encode(const char *szSource, DWORD dwSize, char *szTarget)
{
	for (DWORD nChar = 0; nChar < dwSize; nChar++)
		*(szTarget++) = m_szTable[(BYTE)szSource[nChar]];
}

#endif

//////////////////////////////////////////////////////////////////////////

template class CEncodedFile<char>;
template class CEncodedFile<wchar_t>;

template<class CHAR> CEncodedFile<CHAR>::CEncodedFile(LPCTSTR szFileName)
: m_pDD(NULL)
, m_pDec(NULL)
, m_pDecodedData(NULL)
, m_dwDecodedSize(0)
{
	m_mapFile.Open(szFileName);
	m_pData = m_mapFile;
	m_dwSize = m_mapFile.Size();
}

template<class CHAR> CEncodedFile<CHAR>::CEncodedFile(BYTE *pData, DWORD dwSize)
: m_pDD(NULL)
, m_pDec(NULL)
, m_pDecodedData(NULL)
, m_dwDecodedSize(0)
{
	m_pData = pData;
	m_dwSize = dwSize;
}

template<class CHAR> bool CEncodedFile<CHAR>::Valid() {
	return m_pData != NULL;
}

template<class CHAR> void CEncodedFile<CHAR>::SetSourceUnicode(bool bLE) {
	ClearDecoders();
	if (Unicode) {
		if (bLE) {
			m_pDecodedData  = m_pData;
			m_dwDecodedSize = m_dwSize/sizeof(CHAR);
		} else {
			SetDecoder(new CByteSwapDecoder());
		}
	} else {
		SetDecoder(new CFromUnicodeDecoder(CP_OEMCP, bLE));
	}
}

template<class CHAR> void CEncodedFile<CHAR>::SetSourceDetect(eLikeUnicode nDetect) {
	switch (nDetect) {
	case UNI_LE:
		SetSourceUnicode(true);
		break;
	case UNI_BE:
		SetSourceUnicode(false);
		break;
	default:
		ClearDecoders();
	}
}

template<class CHAR> void CEncodedFile<CHAR>::SetSourceUTF8() {
	ClearDecoders();
	FromUTF8((const char *)m_pData, m_dwSize, m_arrBuffered);

	if (m_arrBuffered.size() > 0) {
		m_pDecodedData  = (const BYTE *)&m_arrBuffered[0];
		m_dwDecodedSize = m_arrBuffered.size();
	}
}

template<class CHAR> void CEncodedFile<CHAR>::SetSourceCP(UINT nCP) {
	ClearDecoders();
	if (Unicode) {
		SetDecoder(new CToUnicodeDecoder(nCP));
	} else {
		if (IsDefCP(nCP)) {
			m_pDecodedData  = m_pData;
			m_dwDecodedSize = m_dwSize/sizeof(CHAR);
		} else {
			SetDecoder(new CToOEMDecoder(nCP));
		}
	}
}

#ifndef UNICODE
template<class CHAR> void CEncodedFile<CHAR>::SetSourceTable(BYTE *szTable) {
	ClearDecoders();
	SetDecoder(new CTableDecoder(szTable));
}
#endif

template<class CHAR> void CEncodedFile<CHAR>::SetDecoder(CDecoder *pDec) {
	m_pDec = pDec;
	m_pDD = new CDelayedDecoder((const char *)m_pData, m_dwSize, *m_pDec);
	m_pDecodedData  = (const BYTE *)(const char *)*m_pDD;
	m_dwDecodedSize = m_pDD->Size()/sizeof(CHAR);
}

template<class CHAR> void CEncodedFile<CHAR>::ClearDecoders() {
	if (m_pDD) {
		delete m_pDD;
		m_pDD = NULL;
	}
	if (m_pDec) {
		delete m_pDec;
		m_pDec = NULL;
	}

	m_arrBuffered.clear();
	m_pDecodedData  = NULL;
	m_dwDecodedSize = 0;
}

template<class CHAR> CEncodedFile<CHAR>::~CEncodedFile() {
	ClearDecoders();
}

template<class CHAR> CEncodedFile<CHAR>::operator const CHAR *() {
	return (const CHAR *)m_pDecodedData;
}

template<class CHAR> DWORD CEncodedFile<CHAR>::Size() {
	return m_dwDecodedSize;
}

template<class CHAR> bool CEncodedFile<CHAR>::Run(bool (*Processor)(const CHAR *szData, DWORD dwSize)) {
	__try {
		return Processor(*this, Size());
	} __except (m_pDD->ExcFilter(GetExceptionInformation())) {
		return false;
	}
}
