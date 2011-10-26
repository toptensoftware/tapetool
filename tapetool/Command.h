//////////////////////////////////////////////////////////////////////////
// Command.h - declaration of CTapeReader class

#ifndef __COMMAND_H
#define __COMMAND_H

class CContext;

class CCommand
{
public:
			CCommand();
	virtual ~CCommand();

	virtual int PreProcess() { return 0; };
	virtual int Process()=0;
	virtual int PostProcess() { return 0; }

	virtual int AddSwitch(const char* arg, const char* val);
	virtual int AddFile(const char* filename);
	virtual const char* GetCommandName() = 0;
	virtual void ShowUsage() = 0;

	bool _stdoutRedirected;
};

#endif	// __COMMAND_H

