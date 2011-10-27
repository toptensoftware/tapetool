//////////////////////////////////////////////////////////////////////////
// Command.cpp - implementation of CCommand class

#include "precomp.h"

#include "Command.h"
#include "CommandStd.h"
#include "CycleDetector.h"
#include "FileReader.h"
#include "MachineType.h"
#include "MachineTypeMicrobee.h"
#include "MachineTypeTrs80.h"
#include "MachineTypeGeneric.h"

#include "TapeReader.h"
#include "TextReader.h"
#include "BinaryReader.h"
#include "WaveWriter.h"
#include "WaveWriterProfiled.h"

// Standard command
CCommandStd::CCommandStd(CContext* ctx)
{
	_ctx = ctx;
	_inputFileName = NULL;
	_outputFileName = NULL;
	showSyncData = false;
	perLine = 0;
	showPositionInfo = true;
	allowBadCycles = false;
	leadingSilence = 2.0;
	leadingZeros = 0;
	autoAnalyze = true;
	renderSampleRate = 24000;
	renderSampleSize = 8;
	renderVolume = 10;
	renderBaud = 300;
	renderSine = false;
	cycle_freq = 0;
	inputFormat = NULL;
	instrument = false;
	profileFileName = NULL;
	speedChangePos= 0x7FFFFFFF;
	speedChangeSpeed = 0;
	byteWrapIndex = 0;
	machine = NULL;
	file = NULL;
	renderFile = NULL;
	binaryFile = NULL;
	_includeProfiledLeadIn = true;
	_includeProfiledLeadOut = true;
	_strict = false;
}

CCommandStd::~CCommandStd()
{

}


