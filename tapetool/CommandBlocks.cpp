//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandBlocks.h"

int CCommandBlocks::Process()
{
	// Blocks are machine dependant so delegate...
	return machine->ProcessBlocks(this);
}

void CCommandBlocks::ShowUsage()
{
	printf("\nUsage: tapetool blocks [OPTIONS] INPUTFILE [OUTPUTFILE]\n");

	printf("\nProcesses a file at block resolution.\n");

	ShowCommonUsage();
}
