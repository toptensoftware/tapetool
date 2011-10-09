//////////////////////////////////////////////////////////////////////////
// CommandContext.h - declaration of CWaveReader class

#ifndef __COMMANDCONTEXT_H
#define __COMMANDCONTEXT_H

#include "FileReader.h"
#include "WaveReader.h"
#include "MachineType.h"
#include "WaveWriter.h"

class CCommandContext;


class CCommandContext
{
public:
// Construction
			CCommandContext();
	virtual ~CCommandContext();

// Command line arguments
	char* files[10];
	int file_count;
	int smoothing;
	bool showSyncData;
	bool perLineMode;
	bool showPositionInfo;
	int from;
	int samples;
	bool showZeroCrossings;
	bool allowBadCycles;
	int leadingSilence;
	int leadingZeros;
	bool analyzeCycles;
	int renderSampleRate;
	int renderSampleSize;
	int renderVolume;
	int renderBaud;
	bool renderSine;
	int dc_offset;
	int cycle_freq;
	CMachineType* machine;
	int byteWrapIndex;
	bool phaseShift;
	const char* outputExtension;
	const char* inputFormat;

	fnCmd cmd;

// The input and output files
	CFileReader* file;
	CWaveWriter* renderFile;
	FILE* binaryFile;

	int Run(int argc,char **argv); 

// Operations
	bool OpenFiles(Resolution res);
	void CloseFiles();

	const char * GetInputFormat() { return inputFormat==NULL ? file->GetDataFormat() : inputFormat; }
	bool IsOutputKind(const char* ext);


	void ResetByteDump();
	void DumpByte(int byte);


protected:
	bool OpenInputFile(Resolution res);
	bool OpenOutputFile(Resolution res);
	bool OpenRenderFile(const char* filename);
	bool OpenBinaryFile(const char* filename);
};

#endif	// __COMMANDCONTEXT_H

