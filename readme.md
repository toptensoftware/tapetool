# TapeTool - A Microbee Tape Diagnotic and Recovery Utility

## Introduction

TapeTool is a command line utility for processing recordings of Microbee tape files with the
main objective of recovering lost data.  

TapeTool is not an automatic data recovery/repair tool - rather it should be considered a toolkit of
useful utilities for data extraction, repair and re-rendering of working audio tape recordings.

Successful use on a damaged recording will generally involve much experimentation.

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
*   Repair an original wave recording by copying good sections over bad.
*   Works with Microbee and TRS80

## Download

Source code:

* <http://github.com/toptensoftware/tapetool>

Windows executable:

* <https://github.com/toptensoftware/tapetool/blob/master/Release/tapetool.exe?raw=true>

## Usage

Usage: 

    > tapetool [COMMAND] [ARGS]


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

## Commands

The first command line argument specifies the type of command to execute:

### analyze

Dumps various statistics about wave data including amplitude ranges, estimated long and short cycle
lengths etc...

### filter

Renders a new wave file from an input wave file, applying the --smooth, --dcoffset and --amplify manipulations

This can be used to apply multiple smoothing passes for example.

### join

Joins two wave files together - they must have the same sample rate and bit depth.

### delete

Deletes a section of a wave file.

### samples

