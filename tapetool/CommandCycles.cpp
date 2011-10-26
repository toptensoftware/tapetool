//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandCycles.h"

// Command handler for dumping cycle lengths
int CCommandCycles::Process()
{
	// Open the file
	if (!OpenFiles(resCycles))
		return 7;

	int perline = perLine ? perLine : 16;

	// Dump all cycles
	int index = 0;
	while (true)
	{
		if ((index++ % perline)==0)
		{
			if (showPositionInfo)
				printf("\n[@%12i] ", file->CurrentPosition());
			else
				printf("\n");
		}

		int cyclelen= file->ReadCycleLen();
		if (cyclelen<0)
			break;


		printf("#%i ", cyclelen);
	}

	printf("\n\n");

	return 0;
}


void CCommandCycles::ShowUsage()
{
	printf("\nUsage: tapetool cycles [OPTIONS] INPUTFILE [OUTPUTFILE]\n");

	printf("\nProcesses a file at cycle-length resolution.\n");

	ShowCommonUsage();
}
