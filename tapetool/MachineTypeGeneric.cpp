//////////////////////////////////////////////////////////////////////////
// MachineType.cpp - implementation of CMachineType class

#include "precomp.h"

#include "MachineTypeGeneric.h"
#include "Context.h"

int ProcessWaveStats(CContext* c);

CMachineTypeGeneric::CMachineTypeGeneric()
{
}

CMachineTypeGeneric::~CMachineTypeGeneric()
{
}

bool CMachineTypeGeneric::OnPreProcess(CContext* c, Resolution resProcess)
{
	// Check the input file doesn't have a resolution finer than 1 bit
	if (resProcess < resBytes && c->cmd->DoesTranslateFromWaveData())
	{
		fprintf(stderr, "You must specify a machine type when working with audio or bit resolution data\n");
		return false;
	}
	if (c->renderFile!=NULL && c->cmd->DoesTranslateToWaveData())
	{
		fprintf(stderr, "You must specify a machine type when rendering audio tape data\n");
		return false;
	}
	return true;
}
