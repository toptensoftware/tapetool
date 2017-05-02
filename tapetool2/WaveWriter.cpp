//////////////////////////////////////////////////////////////////////////
// WaveWriter.cpp - implementation of CWaveWriter class

#include "precomp.h"

#include "WaveWriter.h"

CWaveWriter::CWaveWriter()
{
	_file = NULL;
	_amplitude = 14000;
	_square = false;
	_lastSquareSample = 0;
}

CWaveWriter::~CWaveWriter()
{
	Close();
}


// Create the wave file
bool CWaveWriter::Create(const char* fileName, int sampleRate, int sampleSize)
{
	// Create the file
	_file = fopen(fileName, "wb");
	if (_file==NULL)
	{
	    fprintf(stderr, "Could not create '%s' - %s (%i)\n", fileName, strerror(errno), errno);
		return false;
	}

	// Write the header
	InitWaveHeader(sampleRate, sampleSize);
	fwrite(&_waveHeader, sizeof(_waveHeader), 1, _file);

	return true;
}

int CWaveWriter::GetSampleRate()
{
	return _waveHeader.sampleRate;
}

int CWaveWriter::GetSampleSize()
{
	return _waveHeader.bitsPerSample;
}

void CWaveWriter::SetVolume(int volume)
{
	_amplitude = (_waveHeader.bitsPerSample==8 ? 0x7f : 0x7fff) * volume / 100;
}

int CWaveWriter::GetAmplitude()
{
	return _amplitude;
}

void CWaveWriter::SetSquare(bool square)
{
	_square = square;
}


void CWaveWriter::Close()
{
	if (_file==NULL)
		return;

	// Work out how much data was written
	fseek(_file, 0, SEEK_END);
	int dataBytes = ftell(_file) - sizeof(WAVEHEADER);

	// Update the header info
	_waveHeader.riffChunkSize += dataBytes;
	_waveHeader.dataChunkSize += dataBytes;

	// Seek back to start and rewrite the header
	fseek(_file, 0, SEEK_SET);
	fwrite(&_waveHeader, sizeof(_waveHeader), 1, _file);

	// Close the file and clean up
	fclose(_file);
	_file=NULL;
	memset(&_waveHeader, 0, sizeof(WAVEHEADER));

}

int CWaveWriter::CurrentPosition()
{
	return (ftell(_file) - sizeof(WAVEHEADER)) / (_waveHeader.bitsPerSample/8);
}

void CWaveWriter::InitWaveHeader(int sampleRate, int sampleSize)
{
	// RIFF chunk
	memcpy(&_waveHeader.riffChunkID, "RIFF", 4);
	_waveHeader.riffChunkSize = sizeof(_waveHeader)-8;		// We'll add the rest later
	memcpy(&_waveHeader.waveFormat, "WAVE", 4);

	// FMT chunk
	memcpy(&_waveHeader.fmtChunkID, "fmt ", 4);
	_waveHeader.fmtChunkSize = 16;
	_waveHeader.audioFormat = 1;	// PCM
	_waveHeader.numChannels = 1;	// Mono
	_waveHeader.sampleRate = sampleRate;
	_waveHeader.bitsPerSample = sampleSize;
	_waveHeader.blockAlign = _waveHeader.numChannels * _waveHeader.bitsPerSample/8;
	_waveHeader.byteRate = _waveHeader.sampleRate * _waveHeader.blockAlign;

	// DATA chunk
	memcpy(&_waveHeader.dataChunkID, "data", 4);
	_waveHeader.dataChunkSize = 0;
}

int CWaveWriter::SampleRate()
{
	return _waveHeader.sampleRate;
}

void CWaveWriter::RenderSample(short sample)
{
	if (_waveHeader.bitsPerSample==8)
	{
		char ch = (char)(sample + 128);
		fwrite(&ch, sizeof(ch), 1, _file);	
	}
	else
	{
		fwrite(&sample, sizeof(sample), 1, _file);	
	}

	_lastSquareSample = sample;
}

void CWaveWriter::RenderSquaredOffSample(short sample)
{
	// Make it square
	if (_square)
	{
		if (sample==0)
		{
			sample = _lastSquareSample;
		}
		else
		{
			sample = sample < 0 ? -_amplitude : _amplitude;
		}
	}

	RenderSample(sample);
}

void CWaveWriter::RenderSilence(int samples)
{
	for (int i=0; i<samples; i++)
		RenderSample(0);
}

void CWaveWriter::RenderWave(int cycles, int samples)
{
	for (int i=0; i<samples; i++)
	{
		double in = 2.0*PI*cycles*i/samples;
		double curve = sin(in);
		RenderSquaredOffSample(short(curve * _amplitude));
	}
}

/*
void CWaveWriter::RenderCycleKind(char kind)
{
	if (kind=='S')
	{
		RenderWave(1, _waveHeader.sampleRate / 300 / 8);
	}
	else if (kind=='L')
	{
		RenderWave(1, _waveHeader.sampleRate / 300 / 4);
	}
	else
	{
	}
}

void CWaveWriter::RenderBit(unsigned char bit)
{
	if (_baud == 300)
	{
		if (bit)
		{
			RenderWave(8, _waveHeader.sampleRate / 300);
		}
		else
		{
			RenderWave(4, _waveHeader.sampleRate / 300);
		}
	}
	else
	{
		if (bit)
		{
			RenderWave(2, _waveHeader.sampleRate / 1200);
		}
		else
		{
			RenderWave(1, _waveHeader.sampleRate / 1200);
		}
	}
}

void CWaveWriter::RenderByte(unsigned char byte)
{
	RenderBit(0);
	for (int i=0; i<8; i++)
	{
		RenderBit(byte & 0x01);
		byte>>=1;
	}
	RenderBit(1);
	RenderBit(1);
}

*/

void CWaveWriter::RenderProfiledCycleKind(char ch)
{
	assert(false);
}

void CWaveWriter::RenderProfiledBit(int speed, int bit)
{
	assert(false);
}
