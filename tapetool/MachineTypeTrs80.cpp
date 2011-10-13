//////////////////////////////////////////////////////////////////////////
// MachineTypeTrs80.cpp - implementation of CMachineTypeTrs80 class

#include "precomp.h"

#include "MachineTypeTrs80.h"
#include "FileReader.h"
#include "WaveWriter.h"
#include "Context.h"
#include "WaveAnalysis.h"

const char* basic_keywords[] = {
	"END",
	"FOR",
	"RESET",
	"SET",
	"CLS",
	"CMD",
	"RANDOM",
	"NEXT",
	"DATA",
	"INPUT",
	"DIM",
	"READ",
	"LET",
	"GOTO",
	"RUN",
	"IF",
	"RESTORE",
	"GOSUB",
	"RETURN",
	"REM",
	"STOP",
	"ELSE",
	"TRON",
	"TROFF",
	"DEFSTR",
	"DEFINT",
	"DEFSNG",
	"DEFDBL",
	"LINE",
	"EDIT",
	"ERROR",
	"RESUME",
	"OUT",
	"ON",
	"OPEN",
	"FIELD",
	"GET",
	"PUT",
	"CLOSE",
	"LOAD",
	"MERGE",
	"NAME",
	"KILL",
	"LSET",
	"RSET",
	"SAVE",
	"SYSTEM",
	"LPRINT",
	"DEF",
	"POKE",
	"PRINT",
	"CONT",
	"LIST",
	"LLIST",
	"DELETE",
	"AUTO",
	"CLEAR",
	"CLOAD",
	"CSAVE",
	"NEW",
	"TAB",
	"TO",
	"FN",
	"USING",
	"VARPTR",
	"USR",
	"ERL",
	"ERR",
	"STRING$",
	"INSTR",
	"POINT",
	"TIME$",
	"MEM",
	"INKEY$",
	"THEN",
	"NOT",
	"STEP",
	"+",
	"-",
	"*",
	"/",
	"<up>",
	"AND",
	"OR",
	">",
	"=",
	"<",
	"SGN",
	"INT",
	"ABS",
	"FRE",
	"INP",
	"POS",
	"SQR",
	"RND",
	"LOG",
	"EXP",
	"COS",
	"SIN",
	"TAN",
	"ATN",
	"PEEK",
	"CVI",
	"CVS",
	"CVD",
	"EOF",
	"LOC",
	"LOF",
	"MKI$",
	"MKS$",
	"MKD$",
	"CINT",
	"CSNG",
	"CDBL",
	"FIX",
	"LEN",
	"STR$",
	"VAL",
	"ASC",
	"CHR$",
	"LEFT$",
	"RIGHT$",
	"MID$",
	"'",
	"",
	"",
	"",
	"",
};

CMachineTypeTrs80::CMachineTypeTrs80()
{
	assert(sizeof(basic_keywords)/sizeof(const char*) == 128);
	_syncBytePosition=-1;
}

CMachineTypeTrs80::~CMachineTypeTrs80()
{
}


bool CMachineTypeTrs80::SyncToBit(CFileReader* reader, bool verbose)
{
	if (verbose)
		printf("[BitSync:");

	// Find the first long cycle
	while (true)
	{
		int pos = reader->CurrentPosition();

		// Read next cycle
		char kind = reader->ReadCycleKind();
		int rewindCycles=1;

		if (kind==0)
		{
			if (verbose)
				printf(":eof]");
			return false;
		}

		if (verbose)
			printf("%c", kind);

		if (kind!='S' && kind!='L')
			continue;

		int posContinue = reader->CurrentPosition();

		// If it's a short cycle, it could be the data or the clock pulse, check it
		if (kind=='S')
		{
			int pos2 = reader->CurrentPosition();
			kind = reader->ReadCycleKind();
			if (kind==0)
			{
				if (verbose)
					printf(":eof]");
				return false;
			}

			if (verbose)
				printf("%c", kind);

			if (kind!='S' && kind!='L')
				continue;

			if (reader->ReadCycleKind()=='L')
			{
				// First cycle must have been the data pulse, we want to sync on the clock pulse
				pos = pos2;
			}
			else
			{
				// Two short cycles, the first must have been the clock pulse
				rewindCycles = 2;
			}
		}

		// Make sure we can read at least 8 bytes
		reader->Seek(pos);
		bool error = false;
		for (int i=0; i<8 && !error; i++)
		{
			error = ReadByte(reader, false) < 0;
		}

		// Success?
		if (!error)
		{
			// Go back to the sync pos
			if (verbose)
				printf(" rewound %i cycles to sync at %i]", rewindCycles, pos);
			reader->Seek(pos);
			return true;
		}

		// Skip that supposed "bit" and try again
		reader->Seek(posContinue);
	}


}										 


