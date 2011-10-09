//////////////////////////////////////////////////////////////////////////
// MachineTypeTrs80.h - declaration of CWaveReader class

#ifndef __MACHINETYPETRS80_H
#define __MACHINETYPETRS80_H

#include "MachineType.h"

class CCommandContext;

class CMachineTypeTrs80 : public CMachineType
{
public:
			CMachineTypeTrs80();
	virtual ~CMachineTypeTrs80();

	virtual const char* GetTapeFormatName() { return "cas"; };
	virtual bool SyncToBit(CFileReader* reader, bool verbose);
	virtual int ReadBit(CFileReader* reader, bool verbose);
	virtual bool SyncToByte(CFileReader* reader, bool verbose);
	virtual int ReadByte(CFileReader* reader, bool verbose);

	virtual void RenderPulse(CWaveWriter* writer);
	virtual void RenderCycleKind(CWaveWriter* writer, char kind);
	virtual void RenderBit(CWaveWriter* writer, unsigned char bit);
	virtual void RenderByte(CWaveWriter* writer, unsigned char byte);

	virtual int ProcessBlocks(CCommandContext* c);

	virtual int CycleFrequency();
	virtual int DcOffset(CWaveReader* wave);

	bool ProcessSystemBlock(CCommandContext* c, bool verbose);
	bool ProcessSourceBlock(CCommandContext* c, bool verbose);
	bool ProcessBasicBlock(CCommandContext* c, bool verbose);

	int _syncBytePosition;
	bool _eof;
	unsigned char _blockData[300];
	int _blockDataLen;
	bool _writeBinaryData;
};

#endif	// __MACHINETYPETRS80_H

