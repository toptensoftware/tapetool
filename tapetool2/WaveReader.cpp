//////////////////////////////////////////////////////////////////////////
// WaveReader.cpp - implementation of CWaveReader class

#include "precomp.h"

#include "WaveReader.h"

//////////////////////////////////////////////////////////////////////////
// CWaveReader

// Constructor
CWaveReader::CWaveReader()
{
	_smoothingPeriod = 0;
	_smoothingBuffer = NULL;
	_file = NULL;
	_makeSquareWave = false;
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


const char* CWaveReader::GetFileName()
{
	return _filename;
}

int CWaveReader::CurrentPosition()
{
	return _currentSampleNumber;
}

bool CWaveReader::OpenFile(const char* filename)
{
	// Store filename
	_filename = filename;

	// Open the file
    _file=fopen(filename,"rb");
	if (_file==NULL)
	{
	    fprintf(stderr, "Could not open '%s' - %s (%i)\n", filename, strerror(errno), errno);
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

void CWaveReader::Close()
{
	if (_file!=NULL)
		fclose(_file);

	if (_smoothingBuffer!=NULL)
		delete[] _smoothingBuffer;
	_smoothingBuffer = NULL;
	_smoothingPeriod = 0;
	_waveOffsetInBytes = 0;
	_waveEndInSamples = 0;
	_dataStartInSamples = 0;
	_dataEndInSamples = 0;
	_currentSampleNumber = 0;
	_sampleRate = 0;
	_bytesPerSample = 1;
	_currentSample = 0;
	_filename = NULL;
	_dc_offset = 0;
	_amplify = 1;
}

void CWaveReader::SetSmoothingPeriod(int period)
{
	if (_smoothingBuffer!=NULL)
	{
		delete [] _smoothingBuffer;
		_smoothingBuffer=NULL;
	}

	_smoothingPeriod = period;
	if (_smoothingPeriod!=0)
	{
		_smoothingBuffer = new int[_smoothingPeriod];

		memset(_smoothingBuffer, 0, sizeof(int) * _smoothingPeriod);
		_smoothingBufferPos=0;
		_smoothingBufferTotal=0;
	}

	Seek(CurrentPosition());
}

int CWaveReader::GetSmoothingPeriod()
{
	return _smoothingPeriod;
}

void CWaveReader::SetMakeSquareWave(bool square)
{
	_makeSquareWave = square;
}

bool CWaveReader::GetMakeSquareWave()
{
	return _makeSquareWave;
}

void CWaveReader::SetDCOffset(int offset)
{
	_dc_offset = offset;
}

int CWaveReader::GetDCOffset()
{
	return _dc_offset;
}


void CWaveReader::SetAmplify(double amp)
{
	_amplify = amp;
}

double CWaveReader::GetAmplify()
{
	return _amplify;
}



void CWaveReader::SeekRaw(int sampleNumber)
{
	// Seek to sample
	fseek(_file, _waveOffsetInBytes + sampleNumber * _bytesPerSample, SEEK_SET);

	// Setup position info
	_currentSampleNumber = sampleNumber;

	// Read the sample
	_currentSample=ReadRawSample();
}

void CWaveReader::Seek(int sampleNumber)
{
	// Go back by size of smoothing buffer
	int startAtSampleNumber = sampleNumber - (_smoothingPeriod+1);
	if (startAtSampleNumber<0)
		startAtSampleNumber = 0;

	// Seek to sample
	SeekRaw(startAtSampleNumber);

	// Reset and refill the smoothing buffer
	memset(_smoothingBuffer, 0, _smoothingPeriod * sizeof(int));
	_smoothingBufferPos=0;
	_smoothingBufferTotal=0;
	while (CurrentPosition()  < sampleNumber)
	{
		if (!NextSample())
			break;
	}

}

bool CWaveReader::NextSample()
{
	_currentSample = ReadSample();



	if (_currentSample!=EOF_SAMPLE)
	{
		_currentSampleNumber++;
		return true;
	}
	else
		return false;
}

bool CWaveReader::HaveSample()
{
	return _currentSample!=EOF_SAMPLE;
}

int CWaveReader::CurrentSample()
{
	return _currentSample;
}

int CWaveReader::ReadSample()
{
	// Read the next raw sample
	int sample = ReadRawSample();
	if (sample==EOF_SAMPLE)
		return sample;

	// Translate it
	sample = (short)(_amplify * (_dc_offset + sample));
	if (_makeSquareWave)
	{
		int Range = _bytesPerSample == 1 ? 0x7f : 0x7fff;
		sample = (int)((sample<0 ? -_amplify : _amplify) * Range);
	}

	// Are we smoothing?
	if (_smoothingPeriod==0)
	{
		return sample;
	}

	// Calculate new position in circular buffer
	_smoothingBufferPos = (_smoothingBufferPos + 1) % _smoothingPeriod;

	// Update the moving total
	_smoothingBufferTotal -= _smoothingBuffer[_smoothingBufferPos];
	_smoothingBufferTotal += sample;

	// Update the buffer
	_smoothingBuffer[_smoothingBufferPos]=sample;

	// Return smoothed sample
	return _smoothingBufferTotal / _smoothingPeriod;
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
			return i-128;
		}
	}
	else
	{
		short s;
		if (fread(&s, sizeof(s), 1, _file)==0)
			return EOF_SAMPLE;
		else
		{
			return s;
		}
	}
}

