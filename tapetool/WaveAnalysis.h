//////////////////////////////////////////////////////////////////////////
// WaveAnalysis.h - declaration of CWaveReader class

#ifndef __WAVEANALYSIS_H
#define __WAVEANALYSIS_H

class CWaveReader;

struct WAVE_INFO
{
	int totalSamples;
	int totalCycles;
	int sampleRate;
	int minAmplitude;
	int maxAmplitude;
	int medianMinAmplitude;
	int medianMaxAmplitude;
	int avgSamplesPerCycle;
	double avgCycleFrequency;
	int medianShortCycleLength;
	double medianShortCycleFrequency;
	int medianLongCycleLength;
	double medianLongCycleFrequency;
};


void AnalyseWave(CWaveReader* wf, int from, int samples, WAVE_INFO& info);

#endif	// __WAVEANALYSIS_H

