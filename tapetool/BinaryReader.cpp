//////////////////////////////////////////////////////////////////////////
// BinaryReader.cpp - implementation of CBinaryReader class

#include "precomp.h"

#include "BinaryReader.h"
#include "Context.h"

CBinaryReader::CBinaryReader(CCommandStd* cmd) : CFileReader(cmd)
{
	_file = NULL;
}

CBinaryReader::~CBinaryReader()
{
	if (_file!=NULL)
		fclose(_file);
}

bool CBinaryReader::Open(const char* filename, Resolution res)
{
	// Open the file
    _file=fopen(filename,"rb");
	if (_file==NULL)
	{
	    fprintf(stderr, "Could not open '%s' - %s (%i)\n", filename, strerror(errno), errno);
		return false;
	}

	fseek(_file, 0, SEEK_END);
	_length = ftell(_file);
	fseek(_file, 0, SEEK_SET);
		
	// Work out data format from extension
	_dataFormat = strrchr(filename, '.');
	if (_dataFormat!=NULL)
		_dataFormat++;

	// All good
	return true;
}

const char* CBinaryReader::GetDataFormat()
{
	return _dataFormat;
}

Resolution CBinaryReader::GetResolution()
{
	return resBytes;
}

void CBinaryReader::Delete()
{
	delete this;
}

bool CBinaryReader::IsWaveFile()
{
	return false;
}

int CBinaryReader::CurrentPosition()
{
	return ftell(_file);
}

int CBinaryReader::ReadCycleLen()
{
	// Should never happen
	assert(false);
	return -1;
}

char CBinaryReader::ReadCycleKind()
{
	return 0;
}

void CBinaryReader::Seek(int position)
{
	fseek(_file, position, SEEK_SET);
}

char* CBinaryReader::FormatDuration(int duration)
{
	static char sz[512];

	sprintf(sz, "%i bytes", duration);
	return sz;
}

int CBinaryReader::LastCycleLen()
{
	return 30;
}

bool CBinaryReader::SyncToBit(bool verbose)
{
	return ftell(_file) < _length;
}

int CBinaryReader::ReadBit(bool verbose)
{
	assert(false);
	return -1;
}

bool CBinaryReader::SyncToByte(bool verbose)
{
	return ftell(_file) < _length;
}

int CBinaryReader::ReadByte(bool verbose)
{
	unsigned char byte;
	if (fread(&byte, 1, 1, _file)!=1)
		return -1;

	return byte;
}
