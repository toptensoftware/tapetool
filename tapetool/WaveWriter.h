//////////////////////////////////////////////////////////////////////////
// WaveWriter.h - declaration of CWaveReader class

#ifndef __WAVEWRITER_H
#define __WAVEWRITER_H

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
	void SetVolume(int volume);
	void SetSquare(bool square);
	int GetAmplitude();
	void Close();
	void InitWaveHeader(int sampleRate, int sampleSize);
	int SampleRate();
	void RenderSample(short sample);
	void RenderSilence(int samples);
	void RenderWave(int cycles, int samples);
};

#endif	// __WAVEWRITER_H

