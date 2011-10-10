//////////////////////////////////////////////////////////////////////////
// CommandBlocks.h - declaration of CCommandBlocks

#ifndef __COMMANDBLOCKS_H
#define __COMMANDBLOCKS_H

#include "Command.h"

class CCommandBlocks : public CCommand
{
public:
	CCommandBlocks(CContext* ctx) : CCommand(ctx)
	{
	}

	virtual int Process();
};

#endif	// __COMMANDBLOCKS_H


