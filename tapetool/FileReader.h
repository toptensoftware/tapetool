//////////////////////////////////////////////////////////////////////////
// FileReader.h - declaration of CWaveReader class

#ifndef __FILEREADER_H
#define __FILEREADER_H

class CCommandContext;

// Various resolutions at which data can be processed
enum Resolution
{
	resSamples,
	resCycles,
	resCycleKinds,
	resBits,
	resBytes,
};

// Base virtual file reader, derived classes read data from a text or wave file
// This base class provides the routine for converting up from lower resolution format
// to higher level format (eg: samples -> cycles -> bits -> bytes) and for synchronizing
// to bit/bit boundaries
class CFileReader
{
public:
	CFileReader(CCommandContext* ctx);

	virtual bool Open(const char* fileName, Resolution res)=0;
	virtual const char* GetDataFormat()=0;
	virtual Resolution GetResolution()=0;
	virtual void Delete()=0;
	virtual bool IsWaveFile()=0;
	virtual int CurrentPosition()=0;
	virtual int ReadCycleLen()=0;
	virtual char ReadCycleKind()=0;
	virtual void Analyze()=0;
	virtual void Prepare()=0;
	virtual void Seek(int position)=0;
	virtual char* FormatDuration(int duration)=0;
	virtual int LastCycleLen()=0;

	virtual bool SyncToBit(bool verbose);
	virtual int ReadBit(bool verbose = true);
	virtual bool SyncToByte(bool verbose);
	virtual int ReadByte(bool verbose=true);

	char ReadCycleKindChecked(bool verbose);

	CCommandContext* _ctx;
};

#endif	// __FILEREADER_H

