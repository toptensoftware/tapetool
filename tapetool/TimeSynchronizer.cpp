//////////////////////////////////////////////////////////////////////////
// TimeSynchronizer.cpp - implementation of CTimeSynchronizer class

#include "precomp.h"

#include "TimeSynchronizer.h"
#include "WaveReader.h"
#include "WaveWriter.h"

void linear_resample(short* dest, short* source, int oldLen, int newLen)
{
	double scale = double(oldLen-1) /double(newLen-1);
	for (int i=0; i<newLen-1; i++)
	{
		double pos = scale * i;
		int prev = source[int(pos)];
		int next = source[int(pos+1)];
		*dest++ = short(prev + (next-prev) * (pos - floor(pos)));
	}

	*dest++ = source[oldLen-1];
}



CTimeSynchronizer::CTimeSynchronizer()
{
	_syncPoints = NULL;	
	_syncPointAllocated = 0;
	_syncPointCount = 0;
	_samples = NULL;
	_sampleCount = 0;
	_samplesAllocated = 0;

	_dest = NULL;
}

CTimeSynchronizer::~CTimeSynchronizer()
{
	if (_syncPoints!=NULL)
		free(_syncPoints);
	if (_samples!=NULL)
		free(_samples);
}

void CTimeSynchronizer::Init(CWaveWriter* dest)
{
	_dest = dest;
}

void CTimeSynchronizer::AddSample(short sample)
{
	if (_sampleCount+1 >= _samplesAllocated)
	{
		if (_samples)
		{
			_samplesAllocated *= 2;
			_samples = (short*)realloc(_samples, sizeof(short) * _samplesAllocated);
		}
		else
		{
			_samplesAllocated = 1024;
			_samples = (short*)malloc(sizeof(short) * _samplesAllocated);
		}
	}

	_samples[_sampleCount++] = sample;

}

void CTimeSynchronizer::AddSyncPoint(double actual, double expected)
{
	if (_syncPointCount+1 >= _syncPointAllocated)
	{
		if (_syncPoints)
		{
			_syncPointAllocated *= 2;
			_syncPoints = (SYNC_POINT*)realloc(_syncPoints, sizeof(SYNC_POINT) * _syncPointAllocated);
		}
		else
		{
			_syncPointAllocated = 1024;
			_syncPoints = (SYNC_POINT*)malloc(sizeof(SYNC_POINT) * _syncPointAllocated);
		}
	}

	_syncPoints[_syncPointCount]._actual = actual;
	_syncPoints[_syncPointCount]._expected = expected;
	_syncPointCount++;
}


void CTimeSynchronizer::Complete()
{
	// Resample all cycles
	for (int i=1; i<_syncPointCount; i++)
	{
		SYNC_POINT* pt = _syncPoints + i;
		SYNC_POINT* ptPrev = _syncPoints + i-1;

		double sourceSamples = pt->_actual - ptPrev->_actual;
		double destSamples = pt->_expected - ptPrev->_expected;

		if (destSamples>1024)
		{
			int x=3;
		}

		if (sourceSamples == destSamples)
		{
			for (int i=0; i<int(sourceSamples); i++)
				_dest->RenderSample(_samples[(int)(ptPrev->_actual+i)]);
		}
		else
		{
			short new_samples[1024];


			linear_resample(new_samples, _samples + (int)ptPrev->_actual, (int)sourceSamples, (int)destSamples);

			/*
			SincResample_NIS(
				new_samples,					// output buffer
				_samples + (int)ptPrev->_actual,// input buffer
				0,								// starting sample number
				int(destSamples),				// Number of output samples
				destSamples,					// Destination sample rate	
				sourceSamples,					// Source sample rate
				_sinc,							// The curve
				1								// number of channels
				);
			*/


			for (int i=0; i<int(destSamples); i++)
			{
				_dest->RenderSample(new_samples[i]);
			}
		}
	}
}
