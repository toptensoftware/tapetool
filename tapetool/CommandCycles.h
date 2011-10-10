//////////////////////////////////////////////////////////////////////////
// CommandCycles.h - declaration of CCommandCycles

#ifndef __COMMANDCYCLES_H
#define __COMMANDCYCLES_H

#include "Command.h"

class CCommandCycles : public CCommand
{
public:
	CCommandCycles(CContext* ctx) : CCommand(ctx)
	{
	}

	virtual int Process();
	virtual bool DoesTranslateFromWaveData() { return false; }
};

#endif	// __COMMANDCYCLES_H

