//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandBlocks.h"


int CCommandBlocks::Process()
{
	// Blocks are machine dependant so delegate...
	return _ctx->machine->ProcessBlocks(_ctx);
}
