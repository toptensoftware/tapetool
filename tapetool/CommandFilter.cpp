//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandFilter.h"

void CopyCycle(CWaveReader* reader, CWaveWriter* writer, int cycleStart, int min, int max, int prev)
{
	// Work out scale
	double scale = 1;

	if (prev<0)
	{
		if (min==0)
			scale = 1;
		else
			scale = - double(writer->GetAmplitude()) / (double)min;
	}
	else
	{
		if (max==0)
			scale = 1;
		else
			scale = double(writer->GetAmplitude()) / (double)max;
	}

	// Seek back to start of cycle
	int cycleEnd = reader->CurrentPosition();
	reader->Seek(cycleStart);

	// Write the half cycle, scaled by calculated amount
	while (reader->CurrentPosition()<cycleEnd)
	{
		writer->RenderSample((short)(scale * reader->CurrentSample()));
		reader->NextSample();
	}
}

// Command handler for dumping samples
int CCommandFilter::Process()
{
	// Open the file
	if (!_ctx->OpenFiles(resSamples))
		return 7;

	if (!_ctx->file->IsWaveFile())
	{
		fprintf(stderr, "Can't filter this input file");
		return 7;
	}

	if (_ctx->file_count!=2)
	{
		fprintf(stderr, "Filter command requires an output wave file");
		return 7;
	}

	CWaveReader* wf = static_cast<CWaveReader*>(_ctx->file);

	CWaveWriter writer;
	writer.Create(_ctx->files[1], wf->GetSampleRate(), wf->GetBytesPerSample()*8);

	wf->Seek(_ctx->from);

	CCycleDetector cd(_ctx->cycleMode);

	if (_ctx->norm_cycles)
	{
		int cycleStart = wf->CurrentPosition();
		int prev = 0;
		int min = 0;
		int max = 0;
		while (wf->HaveSample())
		{
			int sample = wf->CurrentSample();

			if (cd.IsNewCycle(sample))
			{
				CopyCycle(wf, &writer, cycleStart, min, max, prev);

				// Prep for next cycle
				cycleStart = wf->CurrentPosition();
				min = 0;
				max = 0;
			}

			if (sample<min)
				min = sample;
			if (sample>max)
				max = sample;
			wf->NextSample();

			prev = sample;
			if (_ctx->samples>0 && wf->_currentSampleNumber >= _ctx->from + _ctx->samples)
				break;
		}

		// Copy the last cycle
		CopyCycle(wf, &writer, cycleStart, min, max, prev);
	}
	else
	{
		while (wf->HaveSample())
		{
			writer.RenderSample(wf->CurrentSample());
			wf->NextSample();
			if (_ctx->samples>0 && wf->_currentSampleNumber >= _ctx->from + _ctx->samples)
				break;
		}
	}

	writer.Close();

	return 0;
}


