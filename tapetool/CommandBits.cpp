//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandBits.h"

// Command handler for dumping bits
int CCommandBits::Process()
{
	// Open files
	if (!_ctx->OpenFiles(resBits))
		return 7;

	printf("\n");
	_ctx->file->SyncToBit(_ctx->showSyncData);
	printf("\n\n");

	int perline = _ctx->perLine ? _ctx->perLine : 64;

	// Dump all bits
	int index = 0;
	while (true)
	{
		int savePos = _ctx->file->CurrentPosition();

		// Read a bit
		int bit = _ctx->file->ReadBit();

		// Force line break on error
		if (bit<0)
		{
			printf("\n\n");

			printf("[last bit ended at %i]\n", savePos);

			_ctx->file->Seek(savePos);
			if (!_ctx->file->SyncToBit(_ctx->showSyncData))
				break;

			int skipped = _ctx->file->CurrentPosition() - savePos;
			printf("\n[skipped %s while re-syncing]\n", _ctx->file->FormatDuration(skipped));

			printf("\n");

			index = 0;
			continue;
		}

		// New line?
		if ((index++ % perline)==0)
		{
			if (_ctx->showPositionInfo)
				printf("\n[@%12i] ", savePos);
			else
				printf("\n");
		}

		printf("%i", bit);
		if (_ctx->renderFile)
			_ctx->machine->RenderBit(_ctx->renderFile, bit);
	}

	printf("\n\n");

	return 0;
}

