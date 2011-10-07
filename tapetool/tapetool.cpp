/*
 * TAPETOOL
 * Utility for working with microbee tape recordings
 *
 * Copyright (c) 2011 Topten Software (Brad Robinson)
 *
 * Based loosely on TAPETOOL utility:
 *
 * Copyright (c) 2010 by Martin D. J. Rosenau
 *
 * This file is provided as freeware under the GPL 2.0 license
 *
 * The file is assumed to be compiled an run on a little-endian
 * machine (such as an i386 compatible)
 */

#include "precomp.h"

#pragma pack(1)

struct WAVEHEADER
{
	unsigned int riffChunkID;
	unsigned int riffChunkSize;
	unsigned int waveFormat;

	unsigned int fmtChunkID;
	unsigned int fmtChunkSize;
	unsigned short audioFormat;
	unsigned short numChannels;
	unsigned int sampleRate;
	unsigned int byteRate;
	unsigned short blockAlign;
	unsigned short bitsPerSample;

	unsigned int dataChunkID;
	unsigned int dataChunkSize;
	
	// wave sample data to follow
};


struct TAPE_HEADER
{
	char			filename[6];
	char			filetype;
	unsigned short	datalen;
	unsigned short  loadaddr;
	unsigned short  startaddr;
	unsigned char	speed;
	unsigned char	autostart;
	unsigned char	unused;
};

#pragma pack()


// Command line arguments
typedef int (*fnCmd)();
char* files[10];
int file_count = 0;
int smoothing = 0;
bool showSyncData = false;
bool perLineMode = false;
bool showPositionInfo = true;
int from = 0;
int samples = 0;
bool showZeroCrossings = false;
bool allowBadCycles = false;
const char* renderFileName = NULL;
const char* binaryFileName = NULL;
int leadingSilence = 2;
int leadingZeros = 0;
bool analyzeCycles = false;
int renderSampleRate = 24000;
int renderSampleSize = 8;
int renderVolume = 10;
int renderBaud = 300;
bool renderSine = false;
bool binaryDgosHeader = false;
bool binaryTapFile = false;
int dc_offset = 0;
int cycle_freq = 2400;
bool trs80 = false;

// The input and output files
class CFileReader;
class CRenderFile;
CFileReader* file = NULL;
CRenderFile* renderFile = NULL;
FILE* binaryFile = NULL;



// Various resolutions at which data can be processed
enum Resolution
{
	resSamples,
	resCycles,
	resCycleKinds,
	resBits,
	resBytes,
};



// Base virtual file reader, derived classes read data from a text or wave file
// This base class provides the routine for converting up from lower resolution format
// to higher level format (eg: samples -> cycles -> bits -> bytes) and for synchronizing
// to bit/bit boundaries
class CFileReader
{
public:
	virtual void Delete()=0;
	virtual bool IsWaveFile()=0;
	virtual int CurrentPosition()=0;
	virtual int ReadCycleLen()=0;
	virtual char ReadCycleKind()=0;
	virtual void Analyze()=0;
	virtual void Seek(int position)=0;
	virtual char* FormatDuration(int duration)=0;
	virtual int LastCycleLen()=0;

	char ReadCycleKindChecked(bool verbose)
	{
		int pos = CurrentPosition();
		char kind = ReadCycleKind();

		// Error?
		if (kind=='<' || kind=='>')
		{
			if (allowBadCycles && (LastCycleLen() >2 && LastCycleLen() < 200))
			{
				return kind;
			}

			if (verbose)
				printf("[Invalid cycle kind '%c' - %i samples - at %i]", kind, LastCycleLen(), pos);
			return 0;
		}

		return kind;
	}

	virtual bool SyncToBit(bool verbose)
	{
		if (verbose)
			printf("[BitSync:");

		char buf[3];
		int offs[3];
		int cyclesRead = 0;

		buf[0]='?';
		buf[1]='?';
		buf[2]='?';
		offs[0]=0;
		offs[1]=0;
		offs[2]=0;

		while (true)
		{
			int cycleStart = CurrentPosition();

			// Read the next cycle kind
			char kind = ReadCycleKind();

			// EOF?
			if (kind == 0)
			{
				if (verbose)
					printf(":eof]");
				return false;
			}

			// Dump it
			if (verbose)
				printf("%c", kind);

			// Shuffle buffer
			buf[0] = buf[1];
			buf[1] = buf[2];
			buf[2] = kind;
			offs[0] = offs[1];
			offs[1] = offs[2];
			offs[2] = cycleStart;

			// Found a boundary?
			if ((buf[0]=='S' && buf[2]=='L') || (buf[0]=='L' && buf[2]=='S'))
			{
				int savePos = CurrentPosition();

				int boundary = (buf[1]=='?' || buf[1]==buf[2]) ? 1 : 2;

				Seek(offs[boundary]);

				// Read at least a 0 and a 1 bit
				bool error = false;
				int bitCount[2] = { 0, 0 };
				while (true)
				{
					int bit = ReadBit(false);
					if (bit<0)
					{
						error = true;
						break;
					}

					bitCount[bit]++;
					if (bitCount[0]>2 && bitCount[1]>2)
						break;
				}

				// Synced?
				if (!error)
				{
					// Yes!
					if (verbose)
						printf(" rewound %i cycles to sync at %i]", 3-boundary, offs[boundary]);
					Seek(offs[boundary]);
					return true;
				}

				// Now where were we?
				Seek(savePos);
			}
		}
	}										 


	virtual int ReadBit(bool verbose = true)
	{	
		int savePos = CurrentPosition();


		// This is where the "rubber meets the road" so to speak
		//
		// To convert wave cycles to bits, we ideally want 4 long cycles for a "0" bit
		// bit, or 8 short cycles for a "1" bit.  In  practice, at the boundary between
		// two different bits there is often an ambiguous cycle - probably because
		// it's half short, half long.
		//
		// So, we look for 3 long cycles or 7 short cycles and allow anything at either end
		//
		// Also, in some files, occassionally things "slip" - so if the first or last two cycles
		// conflict, we allow it but resync the current/next bit to the boundary between those
		// conflicting cycles.

		char bitKind=0;				// The kind of bit we reading
		int cyclesRead = 0;			// The number of cycles in the current bit that have been read
		int actualCyclesRead = 0;	// The number of cycles read, including possible 1 extra for the lead slip
		bool resynced = false;		// Have we lead slip resynced
		
		while (true)
		{
			// Remember where this cycle is, incase we need to 
			int currentCyclePos = CurrentPosition();

			// Get the next cycle
			char cycle = ReadCycleKindChecked(verbose);
			if (cycle==0)
				return -1;
			cyclesRead++;
			actualCyclesRead++;

			// First cycle
			if (cyclesRead == 1)
			{
				if (cycle=='S' || cycle=='L')
				{
					bitKind = cycle;
				}
				continue;
			}

			// Allow a bit resync after the first cycle
			if (cyclesRead == 2 && ((cycle=='S' && bitKind=='L') || (cycle=='L' && bitKind=='S')))
			{
				if (resynced)
				{
					if (verbose)
						printf("[leading bit error at %i - alternating S/L cycles]", savePos);
					return -1;
				}

				// Leading resync
				resynced = true;
				bitKind=cycle;
				cyclesRead--;
				continue;
			}

			// Second cycle setting bitkind?
			if (bitKind==0 && (cycle=='S' || cycle=='L'))
			{
				bitKind=cycle;
			}

			// Second cycle, still unknown type?
			if (bitKind==0)
			{
				if (verbose)
					printf("[leading bit error at %i - two consecutive ambiguous cycles]", savePos);
				return -1;
			}

			// How many cycles are typical for this bit kind?
			int expectedCycles = bitKind == 'S' ? 8 : 4;

			// Internal cycle
			if (cyclesRead <= expectedCycles-1)
			{
				if (cycle != bitKind)
				{
					if (verbose)
						printf("[internal bit error at %i - cycle number %i should have been %c but was %c]", savePos, actualCyclesRead, bitKind, cycle);
					return -1;
				}
				continue;
			}

			// Trailing cycle
			if (cyclesRead == expectedCycles)
			{
				// Conflict? Trailing resync
				if (cycle != bitKind && (cycle=='S' || cycle=='L'))
				{
					Seek(currentCyclePos);
				}

				// Success!
				return bitKind == 'S' ? 1 : 0;
			}
		}
	}

