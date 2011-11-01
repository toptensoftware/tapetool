//////////////////////////////////////////////////////////////////////////
// WaveWriterProfiled.h - declaration of CTapeReader class

#ifndef __WAVEWRITERPROFILED_H
#define __WAVEWRITERPROFILED_H

#include "TapeReader.h"
#include "WaveWriter.h"
#include "Instrumentation.h"

class CTimeSynchronizer;

class CWaveWriterProfiled : public CWaveWriter
{
public:
	CWaveWriterProfiled();
	virtual ~CWaveWriterProfiled();

	bool Create(const char* fileName, const char* profile);
	void SetFixCycleTiming(bool value);
	void SetCycleKindLength(char kind, double lengthInSamples);
	void SetBitLength(int speed, double lengthInSamples);
	virtual void Close();
	virtual Resolution GetProfiledResolution();
	virtual void RenderProfiledCycleKind(char kind);
	virtual void RenderProfiledBit(int speed, int bit);
	virtual bool Flush();


	bool IncludeLeadIn;
	bool IncludeLeadOut;

	void CopySamples(int offset, int count, const char* type, int entries);

	class CSpan
	{
	public:
		CSpan(CSpan* prev, int speed)
		{
			if (prev!=NULL)
				prev->_next = this;
			_length = 0;
			_speed = speed;
			_entries = NULL;
			_next = NULL;
		}

		~CSpan()
		{
			if (_entries!=NULL)
				free(_entries);
			if (_next!=NULL)
				delete _next;
		}

		int _length;
		int _speed;
		char* _entries;
		CSpan* _next;
	};


	CWaveReader _wave;
	CInstrumentation _instrumentation;
	int _allocatedEntries;
	int _totalEntries;
	int _entriesMatched;
	int _slices;
	CSpan* _firstSpan;
	CSpan* _currentSpan;
	int _currentSampleNumber;
	double _cycleLengths[127];
	double _bitLengths[16];
	CTimeSynchronizer* _timeSync;

	void AddRenderEntry(int speed, char kind);
};		

#endif	// __WAVEWRITERPROFILED_H

