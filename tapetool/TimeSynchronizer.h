//////////////////////////////////////////////////////////////////////////
// TimeSynchronizer.h - declaration of CTapeReader class

#ifndef __TIMESYNCHRONIZER_H
#define __TIMESYNCHRONIZER_H

class CWaveReader;
class CWaveWriter;

// Target for rendering a new tape recording - generates a wave file
class CTimeSynchronizer
{
public:
			CTimeSynchronizer();
	virtual ~CTimeSynchronizer();

	void Init(CWaveWriter* dest);
	void AddSample(short sample);
	void AddSyncPoint(double actual, double expected);
	void Complete();

	struct SYNC_POINT
	{
		double _actual;
		double _expected;
	};

	SYNC_POINT* _syncPoints;
	int _syncPointCount;
	int _syncPointAllocated;


	short* _samplesWithLeadin;
	short* _samples;
	int _sampleCount;
	int _samplesAllocated;

	CWaveWriter* _dest;
};

#endif	// __TIMESYNCHRONIZER_H

