//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandWaveStats.h"
#include "WaveAnalysis.h"


// Command handler for dumping samples
int CCommandWaveStats::Process()
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

	fprintf(stderr, "\n\nAnalysing wave data...");

	// Analyse the wave
	WAVE_INFO info;
	AnalyseWave(wf, _ctx->from, _ctx->samples, info);

	fprintf(stderr, "\n\n");

	printf("[\n");
	printf("    Sample Rate:               %iHz\n", wf->GetSampleRate());
	printf("    Bits Per Sample:           %i\n", wf->GetBytesPerSample() * 8);
	printf("    Length:                    %i samples\n", info.totalSamples);
	printf("    Duration:                  %.2f seconds\n", ((double)info.totalSamples)/wf->GetSampleRate() );
	printf("    Total Cycles:              %i\n", info.totalCycles);
	printf("    Average Samples/Cycle:     %i (%.1fHz)\n", info.avgSamplesPerCycle, info.avgCycleFrequency);
	printf("    Min Amplitude:             %i\n", info.minAmplitude);
	printf("    Max Amplitude:             %i\n", info.maxAmplitude);
	printf("    Median Min Amplitude:      %i\n", info.medianMinAmplitude);
	printf("    Median Max Amplitude:      %i\n", info.medianMaxAmplitude);
	printf("    Median Short Cycle:        %i (%.1fHz)\n", info.medianShortCycleLength, info.medianShortCycleFrequency);
	printf("    Median Long Cycle:         %i (%.1fHz)\n", info.medianLongCycleLength, info.medianLongCycleFrequency);
	printf("]");							  

	printf("\n\n");

	return 0;
}


