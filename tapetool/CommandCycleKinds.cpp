//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandCycleKinds.h"

// Command handler for dumping cycle kinds
int CCommandCycleKinds::Process()
{
	// Open files
	if (!_ctx->OpenFiles(resCycleKinds))
		return 7;

	int perline = _ctx->perLine ? _ctx->perLine : 64;

	// Dump all cycles
	int index = 0;
	while (true)
	{
		int pos = _ctx->file->CurrentPosition();

		char kind = _ctx->file->ReadCycleKind();
		if (kind==0)
			break;


		if ((index++ % perline)==0)
		{
			if (_ctx->showPositionInfo)
				printf("\n[@%12i] ", pos);
			else
				printf("\n");
		}

		printf("%c", kind);
		if (_ctx->renderFile)
			_ctx->machine->RenderCycleKind(_ctx->renderFile, kind);
	}

	printf("\n\n");

	return 0;
}

