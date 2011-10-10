//////////////////////////////////////////////////////////////////////////
// WaveReader.cpp - implementation of CWaveReader class

#include "precomp.h"

#include "WaveReader.h"
#include "MachineType.h"
#include "Context.h"

//////////////////////////////////////////////////////////////////////////
// CWaveReader

// Constructor
CWaveReader::CWaveReader(CContext* ctx) : CFileReader(ctx)
{
	_file = NULL;
	_smoothingPeriod = 0;
	_smoothingBuffer = NULL;
	_dc_offset = 0;
	_avgCycleLength=0;
	_shortCycleLength=0;
	_longCycleLength=0;
	Close();
}

// Destructor
CWaveReader::~CWaveReader()
{
	Close();
}


int CWaveReader::GetBytesPerSample()
{
	return _bytesPerSample;
}

int CWaveReader::GetSampleRate()
{
	return _sampleRate;
}

int CWaveReader::GetTotalSamples()
{
	return _waveEndInSamples;
}


const char* CWaveReader::GetDataFormat()
{
	return _ctx->machine->GetTapeFormatName();
}

Resolution CWaveReader::GetResolution()
{
	return resSamples;
}


void CWaveReader::Delete()
{
	delete this;
}

bool CWaveReader::IsWaveFile()
{
	return true;
}

int CWaveReader::CurrentPosition()
{
	return _currentSampleNumber;
}

bool CWaveReader::Open(const char* filename, Resolution res)
{
	// Open the file
    _file=fopen(filename,"rb");
	if (_file==NULL)
	{
	    fprintf(stderr, "Could not open %s\n", filename);
		return false;
	}

	// Check RIFF header
	unsigned long l;
	fread(&l, 1, sizeof(l), _file);
	if (memcmp(&l, "RIFF", 4)!=0)
	{
		fclose(_file);
		_file=NULL;
		fprintf(stderr,"%s is not a wave file.\n",filename);
		return false;
	}

	// Work out the file length
	fseek(_file, 0, SEEK_END);
	int fileLength = ftell(_file);
	fseek(_file, 4, SEEK_SET);

	// Compare file length to data
	fread(&l, 1, 4, _file);
	if (fileLength > (int)(l+8))
	{
		fclose(_file);
		_file=NULL;
		fprintf(stderr,"%s is incomplete - bytes are missing.\n",filename);
		return false;
	}
	else if (fileLength<(int)(l+8))
	{
		fclose(_file);
		_file=NULL;
		fprintf(stderr,"%s has junk bytes at the end of the file.\n",filename);
		return false;
	}

	// Read the WAVE header
	fread(&l, 1, 4, _file);
	if (memcmp(&l,"WAVE",4)!=0)
	{
		fclose(_file);
		_file=NULL;
		fprintf(stderr,"%s is not a wave file.\n",filename);
		return false;
	}

	// Scan chunks
	int chunkLength = 0;
	for (int p=0xC; p<fileLength; p+=chunkLength+8)
	{
		// Read header
		fseek(_file, p, SEEK_SET);
		fread(&l, 1, sizeof(l), _file);
		fread(&chunkLength, 1, sizeof(chunkLength), _file);

		// "FMT"?
		if (memcmp(&l,"fmt ",4)==0)
		{
			unsigned short us[8];
			fread(us, 1, sizeof(us), _file);

			// Check for 8 or 16 bit PCM mono
			if (us[0]!=1 || us[1]!=1 || (us[7]!=8 && us[7]!=16))
			{
				fclose(_file);
				_file=0;
				fprintf(stderr,"Only 8 or 16-bit mono PCM format "
					"wave files are supported.\n"
					"%s is %u-bit %s %s format.\n",
					filename,
					(us[0]==1)?us[7]:1,
					(us[1]==1)?(const char *)"mono":
						(const char *)"stereo",
					(us[0]==1)?(const char *)"PCM":
						(const char *)"compressed");
				return false;
			}

			// Save required info
			_bytesPerSample = us[7]/8;
			_sampleRate=us[2]+(((unsigned long)(us[3]))<<16);

			// Check we got a sample rate
			if (_sampleRate==0)
			{
				fclose(_file);
				_file=0;
				fprintf(stderr, "File has sample rate 0. This is invalid!\n");
				return false;
			}
		}

		// Is it the data chunk
		else if (memcmp(&l, "data", 4)==0)
		{
			if(_sampleRate==0)
			{
				fprintf(stderr,"No \"fmt\" chunk found.\n");
				fclose(_file);
				_file=0;
				return false;
			}

			_waveOffsetInBytes = p+8;
			_waveEndInSamples = chunkLength / _bytesPerSample;
			_dataEndInSamples = _waveEndInSamples;

			// Setup smoothing
			_smoothingPeriod = _ctx->smoothing;
			if (_smoothingPeriod!=0)
			{
				_smoothingBuffer = new int[_smoothingPeriod];

				memset(_smoothingBuffer, 0, sizeof(int) * _smoothingPeriod);
				_smoothingBufferPos=0;
				_smoothingBufferTotal=0;
			}

			// Seek to start
			Seek(0);

			return true;
		}
	}
    
	// Unexpected EOF
	fclose(_file);
	_file=NULL;
	fprintf(stderr,"No \"data\" chunk found.\n");

	return false;
}


