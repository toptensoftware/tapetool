//////////////////////////////////////////////////////////////////////////
// Instrumentation.h - declaration of CTapeReader class

#ifndef __INSTRUMENTATION_H
#define __INSTRUMENTATION_H

enum Resolution;

struct INSTR_ENTRY
{
	char			_kind;			// Bit: 0, 1
	int				_offset;
	bool			_used;
};

struct INSTR_SECTION
{
	int				_speed;
	int				_firstEntry;
	int				_entryCount;
};

struct INSTR_FILE
{
	int				_sig;
	int				_sectionCount;
	int				_entryCount;
	int				_check;
	Resolution		_res;
};

class CInstrumentation
{
public:
			CInstrumentation();
	virtual ~CInstrumentation();	

	Resolution GetResolution();
	void SetResolution(Resolution value);
	const char* GetResolutionString();

	void SectionBreak();
	void AddBitEntry(int speed, int bit, int offset, int end_offset);
	void AddCycleKindEntry(char kind, int offset, int end_offset);
	void AddEntryRaw(int speed, char kind, int offset, int end_offset);
	void StartSync();
	void EndSync();
	int GetTotalEntries();
	int GetUsedEntries();

	Resolution		_res;
	INSTR_SECTION*	_currentSection;
	INSTR_SECTION*	_sections;
	int				_sectionCount;
	INSTR_ENTRY*	_entries;
	int				_entryCount;
	int				_allocatedEntryCount;
	int				_inResync;
	int				_pendingEndOffset;
	int				_totalUsed;

	void Reset();
	bool Save(const char* filename, int checkVal);
	bool SaveText(const char* filename, int checkVal);
	bool Load(const char* filename, int checkVal);

	bool FindSequence(int speed, char* kinds, int count, INSTR_ENTRY** pStart, int* pLength);
	int LeadingSampleCount();
	int TrailingSamplesOffset();

private:
	void EnsureSection(int speed);
	void AddEntryInternal(char kind, int offset);
};


class CSyncBlock
{
public:
	CSyncBlock(CInstrumentation* instr)
	{
		_instr = instr;
		if (_instr)
			_instr->StartSync();
	}

	~CSyncBlock()
	{
		if (_instr)
			_instr->EndSync();
	}

	CInstrumentation* _instr;
};


#endif	// __INSTRUMENTATION_H


