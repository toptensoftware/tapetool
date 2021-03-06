//////////////////////////////////////////////////////////////////////////
// CommandFilter.h - declaration of CCommandFilter

#ifndef __COMMANDFILTER_H
#define __COMMANDFILTER_H

#include "CommandWithRangedInputWaveFile.h"

class CCommandFilter : public CCommandWithRangedInputWaveFile
{
public:
	CCommandFilter();

	const char* _outputFileName;

	virtual int Process();

	virtual int AddFile(const char* filename);
	virtual const char* GetCommandName() { return "filter"; };
	virtual bool DoesUseCycleMode() { return false; }

	void ShowUsage();
};

#endif	// __COMMANDFILTER_H

