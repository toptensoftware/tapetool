//////////////////////////////////////////////////////////////////////////
// Context.h - declaration of CTapeReader class

#ifndef __CONTEXT_H
#define __CONTEXT_H

#include "FileReader.h"
#include "TapeReader.h"
#include "MachineType.h"
#include "WaveWriter.h"
#include "CycleDetector.h"

class CCommand;
class CContext;

class CContext
{
public:
// Construction
			CContext();
	virtual ~CContext();


	CCommand* _cmd;
	int Run(int argc,char **argv); 

	void ShowLogo();
	void ShowUsage();

protected:
	int ProcessCommandLineArg(const char* arg);
};

#endif	// __CONTEXT_H

