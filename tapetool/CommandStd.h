//////////////////////////////////////////////////////////////////////////
// Command.h - declaration of CTapeReader class

#ifndef __COMMANDSTD_H
#define __COMMANDSTD_H

#include "CommandWithInputWaveFile.h"

class CMachineType;
class CFileReader;
class CWaveWriter;
enum CycleMode;
enum Resolution;

class CCommandStd : public CCommandWithInputWaveFile
{
public:
			CCommandStd(CContext* ctx);
	virtual ~CCommandStd();

	virtual int AddSwitch(const char* arg, const char* val);
	virtual int AddFile(const char* filename);



// Command line arguments
	const char* _inputFileName;
	const char* _outputFileName;
	bool showSyncData;
	int perLine;
	bool showPositionInfo;
	bool allowBadCycles;
	double leadingSilence;
	int leadingZeros;
	bool autoAnalyze;
	int renderSampleRate;
	int renderSampleSize;
	int renderVolume;
	int renderBaud;
	bool renderSine;
	const char* cycle_freq;
	CMachineType* machine;
	int byteWrapIndex;
	const char* outputExtension;
	const char* inputFormat;
	bool instrument;
	const char* profileFileName;
	int speedChangePos;
	int speedChangeSpeed;
	bool _includeProfiledLeadIn;
	bool _includeProfiledLeadOut;
	CContext* _ctx;


// The input and output files
	CFileReader* file;
	CWaveWriter* renderFile;
	FILE* binaryFile;


// Operations
	bool OpenFiles(Resolution res);
	void CloseFiles();
	const char * GetInputFormat();
	bool IsOutputKind(const char* ext);
	void ResetByteDump();
	void DumpByte(int byte);

	virtual bool DoesTranslateFromWaveData() { return true; }
	virtual bool DoesTranslateToWaveData() { return true; }
	virtual bool UsesAutoOutputFile() { return true; }

	virtual int PreProcess();
	virtual int PostProcess();

	void ShowCommonUsage();

protected:
	bool OpenInputFile(Resolution res);
	bool OpenOutputFile(Resolution res);
	bool OpenRenderFile(const char* filename);
	bool OpenBinaryFile(const char* filename);
};


#endif	// __COMMANDSTD_H

