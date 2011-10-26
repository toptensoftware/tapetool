//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandJoin.h"

// Constructor
CCommandJoin::CCommandJoin()
{
	_inputFileName1 = NULL;
	_inputFileName2 = NULL;
	_outputFileName = NULL;
	_gap = 0;
}

// Command handler for dumping samples
int CCommandJoin::Process()
{
	if (_outputFileName==NULL)
	{
		fprintf(stderr, "No input file specified");
		return false;
	}

	// Open file 1
	CWaveReader wave1;
	if (!wave1.OpenFile(_inputFileName1))
		return 7;

	CWaveReader wave2;
	if (!wave2.OpenFile(_inputFileName2))
		return 7;

	if (wave1.GetSampleRate()!=wave2.GetSampleRate())
	{
		fprintf(stderr, "Sample rate mismatch (%i vs %i), aborting", wave1.GetSampleRate(), wave2.GetSampleRate());
		return 7;
	}

	if (wave1.GetBytesPerSample()!=wave2.GetBytesPerSample())
	{
		fprintf(stderr, "Sample size mismatch (%i-bit vs %i-bit), aborting", wave1.GetBytesPerSample()*8, wave2.GetBytesPerSample()*8);
		return 7;
	}

	// Open and configure wave writer
	CWaveWriter writer;
	writer.Create(_outputFileName, wave1.GetSampleRate(), wave1.GetBytesPerSample()*8);

	// File 1
	fprintf(stderr, "Copying samples from '%s'...", _inputFileName1);
	while (wave1.HaveSample())
	{
		writer.RenderSample(wave1.CurrentSample());
		wave1.NextSample();
	}

	// Render the gap
	if (_gap!=0.0)
	{
		fprintf(stderr, "\nInserting Gap...");
		writer.RenderSilence((int)(_gap * wave1.GetSampleRate()));
	}

	// File 2
	fprintf(stderr, "\nCopying samples from '%s'...", _inputFileName2);
	while (wave2.HaveSample())
	{
		writer.RenderSample(wave2.CurrentSample());
		wave2.NextSample();
	}


	writer.Close();
	fprintf(stderr, "\n\n");
	return 0;
}

int CCommandJoin::AddSwitch(const char* arg, const char* val)
{
	if (_strcmpi(arg, "gap")==0)
	{
		_gap = val==NULL ? 0 : atof(val);
	}
	else 
	{
		CCommand::AddSwitch(arg, val);
	}

	return 0;
}



int CCommandJoin::AddFile(const char* filename)
{
	if (_inputFileName1==NULL)
		_inputFileName1 = filename;
	else if (_inputFileName2==NULL)
		_inputFileName2 = filename;
	else if (_outputFileName==NULL)
		_outputFileName = filename;
	else
		return CCommand::AddFile(filename);

	return 0;
}

void CCommandJoin::ShowUsage()
{
	printf("\nUsage: tapetool join [options] inputWaveFile1 inputWaveFile2 outputWaveFile\n");

	printf("\nJoin two wave files into a new wave file.  Both files must be in the same format.\n");

	printf("\nOptions:\n");
	printf("  --help                Show these usage instructions\n");
	printf("  --gap:N               Inserts a N second gap of silence between the two files\n");
	printf("\n\n");
}
