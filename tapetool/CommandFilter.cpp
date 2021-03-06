//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandFilter.h"

// Constructor
CCommandFilter::CCommandFilter()
{
	_outputFileName = NULL;
}

// Command handler for dumping samples
int CCommandFilter::Process()
{
	if (_outputFileName==NULL)
	{
		fprintf(stderr, "No input file specified");
		return false;
	}

	// Open and configure wave reader
	CWaveReader wave;
	if (!OpenWaveReader(wave))
		return 7;

	// Open and configure wave writer
	CWaveWriter writer;
	writer.Create(_outputFileName, wave.GetSampleRate(), wave.GetBytesPerSample()*8);

	// Main loop
	fprintf(stderr, "Copying samples...");
	wave.Seek(GetStartSample());
	while (wave.HaveSample() && wave.CurrentPosition() < GetEndSample())
	{
		writer.RenderSample(wave.CurrentSample());
		wave.NextSample();
	}

	writer.Close();
	fprintf(stderr, "\n\n");
	return 0;
}


int CCommandFilter::AddFile(const char* filename)
{
	if (_filename==NULL)
	{
		// First file name is the input file
		CCommandWithRangedInputWaveFile::AddFile(filename);
	}
	else if (_outputFileName==NULL)
	{
		// Second is the output filename
		_outputFileName = filename;
	}
	else
	{
		// Third is text redirection
		return CCommand::AddFile(filename);
	}

	return 0;
}

void CCommandFilter::ShowUsage()
{
	printf("\nUsage: tapetool filter [options] inputWaveFile outputWaveFile\n");

	printf("\nCopies all or some samples from an input wave file to a new output wave\n");
	printf("file, optionally filtering in the process.\n");

	printf("\nOptions:\n");
	printf("  --help                Show these usage instructions\n");
	CCommandWithRangedInputWaveFile::ShowHelp();
	printf("\n\n");
}
