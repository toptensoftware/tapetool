//////////////////////////////////////////////////////////////////////////
// WaveWriterProfiled.cpp - implementation of CWaveWriterProfiled class

#include "precomp.h"

#include "WaveWriterProfiled.h"

CWaveWriterProfiled::CWaveWriterProfiled()
{
	_allocatedBits = 0;
	_firstSpan = NULL;
	_currentSpan = NULL;
	_currentSampleNumber = 0;
	IncludeLeadIn = true;
	IncludeLeadOut = true;
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
		fprintf(stderr, "\nFailed to open instrumentation file %s - use --instrument option to create\n", temp);
		return false;
	}

	// Create the wave file
	if (!CWaveWriter::Create(fileName, _wave.GetSampleRate(), _wave.GetBytesPerSample()*8))
		return false;

	// Done
	return true;
}

void CWaveWriterProfiled::Close()
{
	CWaveWriter::Close();
	_wave.Close();
	_instrumentation.Reset();
}

bool CWaveWriterProfiled::IsProfiled()
{
	return true;
}

void CWaveWriterProfiled::RenderProfiledBit(int bit, int speed)
{
	// Start a new span?
	if (_currentSpan == NULL || speed!=_currentSpan->_speed)
	{
		_currentSpan = new CSpan(_currentSpan, speed);
		if (_firstSpan==NULL)
			_firstSpan = _currentSpan;

		_allocatedBits = 1024;
		_currentSpan->_bits = (char*)malloc(1024 * sizeof(char));
	}

	// Allocate more storage?
	if (_currentSpan->_length + 1 >= _allocatedBits)
	{
		_allocatedBits*=2;
		_currentSpan->_bits = (char*)realloc(_currentSpan->_bits, _allocatedBits);
	}

	_currentSpan->_bits[_currentSpan->_length++] = (char)bit;
}

void CWaveWriterProfiled::CopySamples(int offset, int count, const char* type, int bits)
{
	int _shouldBe = CWaveWriter::CurrentPosition();
	if (_currentSampleNumber != _shouldBe)
	{
		int x = 3;
	}

	printf("   %-10s %10i %10i %10i to %10i -> %10i to %10i\n", type, count, bits, offset, offset + count, _currentSampleNumber, _currentSampleNumber+count);

	_wave.Seek(offset);
	for (int i=0; i<count; i++)
	{
		CWaveWriter::RenderSample(_wave.CurrentSample());
		_wave.NextSample();
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

	printf("   type          samples       bits         original                    repaired\n");
	printf("   ---------- ---------- ---------- ------------------------    ------------------------\n");

	
	// Copy leading samples
	if (IncludeLeadIn)
		CopySamples(0, _instrumentation.LeadingSampleCount(), "lead-in", 0);

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
			if (!_instrumentation.FindSequence(s->_speed, s->_bits + searchPos, s->_length - searchPos, &e, &matchLength))
			{
				fprintf(stderr, "Failed to find matching bit patten in profiled file, aborting\n");
				return false;
			}

			// Get back to our correct position
			if (pos>0)
			{
				matchLength--;
				e++;
			}

			// Unless we matched the whole thing, don't include the last bit
			if (pos + matchLength < s->_length)
			{
				matchLength--;
			}

			// Work out the sample range to copy
			int startSample = e->_offset;
			int endSample = (e+matchLength)->_offset;
			int samples = endSample - startSample;

			CopySamples(startSample, samples, "data", matchLength);

			int show = matchLength;
			if (show>40)
				show = 40;

			/*
			printf("          ");
			for (int i=0; i<show; i++)
			{
				printf("%c", s->_bits[pos+i] ? '1' : '0');
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
		CopySamples(trailingOffset, _wave.GetTotalSamples() - trailingOffset, "lead-out", 0);
	}

	printf("\n\nFinished!\n]\n\n");

	return true;
}