int CMachineTypeTrs80::ReadBit(CFileReader* reader, bool verbose)
{	
	int savePos = reader->CurrentPosition();
	int kind = reader->ReadCycleKindChecked(verbose);

	if (kind==0)
		return -1;

	if (kind=='L')
		return 0;

	if (kind=='S')
	{
		savePos = reader->CurrentPosition();
		kind = reader->ReadCycleKindChecked(verbose);
		if (kind=='S' || kind==0)
			return 1;
	}

	if (verbose)
	{
		if (kind!=0)
			printf("[bit error, unexpected cycle '%c' at %i]", kind, savePos);
		else
		{
			if (kind=='S')
				return 1;
			printf("[bit error, eof at %i]", savePos);
		}
	}

	return -1;
}

bool CMachineTypeTrs80::SyncToByte(CFileReader* reader, bool verbose)
{
	if (verbose)
		printf("[ByteSync:");

	// Sync to bit first
	if (!SyncToBit(reader, verbose))
		return false;

	// Have we rewound after already synced
	if (reader->CurrentPosition() < _syncBytePosition)
	{
		reader->Seek(_syncBytePosition);
		printf("bof]");
		return true;
	}

	// Look for the leading sync byte 
	if (_syncBytePosition<0)
	{
		if (verbose)
			printf("[scanning for a5 sync byte...]\n");

		while (true)
		{
			int syncBit = reader->CurrentPosition();

			while (true)
			{
				int byte = ReadByte(reader, false);
				if (byte<0)
					break;

				if (byte == 0)
					continue;

				if (byte == 0xA5)
				{
					if (verbose)
					{
						printf(":found sync byte at %i, resyncing to %i]", reader->CurrentPosition(), syncBit);
					}
					_syncBytePosition = syncBit;
					reader->Seek(_syncBytePosition);
					return true;
				}
			
				// Found something other that 0x00 or 0xA5, break out
				break;
			}

			// Rewind, skip one cycle and try again
			reader->Seek(syncBit);
			int kind = reader->ReadCycleKind();
			if (!SyncToBit(reader, verbose))
			{
				if (verbose)
					printf(":no bit sync]");
				return false;
			}
			if (verbose)
				printf("%c", kind);
		}
	}


	printf(":bitsynconly]");

	return true;
}

int CMachineTypeTrs80::ReadByte(CFileReader* reader, bool verbose)
{
	unsigned char byte = 0x00;
	for (int i=0; i<8; i++)
	{
		int pos = reader->CurrentPosition();
		int bit = ReadBit(reader, verbose);
		if (bit < 0)
		{
			if (verbose && i>0)
				printf("[Corrupted data at %i, error reading bit]", pos);
			return -1;
		}

		byte = (byte << 1) | bit;
	}

	return byte;
}


void CMachineTypeTrs80::RenderPulse(CWaveWriter* writer)
{
	int shortCycleLength = writer->SampleRate() / 1024;
	int pulseLength = shortCycleLength / 2;
	//int reboundLength = pulseLength * 2 / 3;

	for (int i=0; i<pulseLength; i++)
	{
		writer->RenderSample((short)(sin(2*PI*i/pulseLength) * writer->GetAmplitude()));
	}

	for (int i=0; i<shortCycleLength - pulseLength; i++)
	{
		writer->RenderSample(0);
	}
}

void CMachineTypeTrs80::RenderCycleKind(CWaveWriter* writer, char kind)
{
	RenderPulse(writer);
	if (kind=='L')
	{
		for (int i=0; i<writer->SampleRate() / 1024; i++)
		{
			writer->RenderSample(0);
		}
	}
}

void CMachineTypeTrs80::RenderBit(CWaveWriter* writer, unsigned char bit)
{
	if (bit)
	{
		RenderCycleKind(writer, 'S');
		RenderCycleKind(writer, 'S');
	}
	else
	{
		RenderCycleKind(writer, 'L');
	}
}

