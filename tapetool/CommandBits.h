//////////////////////////////////////////////////////////////////////////
// CommandBits.h - declaration of CCommandBits

#ifndef __COMMANDBITS_H
#define __COMMANDBITS_H

#include "Command.h"

class CCommandBits : public CCommand
{
public:
	CCommandBits(CContext* ctx) : CCommand(ctx)
	{
	}

	virtual int Process();
};

#endif	// __COMMANDBITS_H

