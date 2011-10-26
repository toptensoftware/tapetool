//////////////////////////////////////////////////////////////////////////
// MachineType.h - declaration of CTapeReader class

#ifndef __MACHINETYPEGENERIC_H
#define __MACHINETYPEGENERIC_H

#include "MachineType.h"
#include "FileReader.h"
#include "CommandBytes.h"
#include "CommandStd.h"

class CMachineTypeGeneric : public CMachineType
{
public:
			CMachineTypeGeneric();
	virtual ~CMachineTypeGeneric();

	virtual bool IsGeneric() { return true; }

	virtual bool OnPreProcess(CCommandStd* c, Resolution res);

	virtual const char* GetTapeFormatName()
	{
		return NULL;
	}

	virtual void PrepareWaveMetrics(CCommandStd* c, CTapeReader* wf)
	{
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

	virtual int ProcessBlocks(CCommandStd* c)
	{
		CCommandBytes del(c->_ctx);
		return del.Process();
	}

	virtual bool CanRenderSquare() { return false; }
};

#endif	// __MACHINETYPEGENERIC_H

