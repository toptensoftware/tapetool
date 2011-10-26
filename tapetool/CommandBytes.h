//////////////////////////////////////////////////////////////////////////
// CommandBytes.h - declaration of CCommandBytes

#ifndef __COMMANDBYTES_H
#define __COMMANDBYTES_H

#include "CommandStd.h"

class CCommandBytes : public CCommandStd
{
public:
	CCommandBytes(CContext* ctx) : CCommandStd(ctx)
	{
	}

	virtual int Process();
	virtual const char* GetCommandName() { return "bytes"; }
	virtual void ShowUsage();
};

#endif	// __COMMANDBYTES_H

