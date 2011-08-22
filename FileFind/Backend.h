#pragma once
#include "IFileOperations.h"

class CFileBackend : public IBackend
{
public:
	CFileBackend();
	~CFileBackend();

	bool Init(INT_PTR nBlockSize, IEncoder *pEncoder);
	void Done();

	bool Open(LPCTSTR szFileName);
	void Close();

public:
	virtual char *	Buffer();
	virtual INT_PTR	Size();
	virtual bool	Last();
	virtual bool	Move(INT_PTR nLength);
	virtual bool	WriteBack(INT_PTR nLength);
	virtual bool	WriteThru(char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);

protected:
	HANDLE		m_hFile;

	INT_PTR		m_nBlockSize;
	IEncoder   *m_pEncoder;

	char	   *m_szBuffer;
	INT_PTR		m_nSize;
};
