//////////////////////////////////////////////////////////////////////////
// BinaryReader.h - declaration of CTapeReader class

#ifndef __BINARYREADER_H
#define __BINARYREADER_H

#include "FileReader.h"

// CBinaryReader - reads data from a previously generated text file
class CBinaryReader : public CFileReader
{
public:
			CBinaryReader(CCommandStd* cmc);
	virtual ~CBinaryReader();

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

	FILE* _file;
	int _length;
	const char* _dataFormat;
};


#endif	// __BINARYREADER_H

