#pragma once

#include <assert.h>

#define assert_even(n) assert((n&1)==0);

//	Everything operates on char * since even for Unicode mode we use UTF-8

class IVirtual
{
public:
	virtual ~IVirtual() {}
};

class IBufferSize : public IVirtual
{
public:
	virtual const char *Buffer() = 0;	//	OEM or UTF-8
	virtual INT_PTR	Size() = 0;			//	Bytes
	virtual const wchar_t *BufferW() { return (const wchar_t *)Buffer(); }	//	Unicode
	virtual INT_PTR	SizeW()          { return Size() / 2; }					//	Characters
};

class IBackend : public IBufferSize
{
public:		//	Search functions
	virtual bool	Last() = 0;
	virtual bool	Move(INT_PTR nLength) = 0;
	virtual INT_PTR	DecodedOffset(INT_PTR nOffset) = 0;
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset) = 0;

public:		//	Replace functions

	//	There is a virtual 'current' position in input buffer
	//	We assume we never go back
	virtual bool	CheckWriteReady() = 0;

	//	Write back a part of input buffer up to this offset
	virtual bool	WriteBack(INT_PTR nOffset) = 0;

	//	Write a new data instead of old one
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength) = 0;

	//	Write a new data after seeking to the end
	virtual bool	AppendData(const char *szBuffer, INT_PTR nLength) = 0;

public:

	virtual LPCTSTR	FileName() = 0;
};

//	Source encoding is determined by created instance
//	Target encoding in OEM for ANSI mode and UTF-8 for Unicode

class IDecoder : public IBufferSize
{
public:
	virtual bool		Decode(const char *szBuffer, INT_PTR &nLength) = 0;

	virtual INT_PTR		DecodedOffset (INT_PTR nOffset) = 0;
	virtual INT_PTR		OriginalOffset(INT_PTR nOffset) = 0;

	virtual IDecoder *	GetEncoder() = 0;
};

class ISplitLineProcessor : public IBufferSize
{
public:
	virtual bool		GetNextLine() = 0;
	virtual INT_PTR		Start() = 0;
	virtual INT_PTR		StartW() { return Start() / 2; }

	virtual bool	WriteBack(INT_PTR nOffset) = 0;
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength) = 0;
	virtual void	SkipTo   (INT_PTR nOffset) = 0;
	virtual void	SkipToW  (INT_PTR nOffset) { SkipTo(nOffset*2); }
};

class IFrontend : public IVirtual
{
public:
	IFrontend() : m_Offset(0), m_Length(0) {}

	virtual bool	Process(IBackend *pBackend) = 0;

	virtual __int64	GetOffset() const { return m_Offset; }
	virtual __int64	GetLength() const { return m_Length; }
protected:
	__int64	m_Offset;
	__int64	m_Length;
};
