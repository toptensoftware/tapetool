//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "CommandContext.h"

// Command handler for dumping samples
int ProcessSamples(CCommandContext* c)
{
	// Open the file
	if (!c->OpenFiles(resSamples))
		return 7;

	if (!c->file->IsWaveFile())
	{
		fprintf(stderr, "Can't dump samples from a text file");
		return 7;
	}

	CWaveReader* wf = static_cast<CWaveReader*>(c->file);

	int perline = c->perLineMode ? 1 : 32;

	wf->Seek(c->from);

	int prev = 0;
	bool first = true;

	// Dump all samples
	int index = 0;
	while (wf->HaveSample())
	{
		if (c->showZeroCrossings && !first && prev<0 != wf->CurrentSample()<0)
		{
			index=0;
		}
		else
		{
			first = false;
		}

		if ((index++ % perline)==0)
		{
			if (c->showPositionInfo)
				printf("\n[@%12i] ", wf->_currentSampleNumber);
			else
				printf("\n");
		}

		printf("%i ", wf->CurrentSample());

		prev = wf->CurrentSample();

		wf->NextSample();

		if (c->samples>0 && wf->_currentSampleNumber >= c->from + c->samples)
			break;
	}

	printf("\n\n");

	return 0;
}


