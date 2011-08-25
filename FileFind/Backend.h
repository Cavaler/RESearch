#pragma once
#include "IFileOperations.h"

class CFileBackend : public IBackend
{
public:
	CFileBackend();
	~CFileBackend();

	//	Methods ordered by life cycle
	bool Init(INT_PTR nBlockSize);

	bool Open(LPCTSTR szFileName);

	bool SetDecoder(IDecoder *pDecoder, INT_PTR nSkip = 0);

	void Close();

	void Done();

public:
	virtual const char *Buffer();
	virtual INT_PTR	Size();
	virtual bool	Last();
	virtual bool	Move(INT_PTR nLength);
	virtual bool	WriteBack(INT_PTR nLength);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);

	static void Free();

protected:
	HANDLE		m_hFile;
	bool		m_bEOF;
	bool		ReadUp(INT_PTR nRest);

	static char	   *m_szBuffer;
	static INT_PTR	m_nBlockSize;
	IDecoder   *m_pDecoder;

	INT_PTR		m_nSize;
};
