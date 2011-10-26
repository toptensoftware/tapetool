//////////////////////////////////////////////////////////////////////////
// Command.cpp - implementation of CCommand class

#include "precomp.h"

#include "Command.h"

CCommand::CCommand()
{
	_stdoutRedirected = false;
}

CCommand::~CCommand()
{
}



int CCommand::AddSwitch(const char* arg, const char* val)
{
	if (_strcmpi(arg, "help")==0)
	{
		ShowUsage();
		return 1;
	}
	return -1;
}

int CCommand::AddFile(const char* filename)
{
	if (_stdoutRedirected)
	{
		fprintf(stderr, "Too many file names supplied, aborting");
		return 7;
	}

	if (!freopen(filename, "wt", stdout))
	{
		fprintf(stderr, "Can't create output file '%s'\n", filename);
		return 7;
	}

	fprintf(stderr, "Writing output to '%s'\n", filename);

	_stdoutRedirected = true;
	return true;
}
