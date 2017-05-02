//////////////////////////////////////////////////////////////////////////
// Filter.h - declaration of CTapeReader class

#ifndef __Filter_H
#define __Filter_H

struct DataKind
{
	enum value
	{
		AudioSamples,
		CycleKinds,
		BitStream,
		ByteStream,
		TapeStream,
	};
};

struct CycleKind
{
	enum value
	{
		TooShort,
		Short,
		Indeterminate,
		Long,
		TooLong,
	};
};

class CFilterAudioSamples;
class CFilterCycleKinds;
class CFilterBitStream;
class CFilterByteStream;
class CFilterTapeStream;

class CFilter
{
public:
// Construction
			CFilter();
	virtual ~CFilter();

	virtual DataKind::value GetDataKind() = 0;

	virtual CFilterAudioSamples* AsAudioSamples() 
	{
		return NULL;
	};

	virtual CFilterCycleKinds* AsCycleKinds()
	{
		return NULL;
	}

	virtual CFilterBitStream* AsBitStream()
	{
		return NULL;
	}

	virtual CFilterByteStream* AsByteStream()
	{
		return NULL;
	}

	virtual CFilterTapeStream* AsTapeStream()
	{
		return NULL;
	}


	virtual void SetSource(CFilter* pSource) = 0;
	virtual CFilter* GetSource() = 0;

	virtual void SetRenderBaudRate(int baud) = 0;
	virtual int GetParsedBaudRate() = 0;

	virtual bool IsEOF() = 0;
};

class CFilterAudioSamples : public CFilter
{
	virtual DataKind::value GetDataKind() 
	{
		return DataKind::AudioSamples;
	};

	virtual CFilterAudioSamples* AsAudioSamples()
	{
		return this;
	};

	virtual int GetSampleRate() = 0;
	virtual int GetChannelCount() = 0;
	virtual float ReadSample(int channel) = 0;
};

class CFilterCycleKinds : public CFilter
{
	virtual DataKind::value GetDataKind()
	{
		return DataKind::CycleKinds;
	};

	virtual CFilterCycleKinds* AsCycleKinds()
	{
		return this;
	}

	virtual CycleKind::value ReadCycleKind() = 0;
};

class CFilterBitStream : public CFilter
{
	virtual DataKind::value GetDataKind()
	{
		return DataKind::ByteStream;
	};

	virtual CFilterBitStream* AsBitStream()
	{
		return this;
	}

	virtual uint8_t ReadBit() = 0;
};

class CFilterByteStream : public CFilter
{
	virtual DataKind::value GetDataKind()
	{
		return DataKind::ByteStream;
	};

	virtual CFilterByteStream* AsByteStream()
	{
		return this;
	}

	virtual uint8_t ReadByte() = 0;
};

class CFilterTapeStream : public CFilter
{
	virtual DataKind::value GetDataKind()
	{
		return DataKind::TapeStream;
	};

	virtual CFilterTapeStream* AsTapeStream()
	{
		return this;
	}

	virtual uint8_t ReadByte() = 0;
};

/*

// Audio Files
class CFilterWaveReader;
class CFilterWaveWriter;

// Cycle files
class CFilterCycleReader;		// File -> Cycle Kinds
class CFilterCycleWriter;		// Cycle Kinds -> File

// Binary files
class CFilterBinaryReader;	    // File -> ByteStream
class CFilterBinaryWriter;		// ByteStream -> File

// Tape files
class CFilterTapeReader;		// File -> TapeStream
class CFilterTapeWriter;		// TapeStream -> File

// Audio filters
class CFilterMonoMix;
class CFilterHighPass;
class CFilterLowPass;
class CFilterBandPass;
class CFilterGain;
class CFilterBiasAdjust;


// Type converters
class CFilterCycleDetector;		// Audio -> CycleKinds
class CFilterCycleRenderer;		// CycleKinds -> Audio
class CFilterCycleParser;		// CycleKinds -> Bits
class CFilterCycleGenerator;	// Bits -> CycleKinds
class CFilterBitParser;			// Bits -> Bytes
class CFilterBitGenerator;		// Bytes -> Bits
class CFilterTapeParser;		// Tape byte streeam -> byte stream
class CFilterTapeGenerator;		// Byte stream -> tape byte stream


// TAP Reader -> Bit Generator -> Cycle Generator -> Cycle Renderer -> Wave Writer
// Wave Reader -> Cycle Detector -> Cycle Parser -> Bit Parser ->  TAP Writer

// tapetool2 input.wav baud:1200 output.tcycles output.wav
*/

#endif	// __Filter_H
