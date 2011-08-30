#pragma once
#include "IFileOperations.h"

class CFileBackend : public IBackend
{
public:
	CFileBackend();
	~CFileBackend();

	//	Methods ordered by life cycle
	bool Init(INT_PTR nBlockSize);

	bool Open(LPCTSTR szFileName, INT_PTR nMaxSize = -1);
	bool Open(LPCTSTR szInFileName, LPCTSTR szOutFileName);

	bool SetDecoder(IDecoder *pDecoder, INT_PTR nSkip = 0);

	void Close();

	void Done();

public:
	virtual const char *Buffer();
	virtual const wchar_t *BufferW();
	virtual INT_PTR	Size();
	virtual INT_PTR	SizeW();
	virtual bool	Last();
	virtual bool	Move(INT_PTR nLength);
	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);
	virtual LPCTSTR	FileName();

	static void Free();

protected:
	tstring		m_strFileName;
	__int64		m_nOriginalSizeLimit;

	HANDLE		m_hFile;
	__int64		m_nSizeLimit;
	bool		m_bEOF;
	bool		ReadUp(INT_PTR nRest);

	bool		OpenOutput();
	bool		CatchUpOutput();

	static char	   *m_szBuffer;
	static INT_PTR	m_nBlockSize;
	IDecoder   *m_pDecoder;

	__int64		m_nBlockOffset;
	INT_PTR		m_nSize;

	tstring		m_strOutFileName;
	HANDLE		m_hOutFile;
	__int64		m_nBackedUp;
	IDecoder   *m_pEncoder;
};