void CWaveReader::Prepare()
{
	// Apply initial phase shift
	if (_ctx->phase_shift)
	{
		ReadHalfCycle();
	}

	// Do analysis or whatever...
	_ctx->machine->PrepareWaveMetrics(_ctx, this);

	// Apply user overrides
	if (_ctx->dc_offset!=NULL)
		SetDCOffset(atoi(_ctx->dc_offset));
	if (_ctx->cycle_freq!=NULL)
		SetShortCycleFrequency(atoi(_ctx->cycle_freq));

	// Show info on how wave is handled
	printf("\n[\n");
	printf("    smoothing period:        %i\n", _smoothingPeriod);
	printf("    DC offset:               %i\n", _dc_offset);
	printf("    phase shifed:            %s\n", _ctx->phase_shift ? "yes" : "no");
	if (_avgCycleLength!=0)
	{
		printf("    avg cycle length:        %i (%.1fHz)\n", _avgCycleLength, (double)_sampleRate / _avgCycleLength);
		printf("    short cycle length:      %i (%.1fHz)\n", _shortCycleLength, (double)_sampleRate / _shortCycleLength);
		printf("    long cycle length:       %i (%.1fHz)\n", _longCycleLength, (double)_sampleRate / _longCycleLength);
		printf("    cycle length allowance:  %i (+/-)\n", _cycleLengthAllowance);
	}
	printf("]\n\n");
}

void CWaveReader::SetDCOffset(int offset)
{
	_dc_offset = offset;
}

int CWaveReader::GetDCOffset()
{
	return _dc_offset;
}

void CWaveReader::SetShortCycleFrequency(int freq)
{
	_shortCycleLength = _sampleRate / freq;
	_longCycleLength = 2 * _shortCycleLength;
	_avgCycleLength = (_shortCycleLength + _longCycleLength)/2;
	_cycleLengthAllowance = _avgCycleLength / 3-1;
}

void CWaveReader::SetCycleLengths(int shortCycleSamples, int longCycleSamples)
{
	_shortCycleLength = shortCycleSamples;
	_longCycleLength = longCycleSamples;
	_avgCycleLength = (_shortCycleLength + _longCycleLength)/2;
	_cycleLengthAllowance = _avgCycleLength / 3-1;
}

void CWaveReader::Close()
{
	if (_file!=NULL)
		fclose(_file);

	_waveOffsetInBytes = 0;
	_waveEndInSamples = 0;
	_dataStartInSamples = 0;
	_dataEndInSamples = 0;
	_currentSampleNumber = 0;
	_sampleRate = 0;
	_bytesPerSample = 1;
	_avgCycleLength = 0;
	_currentSample = 0;
	_startOfCurrentHalfCycle = 0;
	if (_smoothingBuffer!=NULL)
		delete[] _smoothingBuffer;
	_smoothingBuffer = NULL;
	_smoothingPeriod = 0;
}

char* CWaveReader::FormatDuration(int duration)
{
	static char sz[512];
	sprintf(sz, "%i samples", duration);
	return sz;
}

void CWaveReader::Seek(int sampleNumber)
{
	// Go back by size of smoothing buffer
	int startAtSampleNumber = sampleNumber -_smoothingPeriod;
	if (startAtSampleNumber<0)
		startAtSampleNumber = 0;

	// Seek to sample
	fseek(_file, _waveOffsetInBytes + startAtSampleNumber * _bytesPerSample, SEEK_SET);

	// Setup position info
	_currentSampleNumber = startAtSampleNumber;

	// Read the sample
	_currentSample=ReadRawSample();

	// Reset and refill the smoothing buffer
	memset(_smoothingBuffer, 0, _smoothingPeriod * sizeof(int));
	_smoothingBufferPos=0;
	_smoothingBufferTotal=0;
	while (_currentSampleNumber < sampleNumber)
	{
		if (!NextSample())
			break;
	}

	_startOfCurrentHalfCycle = _currentSampleNumber;
}

