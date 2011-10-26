//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandBits.h"

// Command handler for dumping bits
int CCommandBits::Process()
{
	// Open files
	if (!OpenFiles(resBits))
		return 7;

	printf("\n");
	file->SyncToBit(showSyncData);
	printf("\n\n");

	int perline = perLine ? perLine : 64;

	// Dump all bits
	int index = 0;
	while (true)
	{
		int savePos = file->CurrentPosition();

		// Read a bit
		int bit = file->ReadBit();

		// Force line break on error
		if (bit<0)
		{
			printf("\n\n");

			printf("[last bit ended at %i]\n", savePos);

			file->Seek(savePos);
			if (!file->SyncToBit(showSyncData))
				break;

			int skipped = file->CurrentPosition() - savePos;
			printf("\n[skipped %s while re-syncing]\n", file->FormatDuration(skipped));

			printf("\n");

			index = 0;
			continue;
		}

		// New line?
		if ((index++ % perline)==0)
		{
			if (showPositionInfo)
				printf("\n[@%12i] ", savePos);
			else
				printf("\n");
		}

		printf("%i", bit);
		if (renderFile)
			machine->RenderBit(renderFile, bit);
	}

	printf("\n\n");

	if (renderFile)
		renderFile->Flush();

	return 0;
}

void CCommandBits::ShowUsage()
{
	printf("\nUsage: tapetool bits [OPTIONS] INPUTFILE [OUTPUTFILE]\n");

	printf("\nProcesses a file at bit resolution.\n");

	ShowCommonUsage();
}
