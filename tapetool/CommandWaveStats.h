//////////////////////////////////////////////////////////////////////////
// CommandWaveStats.h - declaration of CCommandWaveStats

#ifndef __COMMANDWAVESTATS_H
#define __COMMANDWAVESTATS_H

#include "CommandWithRangedInputWaveFile.h"

class CCommandWaveStats : public CCommandWithRangedInputWaveFile
{
public:
	CCommandWaveStats();

	virtual int Process();
	virtual bool DoesTranslateFromWaveData() { return false; }
	virtual const char* GetCommandName() { return "analyse"; }
	virtual void ShowUsage();
};

#endif	// __COMMANDWAVESTATS_H

