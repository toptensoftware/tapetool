//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "CommandContext.h"

int ProcessBlocks(CCommandContext* c)
{
	// Blocks are machine dependant so delegate...
	return c->machine->ProcessBlocks(c);
}
