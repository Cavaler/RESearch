#ifndef __ENCODEDFILE_H
#define __ENCODEDFILE_H

class CEncoder {
public:
	virtual float SizeIncr() = 0;
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget) = 0;
};

#define BLOCK_SIZE	65536

class CDelayedEncoder {
public:
	CDelayedEncoder(const char *szData, DWORD dwSize, CEncoder &pEncoder);
	~CDelayedEncoder();

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
	CEncoder &m_pEncoder;
};

class CNoEncoder : public CEncoder {
public:
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
};

class CToUnicodeEncoder : public CEncoder {
public:
	CToUnicodeEncoder(UINT nCP);
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
protected:
	UINT m_nCP;
};

class CFromUnicodeEncoder : public CEncoder {
public:
	CFromUnicodeEncoder(UINT nCP);
	virtual float SizeIncr();
	virtual void Encode(const char *szSource, DWORD dwSize, char *szTarget);
protected:
	UINT m_nCP;
};

#endif
