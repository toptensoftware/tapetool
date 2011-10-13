//////////////////////////////////////////////////////////////////////////
// CommandFilter.h - declaration of CCommandFilter

#ifndef __COMMANDFILTER_H
#define __COMMANDFILTER_H

#include "Command.h"

class CCommandFilter : public CCommand
{
public:
	CCommandFilter(CContext* ctx) : CCommand(ctx)
	{
	}

	virtual int Process();
	virtual bool DoesTranslateFromWaveData() { return false; }
	virtual bool DoesTranslateToWaveData() { return false; }
	virtual bool UsesAutoOutputFile() { return false; }
};

#endif	// __COMMANDFILTER_H

