//////////////////////////////////////////////////////////////////////////
// CommandCycles.h - declaration of CCommandCycles

#ifndef __COMMANDCYCLES_H
#define __COMMANDCYCLES_H

#include "CommandStd.h"

class CCommandCycles : public CCommandStd
{
public:
	CCommandCycles(CContext* ctx) : CCommandStd(ctx)
	{
	}

	virtual int Process();
	virtual bool DoesTranslateFromWaveData() { return false; }
	virtual const char* GetCommandName() { return "cycles"; }
	virtual void ShowUsage();
};

#endif	// __COMMANDCYCLES_H

