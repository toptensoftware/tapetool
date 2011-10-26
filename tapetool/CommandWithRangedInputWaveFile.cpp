//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "CommandWithRangedInputWaveFile.h"
#include "WaveReader.h"

CCommandWithRangedInputWaveFile::CCommandWithRangedInputWaveFile()
{
	_start = 0;
	_count = 0;
	_end = 0;
}

int CCommandWithRangedInputWaveFile::AddSwitch(const char* arg, const char* val)
{
	if (_strcmpi(arg, "startsample")==0)
	{
		_start = val==NULL ? 0 : atoi(val);
	}
	else if (_strcmpi(arg, "samplecount")==0)
	{
		_count = val==NULL ? 0 : atoi(val);
	}
	else if (_strcmpi(arg, "endsample")==0)
	{
		_end = val==NULL ? 0 : atoi(val);
	}
	else
	{
		return CCommandWithInputWaveFile::AddSwitch(arg, val);
	}
	return 0;
}

bool CCommandWithRangedInputWaveFile::OpenWaveReader(CWaveReader& wave)
{
	if (!CCommandWithInputWaveFile::OpenWaveReader(wave))
		return false;

	// Calculate limits
	_startCalculated = _start;

	if (_count!=0)
	{
		_endCalculated = _startCalculated + _count;
	}
	else
	{
		if (_end==0)
			_endCalculated = wave.GetTotalSamples();
		else
			_endCalculated = _end;
	}

	if (_startCalculated>=_endCalculated)
	{
		fprintf(stderr, "Invalid sample range");
		return false;
	}

	// Check limit
	if (_endCalculated > wave.GetTotalSamples())
		_endCalculated = wave.GetTotalSamples();

	return true;
}



int CCommandWithRangedInputWaveFile::GetStartSample()
{
	return _startCalculated;
}

int CCommandWithRangedInputWaveFile::GetEndSample()
{
	return _endCalculated;
}


void CCommandWithRangedInputWaveFile::ShowHelp()
{
	printf("  --startsample:N       starting sample number\n");
	printf("  --endsample:N         ending sample number\n");
	printf("  --samplecount:N       number of samples\n");
	CCommandWithInputWaveFile::ShowHelp();
}
