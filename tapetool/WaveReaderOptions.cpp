//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "WaveReaderOptions.h"

CWaveReaderOptions::CWaveReaderOptions()
{
	_filename = NULL;
	_from = 0;
	_samples = 0;
	_perline = 0;
	_dcOffset = 0;
	_amplify = 1;
	_smoothing = 0;
	_showCycles = false;
	_cycleMode = cmZeroCrossingUp;
	_showPositionInfo = true;
}

// Command handler for dumping samples
int CWaveReaderOptions::Process()
{
	// Check we got an input file
	if (_filename==NULL)
	{
		fprintf(stderr, "No input file specified");
		return 7;
	}

	// Open the input file
	CWaveReader wave;
	if (!wave.OpenFile(_filename))
	{
		fprintf(stderr, "Failed to open '%s'", _filename);
		return 7;
	}

	wave.SetDCOffset(_dcOffset);
	wave.SetAmplify(_amplify);
	wave.SetSmoothingPeriod(_smoothing);

	// Work out how many perline
	int perline = _perline ? _perline : (_showCycles ? 128 : 32);

	wave.Seek(_from);


	CCycleDetector cd(_cycleMode);

	// Dump all samples
	int index = 0;
	int cycleLen = 0;
	while (wave.HaveSample())
	{
		if (_showCycles && cd.IsNewCycle(wave.CurrentSample()))
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

		if (_samples>0 && wave.CurrentPosition() >= _from + _samples)
			break;
	}

	printf("\n\n");

	return 0;
}

int CWaveReaderOptions::AddSwitch(const char* arg, const char* val)
{
	if (_strcmpi(arg, "startsample")==0)
	{
		_from = val==NULL ? 0 : atoi(val);
		if (_samples==0)
			_samples=200;
	}
	else if (_strcmpi(arg, "samplecount")==0)
	{
		_samples = val==NULL ? 0 : atoi(val);
	}
	else if (_strcmpi(arg, "perline")==0)
	{
		_perline = val==NULL ? 0 : atoi(val);
	}
	else if (_strcmpi(arg, "dcoffset")==0)
	{
		_dcOffset = val==NULL ? 0 : atoi(val);
	}
	else if (_strcmpi(arg, "amplify")==0)
	{
		_amplify = val==NULL ? 1.0 : (atof(val)/100);
	}
	else if (_strcmpi(arg, "smooth")==0)
	{
		if (val==NULL)
			_smoothing = 3;
		else
			_smoothing = atoi(val);
	}
	else if (_strcmpi(arg, "showcycles")==0)
	{
		_showCycles= true;
	}
	else if (_strcmpi(arg, "cyclemode")==0)
	{
		if (!CCycleDetector::FromString(val, _cycleMode))
		{
			fprintf(stderr, "Invalid cycle mode: `%s`\n",  val);
			return 7;
		}
	}
	else if (_strcmpi(arg, "noposinfo")==0)
	{
		_showPositionInfo = false;
	}
	else
	{
		return CCommand::AddSwitch(arg, val);
	}
	return 0;
}

int CWaveReaderOptions::AddFile(const char* filename)
{
	if (_filename!=NULL)
	{
		fprintf(stderr, "multiple input file specified, aborting");
		return 7;
	}

	_filename = filename;

	return 0;
}

