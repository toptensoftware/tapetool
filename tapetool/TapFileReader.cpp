//////////////////////////////////////////////////////////////////////////
// TapFileReader.cpp - implementation of CTapFileReader class

#include "precomp.h"

#include "TapFileReader.h"

CTapFileReader::CTapFileReader(CCommandStd* cmd) : CBinaryReader(cmd)
{
}

CTapFileReader::~CTapFileReader()
{
}

bool CTapFileReader::Open(const char* filename, Resolution res)
{
	if (!CBinaryReader::Open(filename, res))
		return false;

	// Special handling for tap file format
	if (_stricmp(_dataFormat, "tap")==0)
	{
		char sig[20];
		int read = fread(sig, 13, 1, _file);
		sig[13]=0;
		if (strcmp(sig, "TAP_DGOS_MBEE")!=0)
		{
			fprintf(stderr, "Tap file doesn't commence with 'TAP_DGOS_MBEE' header\n");
			return false;
		}
	}

	// All good
	return true;
}

const char* CTapFileReader::GetDataFormat()
{
	return "tap";
}

