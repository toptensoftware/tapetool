//////////////////////////////////////////////////////////////////////////
// CommandDelete.h - declaration of CCommandDelete

#ifndef __COMMANDDELETE_H
#define __COMMANDDELETE_H

#include "CommandWithRangedInputWaveFile.h"

class CCommandDelete : public CCommandWithRangedInputWaveFile
{
public:
	CCommandDelete();

	const char* _outputFileName;

	virtual int Process();

	virtual int AddFile(const char* filename);
	virtual const char* GetCommandName() { return "delete"; };
	virtual bool DoesUseCycleMode() { return false; }

	void ShowUsage();
};

#endif	// __COMMANDDELETE_H

