//////////////////////////////////////////////////////////////////////////
// TapeReader.cpp - implementation of CTapeReader class

#include "precomp.h"

#include "TapeReader.h"
#include "MachineType.h"
#include "Context.h"
#include "CommandStd.h"
#include "Instrumentation.h"

//////////////////////////////////////////////////////////////////////////
// CTapeReader

// Constructor
CTapeReader::CTapeReader(CCommandStd* cmd) : CFileReader(cmd)
{
	_avgCycleLength=0;
	_shortCycleLength=0;
	_longCycleLength=0;
	_instrumentation=NULL;
	Close();
}

// Destructor
CTapeReader::~CTapeReader()
{
	Close();
}


int CTapeReader::GetBytesPerSample()
{
	return _wave.GetBytesPerSample();
}

int CTapeReader::GetSampleRate()
{
	return _wave.GetSampleRate();
}

int CTapeReader::GetTotalSamples()
{
	return _wave.GetTotalSamples();
}

const char* CTapeReader::GetDataFormat()
{
	return _cmd->machine->GetTapeFormatName();
}

Resolution CTapeReader::GetResolution()
{
	return resSamples;
}


void CTapeReader::Delete()
{
	delete this;
}

bool CTapeReader::IsWaveFile()
{
	return true;
}

int CTapeReader::CurrentPosition()
{
	return _wave.CurrentPosition();
}

bool CTapeReader::Open(const char* filename, Resolution res)
{
	if (!_cmd->OpenWaveReader(_wave, filename))
		return false;

 	// Open instrumentation file
	if (_cmd->instrument)
	{
		_instrumentation = new CInstrumentation();
	}

	return true;
}

void CTapeReader::Prepare()
{
	// Do analysis or whatever...
	_cmd->machine->PrepareWaveMetrics(_cmd, this);

	// Apply user overrides
	if (_cmd->cycle_freq!=NULL)
		SetShortCycleFrequency(atoi(_cmd->cycle_freq));

	// Reset the cycle detector
	_cmd->_cycleDetector.Reset();

	// Show info on how wave is handled
	printf("\n[\n");
	printf("    smoothing period:        %i\n", _wave.GetSmoothingPeriod());
	printf("    DC offset:               %i\n", _wave.GetDCOffset());
	printf("    amplify:                 %.1f%%\n", _wave.GetAmplify()*100);
	printf("    cycle mode:              %s\n", CCycleDetector::ToString(_cmd->_cycleDetector.GetMode()));
	if (_avgCycleLength!=0)
	{
		printf("    avg cycle length:        %i (%.1fHz)\n", _avgCycleLength, (double)GetSampleRate() / _avgCycleLength);
		printf("    short cycle length:      %i (%.1fHz)\n", _shortCycleLength, (double)GetSampleRate() / _shortCycleLength);
		printf("    long cycle length:       %i (%.1fHz)\n", _longCycleLength, (double)GetSampleRate() / _longCycleLength);
		printf("    cycle length allowance:  %i (+/-)\n", _cycleLengthAllowance);
	}
	printf("]\n\n");
}


void CTapeReader::SetShortCycleFrequency(int freq)
{
	_shortCycleLength = GetSampleRate() / freq;
	_longCycleLength = 2 * _shortCycleLength;
	_avgCycleLength = (_shortCycleLength + _longCycleLength)/2;
	_cycleLengthAllowance = _avgCycleLength / 3-1;
}

void CTapeReader::SetCycleLengths(int shortCycleSamples, int longCycleSamples)
{
	_shortCycleLength = shortCycleSamples;
	_longCycleLength = longCycleSamples;
	_avgCycleLength = (_shortCycleLength + _longCycleLength)/2;
	_cycleLengthAllowance = _avgCycleLength / 3-1;
}

void CTapeReader::Close()
{
	if (_instrumentation)
	{
		// Save instrumentation
		if (_instrumentation)
		{
			char temp[1024];
			strcpy(temp, _wave.GetFileName());
			strcat(temp, ".profile");
			_instrumentation->Save(temp, _wave.GetTotalSamples());
			/*
			strcat(temp, ".txt");
			_instrumentation->SaveText(temp, _wave.GetTotalSamples());
			*/
		}

		delete _instrumentation;
	}

	_wave.Close();

	_avgCycleLength = 0;
	_startOfCurrentHalfCycle = 0;
	_instrumentation = NULL;
}

char* CTapeReader::FormatDuration(int duration)
{
	static char sz[512];
	sprintf(sz, "%i samples", duration);
	return sz;
}

void CTapeReader::Seek(int sampleNumber)
{
	_wave.Seek(sampleNumber);
	_startOfCurrentHalfCycle = _wave.CurrentPosition();
}

bool CTapeReader::NextSample()
{
	// Get the next sample
	return _wave.NextSample();
}

bool CTapeReader::HaveSample()
{
	return _wave.HaveSample();
}

int CTapeReader::CurrentSample()
{
	return _wave.CurrentSample();
}


// Read one cycle from the file and return its length in samples
int CTapeReader::ReadCycleLen()
{
	while (NextSample())
	{
		if (_cmd->_cycleDetector.IsNewCycle(CurrentSample()))
		{
			int iCycleLen = CurrentPosition() - _startOfCurrentHalfCycle;
			_startOfCurrentHalfCycle = CurrentPosition();
			return iCycleLen;
		}
	}

	return -1;
}

char CTapeReader::ReadCycleKind()
{
	// Remember offset of the current cycle
	//int cycleOffset = CurrentSampleNumber();

	// Read the length of the next cycle
	int iLen = ReadCycleLen();
	if (iLen<0)
		return 0;

	_lastCycleLen = iLen;

	// Check for outside allowances
	if (iLen < _shortCycleLength - _cycleLengthAllowance)
	{
		return '<';
	}
	if (iLen > _longCycleLength + _cycleLengthAllowance)
	{
		return '>';
	}
	if (iLen > _shortCycleLength + _cycleLengthAllowance && iLen < _longCycleLength - _cycleLengthAllowance)
	{
		return '?';
	}

	// Short or long?
	return iLen < _avgCycleLength ? 'S' : 'L';
}

int CTapeReader::LastCycleLen()
{
	return _lastCycleLen;
}

