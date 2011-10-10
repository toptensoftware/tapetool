//////////////////////////////////////////////////////////////////////////
// WaveReader.h - declaration of CWaveReader class

#ifndef __WAVEREADER_H
#define __WAVEREADER_H

#include "FileReader.h"

// CWaveFileReader - reads audio data from a tape recording
class CWaveReader : public CFileReader
{
public:
			CWaveReader(CContext* ctx);
	virtual ~CWaveReader();

	void Close();
	int GetBytesPerSample();
	int GetSampleRate();
	int GetTotalSamples();

	virtual bool Open(const char* filename, Resolution res);
	virtual const char* GetDataFormat();
	virtual Resolution GetResolution();
	virtual void Delete();
	virtual bool IsWaveFile();
	virtual int CurrentPosition();
	virtual char* FormatDuration(int duration);
	virtual void Seek(int sampleNumber);
	virtual int ReadCycleLen();
	virtual char ReadCycleKind();
	virtual int LastCycleLen();
	virtual void Prepare();

	void SetShortCycleFrequency(int freq);
	void SetCycleLengths(int shortCycleSamples, int longCycleSamples);
	void SetDCOffset(int offset);
	int GetDCOffset();

	bool NextSample();
	bool HaveSample();
	int CurrentSample();
	int ReadRawSample();
	int ReadSmoothedSample();
	int ReadHalfCycle();

	FILE* _file;
	int _waveOffsetInBytes;
	int _waveEndInSamples;
	int _dataStartInSamples;
	int _dataEndInSamples;
	int _currentSampleNumber;
	int _sampleRate;
	int _bytesPerSample;
	int _avgCycleLength;
	int _shortCycleLength;
	int _longCycleLength;
	int _cycleLengthAllowance;
	int _currentSample;
	int _startOfCurrentHalfCycle;
	int _smoothingPeriod;
	int* _smoothingBuffer;
	int _smoothingBufferPos;
	int _smoothingBufferTotal;
	int _lastCycleLen;
	int _dc_offset;
	int _cycle_frequency;

};

#endif	// __WAVEREADER_H

