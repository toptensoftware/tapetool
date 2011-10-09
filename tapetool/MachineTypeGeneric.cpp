//////////////////////////////////////////////////////////////////////////
// MachineType.cpp - implementation of CMachineType class

#include "precomp.h"

#include "MachineTypeGeneric.h"
#include "CommandContext.h"

CMachineTypeGeneric::CMachineTypeGeneric()
{
}

CMachineTypeGeneric::~CMachineTypeGeneric()
{
}

bool CMachineTypeGeneric::OnPreProcess(CCommandContext* c, Resolution resProcess)
{
	// Check the input file doesn't have a resolution finer than 1 bit
	if (resProcess < resBytes)
	{
		fprintf(stderr, "You must specify a machine type when working with audio or bit resolution data\n");
		return false;
	}
	if (c->renderFile!=NULL)
	{
		fprintf(stderr, "You must specify a machine type when rendering audio tape data\n");
		return false;
	}
	return true;
}
