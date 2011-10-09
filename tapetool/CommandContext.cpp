//////////////////////////////////////////////////////////////////////////
// CommandContext.cpp - implementation of CCommandContext class

#include "precomp.h"

#include "CommandContext.h"
#include "WaveReader.h"
#include "TextReader.h"
#include "BinaryReader.h"
#include "WaveWriter.h"
#include "MachineType.h"
#include "MachineTypeGeneric.h"
#include "MachineTypeMicrobee.h"
#include "MachineTypeTrs80.h"

// External functions see ProcessXXX.cpp
int ProcessSamples(CCommandContext* c);
int ProcessCycles(CCommandContext* c);
int ProcessCycleKinds(CCommandContext* c);
int ProcessBits(CCommandContext* c);
int ProcessBytes(CCommandContext* c);
int ProcessBlocks(CCommandContext* c);


// Constructor
CCommandContext::CCommandContext()
{
	file_count = 0;
	smoothing = 0;
	showSyncData = false;
	perLineMode = false;
	showPositionInfo = true;
	from = 0;
	samples = 0;
	showZeroCrossings = false;
	allowBadCycles = false;
	leadingSilence = 2;
	leadingZeros = 0;
	analyzeCycles = false;
	renderSampleRate = 24000;
	renderSampleSize = 8;
	renderVolume = 10;
	renderBaud = 300;
	renderSine = false;
	dc_offset = 0;
	cycle_freq = 0;
	phaseShift = false;
	inputFormat = NULL;

	byteWrapIndex = 0;

	machine = NULL;
	file = NULL;
	renderFile = NULL;
	binaryFile = NULL;
}

CCommandContext::~CCommandContext()
{

}

bool CCommandContext::OpenFiles(Resolution res)
{
	if (file_count==0)
	{
		fprintf(stderr, "command requires at least one input file");
		return false;
	}

	// Open the input file
	if (!OpenInputFile(res))
		return false;

	// Open the output file
	if (!OpenOutputFile(res))
		return false;

	// Work out the lowest level resolution we're working at
	Resolution resInput = file->GetResolution();
	if (resInput < res)
		res = resInput;

	// Give the machine type a chance to abort
	if (!machine->OnPreProcess(this, res))
		return false;

	// Display the input format
	printf("[format:%s]", GetInputFormat());

	// Ready
	return true;
}


// Opens the specified input file, could be a wav or txt file
bool CCommandContext::OpenInputFile(Resolution res)
{
	char* ext = strrchr(files[0], '.');

	// Give the machine type the first chance at creating the input reader
	file = machine->CreateFileReader(this, ext);
	if (file==NULL)
	{
		if (ext!=NULL && _stricmp(ext, ".wav")==0)
		{
			file = new CWaveReader(this); 
		}
		else if (ext!=NULL && _stricmp(ext, ".txt")==0)
		{
			file = new CTextReader(this);
		}
		else
		{
			file = new CBinaryReader(this);
		}
	}

	return file->Open(files[0], res);
}

bool CCommandContext::OpenOutputFile(Resolution res)
{
	// Output file?
	if (file_count<2)
	{
		outputExtension="";
		return true;
	}

	// Redirect stdout to the text file
	char* ext = strrchr(files[1], '.');
	if (ext==NULL)
	{
		outputExtension="";
		fprintf(stderr, "Can't determine output file type - no file extension specified\n");
		return false;
	}

	// Store it
	outputExtension = ext+1;

	// Text file?
	if (_stricmp(ext, ".txt")==0)
	{
		if (!freopen(files[1], "wt", stdout))
		{
			fprintf(stderr, "Can't create output file '%s'\n", files[1]);
			return false;
		}

		return true;
	}

	// Wave file?
	if (_stricmp(ext, ".wav")==0)
	{
		return OpenRenderFile(files[1]);
	}

	// Binary file?
	return OpenBinaryFile(files[1]);
}

// Create the rendering file
bool CCommandContext::OpenRenderFile(const char* filename)
{
	// Create the render file
	renderFile = new CWaveWriter();
	if (!renderFile->Create(filename, renderSampleRate, renderSampleSize))
	{
		renderFile->Close();
		return false;
	}

	renderFile->SetVolume(renderVolume);

	if (machine->CanRenderSquare())
	{
		renderFile->SetSquare(!renderSine);
	}

	// Render leading silence
	renderFile->RenderSilence(renderFile->SampleRate() * leadingSilence);

	// Render additional leading zeros
	for (int i=0; i<leadingZeros; i++)
	{
		machine->RenderByte(renderFile, 0);
	}

	return true;
}

