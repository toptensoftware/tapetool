//////////////////////////////////////////////////////////////////////////
// CommandCycleKinds.h - declaration of CCommandCycleKinds

#ifndef __COMMANDCYCLEKINDS_H
#define __COMMANDCYCLEKINDS_H

#include "Command.h"

class CCommandCycleKinds : public CCommand
{
public:
	CCommandCycleKinds(CContext* ctx) : CCommand(ctx)
	{
	}

	virtual int Process();
};

#endif	// __COMMANDCYCLEKINDS_H

