//////////////////////////////////////////////////////////////////////////
// Context.cpp - implementation of CContext class

#include "precomp.h"

#include "Context.h"
#include "WaveReader.h"
#include "TextReader.h"
#include "BinaryReader.h"
#include "WaveWriter.h"
#include "MachineType.h"
#include "MachineTypeGeneric.h"
#include "MachineTypeMicrobee.h"
#include "MachineTypeTrs80.h"

#include "CommandWaveStats.h"
#include "CommandSamples.h"
#include "CommandFilter.h"
#include "CommandCycles.h"
#include "CommandCycleKinds.h"
#include "CommandBits.h"
#include "CommandBytes.h"
#include "CommandBlocks.h"

#define VER_MAJOR	0
#define VER_MINOR	3

// Constructor
CContext::CContext()
{
	file_count = 0;
	smoothing = 0;
	showSyncData = false;
	perLine = 0;
	showPositionInfo = true;
	from = 0;
	samples = 0;
	showZeroCrossings = false;
	allowBadCycles = false;
	leadingSilence = 2.0;
	leadingZeros = 0;
	autoAnalyze = true;
	renderSampleRate = 24000;
	renderSampleSize = 8;
	renderVolume = 10;
	renderBaud = 300;
	renderSine = false;
	dc_offset = 0;
	cycle_freq = 0;
	inputFormat = NULL;
	amplify = 1.0;
	speedChangePos= 0x7FFFFFFF;
	speedChangeSpeed = 0;

	byteWrapIndex = 0;
	cmd = NULL;
	machine = NULL;
	file = NULL;
	renderFile = NULL;
	binaryFile = NULL;

	cycleMode = cmZeroCrossingUp;
}

CContext::~CContext()
{

}

bool CContext::OpenFiles(Resolution res)
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

	// Now prepare the input file
	file->Prepare();

	// Display the input format
	printf("[format:%s]\n", GetInputFormat());

	// Ready
	return true;
}


// Opens the specified input file, could be a wav or txt file
bool CContext::OpenInputFile(Resolution res)
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

bool CContext::OpenOutputFile(Resolution res)
{
	if (!cmd->UsesAutoOutputFile())
		return true;

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
bool CContext::OpenRenderFile(const char* filename)
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
	renderFile->RenderSilence((int)(leadingSilence * renderFile->SampleRate()));

	// Render additional leading zeros
	for (int i=0; i<leadingZeros; i++)
	{
		machine->RenderByte(renderFile, 0);
	}

	return true;
}

// Create the binary file
bool CContext::OpenBinaryFile(const char* filename)
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
void CContext::CloseFiles()
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

bool CContext::IsOutputKind(const char* ext)
{
	return _strcmpi(outputExtension, ext)==0;
}


void CContext::ResetByteDump()
{
	byteWrapIndex = 0;
}

void CContext::DumpByte(int byte)
{
	if ((byteWrapIndex!=0 && ((byteWrapIndex % (perLine==0 ? 16 : perLine))==0)))
	{
		printf("\n");
	}
	printf("0x%.2x ", byte);
	byteWrapIndex++;
}