void CMachineTypeTrs80::RenderByte(CWaveWriter* writer, unsigned char byte)
{
	for (int i=0; i<8; i++)
	{
		RenderBit(writer, byte & 0x80);
		byte <<= 1;
	}

}


enum FileType
{
	ftSystem = 0x00000055,
	ftSource = 0x000000D3,
	ftBasic = 0x0000D3D3,
};

// Command handler for dumping formatted header block and CRC checked data blocks
int CMachineTypeTrs80::ProcessBlocks(CContext* c)
{
	// Open files
	if (!c->OpenFiles(resBytes))
		return 7;


	printf("\n");
	c->file->SyncToByte(c->showSyncData);
	printf("\n\n");

	printf("[lead in]\n");
	c->ResetByteDump();
	while (true)
	{
		// Read a byte
		int byte = c->file->ReadByte();
		if (byte<0)
		{
			printf("\nFailed to read byte\n\n");
			return 7;
		}

		// Dump it
		c->DumpByte(byte);
		if (byte==0xA5)
			break;

		if (byte!=0)
		{
			printf("\nFailed to locate leadin, expected 0x00 or 0xA5\n\n");
			return 7;
		}
	}

	if (c->IsOutputKind("cas") || c->IsOutputKind("wav"))
	{
		unsigned char byte = 0;
		for (int i=0; i<0xF5; i++)
		{
			fwrite(&byte, 1, 1, c->binaryFile);
			if (c->renderFile)
				c->machine->RenderByte(c->renderFile, byte);
		}
		byte=0xA5;
		fwrite(&byte, 1, 1, c->binaryFile);
		if (c->renderFile)
			c->machine->RenderByte(c->renderFile, byte);
	}

	printf("\n\n[header]\n");
	c->ResetByteDump();

	int headerBytesRead = 0;
	int fileType = 0;
	char header[20];
	while (true)
	{
		// Read a byte
		int byte = c->file->ReadByte();
		if (byte<0)
		{
			printf("\nFailed to read byte\n\n");
			return 7;
		}

		header[headerBytesRead] = (char)byte;
		headerBytesRead++;
		header[headerBytesRead] = 0x0;

		// First byte is the header byte
		if (headerBytesRead==1)
		{
			fileType = byte;
			if (byte!=ftSystem && byte!=ftSource)
			{
				printf("\nUnrecognized file type 0x%.2x, expected 0x55 or 0xD3", byte);
				return 7;
			}
		}

		// Is it basic file?
		if (headerBytesRead==2 && fileType==ftSource && byte==0xD3)
		{
			fileType = ftBasic;
		}

		// Check for third 0xD3 BASIC marker
		if (headerBytesRead==3 && fileType==ftBasic && byte!=0xD3)
		{
			printf("\nInvalid BASIC header, expected 0xD3 found 0x%.2i", byte);
			return 7;
		}

		// Dump it
		c->DumpByte(byte);
		if (c->renderFile)
			c->machine->RenderByte(c->renderFile, byte);
		if (c->IsOutputKind("cas"))
			fwrite(&byte, 1, 1, c->binaryFile);

		// End of system header?
		if (headerBytesRead== ((fileType==ftBasic) ? 4 : 7))
			break;
	}

	printf("\n\n[\n");
	if (fileType==ftBasic)
	{
		printf("    file type: BASIC\n");
		printf("    file name: %c\n", header[3]);
	}
	else
	{
		printf("    file type: %s\n", fileType==ftSystem ? "SYSTEM" : "SOURCE");
		printf("    file name: '%s'\n", header+1);
	}

	printf("]\n\n");

	printf("\n");

	// Work out whether block processing should output binary data
	_writeBinaryData = c->binaryFile!=NULL && !c->IsOutputKind("cas");

	_eof = false;
	bool data_ok = true;
	int error_position;
	while (!_eof)
	{
		// Reset block data buffer
		_blockDataLen = 0;

		// Save current position
		int pos = c->file->CurrentPosition();

		if (c->showPositionInfo && data_ok)
		{
			if (c->showPositionInfo)
				printf("\n[@%12i] ", pos);
		}

		bool data_was_ok = data_ok;

		// Read a block
		switch (fileType)
		{
			case ftSystem:
				data_ok = ProcessSystemBlock(c, data_ok);
				break;

			case ftSource:
				data_ok = ProcessSourceBlock(c, data_ok);
				break;

			case ftBasic:
				data_ok = ProcessBasicBlock(c, data_ok);
				break;
		}

		if (!data_was_ok && data_ok)
		{
			printf("[scan succeeded, next block found at %i, %s skipped]\n", pos, c->file->FormatDuration(pos-error_position));
		}

		// Dump the processed data (unless we're in resync mode)
		if (data_was_ok)
		{
			c->ResetByteDump();
			for (int i=0; i<_blockDataLen; i++)
			{
				c->DumpByte(_blockData[i]);
				if (c->renderFile!=NULL)
					RenderByte(c->renderFile, _blockData[i]);
				if (c->IsOutputKind("cas"))
					fwrite(&_blockData[i], 1, 1, c->binaryFile);
			}
			printf("\n");

			// If this is the first error block, save where we are
			if (!data_ok)
			{
				error_position = c->file->CurrentPosition();
				printf("\n[scanning from %i for next valid block]\n", error_position);
			}
		}

		if (!data_ok)
		{
			// Rewind to position
			c->file->Seek(pos);
		
			// Skip and discard one cycle
			if (c->file->ReadCycleKind()==0)
				break;
		}
	}

	printf("\n\n[eof]\n");

	return 0;
}

