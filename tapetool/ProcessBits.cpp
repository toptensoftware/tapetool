//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "CommandContext.h"


// Command handler for dumping bits
int ProcessBits(CCommandContext* c)
{
	// Open files
	if (!c->OpenFiles(resBits))
		return 7;

	// Analyse cycle lengths
	if (c->analyzeCycles)
		c->file->Analyze();

	c->file->Prepare();

	printf("\n");
	c->file->SyncToBit(c->showSyncData);
	printf("\n\n");

	int perline = c->perLineMode ? 1 : 64;

	// Dump all bits
	int index = 0;
	while (true)
	{
		int savePos = c->file->CurrentPosition();

		// Read a bit
		int bit = c->file->ReadBit();

		// Force line break on error
		if (bit<0)
		{
			printf("\n\n");

			printf("[last bit ended at %i]\n", savePos);

			c->file->Seek(savePos);
			if (!c->file->SyncToBit(c->showSyncData))
				break;

			int skipped = c->file->CurrentPosition() - savePos;
			printf("\n[skipped %s while re-syncing]\n", c->file->FormatDuration(skipped));

			printf("\n");

			index = 0;
			continue;
		}

		// New line?
		if ((index++ % perline)==0)
		{
			if (c->showPositionInfo)
				printf("\n[@%12i] ", savePos);
			else
				printf("\n");
		}

		printf("%i", bit);
		if (c->renderFile)
			c->machine->RenderBit(c->renderFile, bit);
	}

	printf("\n\n");

	return 0;
}

