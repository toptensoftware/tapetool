//////////////////////////////////////////////////////////////////////////
// CommandWithInputWaveFile.h - declaration of CCommandWithRangedInputWaveFile

#ifndef __COMMANDWITHRANGEDINPUTWAVEFILE_H
#define __COMMANDWITHRANGEDINPUTWAVEFILE_H

#include "CommandWithInputWaveFile.h"

class CCommandWithRangedInputWaveFile : public CCommandWithInputWaveFile
{
public:
	CCommandWithRangedInputWaveFile();

	virtual int AddSwitch(const char* arg, const char* val);

	int GetStartSample();
	int GetEndSample();

	bool OpenWaveReader(CWaveReader& wave);
	void ShowHelp();

private:
	int _start;
	int _end;
	int _count;

	int _startCalculated;
	int _endCalculated;
};

#endif	// __COMMANDWITHRANGEDINPUTWAVEFILE_H

