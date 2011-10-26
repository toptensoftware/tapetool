//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandCycleKinds.h"

// Command handler for dumping cycle kinds
int CCommandCycleKinds::Process()
{
	// Open files
	if (!OpenFiles(resCycleKinds))
		return 7;

	int perline = perLine ? perLine : 64;

	// Dump all cycles
	int index = 0;
	while (true)
	{
		int pos = file->CurrentPosition();

		char kind = file->ReadCycleKind();
		if (kind==0)
			break;


		if ((index++ % perline)==0)
		{
			if (showPositionInfo)
				printf("\n[@%12i] ", pos);
			else
				printf("\n");
		}

		printf("%c", kind);
		if (renderFile)
			machine->RenderCycleKind(renderFile, kind);
	}

	printf("\n\n");

	return 0;
}

void CCommandCycleKinds::ShowUsage()
{
	printf("\nUsage: tapetool cyclekinds [OPTIONS] INPUTFILE [OUTPUTFILE]\n");

	printf("\nProcesses a file at cycle-kind resolution.\n");

	ShowCommonUsage();
}
