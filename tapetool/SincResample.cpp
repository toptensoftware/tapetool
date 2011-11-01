//////////////////////////////////////////////////////////////////////////
// SincCurve.cpp - implementation of CSincCurve class

#include "precomp.h"
#include "SincResample.h"
#include <malloc.h>

#ifndef PI
#define PI (3.1415926535897932384626433832795)
#endif

#ifndef frac
#define frac(x)  (fabs(x)-floor(fabs(x)))
#endif

#define FP_FRACTION	0x0001FFFF
#define FP_SHIFT	17

const double dblInvFp1=1.0/double(FP_FRACTION);


inline double sqr(double x)
{
	return x*x;
}

// Computes the 0th order modified bessel function of the first kind
double I0(double x)
{
	double dblSum = 1.0;
	double dblHalfX = x / 2.0;
	double dblDiv = 1.0;
	double dblU = 1.0;

	while (true)
		{
		double dblTemp = dblHalfX/dblDiv;
		dblDiv += 1.0;

		dblTemp *= dblTemp;
		dblU *= dblTemp;
		dblSum += dblU;

		if (dblU < 1E-21 * dblSum)
			return dblSum;
		}
}


// Calculates right half of a sinc curve
void CalcHalfSinc(double dblZeroCrossings, int nCount, double dblRollOffFreq, double* pdblSinc)
{
	// Calculate sinc
	pdblSinc[0] = dblRollOffFreq;
	for (int i=1; i<nCount; i++) 
		{
		double dblX=PI*i/nCount*dblZeroCrossings;
		pdblSinc[i] = sin(dblX*dblRollOffFreq)/dblX;
		}
}

// Calculate a kaiser window
void CalcKaiser(double dblAlpha, int nCount, double* pdblKaiser)
{
	double dblI0 = I0(dblAlpha);
	for (int i=0; i<nCount; i++) 
		{
		pdblKaiser[i] = I0(dblAlpha*sqrt(1.0-sqr(double(i) / double(nCount-1)))) / dblI0;
		}
}


// Calculate and apply a kaiser window
void ApplyKaiser(double dblAlpha, int nCount, double* pdblSeries)
{
	double dblI0 = I0(dblAlpha);
	for (int i=0; i<nCount; i++) 
		{
		pdblSeries[i] *= I0(dblAlpha*sqrt(1.0-sqr(double(i) / double(nCount-1)))) / dblI0;
		}
}

/////////////////////////////////////////////////////////////////////////////
// CSincCurve

CSincCurve::CSincCurve()
{
	m_pdblCurveAndDeltas=NULL;
	m_iZeroCrossings=0;
	m_iResolutionPerZeroCrossing=0;
	m_iTotalPoints=0;
	m_szName[0]=0;
}

// Destructor
CSincCurve::~CSincCurve()
{
	Close();
}

void CSincCurve::Init(int iZeroCrossings, int iResolutionPerZeroCrossing, double* pdblCurve, char* pszName)
{
	// Store attributes
	m_iZeroCrossings=iZeroCrossings;
	m_iResolutionPerZeroCrossing=iResolutionPerZeroCrossing;
	m_iTotalPoints=m_iZeroCrossings * m_iResolutionPerZeroCrossing;

	// Allocate memory for the curve
	m_pdblCurveAndDeltas=new double[(m_iTotalPoints+2)*2];

	// (include 2 extra points, one for the Nth crossing, and one for the
	//		value after - basically a little margin on the boundaries, to 
	//		save doing boundary checks during resampling)
	m_pdblCurveAndDeltas[m_iTotalPoints*2]=0;
	m_pdblCurveAndDeltas[m_iTotalPoints*2+1]=0;
	m_pdblCurveAndDeltas[(m_iTotalPoints+1)*2]=0;
	m_pdblCurveAndDeltas[(m_iTotalPoints+1)*2+1]=0;

	// Copy curve
	for (int i=0; i<m_iTotalPoints; i++)
	{
		m_pdblCurveAndDeltas[i*2]=pdblCurve[i];
	}

	// Calculate deltas
	for (int i=0; i<m_iTotalPoints+1; i++)
	{
		m_pdblCurveAndDeltas[i*2+1]=m_pdblCurveAndDeltas[(i+1)*2]-m_pdblCurveAndDeltas[i*2];
	}

	if (pszName)
		strcpy(m_szName, pszName);

}


