//////////////////////////////////////////////////////////////////////////
// Instrumentation.cpp - implementation of CInstrumentation class

#include "precomp.h"

#include "Instrumentation.h"
#include "FileReader.h"

#include <ctype.h>

// Constructor
CInstrumentation::CInstrumentation()
{
	_res = resBits;
	_sections = NULL;
	_sectionCount = 0;
	_entries = NULL;
	_entryCount = 0;
	_allocatedEntryCount = 0;
	_inResync = 0;
	_currentSection = NULL;
	_pendingEndOffset = 0;
	_totalUsed = 0;
}

// Destructor
CInstrumentation::~CInstrumentation()
{
	Reset();
}

int CInstrumentation::GetTotalEntries()
{
	return _entryCount;
}

int CInstrumentation::GetUsedEntries()
{
	return _totalUsed;
}


Resolution CInstrumentation::GetResolution()
{
	return _res;
}

void CInstrumentation::SetResolution(Resolution value)
{
	_res = value;
}

const char* CInstrumentation::GetResolutionString()
{
	switch (_res)
	{
		case resBits:
			return "bits";

		case resCycleKinds:
			return "cycles";
	}

	return "??";
}


// Start a new section
void CInstrumentation::SectionBreak()
{
	// Add the terminating entry
	if (_currentSection!=NULL)
		AddEntryInternal(-1, _pendingEndOffset);

	_currentSection=NULL;
}

void CInstrumentation::EnsureSection(int speed)
{
	if (_currentSection!=NULL && _currentSection->_speed==speed)
		return;

	// Create new section
	_sectionCount++;
	_sections = (INSTR_SECTION*)(_sections ? realloc(_sections, _sectionCount * sizeof(INSTR_SECTION)) : malloc(_sectionCount * sizeof(INSTR_SECTION)));

	_currentSection = _sections + _sectionCount-1;
	_currentSection->_entryCount=0;
	_currentSection->_firstEntry=_entryCount;
	_currentSection->_speed=speed;
}

void CInstrumentation::AddBitEntry(int speed, int bit, int offset, int end_offset)
{
	if (_res == resBits)
		AddEntryRaw(speed, (char)bit, offset, end_offset);
}

void CInstrumentation::AddCycleKindEntry(char kind, int offset, int end_offset)
{
	if (_res == resCycleKinds)
		AddEntryRaw(0, kind, offset, end_offset);
}

// Add an entry to the current section
void CInstrumentation::AddEntryRaw(int speed, char kind, int offset, int end_offset)
{
	// Ignore if section not start
	if (_inResync)
		return;

//	if (_pendingEndOffset!=0 && _currentSection!=NULL && offset != _pendingEndOffset)
	//	SectionBreak();

	// Ensure section started
	EnsureSection(speed);

	AddEntryInternal(kind, offset);

	_pendingEndOffset = end_offset;
}

void CInstrumentation::AddEntryInternal(char kind, int offset)
{
	// Make room for new entry
	if (_entryCount+1 >= _allocatedEntryCount)
	{
		if (_entryCount==0)
		{
			_allocatedEntryCount = 0x4000;
			_entries = (INSTR_ENTRY*)malloc(_allocatedEntryCount * sizeof(INSTR_ENTRY));
		}
		else
		{
			_allocatedEntryCount *= 2;
			_entries = (INSTR_ENTRY*)realloc(_entries, _allocatedEntryCount * sizeof (INSTR_ENTRY));
		}
	}

	// Store instrumentation data
	INSTR_ENTRY* e = &_entries[_entryCount++];
	e->_kind = kind;
	e->_offset = offset;
	e->_used = false;
	_currentSection->_entryCount++;
}

void CInstrumentation::StartSync()
{
	// Insert a section break
	SectionBreak();

	// Block incoming entries while syncing
	_inResync++;
}

void CInstrumentation::EndSync()
{
	// Unblock
	_inResync--;
}

void CInstrumentation::Reset()
{
	if (_sections!=NULL)
		free(_sections);
	if (_entries!=NULL)
		free(_entries);

	_entries = NULL;
	_sections= NULL;
	_sectionCount = 0;
	_entryCount = 0;
	_allocatedEntryCount = 0;
	_inResync = 0;
	_pendingEndOffset = 0;
}

bool CInstrumentation::Save(const char* filename, int checkVal)
{
	SectionBreak();
	/*
	// Create the file
	FILE* file = fopen(filename, "wt");
	if (file==NULL)
		return false;

	// Check value
	fprintf(file, "check:%i\n", checkVal);

	// Sections
	for (int s=0; s<_sectionCount; s++)
	{
		// Get the section
		INSTR_SECTION* sect = _sections+s;

		// Section marker
		fprintf(file, "section:%i\n", sect->_speed);

		for (int e=0; e<sect->_entryCount; e++)
		{
			INSTR_ENTRY* entry = _entries + sect->_firstEntry + e;
			fprintf(file, "bit:%i:%i\n", entry->_kind, entry->_offset);
		}
	}
	*/

	printf("[Instrumentation found %i sections and a total of %i %s]\n", _sectionCount, _entryCount-_sectionCount, GetResolutionString());

	FILE* file = fopen(filename, "wb");
	if (file==NULL)
	{
	    fprintf(stderr, "Could not create '%s' - %s (%i)\n", filename, strerror(errno), errno);
		return false;
	}

	// Write header
	INSTR_FILE header;
	header._sig = 0x92748123;
	header._sectionCount = _sectionCount;
	header._entryCount = _entryCount;
	header._check = checkVal;
	header._res = _res;
	fwrite(&header, sizeof(header), 1, file);

	// Write sections
	fwrite(_sections, sizeof(INSTR_SECTION), _sectionCount, file);

	// Write entries
	fwrite(_entries, sizeof(INSTR_ENTRY), _entryCount, file);

	// Done
	fclose(file);

	return true;
}

