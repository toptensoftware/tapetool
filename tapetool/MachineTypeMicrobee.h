//////////////////////////////////////////////////////////////////////////
// MachineTypeMicrobee.h - declaration of CTapeReader class

#ifndef __MACHINETYPEMICROBEE_H
#define __MACHINETYPEMICROBEE_H

#include "MachineType.h"

class CContext;

class CMachineTypeMicrobee : public CMachineType
{
public:
			CMachineTypeMicrobee();
	virtual ~CMachineTypeMicrobee();

	void SetOutputBaud(int baud);

	virtual const char* GetTapeFormatName() { return "tap"; };
	virtual CFileReader* CreateFileReader(CCommandStd* ctx, const char* pszExt);

	virtual void PrepareWaveMetrics(CCommandStd* c, CTapeReader* wf);
	
	virtual bool SyncToBit(CFileReader* reader, bool verbose);
	virtual int ReadBit(CFileReader* reader, bool verbose);
	virtual bool SyncToByte(CFileReader* reader, bool verbose);
	virtual int ReadByte(CFileReader* reader, bool verbose);

	virtual void RenderCycleKind(CWaveWriter* writer, char kind);
	virtual void RenderBit(CWaveWriter* writer, unsigned char bit);
	virtual void RenderByte(CWaveWriter* writer, unsigned char byte);

	virtual int ProcessBlocks(CCommandStd* c);

	virtual bool CanRenderSquare() { return true; }
	virtual bool InitWaveWriterProfiled(CWaveWriterProfiled* w);

	int _baud;
};

#endif	// __MACHINETYPEMICROBEE_H