	virtual bool SyncToByte(bool verbose)
	{
		if (verbose)
			printf("[ByteSync:");

		// Sync to next bit
		if (!SyncToBit(verbose))
			return false;
		if (verbose)
			printf(" ");

		while (true)
		{
			// Remember start of this bit
			int syncBit = CurrentPosition();

			// Try to read bytes
			int byteSyncMask = 0;
			while (true)
			{
				// Read a byte, bail if can't...
				int byte = ReadByte(false);
				if (byte<0)
					break;

				byteSyncMask |= byte;

				// if we find all 0 or we've a seen a 1 bit in every data byte position
				// then we've synchronized...
				if (byte==0 || byteSyncMask==0xFF)
				{
					Seek(syncBit);
					if (verbose)
						printf(" synced at %i]", syncBit);
					return true;
				}
			}

			// Rewind to start of the bit
			Seek(syncBit);

			// Skip one bit
			int skipBit = ReadBit(false);
			if (skipBit<0)
			{
				// Failed to read a bit, need to resync...
				if (!SyncToBit(verbose))
				{
					if (verbose)
						printf(":no bit sync]");
					return false;
				}
				if (verbose)
					printf(" ");
			}
			else
			{
				// Print the skipped bit
				if (verbose)
					printf("%i", skipBit);
			}

			if (CurrentPosition()==syncBit)
			{
				int x=3;
			}
		}
	}

	virtual int ReadByte(bool verbose=true)
	{
		// Read 11 bits to make a byte : 0nnnnnnnn11 (little endian order)
		int byte = 0;
		for (int i=0; i<11; i++)
		{
			int offset = CurrentPosition();
			int bit = ReadBit(verbose);
			if (bit<0)
				return -1;

			if (i==0)
			{
				if (bit!=0)
				{
					if (verbose)
						printf("[Corrupted data at %i, byte leading bit should be 0, found %i]", offset, bit);
					return -1;
				}
			}
			else if (i>=1 && i<9)
			{
				byte = (byte >> 1) | (bit ? 0x80 : 0);
			}
			else
			{
				if (bit!=1)
				{
					if (verbose)
						printf("[Corrupted data at %i, trailing bit %i should be 1, found %i]", offset, i-9, bit);
					return -1;
				}
			}
		}

		return byte;
	}

};

// CTextFileReader - reads data from a previously generated text file
class CTextFileReader : public CFileReader
{
public:
	CTextFileReader()
	{
		_res = resBytes;
		InitBuffer();
	}

	void InitBuffer()
	{
		_bufferSize = 1024 * 1024;
		_buffer = (unsigned char*)malloc(_bufferSize);		// Grab 1mb for now
		_dataLength = 0;
		_currentPosition = 0;
	}

	~CTextFileReader()
	{
	}

	int HexToInt(char ch)
	{
		if (ch>='0' && ch<='9')
			return ch-'0';
		if (ch>='A' && ch<='F')
			return ch - 'A' + 0x0A;
		if (ch>='a' && ch<='f')
			return ch - 'a' + 0xA;
		return -1;
	}

	enum parseState
	{
		psReady,
		psComment,
		psLeadingZero,
		psHexHi,
		psHexLo,
	};

	bool Open(const char* filename, Resolution res)
	{
		// Store requested resolution
		_res = res;

		// Open the file
        FILE* file=fopen(filename,"rt");
		if (file==NULL)
		{
	        fprintf(stderr, "Could not open %s\n", filename);
			return false;
		}

		// Parse the file
		parseState state = psReady;
		int ch;
		int pos=-1;
		bool ReadNextChar = true;
		int data;
		while ( !ReadNextChar || ((ch=fgetc(file)) >= 0) )
		{
			ReadNextChar = true;
			pos++;
			switch (state)
			{
				case psReady:
					switch (ch)
					{
						case '[':
							state = psComment;
							break;

						case '0':
							state = psLeadingZero;
							break;

						case '1':
							QueueBit(1);
							break;

						case 'S':
						case 'L':
						case '?':
						case '<':
						case '>':
							QueueCycleKind(ch);
							break;

						case ' ':
						case '\t':
						case '\n':
						case '\r':
							break;

						default:
							fprintf(stderr, "Error parsing input text file, unexpected character '%c' at %i\n", (char)ch, pos);
							fclose(file);
							return false;
					}
					break;

				case psComment:
					if (ch==']')
						state = psReady;
					break;

				case psLeadingZero:
					if (ch=='x')
					{
						state = psHexHi;
					}
					else
					{
						QueueBit(0);
						state = psReady;
						ReadNextChar = false;
					}
						
					break;

				case psHexHi:
				case psHexLo:
					int nib = HexToInt(ch);
					if (nib<0)
					{
						fprintf(stderr, "Error parsing input text file, syntax error in hex byte unexpected '%c' at %i\n", (char)ch, pos);
						fclose(file);
						return false;
					}

					if (state==psHexHi)
					{
						data = nib << 4;
						state = psHexLo;
					}
					else
					{
						data |= nib;
						QueueByte(data);
						state = psReady;
					}
					break;
			}
		}

		fclose(file);

		if (state != psReady)
		{
			fprintf(stderr, "Error parsing input text file, unexpected EOF\n");
			return false;
		}
		
		// All good
		return true;
	}

	Resolution _res;
	int _currentPosition;
	unsigned char* _buffer;
	int _bufferSize;
	int _dataLength;

	void QueueData(unsigned char b)
	{
		// Grow the buffer?
		if (_dataLength + 1 > _bufferSize)
		{
			_bufferSize *= 2;
			_buffer = (unsigned char*)realloc(_buffer, _bufferSize);
		}

		_buffer[_dataLength++]=b;
	}

	void EnsureResolution(Resolution resRequired)
	{
		// Do we already have the required resolution?
		if (_res <= resRequired)
			return;

		// No data is the easy case
		if (_dataLength==0)
		{
			_res = resRequired;
			return;
		}

		// Steal the current buffer
		int oldDataLength = _dataLength;
		unsigned char* oldBuffer = _buffer;

		// Allocate a new one
		InitBuffer();

		// Switch resolution
		Resolution oldResolution = _res;
		_res = resRequired;

		switch (oldResolution)
		{
			case resBytes:
				for (int i=0; i<oldDataLength; i++)
				{
					QueueByte(oldBuffer[i]);
				}
				break;

			case resBits:
				for (int i=0; i<oldDataLength; i++)
				{
					QueueBit(oldBuffer[i]);
				}
				break;

			default:
				assert(false);
		}

		// And clean up
		free(oldBuffer);
	}

