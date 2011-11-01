//////////////////////////////////////////////////////////////////////////
// WaveWriterProfiled.cpp - implementation of CWaveWriterProfiled class

#include "precomp.h"

#include "WaveWriterProfiled.h"
#include "TimeSynchronizer.h"
#include "SincResample.h"

CWaveWriterProfiled::CWaveWriterProfiled()
{
	_allocatedEntries = 0;
	_firstSpan = NULL;
	_currentSpan = NULL;
	_currentSampleNumber = 0;
	IncludeLeadIn = true;
	IncludeLeadOut = true;
	memset(_cycleLengths, 0, sizeof(_cycleLengths));
	memset(_bitLengths, 0, sizeof(_bitLengths));
	_timeSync = NULL;
	_totalEntries = 0;
	_entriesMatched = 0;
	_slices = 0;
//	_timeSync = NULL;
}

CWaveWriterProfiled::~CWaveWriterProfiled()
{
	if (_firstSpan!=NULL)
	{
		delete _firstSpan;
		_firstSpan = NULL;
	}
}

bool CWaveWriterProfiled::Create(const char* fileName, const char* profile)
{
	// Open the profile wave
	if (!_wave.OpenFile(profile))
		 return false;

	// Load the instrumentation file
	char temp[1024];
	strcpy(temp, _wave.GetFileName());
	strcat(temp, ".profile");
	if (!_instrumentation.Load(temp, _wave.GetTotalSamples()))
	{
		fprintf(stderr, "\nFailed to open instrumentation file %s - use --createbitprofile or --createcycleprofile option to create\n", temp);
		return false;
	}

	// Create the wave file
	if (!CWaveWriter::Create(fileName, _wave.GetSampleRate(), _wave.GetBytesPerSample()*8))
		return false;

	// Done
	return true;
}

void CWaveWriterProfiled::SetFixCycleTiming(bool value)
{
	if (value)
	{
		if (!_timeSync)
		{
			_timeSync = new CTimeSynchronizer();
			_timeSync->Init(this, srcQualityGood);
		}
	}
	else
	{
		if (_timeSync)
		{
			delete _timeSync;
			_timeSync = NULL;
		}
	}
}


void CWaveWriterProfiled::SetCycleKindLength(char kind, double length)
{
	if (kind>=0 && kind<127)
		_cycleLengths[kind] = length;
}

void CWaveWriterProfiled::SetBitLength(int speed, double lengthInSamples)
{
	if (speed>=0 && speed<16)
		_bitLengths[speed] = lengthInSamples;
		
}


void CWaveWriterProfiled::Close()
{
	CWaveWriter::Close();
	_wave.Close();
	_instrumentation.Reset();
}

Resolution CWaveWriterProfiled::GetProfiledResolution()
{
	return _instrumentation.GetResolution();
}

void CWaveWriterProfiled::AddRenderEntry(int speed, char kind)
{
	// Start a new span?
	if (_currentSpan == NULL || speed!=_currentSpan->_speed)
	{
		_currentSpan = new CSpan(_currentSpan, speed);
		if (_firstSpan==NULL)
			_firstSpan = _currentSpan;

		_allocatedEntries = 1024;
		_currentSpan->_entries = (char*)malloc(1024 * sizeof(char));
	}

	// Allocate more storage?
	if (_currentSpan->_length + 1 >= _allocatedEntries)
	{
		_allocatedEntries*=2;
		_currentSpan->_entries = (char*)realloc(_currentSpan->_entries, _allocatedEntries);
	}

	_currentSpan->_entries[_currentSpan->_length++] = kind;
	_totalEntries++;
}


void CWaveWriterProfiled::RenderProfiledCycleKind(char kind)
{
	AddRenderEntry(0, kind);
}

void CWaveWriterProfiled::RenderProfiledBit(int speed, int bit)
{
	AddRenderEntry(speed, (char)bit);
}

void CWaveWriterProfiled::CopySamples(int offset, int count, const char* type, int entries)
{
	int _shouldBe = CWaveWriter::CurrentPosition();
	if (_currentSampleNumber != _shouldBe)
	{
		int x = 3;
	}

	_slices++;
	printf("%4i %-10s %10i %10i %10i to %10i -> %10i to %10i %3i\n", _slices, type, count, entries, offset, offset + count, _currentSampleNumber, _currentSampleNumber+count, _entriesMatched * 100 / _totalEntries);

	_wave.Seek(offset);

	if (_timeSync)
	{
		for (int i=0; i<count; i++)
		{
			_timeSync->AddSample(_wave.CurrentSample());
			_wave.NextSample();
		}
	}
	else
	{
		for (int i=0; i<count; i++)
		{
			CWaveWriter::RenderSample(_wave.CurrentSample());
			_wave.NextSample();
		}
	}

	_currentSampleNumber += count;

	_shouldBe = CWaveWriter::CurrentPosition();
	if (_currentSampleNumber != _shouldBe)
	{
		int x = 3;
	}
}

