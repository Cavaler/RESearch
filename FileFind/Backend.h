#pragma once
#include "IFileOperations.h"

class CFileBackend : public IBackend
{
public:
	CFileBackend();
	~CFileBackend();

	//	Methods ordered by life cycle
	bool SetBlockSize(INT_PTR nBlockSize);

	bool Open(LPCTSTR szFileName, INT_PTR nMaxSize = -1);
	bool Open(LPCTSTR szInFileName, LPCTSTR szOutFileName);

	bool SetDecoder(IDecoder *pDecoder, INT_PTR nSkip = 0);
	IDecoder *GetDecoder();
	bool ResetDecoder(IDecoder *pDecoder);
	void ClearDecoder();

	void Close();
	void Abort();

	void Done();

public:
	virtual const char *Buffer();
	virtual INT_PTR	Size();
	virtual bool	Last();
	virtual bool	Move(INT_PTR nLength);
	virtual INT_PTR	DecodedOffset(INT_PTR nOffset);
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset);
	virtual bool	CheckWriteReady();
	virtual bool	WriteBack(INT_PTR nOffset);
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength);
	virtual bool	AppendData(const char *szBuffer, INT_PTR nLength);
	virtual LPCTSTR	FileName();

	static void Free();

protected:
	tstring		m_strFileName;
	__int64		m_nOriginalSizeLimit;

	HANDLE		m_hFile;
	__int64		m_nSizeLimit;
	bool		m_bEOF;
	bool		OpenInputFile(LPCTSTR szFileName, bool bShareWrite);
	void 		CloseInputFile();
	bool		ReadUp(INT_PTR nRest, INT_PTR nMax = 0);

	bool		m_bSlurpMode;
	void		InitSlurpMode();

	bool		OpenOutput();
	bool		CatchUpOutput();
	bool		FlushOutputToEnd();

	static char	   *m_szBuffer;
	static INT_PTR	m_nBlockSize;
	IDecoder   *m_pDecoder;
	INT_PTR		m_nSkip;

	__int64		m_nBlockOffset;
	INT_PTR		m_nSize;

	tstring		m_strOutFileName;
	HANDLE		m_hOutFile;
	__int64		m_nBackedUp;
	IDecoder   *m_pEncoder;

	vector<BYTE> m_arrWriteBuffer;
	INT_PTR		m_nBuffered;
	bool		BufferOutput(LPCVOID lpBuffer, DWORD dwWrite);
	bool		FlushBuffer();
};
