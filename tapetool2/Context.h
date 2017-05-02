//////////////////////////////////////////////////////////////////////////
// Context.h - declaration of CTapeReader class

#ifndef __CONTEXT_H
#define __CONTEXT_H


class CCommand;
class CContext;

class CContext
{
public:
// Construction
			CContext();
	virtual ~CContext();


	int Run(int argc,char **argv); 

	void ShowLogo();
	void ShowUsage();

protected:
	int ProcessCommandLineArg(const char* arg);
};

#endif	// __CONTEXT_H

