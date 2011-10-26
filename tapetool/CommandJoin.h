//////////////////////////////////////////////////////////////////////////
// CommandJoin.h - declaration of CCommandJoin

#ifndef __COMMANDJOIN_H
#define __COMMANDJOIN_H

#include "CommandWithRangedInputWaveFile.h"

class CCommandJoin : public CCommand
{
public:
	CCommandJoin();

	const char* _inputFileName1;
	const char* _inputFileName2;
	const char* _outputFileName;
	double _gap;

	virtual int Process();

	virtual int AddSwitch(const char* arg, const char* val);
	virtual int AddFile(const char* filename);
	virtual const char* GetCommandName() { return "join"; };
	virtual bool DoesUseCycleMode() { return false; }

	void ShowUsage();
};

#endif	// __COMMANDJOIN_H

