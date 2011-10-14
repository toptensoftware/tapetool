//////////////////////////////////////////////////////////////////////////
// Context.h - declaration of CWaveReader class

#ifndef __CONTEXT_H
#define __CONTEXT_H

#include "FileReader.h"
#include "WaveReader.h"
#include "MachineType.h"
#include "WaveWriter.h"
#include "CycleDetector.h"

class CCommand;
class CContext;

class CContext
{
public:
// Construction
			CContext();
	virtual ~CContext();

// Command line arguments
	char* files[10];
	int file_count;
	int smoothing;
	bool showSyncData;
	int perLine;
	bool showPositionInfo;
	int from;
	int samples;
	bool showZeroCrossings;
	bool allowBadCycles;
	double leadingSilence;
	int leadingZeros;
	bool autoAnalyze;
	int renderSampleRate;
	int renderSampleSize;
	int renderVolume;
	int renderBaud;
	bool renderSine;
	char* dc_offset;
	char* cycle_freq;
	double amplify;
	bool norm_cycles;
	CMachineType* machine;
	int byteWrapIndex;
	const char* outputExtension;
	const char* inputFormat;
	CycleMode cycleMode;
	int speedChangePos;
	int speedChangeSpeed;

	CCommand* cmd;

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

	void ShowLogo();
	void ShowUsage();


protected:
	bool OpenInputFile(Resolution res);
	bool OpenOutputFile(Resolution res);
	bool OpenRenderFile(const char* filename);
	bool OpenBinaryFile(const char* filename);
};

#endif	// __CONTEXT_H

