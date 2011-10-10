/*
 * TAPETOOL
 * Utility for working with microbee tape recordings
 *
 * Copyright (c) 2011 Topten Software (Brad Robinson)
 *
 * Based loosely on TAPETOOL utility:
 *
 * Copyright (c) 2010 by Martin D. J. Rosenau
 *
 * This file is provided as freeware under the GPL 2.0 license
 *
 * The file is assumed to be compiled an run on a little-endian
 * machine (such as an i386 compatible)
 */

#include "precomp.h"
#include "FileReader.h"
#include "WaveReader.h"
#include "TextReader.h"
#include "WaveWriter.h"
#include "MachineTypeMicrobee.h"
#include "Context.h"



// Main
int main(int argc,char **argv)
{
	CContext ctx;
	return ctx.Run(argc, argv);
}



/*
SYSTEM TAPE FORMAT
TAPE LEADER             256 zeroes followed by a A5 (sync byte)
55                      header byte indicating system format
xx xx xx xx xx xx       6 character file name in ASCII

        3C              data header
        xx              length of data 01-FFH, 00=256 bytes
        lsb,msb         load address
        xx ... xx       data (your program)
        xx              checksum of your data & load address

        .               repeat from 3C through checksum
        .
        .
78                      end of file marker
lsb,msb                 entry point address
						

EDITOR/ASSEMBLER SOURCE TAPE FORMAT
TAPE LEADER             256 zeroes followed by a A5 (sync byte)
D3                      header byte indicating source format
xx xx xx xx xx xx       6 character file name in ASCII

        xx xx xx xx xx  line # in ASCII with bit 7 set in each byte
        20              data header
        xx ... xx       source line (128 byte maximum)
        0D              end of line marker

        .
        .
        .

1A                      end of file marker
						

BASIC TAPE FORMAT
LEADER                  256 zeroes followed by an A5 (sync byte)
D3 D3 D3                BASIC header bytes
xx                      1 character file name in ASCII

        lsb,msb         address pointer to next line
        lsb,msb         line #
        xx ... xx       BASIC line (compressed)
        00              end of line marker

        .
        .
        .

00 00                   end of file markers
						
*/