//////////////////////////////////////////////////////////////////////////
// CommandCycleKinds.h - declaration of CCommandCycleKinds

#ifndef __COMMANDCYCLEKINDS_H
#define __COMMANDCYCLEKINDS_H

#include "CommandStd.h"

class CCommandCycleKinds : public CCommandStd
{
public:
	CCommandCycleKinds(CContext* ctx) : CCommandStd(ctx)
	{
	}

	virtual int Process();
	virtual const char* GetCommandName() { return "cyclekinds"; }
	virtual void ShowUsage();
};

#endif	// __COMMANDCYCLEKINDS_H

