#pragma once

//	Everything operates on char * since even for Unicode mode we use UTF-8

class IBackend
{
public:		//	Search functions
	virtual const char *Buffer() = 0;	//	OEM or UTF-8
	virtual INT_PTR	Size() = 0;			//	Bytes
	virtual const wchar_t *BufferW() = 0;	//	Unicode
	virtual INT_PTR	SizeW() = 0;			//	Characters
	virtual bool	Last() = 0;

	virtual bool	Move(INT_PTR nLength) = 0;

public:		//	Replace functions

	//	There is a virtual 'current' position in input buffer
	//	We assume we never go back

	//	Write back a part of input buffer up to this offset
	virtual bool	WriteBack(INT_PTR nOffset) = 0;

	//	Write a new data instead of old one
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength) = 0;

public:

	virtual LPCTSTR	FileName() = 0;
};

//	Source encoding is determined by created instance
//	Target encoding in OEM for ANSI mode and UTF-8 for Unicode

class IDecoder
{
public:
	virtual bool		Decode(const char *szBuffer, INT_PTR &nLength) = 0;
	virtual const char *Buffer() = 0;
	virtual INT_PTR		Size() = 0;

	virtual INT_PTR		DecodedOffset (INT_PTR nOffset) = 0;
	virtual INT_PTR		OriginalOffset(INT_PTR nOffset) = 0;

	virtual IDecoder *	GetEncoder() = 0;
};

class ISplitLineProcessor
{
public:
	virtual bool		GetNextLine() = 0;
	virtual const char *Buffer() = 0;
	virtual INT_PTR		Size() = 0;

	virtual bool	WriteBack(INT_PTR nOffset) = 0;
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength) = 0;
};

class IFrontend
{
public:
	virtual bool	Process(IBackend *pBackend) = 0;
};
