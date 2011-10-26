//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandBytes.h"

// Command handler for dumping bytes
int CCommandBytes::Process()
{
	// Open files
	if (!OpenFiles(resBytes))
		return 7;

	printf("\n");
	file->SyncToByte(showSyncData);
	printf("\n\n");

	int perline = perLine ? perLine : 16;

	// Dump all bits
	int index = 0;
	while (true)
	{
		int pos = file->CurrentPosition();

		// Read a byte
		int byte = file->ReadByte();

		// Error?
		if (byte<0)
		{
			printf("\n\n");

			printf("[last byte ended at %i]\n", pos);

			file->Seek(pos);
			if (!file->SyncToByte(showSyncData))
				break;

			int skipped = file->CurrentPosition() - pos;
			printf("\n[skipped %s while re-syncing]\n", file->FormatDuration(skipped));

			printf("\n");

			index = 0;
			continue;
		}

		// New line?
		if ((index++ % perline)==0)
		{
			if (showPositionInfo)
				printf("\n[@%12i] ", pos);
			else
				printf("\n");
		}

		printf("0x%.2x ", byte);
		if (renderFile)
			machine->RenderByte(renderFile, byte);
		if (binaryFile)
			fwrite(&byte, 1, 1, binaryFile);
	}

	printf("\n\n");

	if (renderFile)
		renderFile->Flush();

	return 0;
}

void CCommandBytes::ShowUsage()
{
	printf("\nUsage: tapetool bytes [OPTIONS] INPUTFILE [OUTPUTFILE]\n");

	printf("\nProcesses a file at byte resolution.\n");

	ShowCommonUsage();
}
	 
