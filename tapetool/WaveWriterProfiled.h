//////////////////////////////////////////////////////////////////////////
// WaveWriterProfiled.h - declaration of CTapeReader class

#ifndef __WAVEWRITERPROFILED_H
#define __WAVEWRITERPROFILED_H

#include "TapeReader.h"
#include "WaveWriter.h"
#include "Instrumentation.h"

class CWaveWriterProfiled : public CWaveWriter
{
public:
	CWaveWriterProfiled();
	virtual ~CWaveWriterProfiled();

	bool Create(const char* fileName, const char* profile);

	virtual void Close();
	virtual bool IsProfiled();
	virtual void RenderProfiledBit(int bit, int speed);
	virtual bool Flush();

	bool IncludeLeadIn;
	bool IncludeLeadOut;

	void CopySamples(int offset, int count, const char* type, int bits);

	class CSpan
	{
	public:
		CSpan(CSpan* prev, int speed)
		{
			if (prev!=NULL)
				prev->_next = this;
			_length = 0;
			_speed = speed;
			_bits = NULL;
			_next = NULL;
		}

		~CSpan()
		{
			if (_bits!=NULL)
				free(_bits);
			if (_next!=NULL)
				delete _next;
		}

		int _length;
		int _speed;
		char* _bits;
		CSpan* _next;
	};

	CWaveReader _wave;
	CInstrumentation _instrumentation;
	int _allocatedBits;
	CSpan* _firstSpan;
	CSpan* _currentSpan;
	int _currentSampleNumber;
};		

#endif	// __WAVEWRITERPROFILED_H

