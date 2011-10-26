//////////////////////////////////////////////////////////////////////////
// CommandSamples.h - declaration of CCommandSamples

#ifndef __COMMANDSAMPLES_H
#define __COMMANDSAMPLES_H

#include "CommandWithRangedInputWaveFile.h"

class CCommandSamples : public CCommandWithRangedInputWaveFile
{
public:
	CCommandSamples();

	virtual int Process();
	virtual int AddSwitch(const char* arg, const char* val);
	virtual const char* GetCommandName() { return "samples"; };

	void ShowUsage();

	int _perline;
	bool _showCycles;
	bool _showPositionInfo;
};

#endif	// __COMMANDSAMPLES_H

