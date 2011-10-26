//////////////////////////////////////////////////////////////////////////
// Context.cpp - implementation of CContext class

#include "precomp.h"

#include "Context.h"

#include "CommandWaveStats.h"
#include "CommandSamples.h"
#include "CommandFilter.h"
#include "CommandCycles.h"
#include "CommandCycleKinds.h"
#include "CommandBits.h"
#include "CommandBytes.h"
#include "CommandBlocks.h"
#include "CommandJoin.h"
#include "CommandDelete.h"

#define VER_MAJOR	0
#define VER_MINOR	4

// Constructor
CContext::CContext()
{
	_cmd = NULL;
}

CContext::~CContext()
{

}


// Process a single command line argument
int CContext::ProcessCommandLineArg(const char* arg)
{
	const char* arg_orig = arg;

	if (arg[0]=='-' && arg[1]=='-')
		arg++;

	if (arg[0]=='-')
	{
		arg++;

		// Split --<arg>:<val>
		const char* v1 = strchr(arg, ':');
		const char* v2 = strchr(arg, '=');
		if (v1!=NULL && v2!=NULL)
		{
			if (v2<v1)
				v1=v2;
		}
		else if (v2!=NULL && v1==NULL)
		{
			v1 = v2;
		}
		char sw[256];
		const char* val = NULL;
		if (v1!=NULL)
		{
			strncpy(sw, arg, v1-arg);
			sw[v1-arg]='\0';
			arg = sw;
			val = v1+1;
		}

		// Command gets first attempt
		if (_cmd!=NULL)
		{
			int err = _cmd->AddSwitch(arg, val);
			if (err>=0)
				return err;
		}

		if (_strcmpi(arg, "help")==0)
		{
			ShowLogo();
			ShowUsage();
			return 1;
		}
		else if (_strcmpi(arg, "version")==0)
		{
			ShowLogo();
			return 1;
		}
		else
		{
			if (_cmd!=NULL)
				fprintf(stderr, "\nThe '%s' command doesn't support the command line arg: '%s', aborting\n\n", _cmd->GetCommandName(), arg_orig);
			else
				fprintf(stderr, "\nUnknown command line arg: '%s', aborting\n\n", arg_orig);
			return 7;
		}
	}
	else
	{
		if (_cmd==NULL)
		{
			if (_strcmpi(arg, "samples")==0)
			{
				_cmd = new CCommandSamples();
			}
			else if (_strcmpi(arg, "analyse")==0)
			{
				_cmd = new CCommandWaveStats();
			}
			else if (_strcmpi(arg, "filter")==0)
			{
				_cmd = new CCommandFilter();
			}
			else if (_strcmpi(arg, "delete")==0)
			{
				_cmd = new CCommandDelete();
			}
			else if (_strcmpi(arg, "join")==0)
			{
				_cmd = new CCommandJoin();
			}
			else if (_strcmpi(arg, "bits")==0)
			{
				_cmd = new CCommandBits(this);
			}
			else if (_strcmpi(arg, "bytes")==0)
			{
				_cmd = new CCommandBytes(this);
			}
			else if (_strcmpi(arg, "cycles")==0)
			{
				_cmd = new CCommandCycles(this);
			}
			else if (_strcmpi(arg, "cyclekinds")==0)
			{
				_cmd = new CCommandCycleKinds(this);
			}
			else if (_strcmpi(arg, "blocks")==0)
			{
				_cmd = new CCommandBlocks(this);
			}
		}
		else
		{
			_cmd->AddFile(arg);
		}
	}

	return 0;
}


int CContext::Run(int argc,char **argv)
{
	// Process command line arguments
	for (int i=1; i<argc; i++)
	{
		int err = ProcessCommandLineArg(argv[i]);
		if (err)
			return err;
	}

	// Run the command
	if (_cmd!=NULL)
	{
		int err = _cmd->PreProcess();
		if (err!=0)
			return 0;

		err = _cmd->Process();

		_cmd->PostProcess();
		return err;
	}

	// Nothing specified!
	ShowLogo();
	ShowUsage();
	return 0;
}


void CContext::ShowLogo()
{
	// Show some help
	printf("tapetool v%i.%i - Microbee/TRS-80 Tape Diagnotic Utility\n", VER_MAJOR, VER_MINOR);
	printf("Copyright (C) 2011 Topten Software.\n");
}

void CContext::ShowUsage()
{
	printf("\nUsage: tapetool COMMAND [ARGS]\n");

	printf("\nCommands:\n");
	printf("  samples            Prints samples from a wave file.\n");
	printf("  analyse            Prints wave file statistics.\n");
	printf("  filter             Copies and filters all or part of a wave file.\n");
	printf("  join               Join two wave files into a new wave file.\n");
	printf("  bits               Processes a file at bit resolution.\n");
	printf("  bytes              Processes a file at bytes resolution.\n");
	printf("  blocks             Processes a file at blocks resolution.\n");
	printf("  cycles             Processes a file at cycle-length resolution.\n");
	printf("  cyclekinds         Processes a file at cycle-kind resolution.\n");

	printf("\nOptions:\n");
	printf("  --help             Show these usage instructions, or use after command name for help on that command\n");
	printf("  --version          Show version number\n");

	printf("\nSee 'tapetool COMMAND --help' for more information on a specific command\n");

	printf("\n");
}