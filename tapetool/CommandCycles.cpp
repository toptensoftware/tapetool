//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandCycles.h"

// Command handler for dumping cycle lengths
int CCommandCycles::Process()
{
	// Open the file
	if (!_ctx->OpenFiles(resCycles))
		return 7;

	int perline = _ctx->perLine ? _ctx->perLine : 16;

	// Dump all cycles
	int index = 0;
	while (true)
	{
		if ((index++ % perline)==0)
		{
			if (_ctx->showPositionInfo)
				printf("\n[@%12i] ", _ctx->file->CurrentPosition());
			else
				printf("\n");
		}

		int cyclelen= _ctx->file->ReadCycleLen();
		if (cyclelen<0)
			break;


		printf("#%i ", cyclelen);
	}

	printf("\n\n");

	return 0;
}



