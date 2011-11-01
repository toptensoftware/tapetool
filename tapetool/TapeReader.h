//////////////////////////////////////////////////////////////////////////
// TapeReader.h - declaration of CTapeReader class

#ifndef __TAPEREADER_H
#define __TAPEREADER_H

#include "WaveReader.h"
#include "FileReader.h"
#include "CycleDetector.h"

// CWaveFileReader - reads audio data from a tape recording
class CTapeReader : public CFileReader
{
public:
			CTapeReader(CCommandStd* cmd);
	virtual ~CTapeReader();

	void Close();
	int GetBytesPerSample();
	int GetSampleRate();
	int GetTotalSamples();

	bool OpenFile(const char* filename);

	CWaveReader* GetWaveReader() { return &_wave; }

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
	virtual CInstrumentation* GetInstrumentation() { return _instrumentation; }
	virtual bool SyncToBit(bool verbose);
	virtual bool SyncToByte(bool verbose);

	char ReadCycleKindInternal();
	void SetShortCycleFrequency(int freq);
	void SetCycleLengths(int shortCycleSamples, int longCycleSamples);
	void SetCycleMode(CycleMode mode);
	CycleMode GetCycleMode();

	bool NextSample();
	bool HaveSample();
	int CurrentSample();

	CWaveReader _wave;
	int _avgCycleLength;
	int _shortCycleLength;
	int _longCycleLength;
	int _cycleLengthAllowance;
	int _startOfCurrentHalfCycle;
	int _lastCycleLen;
	int _cycle_frequency;
	CInstrumentation* _instrumentation;
};

#endif	// __TAPEREADER_H