bool CMachineTypeTrs80::ProcessSystemBlock(CContext* c, bool verbose)
{
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
*/						

	int blockAddress = 0;
	int blockLen = 0;
	int entryPoint = 0;
	unsigned char checksum = 0;
	bool eofblock = false;
	while (true)
	{
		int byte = c->file->ReadByte(verbose);
		if (byte<0)
		{
			if (verbose)
				printf("\nFailed to read byte\n\n");
			return false;
		}
		_blockData[_blockDataLen++] = byte;

		if (_blockDataLen==1)
		{
			if (byte==0x78 && verbose)		// Don't look for eof bytes while in recovery mode
			{
				eofblock=true;
				continue;
			}

			if (byte!=0x3c)
			{
				if (verbose)
					printf("\nInvalid system block header, expected 0x3C or 0x78 but found 0x%.2x\n", byte);
				return false;
			}
			continue;
		}

		if (_blockDataLen==2)
		{
			if (eofblock)
			{
				entryPoint = byte;
			}
			else
			{
				blockLen = byte==0 ? 0x100 : byte;
			}
			continue;
		}
		if (_blockDataLen==3)
		{
			if (eofblock)
			{
				entryPoint |= byte << 8;
				printf("\n[EOF Block - entry point: 0x%.4x]\n", entryPoint);
				_eof = true;
				return true;
			}
			else
			{
				blockAddress = byte;
				checksum += byte;
				continue;
			}
		}
		if (_blockDataLen==4)
		{
			blockAddress |= byte << 8;
			checksum += byte;
			continue;
		}

		// How many data bytes have we read?
		int dataBytesRead = _blockDataLen - 4;

		if (dataBytesRead <= blockLen)
		{
			// write the byte
			if (_writeBinaryData)
				fwrite(&byte, 1, 1, c->binaryFile);

			checksum+=byte;
		}
		else
		{
			if (checksum!=byte)
			{
				if (verbose)
					printf("\nCheck sum error, should be 0x%.2x but was 0x%.2x", byte, checksum);
				return false;
			}

			printf("\n[Data Block - addr: 0x%.4x, length: 0x%.2x, checksum: 0x%.2x]\n", blockAddress, blockLen, checksum);
			return true;
		}
	}
}

bool CMachineTypeTrs80::ProcessSourceBlock(CContext* c, bool verbose)
{
/*
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
*/
	char basicLine[1024];
	char* bpos = basicLine;
	while (true)
	{
		int byte = c->file->ReadByte(verbose);
		if (byte<0)
		{
			if (verbose)
				printf("\nFailed to read byte\n\n");
			return false;
		}
		_blockData[_blockDataLen++] = byte;

		if (_blockDataLen==1 && byte==0x1A)
		{
			printf("[Eof Terminator]\n");
			_eof = true;
			return true;
		}

		if (_blockDataLen<=5)
		{
			if ((byte & 0x80)==0)
			{
				printf("\nError in file format, expected a line number with bit 7 set, found 0x%.2x\n", byte);
				return false;
			}
			byte &= ~0x80;
			if (byte<'0' || byte>'9')
			{
				printf("\nError in file format, expected a line number digit, found 0x%.2x\n", byte|0x80);
				return false;
			}
			*bpos++ = byte;
			continue;
		}	
		if (_blockDataLen==6)
		{
			if (byte!=0x20)
			{
				printf("\nError in file format, expected line marker leading space, found 0x%.2x\n", byte);
				return false;
			}
			*bpos++ = byte;
			continue;
		}

		if (byte==0x0d)
		{
			// End of line
			*bpos = 0;
			printf("// %s\n", basicLine);
			return true;
		}

		*bpos++ = byte;
	}
}

