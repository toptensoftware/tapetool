//////////////////////////////////////////////////////////////////////////
// MachineTypeTrs80.h - declaration of CTapeReader class

#ifndef __MACHINETYPETRS80_H
#define __MACHINETYPETRS80_H

#include "MachineType.h"

class CContext;

class CMachineTypeTrs80 : public CMachineType
{
public:
			CMachineTypeTrs80();
	virtual ~CMachineTypeTrs80();

	virtual const char* GetTapeFormatName() { return "cas"; };

	virtual void PrepareWaveMetrics(CCommandStd* c, CTapeReader* wf);

	virtual bool SyncToBit(CFileReader* reader, bool verbose);
	virtual int ReadBit(CFileReader* reader, bool verbose);
	virtual bool SyncToByte(CFileReader* reader, bool verbose);
	virtual int ReadByte(CFileReader* reader, bool verbose);

	virtual void RenderPulse(CWaveWriter* writer);
	virtual void RenderCycleKind(CWaveWriter* writer, char kind);
	virtual void RenderBit(CWaveWriter* writer, unsigned char bit);
	virtual void RenderByte(CWaveWriter* writer, unsigned char byte);

	virtual int ProcessBlocks(CCommandStd* c);

	bool ProcessSystemBlock(CCommandStd* c, bool verbose);
	bool ProcessSourceBlock(CCommandStd* c, bool verbose);
	bool ProcessBasicBlock(CCommandStd* c, bool verbose);

	int _syncBytePosition;
	bool _eof;
	unsigned char _blockData[300];
	int _blockDataLen;
	bool _writeBinaryData;
};

#endif	// __MACHINETYPETRS80_H

