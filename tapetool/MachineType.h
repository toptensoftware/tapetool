//////////////////////////////////////////////////////////////////////////
// MachineType.h - declaration of CWaveReader class

#ifndef __MACHINETYPE_H
#define __MACHINETYPE_H

#include "FileReader.h"

class CWaveWriter;
class CWaveReader;
class CCommandContext;

// Command handler
typedef int (*fnCmd)(CCommandContext*);

class CMachineType
{
public:
			CMachineType();
	virtual ~CMachineType();

	virtual bool IsGeneric() { return true; }

	virtual const char* GetTapeFormatName()=0;
	virtual bool OnPreProcess(CCommandContext* c, Resolution res) { return true; };
	virtual CFileReader* CreateFileReader(CCommandContext* ctx, const char* pszExt) { return NULL; };

	virtual bool SyncToBit(CFileReader* reader, bool verbose)=0;
	virtual int ReadBit(CFileReader* reader, bool verbose)=0;
	virtual bool SyncToByte(CFileReader* reader, bool verbose)=0;
	virtual int ReadByte(CFileReader* reader, bool verbose)=0;

	virtual void RenderCycleKind(CWaveWriter* writer, char kind)=0;
	virtual void RenderBit(CWaveWriter* writer, unsigned char bit)=0;
	virtual void RenderByte(CWaveWriter* writer, unsigned char byte)=0;

	virtual int ProcessBlocks(CCommandContext* c)=0;

	virtual int CycleFrequency()=0;
	virtual int DcOffset(CWaveReader* wave)=0;

	virtual bool CanRenderSquare() { return false; }
};

#endif	// __MACHINETYPE_H