	void QueueCycleKind(char kind)
	{
		EnsureResolution(resCycleKinds);
		QueueData(kind);
	}

	void QueueBit(unsigned char bit)
	{
		if (_res < resBits)
		{
			for (int i=0; i<(bit ? 8 : 4); i++)
			{
				QueueCycleKind(bit ? 'S' : 'L');
			}
		}
		else
		{
			EnsureResolution(resBits);
			QueueData(bit);
		}
	}

	void QueueByte(unsigned char byte)
	{
		if (_res < resBytes)
		{
			// Leading bit
			QueueBit(0);

			// Data bits
			for (int i=0; i<8; i++)
			{
				QueueBit(byte & 0x01);
				byte>>=1;
			}

			// Trailing bits
			QueueBit(1);
			QueueBit(1);
		}
		else
		{
			EnsureResolution(resBytes);
			QueueData(byte);
		}
	}

	virtual void Delete()
	{
		delete this;
	}

	virtual bool IsWaveFile()
	{
		return false;
	}

	virtual int CurrentPosition()
	{
		return _currentPosition;
	}

	virtual int ReadCycleLen()
	{
		// Should never happen
		assert(false);
		return -1;
	}

	virtual char ReadCycleKind()
	{
		assert(_res >= resCycleKinds);

		if (_currentPosition>=_dataLength)
			return 0;

		return ((char*)_buffer)[_currentPosition++];
	}

	virtual void Analyze()
	{
		// Ignore
	}

	virtual void Seek(int position)
	{
		_currentPosition = position;
	}

	virtual char* FormatDuration(int duration)
	{
		static char sz[512];

		char* resName = NULL;
		switch (_res)
		{
			case resCycleKinds:
				resName = "cycles";
				break;

			case resBits:
				resName= "bits";
				break;

			case resBytes:
				resName = "bytes";
				break;

			default:
				assert(false);
		}

		sprintf(sz, "%i %s", duration, resName);
		return sz;
	}

	virtual int LastCycleLen()
	{
		return 30;
	}

	virtual bool SyncToBit(bool verbose)
	{
		if (_res < resBits)
			return __super::SyncToBit(verbose);
		else
			return _currentPosition < _dataLength;
	}

	virtual int ReadBit(bool verbose = true)
	{
		if (_res < resBits)
			return __super::ReadBit(verbose);

		if (_res == resBits)
		{
			if (_currentPosition < _dataLength)
				return ((unsigned char*)_buffer)[_currentPosition++];
			else
				return -1;
		}

		assert(false);
		return -1;
	}

	virtual bool SyncToByte(bool verbose)
	{
		if (_res < resBytes)
			return __super::SyncToByte(verbose);
		else
			return _currentPosition < _dataLength;
	}

	virtual int ReadByte(bool verbose=true)
	{
		if (_res < resBytes)
			return __super::ReadByte(verbose);

		if (_res == resBytes)
		{
			if (_currentPosition < _dataLength)
				return ((unsigned char*)_buffer)[_currentPosition++];
			else
				return -1;
		}

		assert(false);
		return -1;
	}
};

// CWaveFileReader - reads audio data from a tape recording
class CWaveFileReader : public CFileReader
{
public:
	CWaveFileReader()
	{
		_file = NULL;
		_smoothingPeriod = 0;
		_smoothingBuffer = NULL;
		_dc_offset = 0;
		Close();
	}

	int _dc_offset;
	int _cycle_frequency;

	~CWaveFileReader()
	{
		Close();
	}

	void SetDcOffset(int dc_offset)
	{
		_dc_offset = dc_offset;
	}

	void SetCycleFrequency(int freq)
	{
		_cycle_frequency = freq;
	}

	virtual void Delete()
	{
		delete this;
	}

	virtual bool IsWaveFile()
	{
		return true;
	}

	virtual int CurrentPosition()
	{
		return _currentSampleNumber;
	}

	bool Open(const char* filename, int smooth)
	{
		// Setup smoothing buffer
		_smoothingPeriod = smooth;
		if (_smoothingPeriod!=0)
		{
			_smoothingBuffer = new int[_smoothingPeriod];

			memset(_smoothingBuffer, 0, sizeof(int) * _smoothingPeriod);
			_smoothingBufferPos=0;
			_smoothingBufferTotal=0;
		}

		// Open the file
        _file=fopen(filename,"rb");
		if (_file==NULL)
		{
	        fprintf(stderr, "Could not open %s\n", filename);
			return false;
		}

		// Check RIFF header
		unsigned long l;
		fread(&l, 1, sizeof(l), _file);
		if (memcmp(&l, "RIFF", 4)!=0)
		{
			fclose(_file);
			_file=NULL;
			fprintf(stderr,"%s is not a wave file.\n",filename);
			return false;
		}

		// Work out the file length
		fseek(_file, 0, SEEK_END);
		int fileLength = ftell(_file);
		fseek(_file, 4, SEEK_SET);

		// Compare file length to data
		fread(&l, 1, 4, _file);
		if (fileLength > (int)(l+8))
		{
			fclose(_file);
			_file=NULL;
			fprintf(stderr,"%s is incomplete - bytes are missing.\n",filename);
			return false;
		}
		else if (fileLength<(int)(l+8))
		{
			fclose(_file);
			_file=NULL;
			fprintf(stderr,"%s has junk bytes at the end of the file.\n",filename);
			return false;
		}

		// Read the WAVE header
		fread(&l, 1, 4, _file);
		if (memcmp(&l,"WAVE",4)!=0)
		{
			fclose(_file);
			_file=NULL;
			fprintf(stderr,"%s is not a wave file.\n",filename);
			return false;
		}

		// Scan chunks
		int chunkLength = 0;
		for (int p=0xC; p<fileLength; p+=chunkLength+8)
		{
			// Read header
			fseek(_file, p, SEEK_SET);
			fread(&l, 1, sizeof(l), _file);
			fread(&chunkLength, 1, sizeof(chunkLength), _file);

			// "FMT"?
			if (memcmp(&l,"fmt ",4)==0)
			{
			    unsigned short us[8];
				fread(us, 1, sizeof(us), _file);

				// Check for 8 or 16 bit PCM mono
				if (us[0]!=1 || us[1]!=1 || (us[7]!=8 && us[7]!=16))
				{
					fclose(_file);
					_file=0;
					fprintf(stderr,"Only 8 or 16-bit mono PCM format "
						"wave files are supported.\n"
						"%s is %u-bit %s %s format.\n",
						filename,
						(us[0]==1)?us[7]:1,
						(us[1]==1)?(const char *)"mono":
							(const char *)"stereo",
						(us[0]==1)?(const char *)"PCM":
							(const char *)"compressed");
					return false;
				}

				// Save required info
				_bytesPerSample = us[7]/8;
				_sampleRate=us[2]+(((unsigned long)(us[3]))<<16);

				// Check we got a sample rate
				if (_sampleRate==0)
				{
					fclose(_file);
					_file=0;
					fprintf(stderr, "File has sample rate 0. This is invalid!\n");
					return false;
				}
			}

			// Is it the data chunk
			else if (memcmp(&l, "data", 4)==0)
			{
				if(_sampleRate==0)
				{
					fprintf(stderr,"No \"fmt\" chunk found.\n");
					fclose(_file);
					_file=0;
					return false;
				}

				_waveOffsetInBytes = p+8;
				_waveEndInSamples = chunkLength / _bytesPerSample;
				_dataEndInSamples = _waveEndInSamples;

				Seek(0);
				return true;
			}
		}
    
		// Unexpected EOF
		fclose(_file);
		_file=NULL;
		fprintf(stderr,"No \"data\" chunk found.\n");
		return false;
	}

