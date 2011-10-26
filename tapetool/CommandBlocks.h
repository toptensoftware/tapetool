//////////////////////////////////////////////////////////////////////////
// CommandBlocks.h - declaration of CCommandBlocks

#ifndef __COMMANDBLOCKS_H
#define __COMMANDBLOCKS_H

#include "CommandStd.h"


class CCommandBlocks : public CCommandStd
{
public:
	CCommandBlocks(CContext* ctx) : CCommandStd(ctx)
	{
	}

	virtual int Process();
	virtual const char* GetCommandName() { return "blocks"; }
	virtual void ShowUsage();
};

#endif	// __COMMANDBLOCKS_H


