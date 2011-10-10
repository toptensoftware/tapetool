//////////////////////////////////////////////////////////////////////////
// TapFileReader.h - declaration of CWaveReader class

#ifndef __TAPFILEREADER_H
#define __TAPFILEREADER_H

#include "BinaryReader.h"

// CTapFileReader - reads data from a previously generated text file
class CTapFileReader : public CBinaryReader
{
public:
			CTapFileReader(CContext* c);
	virtual ~CTapFileReader();

	virtual bool Open(const char* filename, Resolution res);

	virtual const char* GetDataFormat();
};


#endif	// __TAPFILEREADER_H

