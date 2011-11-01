//////////////////////////////////////////////////////////////////////////
// SincCurve.h - declaration of CSincCurve class

#ifndef __SINCRESAMPLE_H
#define __SINCRESAMPLE_H


double I0(double x);
double sqr(double x);
void CalcHalfSinc(double dblZeroCrossings, int nCount, double dblRollOffFreq, double* pdblSinc);
void CalcKaiser(double dblAlpha, int nCount, double* pdblKaiser);
void ApplyKaiser(double dblAlpha, int nCount, double* pdblSeries);

class CSincCurve
{
public:
// Constructor
	CSincCurve();
	~CSincCurve();

	void Init(int iZeroCrossings, int iResolutionPerZeroCrossing, double* pdblCurve, char* pszName=NULL);
	void Init(int iZeroCrossings, int iResolutionPerZeroCrossing, double dblFreq, double dblKaiserAlpha, char* pszName=NULL);
	void Close();

	char* GetName() { return m_szName; };

// Accessors
	int ZeroCrossings() { return m_iZeroCrossings; };
	int ResolutionPerZeroCrossing() { return m_iResolutionPerZeroCrossing; };
	int TotalPoints() { return m_iTotalPoints; }
	double Frequency() { return m_dblFreq; }
	double KaiserAlpha() { return m_dblKaiserAlpha; }
	double* CurveAndDeltas() { return m_pdblCurveAndDeltas; }

// Attributes
	double*		m_pdblCurveAndDeltas;
	int			m_iZeroCrossings;
	int			m_iResolutionPerZeroCrossing;
	int			m_iTotalPoints;
	double		m_dblFreq;
	double		m_dblKaiserAlpha;
	char		m_szName[100];
};

#define SINC_SMALL_ZC		19
#define SINC_SMALL_RES		128
#define SINC_SMALL_FREQ		8.31472372954840555082e-01
#define SINC_SMALL_KA		9.7

#define SINC_MEDIUM_ZC		41
#define SINC_MEDIUM_RES		128
#define SINC_MEDIUM_FREQ	9.20381425342432724079e-01
#define SINC_MEDIUM_KA		9.7

#define SINC_LARGE_ZC		133
#define SINC_LARGE_RES		128
#define SINC_LARGE_FREQ		9.73822959712628111184e-01
#define SINC_LARGE_KA		9.7

#define SINC_SMALL		SINC_SMALL_ZC, SINC_SMALL_RES, SINC_SMALL_FREQ, SINC_SMALL_KA
#define SINC_MEDIUM		SINC_MEDIUM_ZC, SINC_MEDIUM_RES, SINC_MEDIUM_FREQ, SINC_MEDIUM_KA
#define SINC_LARGE		SINC_LARGE_ZC, SINC_LARGE_RES, SINC_LARGE_FREQ, SINC_LARGE_KA


enum SrcQuality
{
	srcQualityFast,
	srcQualityGood,
	srcQualityHigh,
};

CSincCurve* CreateSincCurve(SrcQuality quality);


/*

 pDest  (Interleaved functions)
	- Pointer to output buffer.  This buffer will be filled with iSamples samples

 pSource (Interleaved functions)
	- Pointer to source sample buffer.
	- This buffer will be accessed from 
			pSource[(dblSrcSample-A)*I] to 
			pSource[(dblSrcSample+B+C)*I+(I-1)]
		where A, B, C correspond to values returned from GetSincResampleABCs function
		and I is the interleaved channel count

 dblSrcSample
	- Source sample position that corresponds to first destination sample.
	- This value must be relative to the buffer identified by pSource

 iSamples
	- The number of output samples to generate.

 dblDestRate
	- The destination sampling rate

 dblSourceRate
	- The source sampling rate
	- If dblSourceRate<dblDestRate call SincResampleUp else call SincResampleDown.

 pdblCurve
	- Pointer to the precalculated kaiser windowed sinc curve table
	- This table should be (iResolutionPerZeroCrossing * iZeroCrossings)+2 entries
		in size.

 pdblDeltas
	- Pointer to the precalculated delta table. pdblDeltas[i]=pdblCurve[i+1]-pdblCurve[i]
	- This table should be (iResolutionPerZeroCrossing * iZeroCrossings)+1 entries
		in size.

 iResolutionPerZeroCrossing
	- Resolution of the precalculated sinc curve table.  Number of table values
		per zero crossing.

 iZeroCrossings
	- The number of sinc-curve zero crossings. 

 iChannels
	- The number of interleaved channels

*/


void SincResample_NID(
	double* pDest,							
	double* pSource,							
	double dblSrcSample,					
	int iSamples,							
	double dblDestRate,						
	double dblSourceRate,					
	CSincCurve* pCurve,
	int iChannels							
	);

void SincResample_NIS(
	short* pDest,							
	short* pSource,							
	double dblSrcSample,					
	int iSamples,							
	double dblDestRate,						
	double dblSourceRate,					
	CSincCurve* pCurve,
	int iChannels							
	);

#endif	// __SINCCURVE_H