	void Close()
	{
		if (_file!=NULL)
			fclose(_file);

		_waveOffsetInBytes = 0;
		_waveEndInSamples = 0;
		_dataStartInSamples = 0;
		_dataEndInSamples = 0;
		_currentSampleNumber = 0;
		_sampleRate = 0;
		_bytesPerSample = 1;
		_avgCycleLength = 0;
		_currentSample = 0;
		_startOfCurrentHalfCycle = 0;
		if (_smoothingBuffer!=NULL)
			delete[] _smoothingBuffer;
		_smoothingBuffer = NULL;
		_smoothingPeriod = 0;
	}

	virtual char* FormatDuration(int duration)
	{
		static char sz[512];
		sprintf(sz, "%i samples ~= %.2f bits", duration, duration / (4 * _longCycleLength));
		return sz;
	}

	virtual void Seek(int sampleNumber)
	{
		// Go back by size of smoothing buffer
		int startAtSampleNumber = sampleNumber -_smoothingPeriod;
		if (startAtSampleNumber<0)
			startAtSampleNumber = 0;

		// Seek to sample
		fseek(_file, _waveOffsetInBytes + startAtSampleNumber * _bytesPerSample, SEEK_SET);

		// Setup position info
		_currentSampleNumber = startAtSampleNumber;

		// Read the sample
		_currentSample=ReadRawSample();

		// Reset and refill the smoothing buffer
		memset(_smoothingBuffer, 0, _smoothingPeriod * sizeof(int));
		_smoothingBufferPos=0;
		_smoothingBufferTotal=0;
		while (_currentSampleNumber < sampleNumber)
		{
			if (!NextSample())
				break;
		}

		_startOfCurrentHalfCycle = _currentSampleNumber;
	}

	bool NextSample()
	{
		_currentSample = ReadSmoothedSample();
		_currentSampleNumber++;
		return _currentSample!=EOF_SAMPLE;
	}

	bool HaveSample()
	{
		return _currentSample!=EOF_SAMPLE;
	}

	int CurrentSample()
	{
		return _currentSample;
	}


	int ReadRawSample()
	{
		if (_currentSampleNumber > _dataEndInSamples)
			return EOF_SAMPLE;

		if (_bytesPerSample==1)
		{
			int i = fgetc(_file);
			if (i<0)
				return EOF_SAMPLE;
			else
			{
				return _dc_offset + i-128;
			}
		}
		else
		{
			short s;
			if (fread(&s, sizeof(s), 1, _file)==0)
				return EOF_SAMPLE;
			else
			{
				return _dc_offset + s;
			}
		}
	}

	int ReadSmoothedSample()
	{
		// Smoothing disabled?
		if (_smoothingPeriod==0)
			return ReadRawSample();

		// Read a raw sample
		int iSample = ReadRawSample();
		if (iSample==EOF_SAMPLE)
			return iSample;

		// Calculate new position in circular buffer
		_smoothingBufferPos = (_smoothingBufferPos + 1) % _smoothingPeriod;

		// Update the moving total
		_smoothingBufferTotal -= _smoothingBuffer[_smoothingBufferPos];
		_smoothingBufferTotal += iSample;

		// Update the buffer
		_smoothingBuffer[_smoothingBufferPos]=iSample;

		// Return smoothed sample
		return _smoothingBufferTotal / _smoothingPeriod;
	}

	int ReadHalfCycle()
	{
		int prev = _currentSample;

		while (NextSample())
		{
			if ((_currentSample<0)!=(prev<0))
			{
				int iCycleLen = _currentSampleNumber - _startOfCurrentHalfCycle;
				_startOfCurrentHalfCycle = _currentSampleNumber;
				return iCycleLen;
			}
		}

		return -1;
	}

	// Read one cycle from the file and return its length in samples
	virtual int ReadCycleLen()
	{
		int a = ReadHalfCycle();
		int b = ReadHalfCycle();
		if (a==-1 || b==-1)
			return -1;
		return a+b;
	}

	virtual char ReadCycleKind()
	{
		// Remember offset of the current cycle
		//int cycleOffset = CurrentSampleNumber();

		// Read the length of the next cycle
		int iLen = ReadCycleLen();
		if (iLen<0)
			return 0;

		_lastCycleLen = iLen;

		// Check for outside allowances
		if (iLen < _shortCycleLength - _cycleLengthAllowance)
		{
			return '<';
		}
		if (iLen > _longCycleLength + _cycleLengthAllowance)
		{
			return '>';
		}
		if (iLen > _shortCycleLength + _cycleLengthAllowance && iLen < _longCycleLength - _cycleLengthAllowance)
		{
			return '?';
		}

		// Short or long?
		return iLen < _avgCycleLength ? 'S' : 'L';
	}

	virtual int LastCycleLen()
	{
		return _lastCycleLen;
	}

	virtual void Analyze()
	{
		if (!analyzeCycles)
		{
			_shortCycleLength = _sampleRate / _cycle_frequency;
			_longCycleLength = 2 * _shortCycleLength;
			_avgCycleLength = (_shortCycleLength + _longCycleLength)/2;
		}
		else
		{
			fprintf(stderr, "Analyzing file...\n");

			// Rewind to the first sample
			Seek(0);

			// Calculate average samples per cycle
			__int64 iTotalCycleSamples=0;
			__int64 iTotalCycles=0;
			int cycleLen;
			while ((cycleLen=ReadCycleLen())>0)
			{
				iTotalCycleSamples += cycleLen;
				iTotalCycles++;
			}

			// Assuming there are roughly the same number of 0 and 1 bits we can now guess
			// the length of a short and long cycle.  Since there are twice as many cycles
			// required for 1 bits as zero bits we need to skew the average by 1.1249 to 
			// get the half way cycle length.
			//
			// Eg: say sampleRate is 48000:
			//       a long cycle (1200Hz) is 40 samples
			//     	 a short cycle (2400Hz) is 20 samples.
			// 
			// now lets say there's one 1 bit and one 0 bit:
			//       total samples = 40 + 20 + 20 = 80
			//       total cycles = 3
			//       average cycle = 80/3 = 26.667
			//
			// but the center frequency should be 30 so skew it
			//
			//       30/26.67 = 1.1249

			_avgCycleLength = int(1.1249 * iTotalCycleSamples / iTotalCycles);
			printf("[center cycle length: %i]\n", _avgCycleLength);

			_shortCycleLength = _avgCycleLength * 2 / 3;
			_longCycleLength = _avgCycleLength + (_avgCycleLength - _shortCycleLength);

			Seek(0);
		}

		_cycleLengthAllowance = _avgCycleLength / 3-1;

		printf("[Short cycle length: %i +/- %i]\n", _shortCycleLength, _cycleLengthAllowance);
		printf("[Long cycle length: %i +/- %i]\n", _longCycleLength, _cycleLengthAllowance);
	}


