//////////////////////////////////////////////////////////////////////////
// TextReader.cpp - implementation of CTextReader class

#include "precomp.h"

#include "TextReader.h"

CTextReader::CTextReader(CCommandStd* cmd) : CFileReader(cmd)
{
	_res = resBytes;
	_dataFormat[0]='\0';
	InitBuffer();
}

CTextReader::~CTextReader()
{
}

void CTextReader::InitBuffer()
{
	_bufferSize = 1024 * 1024;
	_buffer = (unsigned char*)malloc(_bufferSize);		// Grab 1mb for now
	_dataLength = 0;
	_currentPosition = 0;
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
	psSlash,
	psLineComment,
};

bool CTextReader::Open(const char* filename, Resolution res)
{
	_res = res;

	// Open the file
    FILE* file=fopen(filename,"rt");
	if (file==NULL)
	{
	    fprintf(stderr, "Could not open '%s' - %s (%i)\n", filename, strerror(errno), errno);
		return false;
	}

	// Parse the file
	parseState state = psReady;
	int ch;
	int pos=-1;
	bool ReadNextChar = true;
	int data;
	char buf[512];
	int bufPos=0;
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
						bufPos=0;
						break;

					case '/':
						state = psSlash;
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
				{
					state = psReady;

					// Data format comment?
					if (strncmp(buf, "format:", 7)==0)
					{
						strcpy(_dataFormat, buf+7);
					}
				}
				else
				{
					if (bufPos+1 < sizeof(buf))
					{
						buf[bufPos++]=ch;
						buf[bufPos]='\0';
					}
				}

				break;

			case psSlash:
				if (ch=='/')
					state = psComment;
				else
				{
					fprintf(stderr, "Error parsing input text file, unexpected character '%c' at %i\n", (char)ch, pos);
					fclose(file);
					return false;
				}
				break;

			case psLineComment:
				if (ch=='\r' || ch=='\n')
				{
					state = psReady;
				}
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

void CTextReader::QueueData(unsigned char b)
{
	// Grow the buffer?
	if (_dataLength + 1 > _bufferSize)
	{
		_bufferSize *= 2;
		_buffer = (unsigned char*)realloc(_buffer, _bufferSize);
	}

	_buffer[_dataLength++]=b;
}

void CTextReader::EnsureResolution(Resolution resRequired)
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

void CTextReader::QueueCycleKind(char kind)
{
	EnsureResolution(resCycleKinds);
	QueueData(kind);
}

void CTextReader::QueueBit(unsigned char bit)
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

void CTextReader::QueueByte(unsigned char byte)
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

const char* CTextReader::GetDataFormat()
{
	return strlen(_dataFormat)>0 ? _dataFormat : NULL;
}

Resolution CTextReader::GetResolution()
{
	return _res;
}

void CTextReader::Delete()
{
	delete this;
}

bool CTextReader::IsWaveFile()
{
	return false;
}

int CTextReader::CurrentPosition()
{
	return _currentPosition;
}

int CTextReader::ReadCycleLen()
{
	// Should never happen
	assert(false);
	return -1;
}

char CTextReader::ReadCycleKind()
{
	assert(_res >= resCycleKinds);

	if (_currentPosition>=_dataLength)
		return 0;

	return ((char*)_buffer)[_currentPosition++];
}

void CTextReader::Seek(int position)
{
	_currentPosition = position;
}

char* CTextReader::FormatDuration(int duration)
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

int CTextReader::LastCycleLen()
{
	return 30;
}

bool CTextReader::SyncToBit(bool verbose)
{
	if (_res < resBits)
		return __super::SyncToBit(verbose);
	else
		return _currentPosition < _dataLength;
}

int CTextReader::ReadBit(bool verbose)
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

bool CTextReader::SyncToByte(bool verbose)
{
	if (_res < resBytes)
		return __super::SyncToByte(verbose);
	else
		return _currentPosition < _dataLength;
}

int CTextReader::ReadByte(bool verbose)
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
