//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandWaveStats.h"
#include "WaveAnalysis.h"

CCommandWaveStats::CCommandWaveStats()
{
}

// Command handler for dumping samples
int CCommandWaveStats::Process()
{
	CWaveReader wave;
	if (!CCommandWithInputWaveFile::OpenWaveReader(wave))
	{
		return 7;
	}

	fprintf(stderr, "\n\nAnalysing wave data...");

	// Analyse the wave
	WAVE_INFO info;
	AnalyseWave(&wave, _cycleDetector.GetMode(), GetStartSample(), GetEndSample(), info);

	fprintf(stderr, "\n\n");

	printf("[\n");
	printf("    Sample Rate:               %iHz\n", wave.GetSampleRate());
	printf("    Bits Per Sample:           %i\n", wave.GetBytesPerSample() * 8);
	printf("    Length:                    %i samples\n", info.totalSamples);
	printf("    Duration:                  %.2f seconds\n", ((double)info.totalSamples)/wave.GetSampleRate() );
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


void CCommandWaveStats::ShowUsage()
{
	printf("\nUsage: tapetool analyse [OPTIONS] INPUTFILE [OUTPUTFILE]\n");

	printf("\nAnalyses a wave file and displays information about it.  The input file is required and must be 8 or 16 bit PCM\n");
	printf("mono .wav file.  The output file is optional and if specified the output text will be redirected there.\n");

	printf("\nOptions:\n");
	printf("  --help                Show these usage instructions\n");
	CCommandWithRangedInputWaveFile::ShowHelp();
	printf("\n\n");
}
