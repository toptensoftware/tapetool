//////////////////////////////////////////////////////////////////////////
// MachineType.h - declaration of CTapeReader class

#ifndef __MACHINETYPE_H
#define __MACHINETYPE_H

#include "FileReader.h"

class CWaveWriter;
class CTapeReader;
class CContext;

// Command handler
typedef int (*fnCmd)(CContext*);

class CMachineType
{
public:
			CMachineType();
	virtual ~CMachineType();

	virtual bool IsGeneric() { return true; }

	virtual const char* GetTapeFormatName()=0;
	virtual bool OnPreProcess(CCommandStd* c, Resolution res) { return true; };
	virtual CFileReader* CreateFileReader(CCommandStd* ctx, const char* pszExt) { return NULL; };

	virtual void PrepareWaveMetrics(CCommandStd* c, CTapeReader* wave)=0;

	virtual bool SyncToBit(CFileReader* reader, bool verbose)=0;
	virtual int ReadBit(CFileReader* reader, bool verbose)=0;
	virtual bool SyncToByte(CFileReader* reader, bool verbose)=0;
	virtual int ReadByte(CFileReader* reader, bool verbose)=0;

	virtual void RenderCycleKind(CWaveWriter* writer, char kind)=0;
	virtual void RenderBit(CWaveWriter* writer, unsigned char bit)=0;
	virtual void RenderByte(CWaveWriter* writer, unsigned char byte)=0;

	virtual int ProcessBlocks(CCommandStd* c)=0;

	virtual bool CanRenderSquare() { return false; }
};

#endif	// __MACHINETYPE_H

