//////////////////////////////////////////////////////////////////////////
// MachineTypeMicrobee.cpp - implementation of CMachineTypeMicrobee class

#include "precomp.h"

#include "MachineTypeMicrobee.h"
#include "FileReader.h"
#include "WaveWriter.h"
#include "CommandStd.h"
#include "TapFileReader.h"
#include "WaveAnalysis.h"
#include "Instrumentation.h"
#include "TapeReader.h"
#include "WaveWriterProfiled.h"

#pragma pack(1)
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


CMachineTypeMicrobee::CMachineTypeMicrobee()
{
	_baud = 300;
}

CMachineTypeMicrobee::~CMachineTypeMicrobee()
{
}

void CMachineTypeMicrobee::SetOutputBaud(int baud)
{
	assert(baud==600 || baud==1200 || baud==300);
	_baud = baud;
}

CFileReader* CMachineTypeMicrobee::CreateFileReader(CCommandStd* c, const char* pszExt)
{
	if (pszExt && _stricmp(pszExt, ".tap")==0)
		return new CTapFileReader(c);
	return NULL;
}

bool CMachineTypeMicrobee::SyncToBit(CFileReader* reader, bool verbose)
{
	CSyncBlock sync(reader->GetInstrumentation());

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
		int cycleStart = reader->CurrentPosition();

		// Read the next cycle kind
		char kind = reader->ReadCycleKind();

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
			int savePos = reader->CurrentPosition();

			int boundary = (buf[1]=='?' || buf[1]==buf[2]) ? 1 : 2;

			reader->Seek(offs[boundary]);

			// Read at least a 0 and a 1 bit
			bool error = false;
			int bitCount[2] = { 0, 0 };
			while (true)
			{
				int bit = ReadBit(reader, false);
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
				reader->Seek(offs[boundary]);
				return true;
			}

			// Now where were we?
			reader->Seek(savePos);
		}
	}
}										 


int CMachineTypeMicrobee::ReadBit(CFileReader* reader, bool verbose)
{	
	CInstrumentation* instr = reader->GetInstrumentation();

	int savePos = reader->CurrentPosition();
	int bitPos = savePos;

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

	// Are we in highspeed mode?
	int speedMultiplier = 1;
	if (savePos >= reader->_cmd->speedChangePos)
	{
		switch (reader->_cmd->speedChangeSpeed)
		{
			case 300:
				speedMultiplier = 1;
				break;

			case 600:
				speedMultiplier = 2;
				break;

			default:
				speedMultiplier = 4;
				break;
		}
	}
		
	while (true)
	{
		// Remember where this cycle is, incase we need to 
		int currentCyclePos = reader->CurrentPosition();

		// Get the next cycle
		char cycle = reader->ReadCycleKindChecked(verbose);
		if (cycle==0)
			return -1;
		cyclesRead++;
		actualCyclesRead++;

		// First cycle
		if (cyclesRead == 1)
		{
			if (cycle=='L' && speedMultiplier==4)
			{ 
				if (instr)
					instr->AddBitEntry(speedMultiplier, 0, savePos, reader->CurrentPosition());
				return 0;
			}

			if (cycle=='S' || cycle=='L')
			{
				bitKind = cycle;
				continue;
			}

			if (reader->_cmd->_strict)
			{
				if (verbose)
					printf("[strict mode leading bit error at %i - expected S or L, found '%c']", savePos, cycle);
			}
			
			continue;
		}

		// Allow a bit resync after the first cycle (only in 300 baud mode)
		if (!reader->_cmd->_strict && speedMultiplier==1 && cyclesRead == 2 && ((cycle=='S' && bitKind=='L') || (cycle=='L' && bitKind=='S')))
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
			bitPos = reader->CurrentPosition();

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
		int expectedCycles = (bitKind == 'S' ? 8 : 4) / speedMultiplier;

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
			if (reader->_cmd->_strict && cycle!=bitKind)
			{
				if (verbose)
					printf("[strict mode trailing bit error at %i - expected '%c', found '%c']", savePos, bitKind, cycle);
			}


			// Success!
			int bit = bitKind == 'S' ? 1 : 0;

			// Conflict? Trailing resync
			if (cycle != bitKind && (cycle=='S' || cycle=='L'))
			{
				reader->Seek(currentCyclePos);
			}

			if (instr)
				instr->AddBitEntry(speedMultiplier, bit, savePos, reader->CurrentPosition());

			return bit;
		}
	}
}

