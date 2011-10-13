//////////////////////////////////////////////////////////////////////////
// CycleDetector.cpp - implementation of CCycleDetector class

#include "precomp.h"

#include "CycleDetector.h"

CCycleDetector::CCycleDetector(CycleMode mode)
{
	Reset(mode);
}

void CCycleDetector::Reset(CycleMode mode)
{
	_mode = mode;
	_prevDirection = 0;
	_prev = 0;
	_first = true;
}


void CCycleDetector::Reset()
{
	Reset(_mode);
}

CycleMode CCycleDetector::GetMode()
{
	return _mode;
}


bool CCycleDetector::IsNewCycle(int sample)
{
	bool retv = false;

	switch (_mode)
	{
		case cmZeroCrossingUp:
		{
			retv = _first || (_prev<=0 && sample>0);
			break;
		}

		case cmZeroCrossingDown:
		{
			retv = _first || (_prev>=0 && sample<0);
			break;
		}

		case cmMaxima:
		{
			int direction = CalculateDirection(sample);
			retv = direction != _prevDirection && direction<0;
			_prevDirection = direction;
			break;
		}

		case cmMinima:
		{
			int direction = CalculateDirection(sample);
			retv = direction != _prevDirection && direction>0;
			_prevDirection = direction;
			break;
		}

		case cmPositiveMaxima:
		{
			int direction = CalculateDirection(sample);
			retv = direction != _prevDirection && direction<0 && sample>0;
			_prevDirection = direction;
			break;
		}

		case cmNegativeMinima:
		{
			int direction = CalculateDirection(sample);
			retv = direction != _prevDirection && direction>0 && sample<0;
			_prevDirection = direction;
			break;
		}
	}

	_prev = sample;
	_first = false;

	return retv;
}

int CCycleDetector::CalculateDirection(int sample)
{
	if (sample > _prev)
		return 1;
	if (sample < _prev)
		return -1;

	return _prevDirection;
}

const char* CCycleDetector::ToString(CycleMode mode)
{
	switch (mode)
	{
		case cmZeroCrossingUp: return "zc+";
		case cmZeroCrossingDown: return "zc-";
		case cmMaxima: return "max";
		case cmMinima: return "min";
		case cmPositiveMaxima: return "max+";
		case cmNegativeMinima: return "min+";
	}

	return "?";
}

bool CCycleDetector::FromString(const char* psz, CycleMode& mode)
{
	if (psz==NULL)
		return false;

	if (_stricmp(psz, "zc+")==0)
		mode = cmZeroCrossingUp;
	else if (_stricmp(psz, "zc-")==0)
		mode = cmZeroCrossingUp;
	else if (_stricmp(psz, "max")==0)
		mode = cmMaxima;
	else if (_stricmp(psz, "min")==0)
		mode = cmMinima;
	else if (_stricmp(psz, "max+")==0)
		mode = cmPositiveMaxima;
	else if (_stricmp(psz, "min-")==0)
		mode = cmNegativeMinima;
	else
		return false;

	return true;
}
