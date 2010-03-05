#ifndef __ENCODEDFILE_H
#define __ENCODEDFILE_H

class CDecoder {
public:
	virtual float SizeIncr() = 0;
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget) = 0;
};

#define BLOCK_SIZE	65536

class CDelayedDecoder {
public:
	CDelayedDecoder(const char *szData, DWORD dwSize, CDecoder &pDecoder);
	~CDelayedDecoder();

	operator const char * ();
	DWORD Size();

	int ExcFilter(const _EXCEPTION_POINTERS *pExcInfo);

protected:
	bool ResizeBuffer(DWORD dwCommitSize);

	const char *m_szData;
	DWORD m_dwSize;

	char *m_szBuffer;
	DWORD m_dwBufferSize;

	DWORD m_dwCommitRead, m_dwCommitWrite;

	SYSTEM_INFO m_Info;
	CDecoder &m_pDecoder;
};

class CNoDecoder : public CDecoder {
public:
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
};

class CToUnicodeDecoder : public CDecoder {
public:
	CToUnicodeDecoder(UINT nCP);
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
protected:
	UINT m_nCP;
};

class CFromUnicodeDecoder : public CDecoder {
public:
	CFromUnicodeDecoder(UINT nCP, bool bLE = true);
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
protected:
	UINT m_nCP;
	bool m_bLE;
};

class CByteSwapDecoder : public CDecoder {
public:
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
};

class CToOEMDecoder : public CDecoder {
public:
	CToOEMDecoder(UINT nCP);
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
protected:
	UINT m_nCP;
};

#ifndef UNICODE
class CTableDecoder : public CDecoder {
public:
	CTableDecoder(BYTE *szTable);
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
protected:
	BYTE *m_szTable;
};
#endif

//////////////////////////////////////////////////////////////////////////

template<class CHAR>
class CEncodedFile {
public:
	CEncodedFile(LPCTSTR szFileName);
	CEncodedFile(BYTE *pData, DWORD dwSize);
	bool Valid();

	void SetSourceUnicode(bool bLE = true);
	void SetSourceDetect(eLikeUnicode nDetect);
	void SetSourceUTF8();
	void SetSourceCP(UINT nCP);
#ifndef UNICODE
	void SetSourceTable(BYTE *szTable);
#endif

	~CEncodedFile();

	static const bool Unicode = (sizeof(CHAR) == 2);
	operator const CHAR *();
	DWORD Size();

	bool Run(bool (*Processor)(const CHAR *szData, DWORD dwSize));

	template<class param1> bool Run(bool (*Processor)(param1 Param1, const CHAR *szData, int dwSize), param1 Param1) {
		__try {
			return Processor(Param1, *this, Size());
		} __except (m_pDD->ExcFilter(GetExceptionInformation())) {
			return false;
		}
	}

	template<class param1, class param2> bool Run(bool (*Processor)(param1 Param1, param2 Param2, const CHAR *szData, int dwSize), param1 Param1, param2 Param2) {
		__try {
			return Processor(Param1, Param2, *this, Size());
		} __except (m_pDD->ExcFilter(GetExceptionInformation())) {
			return false;
		}
	}

protected:
	BYTE *m_pData;
	DWORD m_dwSize;

	const BYTE *m_pDecodedData;
	DWORD m_dwDecodedSize;

	CFileMapping m_mapFile;

	CDelayedDecoder	*m_pDD;
	CDecoder		*m_pDec;
	void SetDecoder(CDecoder *pDec);
	void ClearDecoders();

	vector<CHAR>	m_arrBuffered;
};

typedef CEncodedFile<char>    CEncodedFileA;
typedef CEncodedFile<wchar_t> CEncodedFileW;
typedef CEncodedFile<TCHAR>   CEncodedFileT;

#endif
