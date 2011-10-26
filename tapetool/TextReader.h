//////////////////////////////////////////////////////////////////////////
// TextReader.h - declaration of CTapeReader class

#ifndef __TEXTREADER_H
#define __TEXTREADER_H

#include "FileReader.h"

// CTextReader - reads data from a previously generated text file
class CTextReader : public CFileReader
{
public:
			CTextReader(CCommandStd* ctx);
	virtual ~CTextReader();


	void InitBuffer();

	Resolution _res;
	int _currentPosition;
	unsigned char* _buffer;
	int _bufferSize;
	int _dataLength;
	char _dataFormat[64];

	void QueueData(unsigned char b);
	void EnsureResolution(Resolution resRequired);
	void QueueCycleKind(char kind);
	void QueueBit(unsigned char bit);
	void QueueByte(unsigned char byte);

	virtual bool Open(const char* filename, Resolution res);
	virtual const char* GetDataFormat();
	virtual Resolution GetResolution();
	virtual void Delete();
	virtual bool IsWaveFile();
	virtual int CurrentPosition();
	virtual int ReadCycleLen();
	virtual char ReadCycleKind();
	virtual void Seek(int position);
	virtual char* FormatDuration(int duration);
	virtual int LastCycleLen();
	virtual bool SyncToBit(bool verbose);
	virtual int ReadBit(bool verbose = true);
	virtual bool SyncToByte(bool verbose);
	virtual int ReadByte(bool verbose=true);
};


#endif	// __TEXTREADER_H