	FILE* _file;
	int _waveOffsetInBytes;
	int _waveEndInSamples;
	int _dataStartInSamples;
	int _dataEndInSamples;
	int _currentSampleNumber;
	int _sampleRate;
	int _bytesPerSample;
	int _avgCycleLength;
	int _shortCycleLength;
	int _longCycleLength;
	int _cycleLengthAllowance;
	int _currentSample;
	int _startOfCurrentHalfCycle;
	int _smoothingPeriod;
	int* _smoothingBuffer;
	int _smoothingBufferPos;
	int _smoothingBufferTotal;
	int _lastCycleLen;
};


// Target for rendering a new tape recording - generates a wave file
class CRenderFile
{
public:
	CRenderFile()
	{
		_file = NULL;
		_amplitude = 14000;
		_baud = 300;
		_square = false;
		_lastSquareSample = 0;
	}

	~CRenderFile()
	{
		Close();
	}

	FILE* _file;
	int _amplitude;
	WAVEHEADER _waveHeader;
	int _baud;
	bool _square;
	short _lastSquareSample;

	// Create the wave file
	bool Create(const char* fileName, int sampleRate, int sampleSize)
	{
		// Create the file
		_file = fopen(fileName, "wb");
		if (_file==NULL)
		{
	        fprintf(stderr, "Could not create %s\n", fileName);
			return false;
		}

		// Write the header
		InitWaveHeader(renderSampleRate, renderSampleSize);
		fwrite(&_waveHeader, sizeof(_waveHeader), 1, _file);

		return true;
	}

	void SetVolume(int volume)
	{
		_amplitude = (_waveHeader.bitsPerSample==8 ? 0x7f : 0x7fff) * volume / 100;
	}

	void SetBaud(int baud)
	{
		_baud = baud;
	}

	void SetSquare(bool square)
	{
		_square = square;
	}


	void Close()
	{
		if (_file==NULL)
			return;

		// Work out how much data was written
		fseek(_file, 0, SEEK_END);
		int dataBytes = ftell(_file) - sizeof(WAVEHEADER);

		// Update the header info
		_waveHeader.riffChunkSize += dataBytes;
		_waveHeader.dataChunkSize += dataBytes;

		// Seek back to start and rewrite the header
		fseek(_file, 0, SEEK_SET);
		fwrite(&_waveHeader, sizeof(_waveHeader), 1, _file);

		// Close the file and clean up
		fclose(_file);
		_file=NULL;
		memset(&_waveHeader, 0, sizeof(WAVEHEADER));

	}

	void InitWaveHeader(int sampleRate, int sampleSize)
	{
		// RIFF chunk
		memcpy(&_waveHeader.riffChunkID, "RIFF", 4);
		_waveHeader.riffChunkSize = sizeof(_waveHeader)-8;		// We'll add the rest later
		memcpy(&_waveHeader.waveFormat, "WAVE", 4);

		// FMT chunk
		memcpy(&_waveHeader.fmtChunkID, "fmt ", 4);
		_waveHeader.fmtChunkSize = 16;
		_waveHeader.audioFormat = 1;	// PCM
		_waveHeader.numChannels = 1;	// Mono
		_waveHeader.sampleRate = sampleRate;
		_waveHeader.bitsPerSample = sampleSize;
		_waveHeader.blockAlign = _waveHeader.numChannels * _waveHeader.bitsPerSample/8;
		_waveHeader.byteRate = _waveHeader.sampleRate * _waveHeader.blockAlign;

		// DATA chunk
		memcpy(&_waveHeader.dataChunkID, "data", 4);
		_waveHeader.dataChunkSize = 0;
	}

	int SampleRate()
	{
		return _waveHeader.sampleRate;
	}

	void RenderSample(short sample)
	{
		// Make it square
		if (_square)
		{
			if (sample==0)
			{
				sample = _lastSquareSample;
			}
			else
			{
				sample = sample < 0 ? -_amplitude : _amplitude;
				_lastSquareSample = sample;
			}
		}

		if (_waveHeader.bitsPerSample==8)
		{
			char ch = (char)(sample + 128);
			fwrite(&ch, sizeof(ch), 1, _file);	
		}
		else
		{
			fwrite(&sample, sizeof(sample), 1, _file);	
		}
	}

	void RenderSilence(int samples)
	{
		for (int i=0; i<samples; i++)
			RenderSample(0);
	}

	void RenderWave(int cycles, int samples)
	{
		for (int i=0; i<samples; i++)
		{
			double in = 2.0*PI*cycles*i/samples;
			double curve = sin(in);
			RenderSample(short(curve * _amplitude));
		}
	}

	void RenderCycleKind(char kind)
	{
		if (kind=='S')
		{
			RenderWave(1, _waveHeader.sampleRate / 300 / 8);
		}
		else if (kind=='L')
		{
			RenderWave(1, _waveHeader.sampleRate / 300 / 4);
		}
		else
		{
		}
	}

	void RenderBit(unsigned char bit)
	{
		if (_baud == 300)
		{
			if (bit)
			{
				RenderWave(8, _waveHeader.sampleRate / 300);
			}
			else
			{
				RenderWave(4, _waveHeader.sampleRate / 300);
			}
		}
		else
		{
			if (bit)
			{
				RenderWave(2, _waveHeader.sampleRate / 1200);
			}
			else
			{
				RenderWave(1, _waveHeader.sampleRate / 1200);
			}
		}
	}

	void RenderByte(unsigned char byte)
	{
		RenderBit(0);
		for (int i=0; i<8; i++)
		{
			RenderBit(byte & 0x01);
			byte>>=1;
		}
		RenderBit(1);
		RenderBit(1);
	}

};


// Opens the specified input file, could be a wav or txt file
bool OpenInputFile(Resolution res)
{
	if (file_count!=1)
	{
		fprintf(stderr, "command requires a single input file");
		return false;
	}

	char* ext = strrchr(files[0], '.');
	if (_stricmp(ext, ".wav")==0)
	{
		// Open the file
		CWaveFileReader* wave = new CWaveFileReader();
		if (!wave->Open(files[0], smoothing))
		{
			return false;
		}

		if (smoothing!=0)
		{
			printf("[using a smoothing period of %i]\n", smoothing);
		}

		// On trs80, set a DC offset to get out of the noise and into the pulse
		if (dc_offset == 0 && trs80)
			dc_offset = wave->_bytesPerSample == 2 ? -8192 : -32;

		wave->SetDcOffset(dc_offset);
		wave->SetCycleFrequency(cycle_freq);

		file = wave;
	}
	else
	{
		CTextFileReader* text = new CTextFileReader();
		if (!text->Open(files[0], res))
		{
			return false;
		}

		file = text;
	}

	return true;
}

