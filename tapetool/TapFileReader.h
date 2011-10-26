//////////////////////////////////////////////////////////////////////////
// TapFileReader.h - declaration of CTapeReader class

#ifndef __TAPFILEREADER_H
#define __TAPFILEREADER_H

#include "BinaryReader.h"

// CTapFileReader - reads data from a previously generated text file
class CTapFileReader : public CBinaryReader
{
public:
			CTapFileReader(CCommandStd* cmd);
	virtual ~CTapFileReader();

	virtual bool Open(const char* filename, Resolution res);

	virtual const char* GetDataFormat();
};


#endif	// __TAPFILEREADER_H

