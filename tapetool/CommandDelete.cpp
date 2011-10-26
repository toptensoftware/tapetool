//////////////////////////////////////////////////////////////////////////
// ProcessXXX.cpp

#include "precomp.h"

#include "Context.h"
#include "CommandDelete.h"

// Constructor
CCommandDelete::CCommandDelete()
{
	_outputFileName = NULL;
}

// Command handler for dumping samples
int CCommandDelete::Process()
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
	while (wave.HaveSample())
	{
		if (wave.CurrentPosition()<GetStartSample() && wave.CurrentPosition()>=GetEndSample())
			writer.RenderSample(wave.CurrentSample());
		wave.NextSample();
	}

	writer.Close();
	fprintf(stderr, "\n\n");
	return 0;
}


int CCommandDelete::AddFile(const char* filename)
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

void CCommandDelete::ShowUsage()
{
	printf("\nUsage: tapetool delete [options] inputWaveFile outputWaveFile\n");

	printf("\nDeletes a range of samples from a wave file.\n");

	printf("\nOptions:\n");
	printf("  --help                Show these usage instructions\n");
	CCommandWithRangedInputWaveFile::ShowHelp();
	printf("\n\n");
}
