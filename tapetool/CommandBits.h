//////////////////////////////////////////////////////////////////////////
// CommandBits.h - declaration of CCommandBits

#ifndef __COMMANDBITS_H
#define __COMMANDBITS_H

#include "CommandStd.h"

class CCommandBits : public CCommandStd
{
public:
	CCommandBits(CContext* ctx) : CCommandStd(ctx)
	{
	}

	virtual int Process();
	virtual const char* GetCommandName() { return "bits"; }
	virtual void ShowUsage();
};

#endif	// __COMMANDBITS_H

