//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "CommandSamples.h"
#include "WaveReader.h"

CCommandSamples::CCommandSamples()
{
	_perline = 0;
	_showCycles = false;
	_showPositionInfo = true;
}

// Command handler for dumping samples
int CCommandSamples::Process()
{
	// Work out how many perline
	int perline = _perline ? _perline : (_showCycles ? 128 : 16);

	// Open and configure wave reader
	CWaveReader wave;
	if (!OpenWaveReader(wave))
		return 7;

	// Dump all samples
	int index = 0;
	int cycleLen = 0;
	wave.Seek(GetStartSample());
	while (wave.HaveSample() && wave.CurrentPosition() < GetEndSample())
	{
		if (_showCycles && _cycleDetector.IsNewCycle(wave.CurrentSample()))
		{
			printf("[eoc:%i]", cycleLen);
			cycleLen=0;
			index=0;
		}

		if ((index++ % perline)==0)
		{
			if (_showPositionInfo)
				printf("\n[@%12i] ", wave.CurrentPosition());
			else
				printf("\n");
		}

		printf("%i ", wave.CurrentSample());
		cycleLen++;

		wave.NextSample();
	}

	printf("\n\n");

	return 0;
}

int CCommandSamples::AddSwitch(const char* arg, const char* val)
{
	if (_strcmpi(arg, "noposinfo")==0)
	{
		_showPositionInfo = false;
	}
	else if (_strcmpi(arg, "perline")==0)
	{
		_perline = val==NULL ? 0 : atoi(val);
	}
	else if (_strcmpi(arg, "showcycles")==0)
	{
		_showCycles= true;
	}
	else
	{
		return CCommandWithRangedInputWaveFile::AddSwitch(arg, val);
	}
	return 0;
}


void CCommandSamples::ShowUsage()
{
	printf("\nUsage: tapetool samples [OPTIONS] INPUTFILE [OUTPUTFILE]\n");

	printf("\nDisplays the samples in a wave file.  The input file is required and must be 8 or 16 bit PCM\n");
	printf("mono .wav file.  The output file is optional and if specified the output text will be redirected there.\n");

	printf("\nOptions:\n");
	printf("  --help                Show these usage instructions\n");
	printf("  --perline:N           display N samples per line\n");
	printf("  --noposinfo           don't dump position info\n");
	printf("  --showcycles          show cycle boundaries with --samples\n");
	CCommandWithRangedInputWaveFile::ShowHelp();
	printf("\n\n");
}
