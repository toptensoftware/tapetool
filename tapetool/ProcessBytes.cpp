//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "CommandContext.h"



// Command handler for dumping bytes
int ProcessBytes(CCommandContext* c)
{
	// Open files
	if (!c->OpenFiles(resBytes))
		return 7;

	// Analyse cycle lengths
	if (c->analyzeCycles)
		c->file->Analyze();

	c->file->Prepare();

	printf("\n");
	c->file->SyncToByte(c->showSyncData);
	printf("\n\n");

	int perline = c->perLineMode ? 1 : 16;

	// Dump all bits
	int index = 0;
	while (true)
	{
		int pos = c->file->CurrentPosition();

		// Read a byte
		int byte = c->file->ReadByte();

		// Error?
		if (byte<0)
		{
			printf("\n\n");

			printf("[last byte ended at %i]\n", pos);

			c->file->Seek(pos);
			if (!c->file->SyncToByte(c->showSyncData))
				break;

			int skipped = c->file->CurrentPosition() - pos;
			printf("\n[skipped %s while re-syncing]\n", c->file->FormatDuration(skipped));

			printf("\n");

			index = 0;
			continue;
		}

		// New line?
		if ((index++ % perline)==0)
		{
			if (c->showPositionInfo)
				printf("\n[@%12i] ", pos);
			else
				printf("\n");
		}

		printf("0x%.2x ", byte);
		if (c->renderFile)
			c->machine->RenderByte(c->renderFile, byte);
		if (c->binaryFile)
			fwrite(&byte, 1, 1, c->binaryFile);
	}

	printf("\n\n");

	return 0;
}