// Constructor
void CSincCurve::Init(int iZeroCrossings, int iResolutionPerZeroCrossing, double dblFreq, double dblKaiserAlpha, char* pszName)
{
	// Store attributes
	m_dblFreq=dblFreq;
	m_dblKaiserAlpha=dblKaiserAlpha;

	// Allocate temp memory for the curve
	int iTotalPoints=iZeroCrossings * iResolutionPerZeroCrossing;
	double* pCurve=new double[iTotalPoints];

	// Calculate sinc curve
	CalcHalfSinc(iZeroCrossings, iTotalPoints, dblFreq, pCurve);

	// Apply Kaiser window
	ApplyKaiser(dblKaiserAlpha, iTotalPoints, pCurve);

	// Init self
	Init(iZeroCrossings, iResolutionPerZeroCrossing, pCurve, pszName);

	// Clean up temp memory
	delete [] pCurve;
}


void CSincCurve::Close()
{
	if (m_pdblCurveAndDeltas)
		delete [] m_pdblCurveAndDeltas;
	m_pdblCurveAndDeltas=NULL;
	m_iZeroCrossings=0;
	m_iResolutionPerZeroCrossing=0;
	m_iTotalPoints=0;
	m_szName[0]=0;
}

CSincCurve* CreateSincCurve(SrcQuality quality)
{
	CSincCurve* pSinc=new CSincCurve();

	switch (quality)
	{
		case srcQualityFast:
			pSinc->Init(SINC_SMALL);
			break;

		case srcQualityGood:
			pSinc->Init(SINC_MEDIUM);
			break;

		case srcQualityHigh:
			pSinc->Init(SINC_LARGE);
			break;

		default:
			return NULL;
	}

	return pSinc;
}


/////////////////////////////////////////////////////////////////////////////
// Interleaved Nch sinc resample

extern "C"
void SincResampleUp_NID(
	double* pDest,							
	double* pSource,							
	double dblSrcSample,					
	int iSamples,							
	double dblDestRate,						
	double dblSourceRate,					
	CSincCurve* pCurve,
	int iChannels							
	)
{
	double* pdblCurveAndDeltas = pCurve->CurveAndDeltas();
	int iResolutionPerZeroCrossing = pCurve->ResolutionPerZeroCrossing();	
	int iZeroCrossings = pCurve->ZeroCrossings();

	// Work out inverse conversion rate for incrementing dblSrcSample
	double dblInvRate=dblSourceRate/dblDestRate;

	// Lookup increment is double because curve is interleaved curve/deltas
	int iLookupIncrement=iResolutionPerZeroCrossing*2;

	// Precalculate this
	int iZeroCrossingsByChannels=iZeroCrossings*iChannels;

	// Allocate memory for totals
	double* dblTotals=(double*)_alloca(sizeof(double)*iChannels);

	// Convert all samples...
	double* pfltDestStop=pDest + iSamples * iChannels;
	while (pDest<pfltDestStop)
	{
		// Zero totals
		memset(dblTotals, 0, sizeof(double)*iChannels);

		int iSrcSample=int(dblSrcSample);
		int iSrcSampleByChannels=iSrcSample*iChannels;

		// Process left half of sinc curve
		double dblPhaseL=dblSrcSample-double(iSrcSample);
		double dblFilterIndex=dblPhaseL * iResolutionPerZeroCrossing;
		int iFilterIndex=int(dblFilterIndex);
		double dblInterp=dblFilterIndex-double(iFilterIndex);
		double* pdblC=pdblCurveAndDeltas + iFilterIndex*2;
		double* pfltS=pSource + iSrcSampleByChannels;
		double* pfltStop=pfltS - iZeroCrossingsByChannels;

		while (pfltS>pfltStop)
		{
			double dblSinc=(pdblC[1] * dblInterp + pdblC[0]);
			for (int i=0; i<iChannels; i++)
			{
				dblTotals[i]+=dblSinc * pfltS[i];
			}
			pdblC+=iLookupIncrement;
			pfltS-=iChannels;
		}

		// Process right half of sinc curve
		double dblPhaseR=1.0-dblPhaseL;
		dblFilterIndex=dblPhaseR * iResolutionPerZeroCrossing;
		iFilterIndex=int(dblFilterIndex);
		dblInterp=dblFilterIndex-double(iFilterIndex);
		pdblC=pdblCurveAndDeltas + iFilterIndex*2;
		pfltS=pSource + iSrcSampleByChannels + iChannels;
		pfltStop=pfltS + iZeroCrossingsByChannels;

		while (pfltS<pfltStop)
		{
			double dblSinc=(pdblC[1] * dblInterp + pdblC[0]);
			for (int i=0; i<iChannels; i++)
			{
				dblTotals[i]+=dblSinc * pfltS[i];
			}
			pdblC+=iLookupIncrement;
			pfltS+=iChannels;
		}

		// Store result
		for (int i=0; i<iChannels; i++)
		{
			*pDest++=double(dblTotals[i]);
		}

		// Update source sample
		dblSrcSample+=dblInvRate;
	}
}

