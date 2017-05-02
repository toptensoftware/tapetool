//////////////////////////////////////////////////////////////////////////
// Context.cpp - implementation of CContext class

#include "precomp.h"

#include "Context.h"


#define VER_MAJOR	0
#define VER_MINOR	4

// Constructor
CContext::CContext()
{
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

		/*
		// Command gets first attempt
		if (_cmd!=NULL)
		{
			int err = _cmd->AddSwitch(arg, val);
			if (err>=0)
				return err;
		}
		*/

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
			/*
			if (_cmd!=NULL)
				fprintf(stderr, "\nThe '%s' command doesn't support the command line arg: '%s', aborting\n\n", _cmd->GetCommandName(), arg_orig);
			else
			*/
			fprintf(stderr, "\nUnknown command line arg: '%s', aborting\n\n", arg_orig);
			return 7;
		}
	}
	else
	{
		/*
		if (_cmd==NULL)
		{
			if (_strcmpi(arg, "samples")==0)
			{
				_cmd = new CCommandSamples();
			}
			else if (_strcmpi(arg, "analyze")==0)
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
		*/
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

	// Nothing specified!
	ShowLogo();
	ShowUsage();
	return 0;
}


void CContext::ShowLogo()
{
	// Show some help
	printf("tapetool2 v%i.%i - Microbee/TRS-80 Tape Diagnotic Utility\n", VER_MAJOR, VER_MINOR);
	printf("Copyright (C) 2011-2017 Topten Software.\n");
}

void CContext::ShowUsage()
{
	printf("\nUsage: tapetool2 [filters...]\n");

	printf("\nFilters:\n");
	printf("  *.wav              reads/writes a wave file\n");
	printf("  *.tap              reads/writes a tape file\n");
	printf("  *.tcycle           reads/writes a tcycle file\n");

	printf("\nOptions:\n");
	printf("  --help             Show these usage instructions, or use after command name for help on that command\n");
	printf("  --version          Show version number\n");

	printf("\n");
}