bool CMachineTypeTrs80::ProcessBasicBlock(CContext* c, bool verbose)
{
/*
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

	char basicLine[1024];
	char* bpos = basicLine;
	unsigned int nextLine = 0;
	unsigned int lineNumber = 0;
	while (true)
	{
		int byte = c->file->ReadByte(verbose);
		if (byte<0)
		{
			if (verbose)
				printf("\nFailed to read byte\n\n");
			return false;
		}
		_blockData[_blockDataLen++] = byte;

		if (_blockDataLen==1)
		{
			nextLine = byte;
			continue;
		}
		if (_blockDataLen==2)
		{
			nextLine |= byte << 8;
			if (nextLine==0)
			{
				_eof = true;
				printf("[Eof Terminator]\n");
				return true;
			}
			continue;
		}
		if (_blockDataLen==3)
		{
			lineNumber = byte;
			continue;
		}
		if (_blockDataLen==4)
		{
			lineNumber |= byte << 8;
			continue;
		}

		if (byte==0)
		{
			*bpos = 0;
			printf("[0x%.4x] // %5i %s\n", nextLine, lineNumber, basicLine);
			return true;
		}

		if (byte & 0x80)
		{
			strcpy(bpos, basic_keywords[byte-0x80]);
			bpos += strlen(bpos);
		}
		else
		{
			*bpos++ = byte;
		}
	}
}

void CMachineTypeTrs80::PrepareWaveMetrics(CContext* c, CWaveReader* wf)
{
	if (c->autoAnalyze)
	{
		WAVE_INFO info;

		fprintf(stderr, "\n\nAnalysing wave data...");	
		
		AnalyseWave(wf, 0, 0, info);
		fprintf(stderr, "\n\n");

		int offsetForPulse;
		if (abs(info.medianMinAmplitude) > abs(info.medianMaxAmplitude))
		{
			offsetForPulse = info.medianMinAmplitude * 3 / 4;
		}
		else
		{
			offsetForPulse = info.medianMaxAmplitude * 3 / 4;
		}

		//printf("[using pulse threshold of %i]\n", offsetForPulse);
		wf->SetDCOffset(-offsetForPulse);
		wf->SetCycleLengths(info.medianShortCycleLength, info.medianLongCycleLength);
	}
	else
	{
		wf->SetShortCycleFrequency(1024);
		if (wf->GetDCOffset()==0)
		{
			printf("[WARNING: no DC offset set, pulse detection won't work]\n");
		}
	}
}

/*
int CMachineTypeTrs80::CycleFrequency()
{
	return 1024;
}

int CMachineTypeTrs80::DcOffset(CWaveReader* wave)
{
	fprintf(stderr, "\n\nAnalysing wave to determine pulse threshold...");
	WAVE_INFO info;
	AnalyseWave(wave, 0, 0, info);

	fprintf(stderr, "\n\n");

	int offsetForPulse;
	if (abs(info.medianMinAmplitude) > abs(info.medianMaxAmplitude))
	{
		offsetForPulse = info.medianMinAmplitude * 3 / 4;
	}
	else
	{
		offsetForPulse = info.medianMaxAmplitude * 3 / 4;
	}

	printf("[wave analysis reported median amplitude range of %i to %i, using pulse threshold of %i]\n", info.medianMinAmplitude, info.medianMaxAmplitude, offsetForPulse);
	return -offsetForPulse;
}
*/



/*
	int offsetForPulse;
	if (abs(info.medianMinAmplitude) > abs(info.medianMaxAmplitude))
	{
		offsetForPulse = -info.medianMinAmplitude * 3 / 4;
	}
	else
	{
		offsetForPulse = -info.medianMaxAmplitude * 3 / 4;
	}
	
	printf("    DC Offset For Pulses:  %i\n", offsetForPulse);
*/