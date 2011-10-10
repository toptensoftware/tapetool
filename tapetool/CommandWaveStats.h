//////////////////////////////////////////////////////////////////////////
// CommandWaveStats.h - declaration of CCommandWaveStats

#ifndef __COMMANDWAVESTATS_H
#define __COMMANDWAVESTATS_H

#include "Command.h"

class CCommandWaveStats : public CCommand
{
public:
	CCommandWaveStats(CContext* ctx) : CCommand(ctx)
	{
	}

	virtual int Process();
	virtual bool DoesTranslateFromWaveData() { return false; }
};

#endif	// __COMMANDWAVESTATS_H

