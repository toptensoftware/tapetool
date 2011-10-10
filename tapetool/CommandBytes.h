//////////////////////////////////////////////////////////////////////////
// CommandBytes.h - declaration of CCommandBytes

#ifndef __COMMANDBYTES_H
#define __COMMANDBYTES_H

#include "Command.h"

class CCommandBytes : public CCommand
{
public:
	CCommandBytes(CContext* ctx) : CCommand(ctx)
	{
	}

	virtual int Process();
};

#endif	// __COMMANDBYTES_H