bool CWaveReader::NextSample()
{
	_currentSample = ReadSmoothedSample();
	_currentSampleNumber++;
	return _currentSample!=EOF_SAMPLE;
}

bool CWaveReader::HaveSample()
{
	return _currentSample!=EOF_SAMPLE;
}

int CWaveReader::CurrentSample()
{
	return _currentSample;
}


int CWaveReader::ReadRawSample()
{
	if (_currentSampleNumber > _dataEndInSamples)
		return EOF_SAMPLE;

	if (_bytesPerSample==1)
	{
		int i = fgetc(_file);
		if (i<0)
			return EOF_SAMPLE;
		else
		{
			return _dc_offset + i-128;
		}
	}
	else
	{
		short s;
		if (fread(&s, sizeof(s), 1, _file)==0)
			return EOF_SAMPLE;
		else
		{
			return _dc_offset + s;
		}
	}
}

int CWaveReader::ReadSmoothedSample()
{
	// Smoothing disabled?
	if (_smoothingPeriod==0)
		return ReadRawSample();

	// Read a raw sample
	int iSample = ReadRawSample();
	if (iSample==EOF_SAMPLE)
		return iSample;

	// Calculate new position in circular buffer
	_smoothingBufferPos = (_smoothingBufferPos + 1) % _smoothingPeriod;

	// Update the moving total
	_smoothingBufferTotal -= _smoothingBuffer[_smoothingBufferPos];
	_smoothingBufferTotal += iSample;

	// Update the buffer
	_smoothingBuffer[_smoothingBufferPos]=iSample;

	// Return smoothed sample
	return _smoothingBufferTotal / _smoothingPeriod;
}

int CWaveReader::ReadHalfCycle()
{
	int prev = _currentSample;

	while (NextSample())
	{
		if ((_currentSample<0)!=(prev<0))
		{
			int iCycleLen = _currentSampleNumber - _startOfCurrentHalfCycle;
			_startOfCurrentHalfCycle = _currentSampleNumber;
			return iCycleLen;
		}
	}

	return -1;
}

// Read one cycle from the file and return its length in samples
int CWaveReader::ReadCycleLen()
{
	int a = ReadHalfCycle();
	int b = ReadHalfCycle();
	if (a==-1 || b==-1)
		return -1;
	return a+b;
}

char CWaveReader::ReadCycleKind()
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

int CWaveReader::LastCycleLen()
{
	return _lastCycleLen;
}

/*
void CWaveReader::Analyze()
{
	fprintf(stderr, "Analyzing file...\n");

	// Rewind to the first sample
	int savePos = _currentSampleNumber;

	// Calculate average samples per cycle
	__int64 iTotalCycleSamples=0;
	__int64 iTotalCycles=0;
	int cycleLen;
	while ((cycleLen=ReadCycleLen())>0)
	{
		iTotalCycleSamples += cycleLen;
		iTotalCycles++;
	}

	// Assuming there are roughly the same number of 0 and 1 bits we can now guess
	// the length of a short and long cycle.  Since there are twice as many cycles
	// required for 1 bits as zero bits we need to skew the average by 1.1249 to 
	// get the half way cycle length.
	//
	// Eg: say sampleRate is 48000:
	//       a long cycle (1200Hz) is 40 samples
	//     	 a short cycle (2400Hz) is 20 samples.
	// 
	// now lets say there's one 1 bit and one 0 bit:
	//       total samples = 40 + 20 + 20 = 80
	//       total cycles = 3
	//       average cycle = 80/3 = 26.667
	//
	// but the center frequency should be 30 so skew it
	//
	//       30/26.67 = 1.1249

	_avgCycleLength = int(1.1249 * iTotalCycleSamples / iTotalCycles);
	printf("[center cycle length: %i]\n", _avgCycleLength);

	_shortCycleLength = _avgCycleLength * 2 / 3;
	_longCycleLength = _avgCycleLength + (_avgCycleLength - _shortCycleLength);

	Seek(savePos);
}
*/

/*
void CWaveReader::Prepare()
{
	_cycleLengthAllowance = _avgCycleLength / 3-1;

	printf("[Short cycle length: %i +/- %i]\n", _shortCycleLength, _cycleLengthAllowance);
	printf("[Long cycle length: %i +/- %i]\n", _longCycleLength, _cycleLengthAllowance);
}
*/

