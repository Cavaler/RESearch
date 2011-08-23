#pragma once

//	Everything operates on char * since even for Unicode mode we use UTF-8

class IBackend
{
public:		//	Search functions
	virtual char *	Buffer() = 0;
	virtual INT_PTR	Size() = 0;
	virtual bool	Last() = 0;

	virtual bool	Move(INT_PTR nLength) = 0;

public:		//	Replace functions

	//	There is a virtual 'current' position in input buffer
	//	We assume we never go back

	//	Write back a part of input buffer
	virtual bool	WriteBack(INT_PTR nLength) = 0;

	//	Write a new data instead of old one
	virtual bool	WriteThru(const char *szBuffer, INT_PTR nLength, INT_PTR nSkipLength) = 0;
};

//	Source encoding is determined by created instance
//	Target encoding in OEM for ANSI mode and UTF-8 for Unicode

class IEncoder
{
public:		//	Decoding functions

	virtual bool	Decode(const char *szBuffer, INT_PTR &nLength) = 0;
	virtual char *	Buffer() = 0;
	virtual INT_PTR	Size() = 0;

	virtual INT_PTR	DecodedOffset (INT_PTR nOffset) = 0;
	virtual INT_PTR	OriginalOffset(INT_PTR nOffset) = 0;

public:		//	Encoding functions
			//	To encode, clone a decoder and use same Buffer() / Size()

	virtual IEncoder *Clone() = 0;
	virtual bool	Encode(const char *szBuffer, INT_PTR &nLength) = 0;

};

class IProcessor
{
public:
};

class IFrontend
{
public:
	virtual bool	Process(IBackend *pBackend) = 0;
};