int CCommandStd::AddSwitch(const char* arg, const char* val)
{
	if (_strcmpi(arg, "noanalyze")==0)
	{
		autoAnalyze = false;
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
	else if (_strcmpi(arg, "cyclefreq")==0)
	{
		cycle_freq = val;
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
	else if (_strcmpi(arg, "createprofile")==0)
	{
		instrument = true;
	}
	else if (_strcmpi(arg, "useprofile")==0)
	{
		profileFileName = val;
	}
	else if (_strcmpi(arg, "no-profiled-leadin")==0)
	{
		_includeProfiledLeadIn = false;
	}
	else if (_strcmpi(arg, "no-profiled-leadout")==0)
	{
		_includeProfiledLeadOut = false;
	}
	else if (_strcmpi(arg, "strict")==0)
	{
		_strict = true;
	}
	else
	{
		return CCommandWithInputWaveFile::AddSwitch(arg, val);
	}

	return 0;
}

int CCommandStd::AddFile(const char* filename)
{
	if (_inputFileName==NULL)
	{
		_inputFileName=filename;
		return 0;
	}

	if (_outputFileName==NULL)
	{
		_outputFileName=filename;
		return 0;
	}

	fprintf(stderr, "Too many file names supplied, aborting");
	return 7;
}



bool CCommandStd::OpenFiles(Resolution res)
{
	if (_inputFileName==NULL)
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
bool CCommandStd::OpenInputFile(Resolution res)
{
	const char* ext = strrchr(_inputFileName, '.');

	// Give the machine type the first chance at creating the input reader
	file = machine->CreateFileReader(this, ext);
	if (file==NULL)
	{
		if (ext!=NULL && _stricmp(ext, ".wav")==0)
		{
			file = new CTapeReader(this); 
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

	return file->Open(_inputFileName, res);
}

bool CCommandStd::OpenOutputFile(Resolution res)
{
	if (!UsesAutoOutputFile())
		return true;

	// Output file?
	if (_outputFileName==NULL)
	{
		outputExtension="";
		return true;
	}

	// Redirect stdout to the text file
	const char* ext = strrchr(_outputFileName, '.');
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
		// Use base implementation for stdout redirection
		CCommand::AddFile(_outputFileName);
		return true;
	}

	// Wave file?
	if (_stricmp(ext, ".wav")==0)
	{
		return OpenRenderFile(_outputFileName);
	}

	// Binary file?
	return OpenBinaryFile(_outputFileName);
}

// Create the rendering file
bool CCommandStd::OpenRenderFile(const char* filename)
{
	if (profileFileName!=NULL)
	{
		CWaveWriterProfiled* profiled= new CWaveWriterProfiled();
		profiled->IncludeLeadIn = _includeProfiledLeadIn;
		profiled->IncludeLeadOut = _includeProfiledLeadOut;
		if (!profiled->Create(filename, profileFileName))
		{
			profiled->Close();
			return false;
		}
		renderFile = profiled;
	}
	else
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
	}

	return true;
}

// Create the binary file
bool CCommandStd::OpenBinaryFile(const char* filename)
{
	binaryFile = fopen(filename, "wb");
	if (binaryFile==NULL)
	{									  
	    fprintf(stderr, "Could not create '%s' - %s (%i)\n", filename, strerror(errno), errno);
		return false;
	}

	return true;
}

		
// Close any open files
void CCommandStd::CloseFiles()
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

const char * CCommandStd::GetInputFormat()
{ 
	return inputFormat==NULL ? file->GetDataFormat() : inputFormat; 
}

bool CCommandStd::IsOutputKind(const char* ext)
{
	return _strcmpi(outputExtension, ext)==0;
}


void CCommandStd::ResetByteDump()
{
	byteWrapIndex = 0;
}

void CCommandStd::DumpByte(int byte)
{
	if ((byteWrapIndex!=0 && ((byteWrapIndex % (perLine==0 ? 16 : perLine))==0)))
	{
		printf("\n");
	}
	printf("0x%.2x ", byte);
	byteWrapIndex++;
}

int CCommandStd::PreProcess()
{
	if (machine==NULL)
	{
		machine = new CMachineTypeGeneric();
	}
	return 0;
}

int CCommandStd::PostProcess()
{
	CloseFiles();
	return 0;
}


void CCommandStd::ShowCommonUsage()
{
	printf("\nThe input file can be either:\n");
	printf("  - an 8 or 16 bit PCM mono .wav file, or \n");
	printf("  - a .txt file containing the (possibly edited) previously output of this program\n");
	printf("  - a binary file containing data to be processed\n");

	printf("\nThe type of output is determined by the outputFile extension:\n");
	printf("  - .wav - renders an new audio tape recording\n");
	printf("  - .txt - redirects the normal text output to a text file\n");
	printf("  - .tap - a Microbee cassette file\n");
	printf("  - .bee - a Microbee BEE file\n");
	printf("  - .cas - generates a TRS-80 cassette file\n");
	printf("  - anything else - generates a raw binary containing the transformed input data\n");

	printf("\nMachine Type:\n");
	printf("  --trs80               TRS-80 mode\n");
	printf("  --microbee            Microbee mode\n");
	printf("  --inputformat:fmt     Set the input file format (eg: cas, tap, bin etc...)\n");

	printf("\nWave Input Options:\n");
	CCommandWithInputWaveFile::ShowHelp();
	printf("  --noanalyze           determine cycle length by analysis (don't trust sample rate)\n");
	printf("  --allowbadcycles      don't limit check cycle lengths (within reason)\n");
	printf("  --strict              strictly convert cycle patterns to bits\n");
	printf("  --cyclefreq:N         explicitly set the short cycle frequency\n");
	printf("  --speedchangepos:N    specify an explicit speed change at N\n");
	printf("  --speedchangespeed:N  specify the new speed (in baud) at the speed change point\n");
	printf("  --createprofile       create instrumentation file for subsequent wave file repair\n");

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
	printf("  --useprofile:wavefile render using samples from specified wave file (which must be first instrumented)\n");
	printf("  --no-profiled-leadin  when doing profiled rendered, don't include the lead-in noise\n");
	printf("  --no-profiled-leadout when doing profiled rendered, don't include the lead-out noise\n");
	printf("\n");

}

