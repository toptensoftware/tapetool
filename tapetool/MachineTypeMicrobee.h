//////////////////////////////////////////////////////////////////////////
// MachineTypeMicrobee.h - declaration of CWaveReader class

#ifndef __MACHINETYPEMICROBEE_H
#define __MACHINETYPEMICROBEE_H

#include "MachineType.h"

class CCommandContext;

class CMachineTypeMicrobee : public CMachineType
{
public:
			CMachineTypeMicrobee();
	virtual ~CMachineTypeMicrobee();

	void SetOutputBaud(int baud);

	virtual const char* GetTapeFormatName() { return "tap"; };
	virtual CFileReader* CreateFileReader(CCommandContext* ctx, const char* pszExt);

	virtual bool SyncToBit(CFileReader* reader, bool verbose);
	virtual int ReadBit(CFileReader* reader, bool verbose);
	virtual bool SyncToByte(CFileReader* reader, bool verbose);
	virtual int ReadByte(CFileReader* reader, bool verbose);

	virtual void RenderCycleKind(CWaveWriter* writer, char kind);
	virtual void RenderBit(CWaveWriter* writer, unsigned char bit);
	virtual void RenderByte(CWaveWriter* writer, unsigned char byte);

	virtual int ProcessBlocks(CCommandContext* c);

	virtual int CycleFrequency();
	virtual int DcOffset(CWaveReader* wave);

	virtual bool CanRenderSquare() { return true; }

	int _baud;
};

#endif	// __MACHINETYPEMICROBEE_H