bool CInstrumentation::SaveText(const char* filename, int checkVal)
{
	FILE* file = fopen(filename, "wt");
	if (file==NULL)
	{
	    fprintf(stderr, "Could not create '%s' - %s (%i)\n", filename, strerror(errno), errno);
		return false;
	}

	fprintf(file, "check:%i\n", checkVal);
	fprintf(file, "resolution:%s\n", GetResolutionString());

	for (int i=0; i<_sectionCount; i++)
	{
		INSTR_SECTION* sect = &_sections[i];
		fprintf(file, "\nSection %i\n", i);
		for (int j=0; j<sect->_entryCount; j++)
		{
			INSTR_ENTRY* entry = &_entries[sect->_firstEntry + j];
			fprintf(file, "  %i:%i", entry->_offset, entry->_kind);
			if (entry->_kind!=-1)
			{
				fprintf(file, " (%i)", entry[1]._offset - entry->_offset);
			}
			fprintf(file, "\n");
		}
	}

	// Done
	fclose(file);

	return true;
}


bool CInstrumentation::Load(const char* filename, int checkVal)
{
	FILE* file = fopen(filename, "rb");
	if (file==NULL)
	{
	    fprintf(stderr, "Could not open '%s' - %s (%i)\n", filename, strerror(errno), errno);
		return false;
	}

	// Read header
	INSTR_FILE header;
	if (fread(&header, sizeof(header), 1, file)!=1)
	{
		fclose(file);
		return false;
	}

	// Check the check val
	if (header._check!=checkVal)
	{
		fprintf(stderr, "Profile data doesn't match original wave file - please regenerate");
		fclose(file);
		return false;
	}

	// Check the resolution is ok
	if (header._res != resBits && header._res != resCycleKinds)
	{
		fprintf(stderr, "Invalid profile data - please regenerate");
		fclose(file);
		return false;
	}

	// Store resolution
	_res = header._res;

	// Allocate memory
	_sections = (INSTR_SECTION*)malloc(sizeof(INSTR_SECTION) * header._sectionCount);
	_entries = (INSTR_ENTRY*)malloc(sizeof(INSTR_ENTRY) * header._entryCount);

	// Read
	if (fread(_sections, sizeof(INSTR_SECTION), header._sectionCount, file) != header._sectionCount || 
		fread(_entries, sizeof(INSTR_ENTRY), header._entryCount, file) != header._entryCount)
	{
		fclose(file);
		return false;
	}

	// Init state
	_sectionCount = header._sectionCount;
	_entryCount = header._entryCount;
	_allocatedEntryCount = header._entryCount;

	// Done
	fclose(file);
	return true;
}

bool CInstrumentation::FindSequence(int speed, char* kinds, int count, INSTR_ENTRY** pStart, int* pLength)
{
	int length = 0;
	INSTR_ENTRY* start = NULL;


	for (int iSection = 0; iSection<_sectionCount; iSection++)
	{
		INSTR_SECTION* sect = _sections + iSection;

		if (sect->_speed!=speed)
			continue;
		
		for (int i=0; i<sect->_entryCount; i++)
		{
			// Find first mismatch
			int j;
			for (j=0; j<sect->_entryCount-i && j<count; j++)
			{
				// Same cycle kind
				if (_entries[sect->_firstEntry + i + j]._kind==kinds[j])
					continue;

				// Did we instrument an ambiguous cycle
				if (_entries[sect->_firstEntry + i + j]._kind=='?')
				{
					// Get the next and previous cycle kind
					char prev = i+j>0 ? _entries[sect->_firstEntry + i + j - 1]._kind : 0;
					char next = i+j+1<sect->_entryCount ? _entries[sect->_firstEntry + i + j + 1]._kind : 0;

					// Is the ambiguous cycle is on S->?->L or L->?->S boundary
					if ((prev=='L' || prev=='S') && (next=='S' || next=='L') && prev!=next)
					{
						if (j+1==count || next == kinds[j+1])
							continue;
					}
				}

				// Allow skipping one cycle
				/*
				if (next == kinds[j])
				{
					continue;
				}
				*/

				// Mismatch
				break;
			}

			// New best sequence?
			if (j>length)
			{
				start = _entries + sect->_firstEntry + i;
				length = j;
			}
			else if (j==length)
			{
				// Work out which one is less used
				int a = 0;
				int b = 0;
				for (int e=0; e<length; e++)
				{
					if (_entries[sect->_firstEntry + i + e]._used)
						a++;
					if (start[e]._used)
						b++;
				}

				if (a<b)
				{
					start = _entries + sect->_firstEntry + i;
					length = j;
				}
			}
		}
	}

	if (start==NULL)
		return false;

	// Mark entries as used
	for (int i=0; i<length; i++)
	{
		if (!start[i]._used)
			_totalUsed++;
		start[i]._used = true;
	}

	*pStart = start;
	*pLength = length;

	return true;
}


int CInstrumentation::LeadingSampleCount()
{
	return _entries[0]._offset;
}

int CInstrumentation::TrailingSamplesOffset()
{
	return _entries[_entryCount-1]._offset;
}


// --microbee --bytes robotf_1.wav --instrument
// --microbee --blocks robotf_1.blocks.repaired.txt robotf1.restored.wav --useprofile:robotf_1.wav 
// --microbee --microbee robotf1.restored.wav
