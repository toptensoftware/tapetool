//////////////////////////////////////////////////////////////////////////
// WaveWriter.h - declaration of CTapeReader class

#ifndef __WAVEWRITER_H
#define __WAVEWRITER_H

#include "FileReader.h"

#pragma pack(1)
struct WAVEHEADER
{
	unsigned int riffChunkID;
	unsigned int riffChunkSize;
	unsigned int waveFormat;

	unsigned int fmtChunkID;
	unsigned int fmtChunkSize;
	unsigned short audioFormat;
	unsigned short numChannels;
	unsigned int sampleRate;
	unsigned int byteRate;
	unsigned short blockAlign;
	unsigned short bitsPerSample;

	unsigned int dataChunkID;
	unsigned int dataChunkSize;
	
	// wave sample data to follow
};
#pragma pack()


// Target for rendering a new tape recording - generates a wave file
class CWaveWriter
{
public:
			CWaveWriter();
	virtual ~CWaveWriter();

	FILE* _file;
	int _amplitude;
	WAVEHEADER _waveHeader;
	bool _square;
	short _lastSquareSample;


	bool Create(const char* fileName, int sampleRate, int sampleSize);
	int GetSampleRate();
	int GetSampleSize();

	void SetVolume(int volume);
	void SetSquare(bool square);
	int GetAmplitude();
	void InitWaveHeader(int sampleRate, int sampleSize);
	int SampleRate();
	void RenderSample(short sample);
	void RenderSquaredOffSample(short sample);
	void RenderSilence(int samples);
	void RenderWave(int cycles, int samples);
	int CurrentPosition();
	
	virtual void Close();
	virtual Resolution GetProfiledResolution() { return resNA; };
	virtual void RenderProfiledCycleKind(char kind);
	virtual void RenderProfiledBit(int speed, int bit);
	virtual bool Flush() { return true; };
};

#endif	// __WAVEWRITER_H

