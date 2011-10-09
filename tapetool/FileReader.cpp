//////////////////////////////////////////////////////////////////////////
// FileReader.cpp - implementation of CFileReader class

#include "precomp.h"

#include "FileReader.h"
#include "MachineType.h"
#include "CommandContext.h"

//////////////////////////////////////////////////////////////////////////
// CFileReader

CFileReader::CFileReader(CCommandContext* ctx)
{
	_ctx = ctx;
}

char CFileReader::ReadCycleKindChecked(bool verbose)
{
	int pos = CurrentPosition();
	char kind = ReadCycleKind();

	// Error?
	if (kind=='<' || kind=='>')
	{
		if (_ctx->allowBadCycles && (LastCycleLen() >2 && LastCycleLen() < 200))
		{
			return kind;
		}

		if (verbose)
			printf("[Invalid cycle kind '%c' - %i samples - at %i]", kind, LastCycleLen(), pos);
		return 0;
	}

	return kind;
}

bool CFileReader::SyncToBit(bool verbose)
{
	return _ctx->machine->SyncToBit(this, verbose);
}

int CFileReader::ReadBit(bool verbose)
{
	return _ctx->machine->ReadBit(this, verbose);
}

bool CFileReader::SyncToByte(bool verbose)
{
	return _ctx->machine->SyncToByte(this, verbose);
}

int CFileReader::ReadByte(bool verbose)
{
	return _ctx->machine->ReadByte(this, verbose);
}
