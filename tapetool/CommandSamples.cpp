//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandSamples.h"

// Command handler for dumping samples
int CCommandSamples::Process()
{
	// Open the file
	if (!_ctx->OpenFiles(resSamples))
		return 7;

	if (!_ctx->file->IsWaveFile())
	{
		fprintf(stderr, "Can't dump samples from a text file");
		return 7;
	}

	CWaveReader* wf = static_cast<CWaveReader*>(_ctx->file);

	int perline = _ctx->perLine ? _ctx->perLine : 32;

	wf->Seek(_ctx->from);

	int prev = 0;
	bool first = true;

	// Dump all samples
	int index = 0;
	while (wf->HaveSample())
	{
		if (_ctx->showZeroCrossings && !first && prev<0 != wf->CurrentSample()<0)
		{
			index=0;
		}
		else
		{
			first = false;
		}

		if ((index++ % perline)==0)
		{
			if (_ctx->showPositionInfo)
				printf("\n[@%12i] ", wf->_currentSampleNumber);
			else
				printf("\n");
		}

		printf("%i ", wf->CurrentSample());

		prev = wf->CurrentSample();

		wf->NextSample();

		if (_ctx->samples>0 && wf->_currentSampleNumber >= _ctx->from + _ctx->samples)
			break;
	}

	printf("\n\n");

	return 0;
}