extern "C" 
void SincResampleDown_NID(
	double* pDest,							
	double* pSource,							
	double dblSrcSample,					
	int iSamples,							
	double dblDestRate,						
	double dblSourceRate,					
	CSincCurve* pCurve,
	int iChannels							
	)
{
	double* pdblCurveAndDeltas = pCurve->CurveAndDeltas();
	int iResolutionPerZeroCrossing = pCurve->ResolutionPerZeroCrossing();	
	int iZeroCrossings = pCurve->ZeroCrossings();

	// Calculate conversion rate (and inverse)
	double dblRate=dblDestRate/dblSourceRate;
	double dblInvRate=dblSourceRate/dblDestRate;

	// Calculate number of interations of the internal left/right loops
	int iIterations=int(double(iZeroCrossings)*dblInvRate);
	int iIterationsByChannels=iIterations*iChannels;

	// Calculate fp fraction * sinc resolution
	double	dblFp1ByRes=double(FP_FRACTION) * iResolutionPerZeroCrossing;

	// Calculate fp increment per internal left/right loop
	unsigned int iIncrement=int(dblFp1ByRes * dblDestRate / dblSourceRate);

	// Allocate memory for totals
	double* dblTotals=(double*)_alloca(sizeof(double)*iChannels);

	// Convert all samples...
	double* pfltDestStop=pDest + iSamples * iChannels;
	while (pDest<pfltDestStop)
	{
		// Zero totals
		memset(dblTotals, 0, sizeof(double)*iChannels);

		// Process left half of sinc curve
		int iSrcSample=int(dblSrcSample);
		int iSrcSampleByChannels=iSrcSample*iChannels;
		double dblPhaseL=(dblSrcSample-double(iSrcSample)) * dblRate;

		// Convert phase to fixed point
		unsigned int iFilterIndexFP=(unsigned int)(dblPhaseL * dblFp1ByRes);

		// Get source sample point
		double* pfltS=pSource + iSrcSampleByChannels;

		// Process left half of sinc curve
		double* pfltStop=pfltS - iIterationsByChannels;
		while (pfltS>pfltStop)
		{
			double dblFilterInterp=double(iFilterIndexFP & FP_FRACTION) * dblInvFp1;
			unsigned int iFilterIndex=(iFilterIndexFP >> FP_SHIFT)*2;
			double dblSinc=(pdblCurveAndDeltas[iFilterIndex+1] * dblFilterInterp + pdblCurveAndDeltas[iFilterIndex]);
			for (int i=0; i<iChannels; i++)
			{
				dblTotals[i]+=dblSinc * pfltS[i];
			}
			pfltS-=iChannels;
			iFilterIndexFP+=iIncrement;
		}

		// Calculate phase R
		double dblPhaseR=dblRate-dblPhaseL;

		// Convert phase to fixed point
		iFilterIndexFP=(unsigned int)(dblPhaseR * dblFp1ByRes);

		// Get source sample point
		pfltS=pSource + iSrcSampleByChannels + iChannels;

		// Process right half of sinc curve
		pfltStop=pfltS + iIterationsByChannels;
		while (pfltS<pfltStop)
		{
			double dblFilterInterp=double(iFilterIndexFP & FP_FRACTION) * dblInvFp1;
			unsigned int iFilterIndex=(iFilterIndexFP >> FP_SHIFT)*2;
			double dblSinc=(pdblCurveAndDeltas[iFilterIndex+1] * dblFilterInterp + pdblCurveAndDeltas[iFilterIndex]);
			for (int i=0; i<iChannels; i++)
			{
				dblTotals[i]+=dblSinc * pfltS[i];
			}
			pfltS+=iChannels;
			iFilterIndexFP+=iIncrement;
		}

		// Store result
		for (int i=0; i<iChannels; i++)
		{
			*pDest++=double(dblTotals[i]);
		}

		// Update source sample
		dblSrcSample+=dblInvRate;
	}
}



