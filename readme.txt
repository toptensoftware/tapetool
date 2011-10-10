# TapeTool - A Microbee Tape Diagnotic and Recovery Utility

## Introduction

TapeTool is a command line utility for processing recordings of Microbee tape files with the
main objective of recovering lost data.  

TapeTool can:

*	Read an 8 or 16 bit mono PCM wave file and convert it to text that can be redirected to a 
	text file.
*	Depending on the quality and damage to the recording, tapetool can output:
		- audio sample values
		- cycle lengths in samples (a cycle is a full 360deg audio wave)
		- cycle kinds (1200Hz or 2400Hz)
		- bits
		- bytes
		- CRC checked and decode data blocks
*	The output text file can be re-read by tape tool - no need to edit the original wave file
*   The output text includes detailed information about where in the original recording the each
	piece of data came from.  ie: actual sample numbers.  This can be used to inspect the exact
	location in the audio file in a wave editor to visually determine the data.
*   The output text can be generated one byte per line without position information - this allows
	two or mode recordings of the same file to be compared and merged using a text diff tool.
*   Synthesize a new audio recording in 300 or 1200 baud.
*   Generate a binary dump of the data ie: .bee and .tap files.
*   Works with Microbee and TRS80

Currently TapeTool only works with 300 baud input files, but it can generate 1200 baud wave files.

## Usage

Usage: 

    > tapetool [options] inputfile [outputfile]

The type of output file is determined by the output file extension 

    > tapetool --blocks --microbee myfile.wav myfile.tap

    > tapetool --bytes --microbee myfile.wav myfile.txt

The input file can be a .wav file, a binary file or the captured text output of a previous run of tapetool. 


## Processing Wave Files

In order to process or render cassette audio files, tapetool needs to know the type of machine the tape is 
intended for.  Currently TRS-80 and Microbee are supported, see the command line arguments below.


## Text Format

The text output of tapetool follows a simple format:

* Anything in [square brackets] is a comment.  Tapetool generates these to show position information 
error details, resync data, header information etc...
* A double slash to end of line is also considered a comment
* On reading a text file, all comments are ignored except a square bracket comment [format:<type>] which
   is used to store the file type of the original file dumped from. (tap, cas, etc...)
* Dumped sample and cycle length data can't be re-read by tape tool.
* Cycle kind data is rendered using the following characters:
	- `S` = a short cycle (2400Hz)
	- `L` = a long cycle (1200Hz)
	- `?` = an ambiguous cycle (somewhere between 1200 and 2400Hz)
	- `<` = a too short cycle (shorter than 2400Hz)
	- `>` = a too long cycle (longer than 1200Hz)
* Bit data is rendered as `0` and `1`
* Byte data is rendered as `0xHH` where HH is the hex value of the byte.
* An input text file can contained mixed cyclekind, bit and byte data.

## Comamnd Line Arguments

### --trs80

Specifies the target machine type as "TRS80".  Only 500 baud system, basic and source files are supported.

### --microbee

Specifies the target machine type as "Microbee".  Only 300 baud input is supported, but 300 and 1200 baud
rendering is supported.

### --inputformat:<fmt>

Set the format of the input data ("cas", "tap", etc...)

### --wavestats:[from]

Dumps various statistics about wave data including amplitude ranges, estimated long and short cycle
lengths etc...

The output of this processing kind can't be re-read by tapetool

### --samples[:from]

