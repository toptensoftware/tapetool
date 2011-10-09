//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "CommandContext.h"

// Command handler for dumping cycle kinds
int ProcessCycleKinds(CCommandContext* c)
{
	// Open files
	if (!c->OpenFiles(resCycleKinds))
		return 7;

	// Analyse cycle lengths
	if (c->analyzeCycles)
		c->file->Analyze();

	c->file->Prepare();

	int perline = c->perLineMode ? 1 : 64;

	// Dump all cycles
	int index = 0;
	while (true)
	{
		int pos = c->file->CurrentPosition();

		char kind = c->file->ReadCycleKind();
		if (kind==0)
			break;


		if ((index++ % perline)==0)
		{
			if (c->showPositionInfo)
				printf("\n[@%12i] ", pos);
			else
				printf("\n");
		}

		printf("%c", kind);
		if (c->renderFile)
			c->machine->RenderCycleKind(c->renderFile, kind);
	}

	printf("\n\n");

	return 0;
}

