//////////////////////////////////////////////////////////////////////////
// Command.h - declaration of CWaveReader class

#ifndef __COMMAND_H
#define __COMMAND_H

class CContext;

class CCommand
{
public:
			CCommand(CContext* ctx);
	virtual ~CCommand();

	virtual int Process()=0;
	virtual bool DoesTranslateFromWaveData() { return true; }

	CContext* _ctx;
};

#endif	// __COMMAND_H

