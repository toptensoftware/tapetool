//////////////////////////////////////////////////////////////////////////
// MachineType.h - declaration of CWaveReader class

#ifndef __MACHINETYPEGENERIC_H
#define __MACHINETYPEGENERIC_H

#include "MachineType.h"
#include "FileReader.h"

int ProcessBytes(CCommandContext* c);

class CMachineTypeGeneric : public CMachineType
{
public:
			CMachineTypeGeneric();
	virtual ~CMachineTypeGeneric();

	virtual bool IsGeneric() { return true; }

	virtual bool OnPreProcess(CCommandContext* c, Resolution res);

	virtual const char* GetTapeFormatName()
	{
		return NULL;
	}

	virtual bool SyncToBit(CFileReader* reader, bool verbose)
	{
		return false;
	}

	virtual int ReadBit(CFileReader* reader, bool verbose)
	{
		return -1;
	}

	virtual bool SyncToByte(CFileReader* reader, bool verbose)
	{
		return false;
	}

	virtual int ReadByte(CFileReader* reader, bool verbose)
	{
		return -1;
	}

	virtual void RenderCycleKind(CWaveWriter* writer, char kind)
	{
	}

	virtual void RenderBit(CWaveWriter* writer, unsigned char bit)
	{
	}

	virtual void RenderByte(CWaveWriter* writer, unsigned char byte)
	{
	}

	virtual int ProcessBlocks(CCommandContext* c)
	{
		return ProcessBytes(c);
	}

	virtual int CycleFrequency()
	{
		return 1;
	}

	virtual int DcOffset(CWaveReader* wave)
	{
		return 0;
	}

	virtual bool CanRenderSquare() { return false; }
};

#endif	// __MACHINETYPEGENERIC_H