bool CMachineTypeMicrobee::SyncToByte(CFileReader* reader, bool verbose)
{	
	CSyncBlock sync(reader->GetInstrumentation());

	if (verbose)
		printf("[ByteSync:");

	// Sync to next bit
	if (!reader->SyncToBit(verbose))
		return false;
	if (verbose)
		printf(" ");

	while (true)
	{
		// Remember start of this bit
		int syncBit = reader->CurrentPosition();

		// Try to read bytes
		int byteSyncMask = 0;
		while (true)
		{
			// Read a byte, bail if can't...
			int byte = ReadByte(reader, false);
			if (byte<0)
				break;

			byteSyncMask |= byte;

			// if we find all 0 or we've a seen a 1 bit in every data byte position
			// then we've synchronized...
			if (byte==0 || byteSyncMask==0xFF)
			{
				reader->Seek(syncBit);
				if (verbose)
					printf(" synced at %i]", syncBit);
				return true;
			}
		}

		// Rewind to start of the bit
		reader->Seek(syncBit);

		// Skip one bit
		int skipBit = ReadBit(reader, false);
		if (skipBit<0)
		{
			// Failed to read a bit, need to resync...
			if (!reader->SyncToBit(verbose))
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

		if (reader->CurrentPosition()==syncBit)
		{
			int x=3;
		}
	}
}

int CMachineTypeMicrobee::ReadByte(CFileReader* reader, bool verbose)
{
	// Read 11 bits to make a byte : 0nnnnnnnn11 (little endian order)
	int byte = 0;
	for (int i=0; i<11; i++)
	{
		int offset = reader->CurrentPosition();
		int bit = ReadBit(reader, verbose);
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


void CMachineTypeMicrobee::RenderCycleKind(CWaveWriter* writer, char kind)
{
	if (kind=='S')
	{
		writer->RenderWave(1, writer->SampleRate() / 300 / 8);
	}
	else if (kind=='L')
	{
		writer->RenderWave(1, writer->SampleRate() / 300 / 4);
	}
	else
	{
	}
}

void CMachineTypeMicrobee::RenderBit(CWaveWriter* writer, unsigned char bit)
{
	int cycles = (bit ? 8 : 4) / (_baud/300);
	if (writer->GetProfiledResolution() == resCycleKinds)
	{
		for (int i=0; i<cycles; i++)
		{
			writer->RenderProfiledCycleKind(bit ? 'S' : 'L');
		}
	}
	else if (writer->GetProfiledResolution() ==resBits)
	{
		writer->RenderProfiledBit(_baud/300, bit);
	}
	else
	{
		writer->RenderWave( cycles, writer->SampleRate() / _baud);
	}

	/*
	if (_baud == 300)
	{
		if (bit)
		{
			writer->RenderWave(8, writer->SampleRate() / _baud);
		}
		else
		{
			writer->RenderWave(4 , writer->SampleRate() / _baud);
		}
	}
	else if (
	{
		if (bit)
		{
			writer->RenderWave(2, writer->SampleRate() / 1200);
		}
		else
		{
			writer->RenderWave(1, writer->SampleRate() / 1200);
		}
	}
	else
	{
		if (bit)
		{
			writer->RenderWave(2, writer->SampleRate() / 1200);
		}
		else
		{
			writer->RenderWave(1, writer->SampleRate() / 1200);
		}
	}
	*/
}

void CMachineTypeMicrobee::RenderByte(CWaveWriter* writer, unsigned char byte)
{
	RenderBit(writer, 0);
	for (int i=0; i<8; i++)
	{
		RenderBit(writer, byte & 0x01);
		byte>>=1;
	}
	RenderBit(writer, 1);
	RenderBit(writer, 1);
}


// Command handler for dumping formatted header block and CRC checked data blocks
int CMachineTypeMicrobee::ProcessBlocks(CCommandStd* c)
{
	// Open files
	if (!c->OpenFiles(resBytes))
		return 7;

	printf("\n");
	c->file->SyncToByte(c->showSyncData);
	printf("\n\n");

	if (c->IsOutputKind("tap"))
	{
		fwrite("TAP_DGOS_MBEE",0x0d, 1, c->binaryFile);
		unsigned char b = 0;
		for (int i=0; i<0x40; i++)
		{
			fwrite(&b, 1, 1, c->binaryFile);
		}
		b = 1;
		fwrite(&b, 1, 1, c->binaryFile);
	}

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
		if (c->renderFile)
			c->machine->RenderByte(c->renderFile, byte);
		if (byte==1)
			break;

		if (byte!=0)
		{
			printf("\nFailed to locate leadin, expected 0x00 or 0x01\n\n");
			return 7;
		}
	}

	printf("\n\n[header]\n");
	c->ResetByteDump();
	unsigned char checksum=16;
	TAPE_HEADER header;
	unsigned char* header_bytes = (unsigned char*)&header;

	for (int i=0; i<17; i++)
	{
		// Read a byte
		int byte = c->file->ReadByte();
		if (byte<0)
		{
			printf("\nFailed to read byte\n\n");
			return 7;
		}

		if (i<sizeof(TAPE_HEADER))
		{
			header_bytes[i]=byte;
			// Dump it
			c->DumpByte(byte);
		}
		else
			printf("\n[checksum byte:] 0x%2x\n\n", byte);


		if (c->IsOutputKind("tap"))
			fwrite(&byte, 1, 1, c->binaryFile);

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

	if (c->IsOutputKind("bee"))
		fwrite(header_bytes, 16, 1, c->binaryFile);

	// Highspeed read?
	if (header.speed && c->speedChangePos==0x7FFFFFFF)
	{
		c->speedChangePos = c->file->CurrentPosition();
		c->speedChangeSpeed = header.speed == 2 ? 600 : 1200;
	}

	if (c->renderFile!=NULL)
	{
		// Flip the speed byte if necessary
		switch (c->renderBaud)
		{
			case 1200:
				header.speed = 0xFF;
				break;

			case 600:
				header.speed = 2;		// not documented but used by the Machine Code Tutorial tapes for 
										// main content after the tiny loader
				break;

			default:
				header.speed = 0;
				break;
		}

		// Calculate a new checksum
		checksum=sizeof(header);
		for (int i=0; i<sizeof(header); i++)
		{
			c->machine->RenderByte(c->renderFile, header_bytes[i]);
			checksum += header_bytes[i];
		}

		c->machine->RenderByte(c->renderFile, 0x100-checksum);
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
	printf("    file name:    '%c%c%c%c%c%c'\n", header.filename[0], header.filename[1], header.filename[2], header.filename[3], header.filename[4], header.filename[5]);
	printf("    file type:    %c\n", header.filetype);
	printf("    data length:  0x%.4x (%i) bytes\n", header.datalen, header.datalen);
	printf("    load addr:    0x%.4x\n", header.loadaddr);
	printf("    start addr:   0x%.4x\n", header.startaddr);
	printf("    speed:        %s baud\n", header.speed == 0 ? "300" : (header.speed==2 ? "600" : "1200" ));
	printf("    auto start:   %s\n", header.autostart == 0xFF ? "yes" : "no" );

	printf("]\n\n");

	// Switch bit rendering to correct baud rate
	if (c->renderFile!=NULL)
	{
		if (c->renderBaud==1200 || c->renderBaud==600)
			SetOutputBaud(c->renderBaud);
	}

	int blockAddr = 0;

	while (blockAddr < header.datalen)
	{
		int bytesRemaining = header.datalen - blockAddr;
		int iBytesThisBlock = bytesRemaining > 256 ? 256 : bytesRemaining;

		printf("\n[@%12i][data block 0x%.4x, %i bytes]\n", c->file->CurrentPosition(), blockAddr, iBytesThisBlock);
		c->ResetByteDump();
		unsigned char checksum=iBytesThisBlock;

		for (int i=0; i<iBytesThisBlock+1; i++)
		{
			// Read a byte
			int byte = c->file->ReadByte();

			if (byte<0 && i==bytesRemaining)
			{
				byte = 256-checksum;
				printf("\n[guessing trailing checksum value]");
			}
			else if (byte<0)
			{

				printf("\nFailed to read byte, %i bytes missing\n\n", bytesRemaining - i + 1);		// plus 1 for the trailing checksum
				return 7;
			}

			if (i==iBytesThisBlock)
			{
				printf("\n[checksum byte:] 0x%.2x", byte);

				if (c->IsOutputKind("tap"))
					fwrite(&byte, 1, 1, c->binaryFile);
			}
			else
			{
				// Dump it
				c->DumpByte(byte);

				if (c->binaryFile)
					fwrite(&byte, 1, 1, c->binaryFile);
			}

			if (c->renderFile)
				c->machine->RenderByte(c->renderFile, byte);

			checksum += (unsigned char)byte;
		}
		if (checksum!=0)
		{
			printf("\nCheck sum error: %.2x\n\n", checksum);
			return 7;
		}

		blockAddr += iBytesThisBlock;

		printf("\n");
	}

	printf("\n\n[eof]\n");

	if (c->renderFile)
		c->renderFile->Flush();

	return 0;
}

void CMachineTypeMicrobee::PrepareWaveMetrics(CCommandStd* c, CTapeReader* wf)
{
	if (c->autoAnalyze)
	{
		WAVE_INFO info;

		fprintf(stderr, "\n\nAnalysing wave data...");
		AnalyseWave(wf->GetWaveReader(), c->_cycleDetector.GetMode(), 0, 0, info);
		fprintf(stderr, "\n\n");

		wf->SetCycleLengths(info.medianShortCycleLength, info.medianLongCycleLength);
	}
	else
	{
		wf->SetShortCycleFrequency(2400);
	}
}

bool CMachineTypeMicrobee::InitWaveWriterProfiled(CWaveWriterProfiled* w)
{
	// Setup cycle lengths
	w->SetCycleKindLength('L', double(w->GetSampleRate()) / 1200.0);
	w->SetCycleKindLength('S', double(w->GetSampleRate()) / 2400.0);

	// Setup bit lengths
	w->SetBitLength(1, 4 * double(w->GetSampleRate()) / 1200.0);
	w->SetBitLength(2, 2 * double(w->GetSampleRate()) / 1200.0);
	w->SetBitLength(4, 1 * double(w->GetSampleRate()) / 1200.0);

	/*
	w->SetBitLength(1, 0, 8 * double(w->GetSampleRate()) / 2400.0);
	w->SetBitLength(1, 1, 4 * double(w->GetSampleRate()) / 1200.0);

	w->SetBitLength(2, 0, 4 * double(w->GetSampleRate()) / 2400.0);
	w->SetBitLength(2, 1, 2 * double(w->GetSampleRate()) / 1200.0);
	
	w->SetBitLength(4, 0, 2 * double(w->GetSampleRate()) / 2400.0);
	w->SetBitLength(4, 1, 1 * double(w->GetSampleRate()) / 1200.0);
	*/

	return true;
}

/*
int CMachineTypeMicrobee::CycleFrequency()
{
	return 2400;
}

int CMachineTypeMicrobee::DcOffset(CTapeReader* wave)
{
	return 0;
}
*/