void SincResample_NID(
	double* pDest,							
	double* pSource,							
	double dblSrcSample,					
	int iSamples,							
	double dblDestRate,						
	double dblSourceRate,					
	CSincCurve* pCurve,
	int iChannels							
	)
{
	if (dblDestRate < dblSourceRate)
	{
		SincResampleDown_NID(pDest, pSource, dblSrcSample, iSamples, dblDestRate, dblSourceRate, pCurve, iChannels);
	}
	else
	{
		SincResampleUp_NID(pDest, pSource, dblSrcSample, iSamples, dblDestRate, dblSourceRate, pCurve, iChannels);
	}
}

extern "C"
void SincResampleUp_NIS(
	short* pDest,							
	short* pSource,							
	double dblSrcSample,					
	int iSamples,							
	double dblDestRate,						
	double dblSourceRate,					
	CSincCurve* pCurve,
	int iChannels							
	)
{
	double* pdblCurveAndDeltas = pCurve->CurveAndDeltas();
	int iResolutionPerZeroCrossing = pCurve->ResolutionPerZeroCrossing();	
	int iZeroCrossings = pCurve->ZeroCrossings();

	// Work out inverse conversion rate for incrementing dblSrcSample
	double dblInvRate=dblSourceRate/dblDestRate;

	// Lookup increment is double because curve is interleaved curve/deltas
	int iLookupIncrement=iResolutionPerZeroCrossing*2;

	// Precalculate this
	int iZeroCrossingsByChannels=iZeroCrossings*iChannels;

	// Allocate memory for totals
	double* dblTotals=(double*)_alloca(sizeof(double)*iChannels);

	// Convert all samples...
	short* pfltDestStop=pDest + iSamples * iChannels;
	while (pDest<pfltDestStop)
	{
		// Zero totals
		memset(dblTotals, 0, sizeof(double)*iChannels);

		int iSrcSample=int(dblSrcSample);
		int iSrcSampleByChannels=iSrcSample*iChannels;

		// Process left half of sinc curve
		double dblPhaseL=dblSrcSample-double(iSrcSample);
		double dblFilterIndex=dblPhaseL * iResolutionPerZeroCrossing;
		int iFilterIndex=int(dblFilterIndex);
		double dblInterp=dblFilterIndex-double(iFilterIndex);
		double* pdblC=pdblCurveAndDeltas + iFilterIndex*2;
		short* pfltS=pSource + iSrcSampleByChannels;
		short* pfltStop=pfltS - iZeroCrossingsByChannels;

		while (pfltS>pfltStop)
		{
			double dblSinc=(pdblC[1] * dblInterp + pdblC[0]);
			for (int i=0; i<iChannels; i++)
			{
				dblTotals[i]+=dblSinc * pfltS[i];
			}
			pdblC+=iLookupIncrement;
			pfltS-=iChannels;
		}

		// Process right half of sinc curve
		double dblPhaseR=1.0-dblPhaseL;
		dblFilterIndex=dblPhaseR * iResolutionPerZeroCrossing;
		iFilterIndex=int(dblFilterIndex);
		dblInterp=dblFilterIndex-double(iFilterIndex);
		pdblC=pdblCurveAndDeltas + iFilterIndex*2;
		pfltS=pSource + iSrcSampleByChannels + iChannels;
		pfltStop=pfltS + iZeroCrossingsByChannels;

		while (pfltS<pfltStop)
		{
			double dblSinc=(pdblC[1] * dblInterp + pdblC[0]);
			for (int i=0; i<iChannels; i++)
			{
				dblTotals[i]+=dblSinc * pfltS[i];
			}
			pdblC+=iLookupIncrement;
			pfltS+=iChannels;
		}

		// Store result
		for (int i=0; i<iChannels; i++)
		{
			*pDest++=short(dblTotals[i]);
		}

		// Update source sample
		dblSrcSample+=dblInvRate;
	}
}