int CContext::Run(int argc,char **argv)
{
	// Process command line arguments
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

			if (_strcmpi(arg, "noanalyze")==0)
			{
				autoAnalyze = false;
			}
			else if (_strcmpi(arg, "wavestats")==0)
			{
				autoAnalyze = false;
				cmd = new CCommandWaveStats(this);

				if (val!=NULL)
				{
					from = atoi(val);
				}
			}
			else if (_strcmpi(arg, "samples")==0)
			{
				cmd = new CCommandSamples(this);

				if (val!=NULL)
				{
					from = atoi(val);
					if (samples==0)
						samples=200;
				}
			}
			else if (_strcmpi(arg, "filter")==0)
			{
				cmd = new CCommandFilter(this);

				if (val!=NULL)
				{
					from = atoi(val);
					if (samples==0)
						samples=200;
				}
			}
			else if (_strcmpi(arg, "cycles")==0)
			{
				cmd = new CCommandCycles(this);
			}
			else if (_strcmpi(arg, "cyclekinds")==0)
			{
				cmd = new CCommandCycleKinds(this);
			}
			else if (_strcmpi(arg, "bits")==0)
			{
				cmd = new CCommandBits(this);
			}
			else if (_strcmpi(arg, "bytes")==0)
			{
				cmd = new CCommandBytes(this);
			}
			else if (_strcmpi(arg, "blocks")==0)
			{
				cmd = new CCommandBlocks(this);
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
				perLine = val==NULL ? 0 : atoi(val);
			}
			else if (_strcmpi(arg, "noposinfo")==0)
			{
				showPositionInfo = false;
			}
			else if (_strcmpi(arg, "showcycles")==0)
			{
				showZeroCrossings = true;
			}
			else if (_strcmpi(arg, "allowbadcycles")==0)
			{
				allowBadCycles = true;
			}
			else if (_strcmpi(arg, "leadingsilence")==0)
			{
				leadingSilence = val==NULL ? 0 : atof(val);
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
				renderBaud = val==NULL ? 0 : atoi(val);
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
				dc_offset = val;
			}
			else if (_strcmpi(arg, "cyclefreq")==0)
			{
				cycle_freq = val;
			}
			else if (_strcmpi(arg, "amplify")==0)
			{
				amplify = val==NULL ? 1.0 : (atof(val)/100);
			}
			else if (_strcmpi(arg, "cyclemode")==0)
			{
				if (!CCycleDetector::FromString(val, cycleMode))
				{
					fprintf(stderr, "Invalid cycle mode: `%s`\n",  val);
					return 7;
				}
			}
			else if (_strcmpi(arg, "normcycles")==0)
			{
				norm_cycles = true;
			}
			else if (_strcmpi(arg, "speedchangepos")==0)
			{
				speedChangePos= val==NULL ? 0 : atoi(val);
			}
			else if (_strcmpi(arg, "speedchangespeed")==0)
			{
				speedChangeSpeed= val==NULL ? 0 : atoi(val);
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
			else if (_strcmpi(arg, "help")==0 || _strcmpi(arg, "?")==0)
			{
				ShowLogo();
				ShowUsage();
				return 0;
			}
			else if (_strcmpi(arg, "version")==0)
			{
				ShowLogo();
				return 0;
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
		cmd = new CCommandBlocks(this);
	}

	// Run the command
	if (cmd!=NULL)
	{
		int retv = cmd->Process();
		CloseFiles();
		return retv;
	}

	ShowLogo();
	ShowUsage();
	return 0;
}


void CContext::ShowLogo()
{
	// Show some help
	printf("tapetool v%i.%i - Microbee/TRS-80 Tape Diagnotic Utility\n", VER_MAJOR, VER_MINOR);
	printf("Copyright (C) 2011 Topten Software.\n");
}

void CContext::ShowUsage()
{
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

	printf("\nGeneral:\n");
	printf("  --version             Show version number\n");
	printf("  --help                Show these usage instructions\n");

	printf("\nMachine Type:\n");
	printf("  --trs80               TRS-80 mode\n");
	printf("  --microbee            Microbee mode\n");
	printf("  --inputformat:fmt     Set the input file format (eg: cas, tap, bin etc...)\n");


	printf("\nWhat to Output:\n");
	printf("  --wavestats[:from]    display various stats about the wave file\n");
	printf("  --filter[:from]       process wave samples to a new wave file applying --smooth, --dcoffset and --amplify\n");
	printf("  --samples[:from]      process wave samples (optionally starting at sample number <from>)\n");
	printf("  --cycles              process wave cycle lengths\n");
	printf("  --cyclekinds          process wave cycle kinds (long/short)\n");
	printf("  --bits                process raw bit data\n");
	printf("  --bytes               process raw byte data\n");
	printf("  --blocks              process data blocks (default)\n");

	printf("\nWave Input Options:\n");
	printf("  --smooth[:N]          smooth waveform with a moving average of N samples (N=3 if not specified)\n");
	printf("  --noanalyze           determine cycle length by analysis (don't trust sample rate)\n");
	printf("  --allowbadcycles      dont limit check cycle lengths (within reason)\n");
	printf("  --dcoffset:N          offset sample values by this amount\n");
	printf("  --amplify:N           amplify input signal by N%\n");
	printf("  --cyclefreq:N         explicitly set the short cycle frequency\n");
	printf("  --speedchangepos:N    specify an explicit speed change at N\n");
	printf("  --speedchangespeed:N  specify the new speed (in baud) at the speed change point\n");
	printf("  --cyclemode:mode      cycle detection mode\n");                 
	printf("                             'zc+' = zero crossing upwards\n");
	printf("                             'zc-' = zero crossing downwards\n");
	printf("                             'max' = local maximum\n");
	printf("                             'min' = local minimum\n");
	printf("                             'max+' = local maximum with positive sample value\n");
	printf("                             'max-' = local minimum with negative sample value\n");

	printf("\nText Output Formatting:\n");
	printf("  --syncinfo            show details of bit and byte sync operations\n");
	printf("  --perline:N           display N piece of data per line (default depends on data kind)\n");
	printf("  --noposinfo           don't dump position info\n");
	printf("  --showcycles          show cycle boundaries with --samples\n");
	printf("  --samplecount         number of samples to dump with --samples\n");

	printf("\nWave Output Options:\n");
	printf("  --leadingsilence:N    render N seconds of leading silence (default = 2.0)\n");
	printf("  --leadingzeros:N      render an additional N leading zeros\n");
	printf("  --samplerate:N        render using sample rate of N (default = 24000)\n");
	printf("  --samplesize:N        render using 8 or 16 bit samples (default = 8)\n");
	printf("  --volume:N            render volume percent (default = 10%)\n");
	printf("  --baud:N              render baud rate\n");
	printf("  --sine                render using sine (instead of square) waves\n");
	printf("\n");
}