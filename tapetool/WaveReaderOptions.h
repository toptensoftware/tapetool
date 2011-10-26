//////////////////////////////////////////////////////////////////////////
// WaveReaderOptions.h - declaration of CWaveReaderOptions

#ifndef __WAVEREADEROPTIONS_H
#define __WAVEREADEROPTIONS_H

class CWaveReaderOptions
{
public:
	CWaveReaderOptions();
	int AddSwitch(const char* arg, const char* val);

	int _from;
	int _samples;
	int _perline;
	int _dcOffset;
	double _amplify;
	int _smoothing;
};

#endif	// __WAVEREADEROPTIONS_H

