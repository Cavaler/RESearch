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
	CFromUnicodeDecoder(UINT nCP);
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
protected:
	UINT m_nCP;
};

class CTableDecoder : public CDecoder {
public:
	CTableDecoder(TCHAR *szTable);
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
protected:
	TCHAR *m_szTable;
};

#endif