extern "C" 
void SincResampleDown_NIS(
	short* pDest,							
	short* pSource,							
	double dblSrcSample,					
	int iSamples,							
	double dblDestRate,						
	double dblSourceRate,					
	CSincCurve* pCurve,
	int iChannels							
	)
{
	double* pdblCurveAndDeltas = pCurve->CurveAndDeltas();
	int iResolutionPerZeroCrossing = pCurve->ResolutionPerZeroCrossing();	
	int iZeroCrossings = pCurve->ZeroCrossings();

	// Calculate conversion rate (and inverse)
	double dblRate=dblDestRate/dblSourceRate;
	double dblInvRate=dblSourceRate/dblDestRate;

	// Calculate number of interations of the internal left/right loops
	int iIterations=int(double(iZeroCrossings)*dblInvRate);
	int iIterationsByChannels=iIterations*iChannels;

	// Calculate fp fraction * sinc resolution
	double	dblFp1ByRes=double(FP_FRACTION) * iResolutionPerZeroCrossing;

	// Calculate fp increment per internal left/right loop
	unsigned int iIncrement=int(dblFp1ByRes * dblDestRate / dblSourceRate);

	// Allocate memory for totals
	double* dblTotals=(double*)_alloca(sizeof(double)*iChannels);

	// Convert all samples...
	short* pfltDestStop=pDest + iSamples * iChannels;
	while (pDest<pfltDestStop)
	{
		// Zero totals
		memset(dblTotals, 0, sizeof(double)*iChannels);

		// Process left half of sinc curve
		int iSrcSample=int(dblSrcSample);
		int iSrcSampleByChannels=iSrcSample*iChannels;
		double dblPhaseL=(dblSrcSample-double(iSrcSample)) * dblRate;

		// Convert phase to fixed point
		unsigned int iFilterIndexFP=(unsigned int)(dblPhaseL * dblFp1ByRes);

		// Get source sample point
		short* pfltS=pSource + iSrcSampleByChannels;

		// Process left half of sinc curve
		short* pfltStop=pfltS - iIterationsByChannels;
		while (pfltS>pfltStop)
		{
			double dblFilterInterp=double(iFilterIndexFP & FP_FRACTION) * dblInvFp1;
			unsigned int iFilterIndex=(iFilterIndexFP >> FP_SHIFT)*2;
			double dblSinc=(pdblCurveAndDeltas[iFilterIndex+1] * dblFilterInterp + pdblCurveAndDeltas[iFilterIndex]);
			for (int i=0; i<iChannels; i++)
			{
				dblTotals[i]+=dblSinc * pfltS[i];
			}
			pfltS-=iChannels;
			iFilterIndexFP+=iIncrement;
		}

		// Calculate phase R
		double dblPhaseR=dblRate-dblPhaseL;

		// Convert phase to fixed point
		iFilterIndexFP=(unsigned int)(dblPhaseR * dblFp1ByRes);

		// Get source sample point
		pfltS=pSource + iSrcSampleByChannels + iChannels;

		// Process right half of sinc curve
		pfltStop=pfltS + iIterationsByChannels;
		while (pfltS<pfltStop)
		{
			double dblFilterInterp=double(iFilterIndexFP & FP_FRACTION) * dblInvFp1;
			unsigned int iFilterIndex=(iFilterIndexFP >> FP_SHIFT)*2;
			double dblSinc=(pdblCurveAndDeltas[iFilterIndex+1] * dblFilterInterp + pdblCurveAndDeltas[iFilterIndex]);
			for (int i=0; i<iChannels; i++)
			{
				dblTotals[i]+=dblSinc * pfltS[i];
			}
			pfltS+=iChannels;
			iFilterIndexFP+=iIncrement;
		}

		// Store result
		for (int i=0; i<iChannels; i++)
		{
			*pDest++=short(dblTotals[i]);
		}

		// Update source sample
		dblSrcSample+=dblInvRate;
	}
}



void SincResample_NIS(
	short* pDest,							
	short* pSource,							
	double dblSrcSample,					
	int iSamples,							
	double dblDestRate,						
	double dblSourceRate,					
	CSincCurve* pCurve,
	int iChannels							
	)
{
	if (dblDestRate < dblSourceRate)
	{
		SincResampleDown_NIS(pDest, pSource, dblSrcSample, iSamples, dblDestRate, dblSourceRate, pCurve, iChannels);
	}
	else
	{
		SincResampleUp_NIS(pDest, pSource, dblSrcSample, iSamples, dblDestRate, dblSourceRate, pCurve, iChannels);
	}
}

