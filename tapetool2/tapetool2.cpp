/*
 * TAPETOOL2
 * Utility for working with microbee tape recordings
 *
 * Copyright (c) 2011 Topten Software (Brad Robinson)
 *
 * Based loosely on TAPETOOL2 utility:
 *
 * Copyright (c) 2010 by Martin D. J. Rosenau
 *
 * This file is provided as freeware under the GPL 2.0 license
 *
 * The file is assumed to be compiled an run on a little-endian
 * machine (such as an i386 compatible)
 */

#include "precomp.h"
#include "Context.h"

// Main
int main(int argc,char **argv)
{
	CContext ctx;
	return ctx.Run(argc, argv);
}