Dumps the raw samples of a wave file (can't be used with text input data).  If `from` is specified
dumps sample starting at that sample number.  Use --samplecount to specify how many samples to dump.

The output of this processing kind can't be re-read by tapetool

### cycles

Dumps the length (in samples) of each cycle in a wave file.

The output of this processing kind can't be re-read by tapetool

### cyclekinds

Dumps an input file as cycle kinds - short, long, ambiguous etc...

### bits

Dumps an input file as a series of `1` and `0` bits.

### bytes

Dumps the raw bytes of an input file, without checking headers, blocks, checksums

### blocks

Dumps the input file as a series of data blocks and computes and checks the checksum byte 
of each block.  A full dump from the command wihtout errors indicates a successful load.


## Comamnd Line Arguments

The available command line arguments depend on the selected command.  For more information on availability
of an option on a command use `tapetool COMMAND --help`

### --trs80

Specifies the target machine type as "TRS80".  Only 500 baud system, basic and source files are supported.

### --microbee

Specifies the target machine type as "Microbee".  Only 300 baud input is supported, but 300 and 1200 baud
rendering is supported.

### --inputformat:<fmt>

Set the format of the input data ("cas", "tap", etc...)

### --smooth[:N]

Smooths the input sample data using a moving average of period N, or 3 if N is not specified.

Using this option can:

* eliminate erroneous zero crossings caused by noise
* even out cycle lengths 

### --noanalyze

By default, tapetool analyzes audio files to automatically determine settings for how to best process
it. Use this option to prevent this auto analysis and use default settings for the machine type.

### --allowbadcycles

Normally an out of range cycle kind `<` or `>` causes processing of bit data to fail.  Use this option
to allow out of range cycle lengths.  TapeTool effective ignores these cycles and relies on the surrounding
cycles to determine the bit.

### --dcoffset

Apply an offset to all samples.  Use with pulse based audio (eg:TRS80) to shift the pulses into the zero
crossing range.  Also useful for shifting noisy silence/hiss out of the zero crossing range.

### --amplify 

Amplify the input audio signal by the specified percentage.  eg: `--amplify:50` will halve the volume of the 
input signal.

Note however that amplification rarely has any effect on the ability to decode digital data from a recording.

### --cyclefreq

Explicitly set the short cycle frequency in Hz.  (Default for Microbee is 2400Hz, TRS80 is 1024Hz)

## --cyclemode

Set the cycle detection mode.  By default tapetool uses a rising zero crossing to detect cycle boundaries. For
badly distorted signals however, sometimes a different approach can work more effectively.  The available modes
are:
	
	* `zc+` = zero crossing upwards
	* `zc-` = zero crossing downwards
	* `max` = local maximum
	* `min` = local minimum
	* `max+` = local maximum with positive sample value
	* `max-` = local minimum with negative sample value

Note that the local maximum/minimum options will nearly always require heavy, possibly multiple pass smoothing 
to work effectively.  Use the --filter command to apply multiple smoothing passes.

### --startsample:N

Available on some commands to specify the sample number in the input wave file to start at.

### --endsample:N

Available on some commands to specify the sample number in the input wave file to stop at.

### --samplecount:N

Available on some commands to specify the number of samples to process.

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

### --showcycles

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

### --baud:N

Sets the baud rate of the rendered wave file.  This option only works when processing in --blocks mode and
will automatically set the appropriate speed byte in the DGOS header.

### --sine

Renders the wave as a sine waves instead of square waves.  

### --createbitprofile

Use with `bits` or `bytes` commands to generate a bit resolution profile of the data in a wave file. The resulting
profile can then be used to render a repaired version of the original wave file.

### --createcycleprofile

Use with `bits` or `bytes` or `cyclekinds` commands to generate a cycle resolution profile of the data in a wave file. The resulting
profile can then be used to render a repaired version of the original wave file.

### --useprofile:wavefile

Render the output wave file using `wavefile` as the source for audio waveform data.  Tapetool will
look for the longest matching sequences of bits to build a new, restored audio file.  The wave file 
must have been previously profiled with either --createbitprofile or --createcycleprofile

### --gap

Used with the `join` command to insert a silent gap between the two joined wave file.

### --speedchangepos:N

Declare that a speed change occurs at sample N.  

eg: 1200 baud Microbee files start at 300 baud before switching to 1200 baud.  If processing at bit, byte
resolutions tapetool doesn't have the required information to determine where the speed change occurs.  

Use this option, combined with --speedchangespeed to explicitly declare where the change occurs.

Only a single speed change is supported.  

### --speedchangespeed:N

Use with --speedchangepos to declare the baud rate at the change of speed.

### --no-profiled-leadin

Don't include the lead-in noise in a profiled rendering.  By default, tapetool will include leading
noise before the actual data in a profiled rendering.  Use thi option to omit this leading silence/noise.

### --no-profiled-leadout

Don't include the trailing lead-out noise in a profiled rendering.

### --strict

Generate errors and resync if cycle kinds don't match exactly the type required for a particular bit.

### --fixtiming

Use with profiled renderings to resample cycles and bit patterns onto the exact timing boundaries required.


## Examples

Dump the blocks from a clean working wave file:

	> tapetool blocks --microbee myfile.wav

Dump the raw bytes from a wave file to a text file

	> tapetool bytes --microbee myfile.wav myfile.bytes.txt

Dump the bits from a wave file, smoothing the audio data over 3 samples and using heuristics
to calculate cycle lengths by analying the wave data rather than relying on the sample rate:

	> tapetool bits --microbee --smooth:3 --analyze myfile.wav myfile.bits.txt

Convert the output of the previous example into a series of bytes:

	> tapetool bytes --microbee myfile.bits.txt myfile.bytes.txt

Convert a byte stream into blocks and check the checksums, dump header info etc...

	> tapetool blocks --microbee myfile.bytes.txt

Render a wave file:

	> tapetool blocks --microbee myfile.bytes.txt myfile.wave

Create a ubee512 .tap file directly from a wave file:
	
	> tapetool blocks --microbee myfile.wav myfile.tap

Create a NanoWasp .mac file from text file: (take note of the header info and enter it
into NanoWasp web page where you configure the tape file).

	> tapetool blocks --microbee myfile.bits.txt myfile.mac

Create an arbitrary format binary file from a text file of bits:

	> tapetool bytes --microbee myfile.bits.txt myfile.bin

Join two wave files:

	> tapetool join file1.wav file2.wav joined.wav

Extract a part of a file, and apply an 8 sample smoothing:

	> tapetool filter file.wav --startsample:10000 --endsample:20000 --smooth:8 output.wav

Render a new wave file using the sample data from an original recording, assuming game.tap is a 
tap file with the recovered/repaired data and game.wav is the original damaged recording.

This is very experimental and mileage will vary depending on many factors.

	> tapetool bytes --microbee game.wav --createbitprofile
	> tapetool blocks --microbee game.tap game.repaired.wav --useprofile:game.wav


If a bit resolution profiled rendering doesn't work, you can try cycle resolution with timing 
correction:

	> tapetool bytes --microbee game.wav --createcycleprofile
	> tapetool blocks --microbee game.tap game.repaired.wav --useprofile:game.wav --fixtiming


	