// Create the rendering file
bool OpenRenderFile()
{
	if (renderFileName == NULL)
		return true;

	// Create the render file
	renderFile = new CRenderFile();
	if (!renderFile->Create(renderFileName, renderSampleRate, renderSampleSize))
	{
		renderFile->Close();
		return false;
	}

	renderFile->SetVolume(renderVolume);
	renderFile->SetSquare(!renderSine);

	// Render leading silence
	renderFile->RenderSilence(renderFile->SampleRate() * leadingSilence);

	// Render additional leading zeros
	for (int i=0; i<leadingZeros; i++)
	{
		renderFile->RenderByte(0);
	}

	return true;
}

// Create the binary file
bool OpenBinaryFile()
{
	if (binaryFileName == NULL)
		return true;

	binaryFile = fopen(binaryFileName, "wb");
	if (binaryFile==NULL)
	{									  
	    fprintf(stderr, "Could not create %s\n", binaryFileName);
		return false;
	}

	return true;
}

		
// Close any open files
void CloseFiles()
{
	if (file!=NULL)
	{
		file->Delete();
		file=NULL;
	}

	if (renderFile!=NULL)
	{
		renderFile->Close();
		delete renderFile;
		renderFile = NULL;
	}

	if (binaryFile!=NULL)
	{
		fclose(binaryFile);
		binaryFile=NULL;
	}
}


// Command handler for dumping samples
int DumpSamples()
{
	// Open the file
	if (!OpenInputFile(resSamples))
		return 7;

	if (!file->IsWaveFile())
	{
		fprintf(stderr, "Can't dump samples from a text file");
		return 7;
	}

	CWaveFileReader* wf = static_cast<CWaveFileReader*>(file);

	int perline = perLineMode ? 1 : 32;

	wf->Seek(from);

	int prev = 0;
	bool first = true;

	// Dump all samples
	int index = 0;
	while (wf->HaveSample())
	{
		if (showZeroCrossings && !first && prev<0 != wf->CurrentSample()<0)
		{
			index=0;
		}
		else
		{
			first = false;
		}

		if ((index++ % perline)==0)
		{
			if (showPositionInfo)
				printf("\n[@%12i] ", wf->_currentSampleNumber);
			else
				printf("\n");
		}

		printf("%i ", wf->CurrentSample());

		prev = wf->CurrentSample();

		wf->NextSample();

		if (samples>0 && wf->_currentSampleNumber >= from + samples)
			break;
	}

	printf("\n\n");

	return 0;
}



// Command handler for dumping cycle lengths
int DumpCycles()
{
	// Open the file
	if (!OpenInputFile(resCycles))
		return 7;

	int perline = perLineMode ? 1 : 16;

	// Dump all cycles
	int index = 0;
	while (true)
	{
		if ((index++ % perline)==0)
		{
			if (showPositionInfo)
				printf("\n[@%12i] ", file->CurrentPosition());
			else
				printf("\n");
		}

		int cyclelen= file->ReadCycleLen();
		if (cyclelen<0)
			break;


		printf("#%i ", cyclelen);
	}

	printf("\n\n");

	return 0;
}



// Command handler for dumping cycle kinds
int DumpCycleKinds()
{
	// Open files
	if (!OpenInputFile(resCycleKinds) || !OpenRenderFile())
		return 7;

	// Analyse cycle lengths
	file->Analyze();

	int perline = perLineMode ? 1 : 64;

	// Dump all cycles
	int index = 0;
	while (true)
	{
		int pos = file->CurrentPosition();

		char kind = file->ReadCycleKind();
		if (kind==0)
			break;


		if ((index++ % perline)==0)
		{
			if (showPositionInfo)
				printf("\n[@%12i] ", pos);
			else
				printf("\n");
		}

		printf("%c", kind);
		if (renderFile)
			renderFile->RenderCycleKind(kind);
	}

	printf("\n\n");

	return 0;
}


// Command handler for dumping bits
int DumpBits()
{
	// Open files
	if (!OpenInputFile(resBits) || !OpenRenderFile())
		return 7;

	// Prepare
	file->Analyze();

	printf("\n");
	file->SyncToBit(showSyncData);
	printf("\n\n");

	int perline = perLineMode ? 1 : 64;

	// Dump all bits
	int index = 0;
	while (true)
	{
		int savePos = file->CurrentPosition();

		// Read a bit
		int bit = file->ReadBit();

		// Force line break on error
		if (bit<0)
		{
			printf("\n\n");

			printf("[last bit ended at %i]\n", savePos);

			file->Seek(savePos);
			if (!file->SyncToBit(showSyncData))
				break;

			int skipped = file->CurrentPosition() - savePos;
			printf("\n[skipped %s while re-syncing]\n", file->FormatDuration(skipped));

			printf("\n");

			index = 0;
			continue;
		}

		// New line?
		if ((index++ % perline)==0)
		{
			if (showPositionInfo)
				printf("\n[@%12i] ", savePos);
			else
				printf("\n");
		}

		printf("%i", bit);
		if (renderFile)
			renderFile->RenderBit(bit);
	}

	printf("\n\n");

	return 0;
}



// Command handler for dumping bytes
int DumpBytes()
{
	// Open files
	if (!OpenInputFile(resBytes) || !OpenRenderFile() || !OpenBinaryFile())
		return 7;

	// Prepare
	file->Analyze();

	printf("\n");
	file->SyncToByte(showSyncData);
	printf("\n\n");

	int perline = perLineMode ? 1 : 16;

	// Dump all bits
	int index = 0;
	while (true)
	{
		int pos = file->CurrentPosition();

		// Read a byte
		int byte = file->ReadByte();

		// Error?
		if (byte<0)
		{
			printf("\n\n");

			printf("[last byte ended at %i]\n", pos);

			file->Seek(pos);
			if (!file->SyncToByte(showSyncData))
				break;

			int skipped = file->CurrentPosition() - pos;
			printf("\n[skipped %s while re-syncing]\n", file->FormatDuration(skipped));

			printf("\n");

			index = 0;
			continue;
		}

		// New line?
		if ((index++ % perline)==0)
		{
			if (showPositionInfo)
				printf("\n[@%12i] ", pos);
			else
				printf("\n");
		}

		printf("0x%.2x ", byte);
		if (renderFile)
			renderFile->RenderByte(byte);
		if (binaryFile)
			fwrite(&byte, 1, 1, binaryFile);
	}

	printf("\n\n");

	return 0;
}


int byteIndex = 0;
void ResetByteDump()
{
	byteIndex = 0;
}

void DumpByte(int byte)
{
	if (byteIndex!=0 && ((byteIndex % 16)==0 || perLineMode))
	{
		printf("\n");
	}
	printf("0x%.2x ", byte);
	byteIndex++;
}


