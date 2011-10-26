//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "CommandWithInputWaveFile.h"
#include "WaveReader.h"

CCommandWithInputWaveFile::CCommandWithInputWaveFile() : _cycleDetector(cmZeroCrossingUp)
{
	_filename = NULL;
	_dcOffset = 0;
	_amplify = 1;
	_smoothing = 0;
}

bool CCommandWithInputWaveFile::OpenWaveReader(CWaveReader& wave, const char* filename)
{
	_filename = filename;
	return OpenWaveReader(wave);
}

bool CCommandWithInputWaveFile::OpenWaveReader(CWaveReader& wave)
{
	// Check we got an input file
	if (_filename==NULL)
	{
		fprintf(stderr, "No input file specified");
		return false;
	}

	// Open the input file
	if (!wave.OpenFile(_filename))
	{
		fprintf(stderr, "Failed to open '%s'", _filename);
		return false;
	}

	wave.SetDCOffset(_dcOffset);
	wave.SetAmplify(_amplify);
	wave.SetSmoothingPeriod(_smoothing);

	return true;
}

int CCommandWithInputWaveFile::AddSwitch(const char* arg, const char* val)
{
	if (_strcmpi(arg, "dcoffset")==0)
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
	else if (DoesUseCycleMode() && _strcmpi(arg, "cyclemode")==0)
	{
		CycleMode mode;
		if (!CCycleDetector::FromString(val, mode))
		{
			fprintf(stderr, "Invalid cycle mode: `%s`\n",  val);
			return 7;
		}
		_cycleDetector.Reset(mode);
	}
	else
	{
		return CCommand::AddSwitch(arg, val);
	}
	return 0;
}

int CCommandWithInputWaveFile::AddFile(const char* filename)
{
	if (_filename==NULL)
	{
		_filename = filename;
		return 0;
	}

	return CCommand::AddFile(filename);
}

void CCommandWithInputWaveFile::ShowHelp()
{
	printf("  --smooth[:N]          smooth waveform with a moving average of N samples (N=3 if not specified)\n");
	printf("  --dcoffset:N          offset sample values by this amount (DC Offset)\n");
	printf("  --amplify:N           amplify input signal by N%% (eg: 50 halves the signal amplitude)\n");
	if (DoesUseCycleMode())
	{
	printf("  --cyclemode:mode      cycle detection mode\n");                 
	printf("                             'zc+' = zero crossing upwards\n");
	printf("                             'zc-' = zero crossing downwards\n");
	printf("                             'max' = local maximum\n");
	printf("                             'min' = local minimum\n");
	printf("                             'max+' = local maximum with positive sample value\n");
	printf("                             'max-' = local minimum with negative sample value\n");
	}
}
