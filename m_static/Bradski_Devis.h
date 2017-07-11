#include "globals.h"
#include <tracking.hpp>
#include <imgproc_c.h>
#include <highgui.hpp>
#include <ippi.h>
#include <vector>

//void cvCvtColorr( const CvArr* srcarr, CvArr* dstarr, int code )
//{
//    cv::Mat src = cv::cvarrToMat(srcarr), dst0 = cv::cvarrToMat(dstarr), dst = dst0;
//    CV_Assert( src.depth() == dst.depth() );
//
//    cv::cvtColor(src, dst, code, dst.channels());
//    CV_Assert( dst.data == dst0.data );
//}

enum
	{
	xx=320,
	yy=240
	};

class Bradski_Devis
{
public:
	// ring image buffer
	IplImage **buf;
	int last;

	// temporary images
	IplImage *mhi; // MHI
	IplImage *orient; // orientation
	IplImage *mask; // valid orientation mask
	IplImage *segmask; // motion segmentation map
	CvMemStorage* storage; // temporary storage
	//Det02Params* structParams;
	CvSeq* m_pSeq;
	IplImage* m_pbImage;
	IplImage* m_pbMotion;
	BOOL bAtLeastOneObject;
	int nFrameCounter;
	BITMAPINFOHEADER m_Bih;
	structMovingTarget** m_ppstructArr;
public:
	Bradski_Devis(void);
	//Bradski_Devis(__int64 nStruct);
	~Bradski_Devis(void);
	void Update_IplInvMetaResize(BYTE* img,BYTE* dst,int diff_threshold,__int64 nStruct,BOOL nMetaRequired,double dTimeStamp);
	void  update_mhiInvMetaResize( IplImage* img, IplImage* dst, int diff_threshold,BOOL nMetaRequired,double dTimeStamp);
	//void SetMinObjSize(int nSize){m_nMinObjectSize=nSize;}
	void SetMinObjSize(double dSize){m_dMinObjectSize=dSize;}
	//int GetMinObjSize(void){return m_nMinObjectSize;}
	double GetMinObjSize(void){return m_dMinObjectSize;}
	void ReleaseArrayOfStructures();
	int ResetStatistics(const IplImage* img, int IsWideScreen);
	int m_nBigObjNum;
	//variables for history length configuration
	double m_dMHIDuration;
	double m_dMHIMinTimeDelta;
	double m_dMHIMaxTimeDelta;
	int m_nStatLength;
private:
	void FreeRingBuffer(void);
	bool PutImageInBuffer(const IplImage* img, bool reset);

	// return linesize of image
	void InitImageProcessingParams(const BITMAPINFOHEADER & bih);

private:
	//int m_nMinObjectSize;
	std::vector<char> m_tmpColorBuf;
	double m_dMinObjectSize;
	int m_nChannelNumber;

	std::pair< IppiResizeSpec_32f *, Ipp8u* > m_inResizeFilter, m_outResizeFilter;

	IppStatus (__STDCALL *m_ippiMirror_8u)(const Ipp8u*, int, Ipp8u*, int, IppiSize, IppiAxis);
	IppStatus (__STDCALL *m_ippiRGBToGray_8u)(const Ipp8u*, int, Ipp8u*, int, IppiSize);
	IppStatus (__STDCALL *m_ippiResizeLanczos_8u)(const Ipp8u*, Ipp32s, Ipp8u*, Ipp32s, IppiPoint, IppiSize, IppiBorderType, Ipp8u*, IppiResizeSpec_32f*, Ipp8u*);
};

