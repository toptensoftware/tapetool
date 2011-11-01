//////////////////////////////////////////////////////////////////////////
// FileReader.h - declaration of CTapeReader class

#ifndef __FILEREADER_H
#define __FILEREADER_H

class CInstrumentation;
class CCommandStd;

// Various resolutions at which data can be processed
enum Resolution
{
	resNA,
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
	CFileReader(CCommandStd* cmd);

	virtual bool Open(const char* fileName, Resolution res)=0;
	virtual const char* GetDataFormat()=0;
	virtual Resolution GetResolution()=0;
	virtual void Delete()=0;
	virtual bool IsWaveFile()=0;
	virtual int CurrentPosition()=0;
	virtual int ReadCycleLen()=0;
	virtual char ReadCycleKind()=0;
	virtual void Seek(int position)=0;
	virtual char* FormatDuration(int duration)=0;
	virtual int LastCycleLen()=0;
	virtual void Prepare() {};

	virtual bool SyncToBit(bool verbose);
	virtual int ReadBit(bool verbose = true);
	virtual bool SyncToByte(bool verbose);
	virtual int ReadByte(bool verbose=true);
	virtual CInstrumentation* GetInstrumentation() { return NULL; }

	char ReadCycleKindChecked(bool verbose);

	CCommandStd* _cmd;
};

#endif	// __FILEREADER_H

