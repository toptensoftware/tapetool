//////////////////////////////////////////////////////////////////////////
// CommandSamples.h - declaration of CCommandSamples

#ifndef __COMMANDSAMPLES_H
#define __COMMANDSAMPLES_H

#include "Command.h"

class CCommandSamples : public CCommand
{
public:
	CCommandSamples(CContext* ctx) : CCommand(ctx)
	{
	}

	virtual int Process();
	virtual bool DoesTranslateFromWaveData() { return false; }
};

#endif	// __COMMANDSAMPLES_H

