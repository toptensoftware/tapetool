//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandBytes.h"


// Command handler for dumping bytes
int CCommandBytes::Process()
{
	// Open files
	if (!_ctx->OpenFiles(resBytes))
		return 7;

	printf("\n");
	_ctx->file->SyncToByte(_ctx->showSyncData);
	printf("\n\n");

	int perline = _ctx->perLine ? _ctx->perLine : 16;

	// Dump all bits
	int index = 0;
	while (true)
	{
		int pos = _ctx->file->CurrentPosition();

		// Read a byte
		int byte = _ctx->file->ReadByte();

		// Error?
		if (byte<0)
		{
			printf("\n\n");

			printf("[last byte ended at %i]\n", pos);

			_ctx->file->Seek(pos);
			if (!_ctx->file->SyncToByte(_ctx->showSyncData))
				break;

			int skipped = _ctx->file->CurrentPosition() - pos;
			printf("\n[skipped %s while re-syncing]\n", _ctx->file->FormatDuration(skipped));

			printf("\n");

			index = 0;
			continue;
		}

		// New line?
		if ((index++ % perline)==0)
		{
			if (_ctx->showPositionInfo)
				printf("\n[@%12i] ", pos);
			else
				printf("\n");
		}

		printf("0x%.2x ", byte);
		if (_ctx->renderFile)
			_ctx->machine->RenderByte(_ctx->renderFile, byte);
		if (_ctx->binaryFile)
			fwrite(&byte, 1, 1, _ctx->binaryFile);
	}

	printf("\n\n");

	return 0;
}

