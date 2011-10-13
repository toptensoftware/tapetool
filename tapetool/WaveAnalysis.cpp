//////////////////////////////////////////////////////////////////////////
// WaveAnalysis.cpp - implementation of CWaveAnalysis class

#include "precomp.h"

#include "WaveAnalysis.h"
#include "WaveReader.h"

struct BLOCK_DATA
{
	int minAmplitude;
	int maxAmplitude;
};

int compareMinAmplitude(const void* va, const void* vb)
{
	BLOCK_DATA* a = (BLOCK_DATA*)va;
	BLOCK_DATA* b = (BLOCK_DATA*)vb;
								 
	return a->minAmplitude - b->minAmplitude;
}

int compareMaxAmplitude(const void* va, const void* vb)
{
	BLOCK_DATA* a = (BLOCK_DATA*)va;
	BLOCK_DATA* b = (BLOCK_DATA*)vb;

	return a->maxAmplitude - b->maxAmplitude;
}

int compareInts(const void* va, const void* vb)
{
	return *(int*)va - *(int*)vb;
}

void AnalyseWave(CWaveReader* wf, int from, int samples, WAVE_INFO& info)
{
	memset(&info, 0, sizeof(info));

	int savePos = wf->CurrentPosition();

	// Allocate block data structures
	int numBlocks =  wf->GetTotalSamples() / wf->GetSampleRate();
	if (wf->GetTotalSamples() % wf->GetSampleRate())
		numBlocks++;
	int cbBlocks = sizeof(BLOCK_DATA) * numBlocks;
	BLOCK_DATA* blocks = (BLOCK_DATA*)malloc(cbBlocks);
	memset(blocks, 0, cbBlocks);

	int prev = 0;
	bool first = true;

	info.maxAmplitude = 0;
	info.minAmplitude = 0;
	info.sampleRate = wf->GetSampleRate();
	info.totalCycles = 0;

	int samplesLeftInBlock = wf->GetSampleRate();
	BLOCK_DATA* currBlock = blocks;

	wf->Seek(from);

	int startPos = wf->CurrentPosition();

	int to = samples > 0 ? startPos + samples : 0;

	CCycleDetector cd(wf->GetCycleMode());

	// Process all samples
	int index = 0;
	while (wf->HaveSample())
	{
		int sample = wf->CurrentSample();
		if (cd.IsNewCycle(sample))
		{
			info.totalCycles++;
		}


		if (sample<info.minAmplitude)
			info.minAmplitude = sample;
		if (sample>info.maxAmplitude)
			info.maxAmplitude = sample;

		if (sample<currBlock->minAmplitude)
			currBlock->minAmplitude = sample;
		if (sample>currBlock->maxAmplitude)
			currBlock->maxAmplitude = sample;

		samplesLeftInBlock--;
		if (samplesLeftInBlock==0)
		{
			samplesLeftInBlock = wf->GetSampleRate();
			currBlock++;
		}

		prev = sample;
		wf->NextSample();
		if (to>0 && wf->_currentSampleNumber >= to)
			break;

		first = false;
	}

	info.totalSamples = wf->CurrentPosition() - startPos;

	// Work out median amplitudes
	qsort(blocks, numBlocks, sizeof(BLOCK_DATA), compareMinAmplitude);
	info.medianMinAmplitude = blocks[numBlocks/2].minAmplitude;
	qsort(blocks, numBlocks, sizeof(BLOCK_DATA), compareMaxAmplitude);
	info.medianMaxAmplitude = blocks[numBlocks/2].maxAmplitude;

	if (info.totalCycles!=0)
	{
		info.avgSamplesPerCycle = info.totalSamples / info.totalCycles;
		info.avgCycleFrequency = (double)wf->GetSampleRate() * info.totalCycles / info.totalSamples;
	}
	
	free(blocks);

	if (info.totalCycles>0)
	{
		// Repeat, this time building a list of cycle times
		// Process all samples
		int* cycleList = (int*)malloc(sizeof(int) * info.totalCycles);
		int crossingCount = 0;
		int cycleCount = 0;
		first = true;
		wf->Seek(startPos);
		int cyclePos = wf->CurrentPosition();
		cd.Reset();
		while (wf->HaveSample())
		{
			int sample = wf->CurrentSample();
			if (cd.IsNewCycle(sample))
			{
				crossingCount++;

				int cycleLen = wf->CurrentPosition() - cyclePos;
				cycleList[cycleCount++] = cycleLen;
				cyclePos = wf->CurrentPosition();
			}

			prev = sample;
			wf->NextSample();
			if (to>0 && wf->_currentSampleNumber >= to)
				break;

			first = false;
		}

		// Sort cycles lengths
		qsort(cycleList, cycleCount, sizeof(int), compareInts);

		if (cycleCount>0)
		{
			info.medianShortCycleLength = cycleList[cycleCount/3];
			info.medianShortCycleFrequency = (double)wf->GetSampleRate() / info.medianShortCycleLength;
			info.medianLongCycleLength = cycleList[cycleCount * 5/6];
			info.medianLongCycleFrequency = (double)wf->GetSampleRate() / info.medianLongCycleLength;
		}

		free(cycleList);
	}

	// Rewind
	wf->Seek(savePos);
}