bool CWaveWriterProfiled::Flush()
{
	if (_firstSpan==NULL)
		return true;

	printf("\n[\nRendering repaired wave file:\n\n");
	_currentSampleNumber=0;

	printf("   # type          samples %10s         original                    repaired           %%\n", _instrumentation.GetResolutionString());
	printf("---- ---------- ---------- ---------- ------------------------    ------------------------  ---\n");

	if (_timeSync)
		_timeSync->AddSyncPoint(0, 0);
	
	double syncActual = 0;
	double syncExpected = 0;

	// Copy leading samples
	if (IncludeLeadIn)
	{
		CopySamples(0, _instrumentation.LeadingSampleCount(), "lead-in", 0);
		if (_timeSync)
		{
			syncActual = syncExpected = _instrumentation.LeadingSampleCount();
			_timeSync->AddSyncPoint(syncActual, syncExpected);
		}
	}

	_entriesMatched = 0;
	int longestMatch = 0;
	int shortestMatch = 0x7FFFFFFF;

	// So by now we should have a full list of rendered bits that we can try to match up with the instrumentation
	for (CSpan* s = _firstSpan; s!=NULL; s=s->_next)
	{
		int pos = 0;
		while (pos < s->_length)
		{
			// Find matching sequence.  If we're not at the start, start one sample before
			INSTR_ENTRY* e;
			int matchLength;
			int searchPos = pos == 0 ? 0 : pos-1;
			if (!_instrumentation.FindSequence(s->_speed, s->_entries + searchPos, s->_length - searchPos, &e, &matchLength))
			{
				fprintf(stderr, "Failed to find matching patten in profiled file, aborting\n");
				return false;
			}

			// Get back to our correct position
			if (pos>0)
			{
				matchLength--;
				e++;
			}

//			printf("needed: '%c' found '%c'\n", s->_entries[pos + matchLength], e[matchLength]._kind);

			// Unless we matched the whole thing, don't include the last bit
			if (pos + matchLength < s->_length)
			{
				matchLength--;
			}

			// Work out the sample range to copy
			int startSample = e->_offset;
			int endSample = (e+matchLength)->_offset;
			int samples = endSample - startSample;

			_entriesMatched += matchLength;

			if (matchLength > longestMatch)
				longestMatch = matchLength;
			if (matchLength < shortestMatch)
				shortestMatch = matchLength;

			CopySamples(startSample, samples, "data", matchLength);

			// Create sync points
			if (_timeSync)
			{
				for (int i=0; i<matchLength; i++)
				{
					syncActual += e[i+1]._offset - e[i]._offset;
					
					if (_instrumentation.GetResolution()==resCycleKinds)
						syncExpected += _cycleLengths[e[i]._kind];
					else
						syncExpected += _bitLengths[s->_speed];

					_timeSync->AddSyncPoint(syncActual, syncExpected);
				}
			}

			/*
			int show = matchLength;
			if (show>40)
				show = 40;

			printf("          ");
			for (int i=0; i<show; i++)
			{
				printf("%c", s->_entries[pos+i] ? '1' : '0');
			}
			printf("\n\n");

			for (int i=0; i<show; i++)
			{
				printf("%c", e[i]._kind ? '1' : '0');
			}
			printf("\n");
			*/


			pos+=matchLength;
		}
	}

	// Copy trailing samples
	if (IncludeLeadOut)
	{
		int trailingOffset = _instrumentation.TrailingSamplesOffset();
		int trailingSamples = _wave.GetTotalSamples() - trailingOffset;
		CopySamples(trailingOffset, trailingSamples, "lead-out", 0);
		if (_timeSync)
		{
			syncActual += _wave.GetTotalSamples() - trailingOffset;
			syncExpected += _wave.GetTotalSamples() - trailingOffset;
			_timeSync->AddSyncPoint(syncActual, syncExpected);
		}
	}

	printf("\nProfiled rendering complete:\n");
	printf("  used %i of %i %s (%.2f%%) from profile wave\n", 
			_instrumentation.GetUsedEntries(), _instrumentation.GetTotalEntries(),
			_instrumentation.GetResolutionString(),
			double(_instrumentation.GetUsedEntries()) * 100.0 / double(_instrumentation.GetTotalEntries())
			);
	printf("  longest match: %i\n", longestMatch);
	printf("  shortest match: %i\n", shortestMatch);
	printf("  total slices: %i\n", _slices);



	if (_timeSync)
	{
		printf("\n\nFixing timing...\n");
		_timeSync->Complete();
	}

	printf("\nFinished!\n]\n\n");

	return true;
}