// Command handler for dumping formatted header block and CRC checked data blocks
int DumpBlocks()
{
	// Open files
	if (!OpenInputFile(resBytes) || !OpenRenderFile() || !OpenBinaryFile())
		return 7;

	// Prepare
	file->Analyze();

	printf("\n");
	file->SyncToByte(showSyncData);
	printf("\n\n");

	if (binaryFile!=NULL && binaryTapFile)
	{
		fwrite("TAP_DGOS_MBEE",0x0d, 1, binaryFile);
		unsigned char b = 0;
		for (int i=0; i<0x40; i++)
		{
			fwrite(&b, 1, 1, binaryFile);
		}
		b = 1;
		fwrite(&b, 1, 1, binaryFile);
	}

	printf("[lead in]\n");
	ResetByteDump();
	while (true)
	{
		// Read a byte
		int byte = file->ReadByte();
		if (byte<0)
		{
			printf("\nFailed to read byte\n\n");
			return 7;
		}

		// Dump it
		DumpByte(byte);
		if (renderFile)
			renderFile->RenderByte(byte);
		if (byte==1)
			break;

		if (byte!=0)
		{
			printf("\nFailed to locate leadin, expected 0x00 or 0x01\n\n");
			return 7;
		}
	}

	printf("\n\n[header]\n");
	ResetByteDump();
	unsigned char checksum=16;
	TAPE_HEADER header;
	unsigned char* header_bytes = (unsigned char*)&header;

	for (int i=0; i<17; i++)
	{
		// Read a byte
		int byte = file->ReadByte();
		if (byte<0)
		{
			printf("\nFailed to read byte\n\n");
			return 7;
		}

		if (i<sizeof(TAPE_HEADER))
		{
			header_bytes[i]=byte;
			// Dump it
			DumpByte(byte);
		}
		else
			printf("\n[checksum byte:] 0x%2x\n\n", byte);


		if (binaryFile!=NULL && binaryTapFile)
			fwrite(&byte, 1, 1, binaryFile);

		checksum += (unsigned char)byte;
	}
	if (checksum!=0)
	{
		printf("\nCheck sum error: %i\n\n", checksum);
		return 7;
	}

	if (sizeof(header)!=16)
	{
		fprintf(stderr, "Internal Error\n");
		return 7;
	}

	if (binaryFile!=NULL && binaryDgosHeader && !binaryTapFile)
		fwrite(header_bytes, 16, 1, binaryFile);

	if (renderFile!=NULL)
	{
		// Flip the speed byte if necessary
		header.speed = renderBaud == 300 ? 0 : 0xFF;

		// Calculate a new checksum
		checksum=sizeof(header);
		for (int i=0; i<sizeof(header); i++)
		{
			renderFile->RenderByte(header_bytes[i]);
			checksum += header_bytes[i];
		}

		renderFile->RenderByte(0x100-checksum);
	}

	/*
	if (renderFile)
	{
		if (i==13)		// Speed baud
		{
			checksum += (unsigned char)byte;
			if (renderBaud==1200)
				byte = 0xFF;
			else
				byte = 0;
			checksum -= (unsigned char)byte;
		}

		renderFile->RenderByte(byte);
	}
	*/


		
	printf("\n[\n");
	printf("    filename:     '%c%c%c%c%c%c'\n", header.filename[0], header.filename[1], header.filename[2], header.filename[3], header.filename[4], header.filename[5]);
	printf("    filetype:     %c\n", header.filetype);
	printf("    data length:  0x%.4x (%i) bytes\n", header.datalen, header.datalen);
	printf("    load addr:    0x%.4x\n", header.loadaddr);
	printf("    start addr:   0x%.4x\n", header.startaddr);
	printf("    speed:        %s\n", header.speed == 0 ? "300 baud" : "1200 baud" );
	printf("    auto start:   %s\n", header.autostart == 0xFF ? "yes" : "no" );

	printf("]\n\n");

	if (renderFile!=NULL && renderBaud==1200)
	{
		renderFile->SetBaud(1200);
	}

	int blockAddr = 0;

	while (blockAddr < header.datalen)
	{
		int bytesRemaining = header.datalen - blockAddr;
		int iBytesThisBlock = bytesRemaining > 256 ? 256 : bytesRemaining;

		printf("\n[data block 0x%.4x, %i bytes]\n", blockAddr, iBytesThisBlock);
		ResetByteDump();
		unsigned char checksum=iBytesThisBlock;

		for (int i=0; i<iBytesThisBlock+1; i++)
		{
			// Read a byte
			int byte = file->ReadByte();

			if (byte<0 && i==bytesRemaining)
			{
				byte = 256-checksum;
				printf("\n[guessing trailing checksum value]", byte);
			}
			else if (byte<0)
			{

				printf("\nFailed to read byte, %i bytes missing\n\n", bytesRemaining - i + 1);		// plus 1 for the trailing checksum
				return 7;
			}

			if (i==iBytesThisBlock)
			{
				printf("\n[checksum byte:] 0x%.2x", byte);

				if (binaryFile && binaryTapFile)
					fwrite(&byte, 1, 1, binaryFile);
			}
			else
			{
				// Dump it
				DumpByte(byte);

				if (binaryFile)
					fwrite(&byte, 1, 1, binaryFile);
			}

			if (renderFile)
				renderFile->RenderByte(byte);

			checksum += (unsigned char)byte;
		}
		if (checksum!=0)
		{
			printf("\nCheck sum error: %i\n\n", checksum);
			return 7;
		}

		blockAddr += iBytesThisBlock;

		printf("\n");
	}

	printf("\n\n[eof]\n");

	return 0;
}


