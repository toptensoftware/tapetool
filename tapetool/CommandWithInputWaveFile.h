//////////////////////////////////////////////////////////////////////////
// CommandWithInputWaveFile.h - declaration of CCommandWithInputWaveFile

#ifndef __COMMANDWITHINPUTWAVEFILE_H
#define __COMMANDWITHINPUTWAVEFILE_H

#include "Command.h"
#include "CycleDetector.h"

class CWaveReader;

class CCommandWithInputWaveFile : public CCommand
{
public:
	CCommandWithInputWaveFile();

	virtual int AddSwitch(const char* arg, const char* val);
	virtual int AddFile(const char* filename);
	virtual bool DoesUseCycleMode() { return true; }

	bool OpenWaveReader(CWaveReader& wave, const char* filename);
	bool OpenWaveReader(CWaveReader& wave);
	void ShowHelp();

	const char* _filename;
	int _dcOffset;
	double _amplify;
	int _smoothing;
	bool _makeSquareWave;
	CCycleDetector _cycleDetector;
};

#endif	// __COMMANDWITHINPUTWAVEFILE_H