Dumps the raw samples of a wave file (can't be used with text input data).  If `from` is specified
dumps sample starting at that sample number.  Use --samplecount to specify how many samples to dump.

The output of this processing kind can't be re-read by tapetool

### --cycles

Dumps the length (in samples) of each cycle in a wave file.

The output of this processing kind can't be re-read by tapetool

### --cyclekinds

Dumps an input file as cycle kinds - short, long, ambiguous etc...

### --bits

Dumps an input file as a series of `1` and `0` bits.

### --bytes

Dumps the raw bytes of an input file, without checking headers, blocks, checksums

### --blocks

The default processing kind, dumps the input file as a series of data blocks and computes
and checks the checksum byte of each block.  A full dump from the command wihtout errors
indicates a successful load.

### --smooth[:N]

Smooths the input sample data using a moving average of period N, or 3 if N is not specified.

Using this option can:

* eliminate erroneous zero crossings caused by noise
* even out cycle lengths 

### --noanalyze

By default, tapetool analyses audio files to automatically determine settings for how to best process
the file. Use this option to prevent this auto analysis.

### --allowbadcycles

Normally an out of range cycle kind `<` or `>` causes processing of bit data to fail.  Use this option
to allow out of range cycle lengths.  TapeTool effective ignores these cycles and relies on the surrounding
cycles to determine the bit.

### --dcoffset

Apply an offset to all samples.  Use with pulse based audio (eg:TRS80) to shift the pulses into the zero
crossing range.  Also useful for shifting noisy silence/hiss out of the zero crossing range.

### --cyclefreq

Explicitly set the short cycle frequency in Hz.  (Default for Microbee is 2400Hz, TRS80 is 1024Hz)

### --phaseshift

Skips one audio cycle at the start of the file, cause the phase of each cycle to be inverted.  Generally this
is not a particularly useful setting however it can help in some circumstances where tapetool sees out of phase
wave cycles that appear as a combination of the long and short frequencies.

### --samplecount:N

Use with --samples to control how many samples to dump.

### --syncinfo

TapeTool uses various heuristice to synchronize itself on bit and byte boundaries.  Use this option
to output the data consumed in performing these synchronization operations.

Synchronization occurs at the start of the file and after any error.

### --perline:N

This option causes data elements (cyclekinds/bits/bytes) to output N per line.  

Use this option to get detailed sample position information, or combine --perline:1 with --noposinfo 
to generate data suitable for comparison with a diff program

### --noposinfo

By default tapetool outputs comments indicating the current position in the input file.  This extra
information however makes it impossible to compare to files using a text diff tool. --noposinfo
suppresses this information.

### --zc

When output raw sample data, inserts a new line at each detected zero crossing, making it easier to 
inspect audio wave form data.


### --leadingsilence:N

Adds N seconds of leading silence to the rendered wave file.  The default is 2 seconds if not specified.

### --leadingzeros:N

Adds N leading zeros to the rendered wave file.  By default, no additional leading zeros are written
to the file.

### --samplerate:N

Sets the sample rate of the rendered wave file.  The default is 24000Hz (because it provides and even 
division for 1200 and 2400Hz cycles).

### --samplesize:[8|16]

Sets the sample size of the rendered wave file.  ie: 8 or 16bit PCM data.

### --volume:N

Set the volume of the rendered wave file (as a percentage).  The default is 10%.

### --baud:[300|1200]

Sets the baud rate of the rendered wave file.  This option only works when processing in --blocks mode and
will automatically set the appropriate speed byte in the DGOS header.

### --sine

Renders the wave as a sine waves instead of square waves.  


## Examples

Dump the blocks from a clean working wave file:

	> tapetool myfile.wav

Dump the raw bytes from a wave file to a text file

	> tapetool --bytes --microbee myfile.wav myfile.bytes.txt

Dump the bits from a wave file, smoothing the audio data over 3 samples and using heuristics
to calculate cycle lengths by analying the wave data rather than relying on the sample rate:

	> tapetool --bits --microbee --smooth:3 --analyze myfile.wav myfile.bits.txt

Convert the output of the previous example into a series of bytes:

	> tapetool --bytes --microbee myfile.bits.txt myfile.bytes.txt

Convert a byte stream into blocks and check the checksums, dump header info etc...

	> tapetool --blocks --microbee myfile.bytes.txt

Render a wave file:

	> tapetool --blocks --microbee myfile.bytes.txt myfile.wave

Create a ubee512 .tap file directly from a wave file:
	
	> tapetool --blocks --microbee myfile.wav myfile.tap

Create a NanoWasp .mac file from text file: (take note of the header info and enter it
into NanoWasp web page where you configure the tape file).

	> tapetool --blocks --microbee myfile.bits.txt myfile.mac

Create an arbitrary format binary file from a text file of bits:

	> tapetool --bytes --microbee myfile.bits.txt myfile.bin