// Main
int main(int argc,char **argv)
{
	// Process command line arguments
	fnCmd cmd = NULL;
	for (int i=1; i<argc; i++)
	{
		char* arg = argv[i];

		if (arg[0]=='-' && arg[1]=='-')
			arg++;

		if (arg[0]=='-')
		{
			arg++;

			// Split --<arg>:<val>
			char* v1 = strchr(arg, ':');
			char* v2 = strchr(arg, '=');
			if (v1!=NULL && v2!=NULL)
			{
				if (v2<v1)
					v1=v2;
			}
			else if (v2!=NULL && v1==NULL)
			{
				v1 = v2;
			}
			char sw[256];
			char* val = NULL;
			if (v1!=NULL)
			{
				strncpy(sw, arg, v1-arg);
				sw[v1-arg]='\0';
				arg = sw;
				val = v1+1;
			}

			if (_strcmpi(arg, "analyze")==0)
			{
				analyzeCycles = true;
			}
			else if (_strcmpi(arg, "samples")==0)
			{
				cmd = DumpSamples;

				if (val!=NULL)
				{
					from = atoi(val);
					if (samples==0)
						samples=200;
				}
			}
			else if (_strcmpi(arg, "cycles")==0)
			{
				cmd = DumpCycles;
			}
			else if (_strcmpi(arg, "cyclekinds")==0)
			{
				cmd = DumpCycleKinds;
			}
			else if (_strcmpi(arg, "bits")==0)
			{
				cmd = DumpBits;
			}
			else if (_strcmpi(arg, "bytes")==0)
			{
				cmd = DumpBytes;
			}
			else if (_strcmpi(arg, "blocks")==0)
			{
				cmd = DumpBlocks;
			}
			else if (_strcmpi(arg, "smooth")==0)
			{
				if (val==NULL)
					smoothing = 3;
				else
					smoothing = atoi(val);
			}
			else if (_strcmpi(arg, "samplecount")==0)
			{
				samples = val==NULL ? 0 : atoi(val);
			}
			else if (_strcmpi(arg, "syncinfo")==0)
			{
				showSyncData = true;
			}
			else if (_strcmpi(arg, "perline")==0)
			{
				perLineMode = true;
			}
			else if (_strcmpi(arg, "noposinfo")==0)
			{
				showPositionInfo = false;
			}
			else if (_strcmpi(arg, "zc")==0)
			{
				showZeroCrossings = true;
			}
			else if (_strcmpi(arg, "allowbadcycles")==0)
			{
				allowBadCycles = true;
			}
			else if (_strcmpi(arg, "render")==0)
			{
				renderFileName = val;
			}
			else if (_strcmpi(arg, "binary")==0)
			{
				binaryFileName = val;
			}
			else if (_strcmpi(arg, "dgosheader")==0)
			{
				binaryDgosHeader = true;
			}
			else if (_strcmpi(arg, "tapfile")==0)
			{
				binaryTapFile = true;
				cmd = DumpBlocks;
			}
			else if (_strcmpi(arg, "leadingsilence")==0)
			{
				leadingSilence = val==NULL ? 0 : atoi(val);
			}
			else if (_strcmpi(arg, "leadingzeros")==0)
			{
				leadingZeros = val==NULL ? 0 : atoi(val);
			}
			else if (_strcmpi(arg, "samplerate")==0)
			{
				renderSampleRate = val==NULL ? 48000 : atoi(val);
			}
			else if (_strcmpi(arg, "samplesize")==0)
			{
				renderSampleSize = val==NULL ? 16: atoi(val);
				if (renderSampleSize!=8 && renderSampleSize!=16)
				{
					fprintf(stderr, "Invalid render sample size, must be 8 or 16");
					return 7;
				}
			}
			else if (_strcmpi(arg, "baud")==0)
			{
				renderBaud = val==NULL ? 300 : atoi(val);
				if (renderBaud!=300 && renderBaud!=1200)
				{
					fprintf(stderr, "Invalid render baud rate - must be 300 or 1200");
					return 7;
				}
			}
			else if (_strcmpi(arg, "volume")==0)
			{
				renderVolume = val==NULL ? 75 : atoi(val);
				if (renderVolume<1 || renderVolume*100)
				{
					fprintf(stderr, "Invalid render volume, must be between 1 and 100");
				}
			}
			else if (_strcmpi(arg, "dcoffset")==0)
			{
				dc_offset = val==NULL ? 0 : atoi(val);
			}
			else if (_strcmpi(arg, "cyclefreq")==0)
			{
				cycle_freq = val==NULL ? 2400 : atoi(val);
			}
			else if (_strcmpi(arg, "trs80")==0)
			{
				trs80 = true;
				cycle_freq = 1024;
			}
			else if (_strcmpi(arg, "sine")==0)
			{
				renderSine = true;
			}
			else
			{
				fprintf(stderr, "Unknown command line arg: '%s', aborting\n", argv[i]);
				return 7;
			}
		}
		else
		{
			files[file_count++] = argv[i];
		}
	}

	// Default command is blocks (if a file is specified)
	if (cmd==NULL && file_count>0)
	{
		cmd = DumpBlocks;
	}

	// Tap file requires block processing
	if (binaryTapFile && cmd!=DumpBlocks)
	{
		fprintf(stderr, "The --tapfile options requires --blocks");
		return 7;
	}

	// Tap file requires block processing
	if (binaryDgosHeader && cmd!=DumpBlocks)
	{
		fprintf(stderr, "The --dgosheader options requires --blocks");
		return 7;
	}

	// Run the command
	if (cmd!=NULL)
	{
		int retv = cmd();
		CloseFiles();
		return retv;
	}

	// Show some help
	printf("tapetool - Microbee Tape Diagnotic Utility\n");
	printf("Copyright (C) 2011 Topten Software.\n");

	printf("\nUsage: tapetool [options] filename.[wav|txt]\n\n");

	printf("The input file can be either:\n");
	printf("  an 8 or 16 bit PCM mono wave file, or \n");
	printf("  a text file containing the (possibly edited) previously output of this program\n");

	printf("\nWhat to Output:\n");
	printf("  --samples[:from]      dump wave samples (optionally starting at sample number <from>)\n");
	printf("  --cycles              dump wave cycle lengths\n");
	printf("  --cyclekinds          dump wave cycle kinds (long/short)\n");
	printf("  --bits                dump raw bit data\n");
	printf("  --bytes               dump raw byte data\n");
	printf("  --blocks              dump data blocks (default)\n");

	printf("\nWave Input Options:\n");
	printf("  --smooth[:N]          smooth waveform with a moving average of N samples (N=3 if not specified)\n");
	printf("  --analyze             determine cycle length by analysis (don't trust sample rate)\n");
	printf("  --allowbadcycles      dont limit check cycle lengths (within reason)\n");
	printf("  --dcoffset:[N]        offset sample values by this amount\n");
	printf("  --cyclefreq:[N]       explicitly set the short cycle frequency (default=2400Hz)\n");
	printf("  --trs80               TRS-80 mode (default is MicroBee)\n");

	printf("\nText Output Formatting:\n");
	printf("  --syncinfo            show details of bit and byte sync operations\n");
	printf("  --perline             display one piece of data per line\n");
	printf("  --noposinfo           don't dump position info\n");
	printf("  --zc                  start a new line at zero crossings with --samples\n");
	printf("  --samplecount         number of samples to dump with --samples\n");

	printf("\nBinary Output Options\n");
	printf("  --binary:<file>       render to a binary file (use with --bytes or --blocks)\n");
	printf("  --dgosheader          include the dgos header in binary output\n");
	printf("  --tapfile             generate a tapfile\n");

	printf("\nWave Output Options\n");
	printf("  --render:<file>       render to specified wave file\n");
	printf("  --leadingsilence:N    render N seconds of leading silence (default = 2)\n");
	printf("  --leadingzeros:N      render an additional N leading zeros\n");
	printf("  --samplerate:N        render using sample rate of N (default = 24000)\n");
	printf("  --samplesize:N        render using 8 or 16 bit samples (default = 8)\n");
	printf("  --volume:N            render volume percent (default = 10%)\n");
	printf("  --baud:N              render baud rate 300 or 1200 (default = 300)\n");
	printf("  --square              render using square (instead of sine) waves\n");
	printf("\n");

	return 7;
}



/*
SYSTEM TAPE FORMAT
TAPE LEADER             256 zeroes followed by a A5 (sync byte)
55                      header byte indicating system format
xx xx xx xx xx xx       6 character file name in ASCII

        3C              data header
        xx              length of data 01-FFH, 00=256 bytes
        lsb,msb         load address
        xx ... xx       data (your program)
        xx              checksum of your data & load address

        .               repeat from 3C through checksum
        .
        .
78                      end of file marker
lsb,msb                 entry point address
						

EDITOR/ASSEMBLER SOURCE TAPE FORMAT
TAPE LEADER             256 zeroes followed by a A5 (sync byte)
D3                      header byte indicating source format
xx xx xx xx xx xx       6 character file name in ASCII

        xx xx xx xx xx  line # in ASCII with bit 7 set in each byte
        20              data header
        xx ... xx       source line (128 byte maximum)
        0D              end of line marker

        .
        .
        .

1A                      end of file marker
						

BASIC TAPE FORMAT
LEADER                  256 zeroes followed by an A5 (sync byte)
D3 D3 D3                BASIC header bytes
xx                      1 character file name in ASCII

        lsb,msb         address pointer to next line
        lsb,msb         line #
        xx ... xx       BASIC line (compressed)
        00              end of line marker

        .
        .
        .

00 00                   end of file markers
						
*/