// Create the binary file
bool CCommandContext::OpenBinaryFile(const char* filename)
{
	binaryFile = fopen(filename, "wb");
	if (binaryFile==NULL)
	{									  
	    fprintf(stderr, "Could not create %s\n", filename);
		return false;
	}

	return true;
}

		
// Close any open files
void CCommandContext::CloseFiles()
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

bool CCommandContext::IsOutputKind(const char* ext)
{
	return _strcmpi(outputExtension, ext)==0;
}


void CCommandContext::ResetByteDump()
{
	byteWrapIndex = 0;
}

void CCommandContext::DumpByte(int byte)
{
	if ((byteWrapIndex!=0 && ((byteWrapIndex % 16)==0) || perLineMode))
	{
		printf("\n");
	}
	printf("0x%.2x ", byte);
	byteWrapIndex++;
}


int CCommandContext::Run(int argc,char **argv)
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
				cmd = ProcessSamples;

				if (val!=NULL)
				{
					from = atoi(val);
					if (samples==0)
						samples=200;
				}
			}
			else if (_strcmpi(arg, "cycles")==0)
			{
				cmd = ProcessCycles;
			}
			else if (_strcmpi(arg, "cyclekinds")==0)
			{
				cmd = ProcessCycleKinds;
			}
			else if (_strcmpi(arg, "bits")==0)
			{
				cmd = ProcessBits;
			}
			else if (_strcmpi(arg, "bytes")==0)
			{
				cmd = ProcessBytes;
			}
			else if (_strcmpi(arg, "blocks")==0)
			{
				cmd = ProcessBlocks;
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
				cycle_freq = val==NULL ? 0 : atoi(val);
			}
			else if (_strcmpi(arg, "trs80")==0)
			{
				machine = new CMachineTypeTrs80();
			}
			else if (_strcmpi(arg, "microbee")==0)
			{
				machine = new CMachineTypeMicrobee();
			}
			else if (_strcmpi(arg, "sine")==0)
			{
				renderSine = true;
			}
			else if (_strcmpi(arg, "inputformat")==0)
			{
				inputFormat	= val;
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

	if (machine==NULL)
	{
		machine = new CMachineTypeGeneric();
	}

	// Default command is blocks (if a file is specified)
	if (cmd==NULL && file_count>0)
	{
		cmd = ProcessBlocks;
	}

	// Run the command
	if (cmd!=NULL)
	{
		int retv = cmd(this);
		CloseFiles();
		return retv;
	}

	// Show some help
	printf("tapetool - Microbee Tape Diagnotic Utility\n");
	printf("Copyright (C) 2011 Topten Software.\n");

	printf("\nUsage: tapetool [options] inputFile [outputFile]\n\n");

	printf("The input file can be either:\n");
	printf("  - an 8 or 16 bit PCM mono .wav file, or \n");
	printf("  - a .txt file containing the (possibly edited) previously output of this program\n");

	printf("The type of output is determined by the outputFile extension:\n");
	printf("  - .wav - renders an new audio tape recording\n");
	printf("  - .txt - redirects the normal text output to a text file\n");
	printf("  - .tap - a Microbee cassette file\n");
	printf("  - .bee - a Microbee BEE file\n");
	printf("  - .cas - generates a TRS-80 cassette file\n");
	printf("  - anything else - generates a raw binary containing the input data\n");

	printf("\nMachine Type:\n");
	printf("  --trs80               TRS-80 mode\n");
	printf("  --microbee            Microbee mode\n");
	printf("  --inputformat:fmt		Set the input file format (eg: cas, tap, bin etc...)\n");


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
	printf("  --cyclefreq:[N]       explicitly set the short cycle frequency\n");
	printf("  --phaseshift          skip the first half-cycle causing a 180deg phase shift\n");

	printf("\nText Output Formatting:\n");
	printf("  --syncinfo            show details of bit and byte sync operations\n");
	printf("  --perline             display one piece of data per line\n");
	printf("  --noposinfo           don't dump position info\n");
	printf("  --zc                  start a new line at zero crossings with --samples\n");
	printf("  --samplecount         number of samples to dump with --samples\n");

	printf("\nWave Output Options\n");
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
