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
	virtual char *	Buffer();
	virtual INT_PTR	Size();
	virtual bool	Last();
	virtual bool	Move(INT_PTR nLength);
	virtual bool	WriteBack(INT_PTR nLength);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);

protected:
	HANDLE		m_hFile;
	bool		m_bEOF;
	bool		ReadUp(INT_PTR nRest);

	INT_PTR		m_nBlockSize;
	IDecoder   *m_pDecoder;

	char	   *m_szBuffer;
	INT_PTR		m_nSize;
};
