//////////////////////////////////////////////////////////////////////////
// CycleDetector.h - declaration of CTapeReader class

#ifndef __CYCLEDETECTOR_H
#define __CYCLEDETECTOR_H

enum CycleMode
{
	cmZeroCrossingUp,
	cmZeroCrossingDown,
	cmMaxima,
	cmMinima,
	cmPositiveMaxima,
	cmNegativeMinima,
};

class CCycleDetector
{
public:
	CCycleDetector(CycleMode mode);

	void Reset();
	void Reset(CycleMode mode);
	CycleMode GetMode();
	bool IsNewCycle(int sample);

	static const char* ToString(CycleMode mode);
	static bool FromString(const char* pszm, CycleMode& mode);

protected:
	int CalculateDirection(int sample);

	CycleMode _mode;
	int _prev;
	int _prevDirection;
	bool _first;
};

#endif	// __CYCLEDETECTOR_H

