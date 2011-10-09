//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "CommandContext.h"

// Command handler for dumping cycle lengths
int ProcessCycles(CCommandContext* c)
{
	// Open the file
	if (!c->OpenFiles(resCycles))
		return 7;

	int perline = c->perLineMode ? 1 : 16;

	// Dump all cycles
	int index = 0;
	while (true)
	{
		if ((index++ % perline)==0)
		{
			if (c->showPositionInfo)
				printf("\n[@%12i] ", c->file->CurrentPosition());
			else
				printf("\n");
		}

		int cyclelen= c->file->ReadCycleLen();
		if (cyclelen<0)
			break;


		printf("#%i ", cyclelen);
	}

	printf("\n\n");

	return 0;
}



