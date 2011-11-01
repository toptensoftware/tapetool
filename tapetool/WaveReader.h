//////////////////////////////////////////////////////////////////////////
// WaveReader.h - declaration of CWaveReader class

#ifndef __WAVEREADER_H
#define __WAVEREADER_H

// CWaveFileReader - reads audio data from a tape recording
class CWaveReader
{
public:
			CWaveReader();
	virtual ~CWaveReader();

	void Close();
	int GetBytesPerSample();
	int GetSampleRate();
	int GetTotalSamples();
	const char* GetFileName();

	bool OpenFile(const char* filename);

	void SetDCOffset(int offset);
	int GetDCOffset();
	void SetAmplify(double dbl);
	double GetAmplify();
	void SetSmoothingPeriod(int period);
	int GetSmoothingPeriod();
	void SetMakeSquareWave(bool square);
	bool GetMakeSquareWave();


	int CurrentPosition();
	void SeekRaw(int sampleNumber);
	void Seek(int sampleNumber);

	bool NextSample();
	bool HaveSample();
	int CurrentSample();
	int ReadRawSample();
	int ReadSample();

	FILE* _file;
	int _waveOffsetInBytes;
	int _waveEndInSamples;
	int _dataStartInSamples;
	int _dataEndInSamples;
	int _currentSampleNumber;
	int _sampleRate;
	int _bytesPerSample;
	int _currentSample;
	const char* _filename;
	int _smoothingPeriod;
	int* _smoothingBuffer;
	int _smoothingBufferPos;
	int _smoothingBufferTotal;
	int _dc_offset;
	double _amplify;
	bool _makeSquareWave;
};

#endif	// __WAVEREADER_H

