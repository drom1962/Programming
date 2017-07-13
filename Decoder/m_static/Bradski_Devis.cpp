#include "Bradski_Devis.h"

//#include <iostream>
#include <ctype.h>
#include <time.h>
#include <ipp.h>
#include <ippi.h>

using namespace cv;
using namespace std;

// various tracking parameters (in seconds)
const double MHI_DURATION = 1;
const double MAX_TIME_DELTA = 0.5;
const double MIN_TIME_DELTA = 0.05;
// number of cyclic frame buffer used for motion detection
// (should, probably, depend on FPS)
//const int N = 4;

// ring image buffer
//IplImage **buf = 0;
//int last = 0;

//// temporary images
//IplImage *mhi = 0; // MHI
//IplImage *orient = 0; // orientation
//IplImage *mask = 0; // valid orientation mask
//IplImage *segmask = 0; // motion segmentation map
//CvMemStorage* storage = 0; // temporary storage

IppStatus InitResizeCoefs(IppiSize srcSize, IppiSize dstSize, Ipp32u numChannels, IppiResizeSpec_32f *& pOutSpec, Ipp8u* &pOutBuffer)
{
	IppiResizeSpec_32f * pSpec = NULL;
	Ipp8u* pBuffer = NULL;
	Ipp32s specSize = 0, initSize = 0, bufSize = 0;
	/* Spec and init buffer sizes */
	ippiResizeGetSize_8u(srcSize, dstSize, ippLanczos, 0, &specSize, &initSize);

	Ipp8u* pInitBuf = ippsMalloc_8u(initSize);
	pSpec = (IppiResizeSpec_32f*)ippsMalloc_8u(specSize);
	/* Filter initialization */
	IppStatus status = ippiResizeLanczosInit_8u(srcSize, dstSize, 3, pSpec, pInitBuf);
	ippsFree(pInitBuf);

	if (status != ippStsNoErr) {
		ippsFree(pSpec);
		return status;
	}
	/* work buffer size */
	status = ippiResizeGetBufferSize_8u(pSpec, dstSize, numChannels, &bufSize);

	if (status != ippStsNoErr) {
		ippsFree(pSpec);
		return status;
	}
	pBuffer = ippsMalloc_8u(bufSize);

	if (!pBuffer) {
		ippsFree(pSpec);
		return ippStsNoMemErr;
	}
	ippsFree(pOutSpec);
	ippFree(pOutBuffer);
	pOutSpec = pSpec;
	pOutBuffer = pBuffer;

	return ippStsNoErr;
}


Bradski_Devis::Bradski_Devis(void)
{
	m_pSeq=NULL;
	m_dMinObjectSize=50;
	m_pbImage=NULL;
	m_pbMotion=NULL;
	m_ppstructArr=NULL;
	this->m_nBigObjNum=0;
	m_dMHIDuration=1.0;
	m_dMHIMinTimeDelta=0.05;
	m_dMHIMaxTimeDelta=0.5;
	m_nStatLength=4;

	mhi=NULL;
	buf=NULL;
	orient=NULL;
	mask=NULL;
	segmask=NULL;
	storage=NULL;
	bAtLeastOneObject=FALSE;
	nFrameCounter=0;
	last=0;
	ippInit();
	ippSetNumThreads(1);

	m_ippiMirror_8u = NULL;
	m_ippiRGBToGray_8u = NULL;
	m_ippiResizeLanczos_8u = NULL;
}

void Bradski_Devis::ReleaseArrayOfStructures()
{
	if(m_ppstructArr!=NULL)
	{
		for(int nCounter=0;nCounter<this->m_nBigObjNum;nCounter++)
		{
			DELETE_OBJECT(m_ppstructArr[nCounter]);
		}
	}
	DELETE_ARRAY(this->m_ppstructArr);
}

void Bradski_Devis::FreeRingBuffer(void)
{
	if (buf)
	{
		for( int i = 0; i < m_nStatLength; i++ ) {
			cvReleaseImage( &buf[i] );
		}
		DELETE_ARRAY(buf);
	}
}

Bradski_Devis::~Bradski_Devis(void)
{
	if (m_pSeq) {
		cvClearSeq( m_pSeq );
	}
	if (storage) {
		cvClearMemStorage( storage );
	}

	cvReleaseMemStorage( &storage );

	cvReleaseImage( &mhi );
	cvReleaseImage( &orient );
	cvReleaseImage( &segmask );
	cvReleaseImage( &mask );
	cvReleaseImage( &m_pbImage );
	cvReleaseImage( &m_pbMotion );

	FreeRingBuffer();
	ReleaseArrayOfStructures();

	ippFree(m_inResizeFilter.first);
	ippFree(m_inResizeFilter.second);
	ippFree(m_outResizeFilter.first);
	ippFree(m_outResizeFilter.second);
}


int Bradski_Devis::ResetStatistics(const IplImage* img, int nIsWideScreen)
{
	int nResult=0;
	int i=0;
	int nWidth=0;
	int nHeight=0;

	try
	{
		if(nIsWideScreen==1)
		{
			nWidth=400;
			nHeight=225;
		}
		else
		{
			nWidth=xx;
			nHeight=yy;
		}
		if( buf == 0 ) {
			//buf = (IplImage**)malloc(N*sizeof(buf[0]));
			buf = new IplImage * [m_nStatLength];
			//memset( buf, 0, N*sizeof(buf[0]));
			memset( buf, 0, m_nStatLength*sizeof(buf[0]));
		}
		cvReleaseImage( &mhi );
		cvReleaseImage( &orient );
		cvReleaseImage( &segmask );
		cvReleaseImage( &mask );

		if (img) {
			CvSize size = cvSize(nWidth,nHeight); // get current frame size
			//for( i = 0; i < N; i++ ) {
			for( i = 0; i < m_nStatLength; i++ ) {
				cvReleaseImage( &buf[i] );
				buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
				//cvZero( buf[i] );
			}
			mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
			segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
			orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
			mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
			//cvZero( mhi ); // clear MHI at the beginning

			nResult = PutImageInBuffer(img, true) ? 0 : -1;
		}
	}
	catch(...)
	{
		nResult = -1;
	}
	return nResult;
}

// save image in buffer (without change position)
// if reset = true then image copy in all frames of buffer
bool Bradski_Devis::PutImageInBuffer(const IplImage* img, bool reset)
{
	ippSetNumThreads(1);
	IppiSize ipSize = {img->width, img->height};
	Ipp8u* ippBufColor = NULL;
	bool result = true;

	if (!img) {
		return false;
	}
	try
	{
		// allocate once (realloc if need)
		m_tmpColorBuf.resize(img->imageSize);
		ippBufColor = reinterpret_cast<Ipp8u*>(&m_tmpColorBuf[0]);

		if (m_ippiMirror_8u) {
			m_ippiMirror_8u(reinterpret_cast<Ipp8u*>(img->imageData), img->widthStep, ippBufColor, img->widthStep, ipSize, ippAxsHorizontal);
		} else {
			ippsCopy_8u(reinterpret_cast<Ipp8u*>(img->imageData), ippBufColor, img->imageSize);
		}
		IplImage const * const & cur_gray_img = buf[last];
		// convert RGB to gray (if need)
		if (m_ippiRGBToGray_8u) {
			m_ippiRGBToGray_8u(ippBufColor, img->widthStep, reinterpret_cast<Ipp8u*>(cur_gray_img->imageData), cur_gray_img->width, ipSize);
		} else {
			ippsCopy_8u(ippBufColor, reinterpret_cast<Ipp8u*>(cur_gray_img->imageData), cur_gray_img->imageSize);
		}
		if (reset) {
			for ( int i = 0; i < m_nStatLength; i++ ) {
				if (i != last)
					ippsCopy_8u(reinterpret_cast<Ipp8u*>(cur_gray_img->imageData),
								reinterpret_cast<Ipp8u*>(buf[i]->imageData),
								buf[i]->imageSize);
			}
		}
	}
	catch (...)
	{
		result = false;
	}
	return result;
}



void Bradski_Devis::InitImageProcessingParams(const BITMAPINFOHEADER & bih)
{
	//unsigned nSize;
	unsigned bitcount;
	if (BI_RGB == bih.biCompression) {
		bitcount = bih.biBitCount;
	}
	else {
		bitcount = 0;    // throw exception later
	}

	switch (bitcount)
	{
	case 32:
		m_ippiResizeLanczos_8u = ippiResizeLanczos_8u_C4R;
		m_ippiMirror_8u = ippiMirror_8u_AC4R;
		m_ippiRGBToGray_8u = ippiRGBToGray_8u_AC4C1R;
		//
		//nSize = bih.biWidth * 4 * ::abs(bih.biHeight);
		break;
	case 24:
		m_ippiResizeLanczos_8u = ippiResizeLanczos_8u_C3R;
		m_ippiMirror_8u = ippiMirror_8u_C3R;
		m_ippiRGBToGray_8u = ippiRGBToGray_8u_C3C1R;
		//
		//nSize = (((bih.biWidth * 3) + 3) & 0xFFFC) * ::abs(bih.biHeight);
		break;
	case 8:
		m_ippiResizeLanczos_8u = ippiResizeLanczos_8u_C1R;
		m_ippiMirror_8u = ippiMirror_8u_C1R;
		m_ippiRGBToGray_8u = NULL;   // transform no need
		//
		//nSize = bih.biWidth * ::abs(bih.biHeight);
		break;
	default:
		throw std::runtime_error("Unknown bitmap format");
	}
	if (bih.biHeight > 0) {
		m_ippiMirror_8u = NULL;
	}
	m_nChannelNumber = bitcount / 8;
}


void Bradski_Devis::Update_IplInvMetaResize(BYTE* img, BYTE* dst, int diff_threshold, __int64 nStruct,BOOL nMetaRequired,double dTimeStamp)
{
	ippSetNumThreads(1);
	int nResult=0;
	BITMAPINFOHEADER* pStruct=NULL;
	pStruct=(BITMAPINFOHEADER*)nStruct;

	InitImageProcessingParams(*pStruct);

	const int original_height( ::abs(pStruct->biHeight) );
	IppiSize srcSize = {pStruct->biWidth, original_height};
	Ipp32s srcStep = pStruct->biWidth * m_nChannelNumber;
	IppiSize dstSize = {0};
	Ipp32s dstStep = 0;

	double dRatio = (1.0*pStruct->biWidth) / original_height;
	//!!!nIsWideScreen is always equal to 0 now!!!
	const int nIsWideScreen = 0;//(dRatio >= 1.6);

	if(nIsWideScreen) {
		dstSize.width = 400;
		dstSize.height = 225;
	}
	else {
		dstSize.width = xx;
		dstSize.height = yy;
	}
	dstStep = dstSize.width * m_nChannelNumber;

	BOOL bNeedRefresh=FALSE;

	if((m_Bih.biWidth!=srcSize.width)||(m_Bih.biHeight!=srcSize.height))
		{
		cvReleaseImage(&m_pbImage);
		cvReleaseImage(&m_pbMotion);
		cvReleaseImage( &mhi );
		FreeRingBuffer();
		bNeedRefresh=TRUE;
		}

	memcpy(&m_Bih,pStruct,sizeof(BITMAPINFOHEADER));

	m_Bih.biHeight = original_height;

	if (!m_pbImage || bNeedRefresh==TRUE)
		{
		m_pbImage = cvCreateImage( cvSize(dstSize.width, dstSize.height), 8, m_nChannelNumber );
		//cvZero( m_pbImage );  //  999
		InitResizeCoefs(srcSize, dstSize, m_nChannelNumber, m_inResizeFilter.first, m_inResizeFilter.second);
		}

	if (!m_pbMotion || bNeedRefresh==TRUE)
		{
		m_pbMotion = cvCreateImage( cvSize(dstSize.width, dstSize.height), 8, m_nChannelNumber );
	    //cvZero( m_pbMotion );  // 999
		InitResizeCoefs(dstSize, srcSize, m_nChannelNumber, m_outResizeFilter.first, m_outResizeFilter.second);
		//mhi=NULL;
		//buf=0;
		//orient=0;
		//mask=0;
		//segmask=0;
		//storage=NULL;
		bAtLeastOneObject=FALSE;
		nFrameCounter=0;
		last=0;
		//motion->origin = image->origin;
		}
	
	BYTE* pbPicture=(BYTE*)(img) + reinterpret_cast<PBITMAPFILEHEADER>(img)->bfOffBits;
	//Prepare IplImage for source picture
	BYTE* pbFrame = (BYTE*)this->m_pbImage->imageData;
	
	int nInnerCounter=0;
	int nOuterCounter=0;
	int nStandardCounter=0;
	//memcpy(pbFrame,pbPicture,921600);
	//copy image bytes to local temporary buffer (just pixels)
	//memcpy(pbSrcPicture+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER),pbPicture,nSize);


	//local buffer has been filled (just pixel part)
	//now we have to resize image to 160*90 and fill IplImage
	//IppiSize dstSize={400,225};
	Ipp32u numChannels = m_nChannelNumber;
	IppiPoint dstOffset = {0, 0};
	IppStatus status = ippStsNoErr;
	IppiBorderType border = ippBorderRepl;
	if((srcSize.width==xx)&&(srcSize.height==yy))
		{
		const int frameSize = this->m_pbImage->imageSize;
		ippsCopy_8u(pbPicture, pbFrame, frameSize);
		//perform MHI update procedure
		update_mhiInvMetaResize(this->m_pbImage, dst ? this->m_pbMotion : NULL, diff_threshold, nMetaRequired, dTimeStamp);
		ippsCopy_8u(reinterpret_cast<BYTE*>(this->m_pbMotion->imageData), dst, frameSize);
		}
	else
		{
		/* Resize processing (Finally fill IplImage with data)*/
		status = m_ippiResizeLanczos_8u(pbPicture, srcStep, pbFrame, dstStep, dstOffset, dstSize, border, 0, m_inResizeFilter.first, m_inResizeFilter.second);
		
		char File[128], File1[128];;
		sprintf_s(File,"F:\\%I64d.jpg", dTimeStamp);
		//cvSaveImage(File, m_pbImage);

		//(already done)Finally fill IplImage with data
		//(already done)memcpy(pbFrame,pbSrcPicture+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER),nSize);
		update_mhiInvMetaResize(this->m_pbImage, dst ? this->m_pbMotion : NULL, diff_threshold, nMetaRequired, dTimeStamp); //999




		//Now we have motion map inside 160*90 RGB image without BITMAPFILEHEADER and BITMAPINFOHEADER
		//We have to resize it to initial dimentions from BITMAPINFOHEADER
		//and copy to pbDstPicture
		std::swap(srcSize, dstSize);
		std::swap(srcStep, dstStep);
		border = ippBorderRepl;

		/* Resize processing (Finally fill IplImage with data)*/
		status = m_ippiResizeLanczos_8u(reinterpret_cast<BYTE*>(this->m_pbMotion->imageData), srcStep, dst, dstStep, dstOffset, dstSize, border, 0, m_outResizeFilter.first, m_outResizeFilter.second);
		}

	//if(dst!=NULL)
	//{
	//copy image bytes to motion map DIB
	//memcpy(pbPicture02,this->m_pbMotion->imageData,nSize);
	//copy all bytes (BITMAPINFOHEADER+image bytes) from temporary buffer for motion map to actual pointer
	//memcpy(dst,pbDstPicture,nSize+40);
	//memcpy(dst,pbDstPicture+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER),nSize);
	//memcpy(dst,pbSrcPicture+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER),nSize);
	//}
	//release temporary buffers
	//DELETE_ARRAY(pbSrcPicture);
	//DELETE_ARRAY(pbDstPicture);
}

void Bradski_Devis::update_mhiInvMetaResize(IplImage* img, IplImage* dst, int diff_threshold,BOOL nRequired,double dTimeStamp)
{
	ippSetNumThreads(1);
	bAtLeastOneObject=FALSE;
	//double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
	double timestamp = dTimeStamp/CLOCKS_PER_SEC; // get current time in seconds
	CvSize size = cvSize(img->width,img->height); // get current frame size
	int i, idx1 = last, idx2;
	IplImage* silh=NULL;
	CvRect comp_rect;
	double count=0;
	double angle=0;
	CvPoint center;
	double magnitude=0;
	CvScalar color;

	double dRatioX = 1.0*m_Bih.biWidth / img->width;
	double dRatioY = 1.0*m_Bih.biHeight / img->height;

	char File[128], File1[128];;
	sprintf_s(File, "F:\\%I64d_1.jpg", dTimeStamp);
	//cvSaveImage(File, img);

	// показываем картинку
	//cvShowImage("original", img);
	//cvWaitKey(1);


	// allocate images at the beginning or
	// reallocate them if the frame size is changed
	if( !mhi || mhi->width != size.width || mhi->height != size.height )
	{
		this->ResetStatistics(img, (img->width == xx) ? 0 : 1);
	}
	else {
		this->PutImageInBuffer(img, false);
	}
	IppiSize ipSize={img->width,img->height};

	//idx2 = (last + 1) % N; // index of (last - (N-1))th frame
	idx2 = (last + 1) % m_nStatLength;
	last = idx2;

	silh = buf[idx2];
	cvAbsDiff( buf[idx1], buf[idx2], silh ); // get difference between frames

	//ippiThreshold_Val_8u_C1IR((Ipp8u*)silh->imageData,silh->widthStep,ipSize,diff_threshold,0,ippCmpLess); // and threshold it
	ippiThreshold_Val_8u_C1IR(reinterpret_cast<Ipp8u*>(silh->imageData),silh->widthStep,ipSize,diff_threshold,0,ippCmpLess); // and threshold it
	//cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); // update MHI
	cvUpdateMotionHistory( silh, mhi, timestamp, m_dMHIDuration ); // update MHI

	// convert MHI to blue 8u image
	//cvCvtScale( mhi, mask, 255./MHI_DURATION,(MHI_DURATION - timestamp)*255./MHI_DURATION );
	//cvCvtScale( mhi, mask, 255./m_dMHIDuration,(m_dMHIDuration - timestamp)*255./m_dMHIDuration );
	const double cvtScaleCoef = 255./m_dMHIDuration;
	cvCvtScale( mhi, mask, cvtScaleCoef, (m_dMHIDuration - timestamp)*cvtScaleCoef );
	if(dst)
	{
		cvZero( dst );
		cvMerge( mask, 0, 0, 0, dst );
	}

	// calculate motion gradient orientation and valid orientation mask
	//cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 );
	cvCalcMotionGradient( mhi, mask, orient, m_dMHIMaxTimeDelta, m_dMHIMinTimeDelta, 3 );

	if( !storage )
		storage = cvCreateMemStorage(0);
	else
		cvClearMemStorage(storage);

	// segment motion: get sequence of motion components
	// segmask is marked motion components map. It is not used further
	//m_pSeq = cvSegmentMotion( mhi, segmask, storage, timestamp, MAX_TIME_DELTA );
	m_pSeq = cvSegmentMotion( mhi, segmask, storage, timestamp, m_dMHIMaxTimeDelta );
	if (nRequired==TRUE)
	{
		this->m_ppstructArr=new structMovingTarget*[m_pSeq->total];
		memset(m_ppstructArr,0,m_pSeq->total);
	}
	else
	{
		this->m_ppstructArr=NULL;
	}
	int nStructCounter=0;

	// iterate through the motion components,
	// One more iteration (i == -1) corresponds to the whole image (global motion)
	const double imageSquare = img->width * img->height;

	for( i = -1; i < m_pSeq->total; i++ )
		{
		if( i < 0 ) 
			{ // case of the whole image
			comp_rect = cvRect( 0, 0, size.width, size.height );
			color = CV_RGB(255,255,255);
			magnitude = 100;
			}
		else 
			{ // i-th motion component
			comp_rect = ((CvConnectedComp*)cvGetSeqElem( m_pSeq, i ))->rect;
			//if( comp_rect.width + comp_rect.height < m_nMinObjectSize ) // reject very small components
			//if( 1.0*comp_rect.width*comp_rect.height/img->width/img->height < m_dMinObjectSize ) // reject very small components
			if( static_cast<double>(comp_rect.width * comp_rect.height) / imageSquare < m_dMinObjectSize ) // reject very small components
				continue;
			else
				{
				//nPairCounter++;
				bAtLeastOneObject=TRUE;
				//cout << "MOVEMENT!!!";
				}
			color = CV_RGB(255,0,0);
			magnitude = 10;
			}

		// select component ROI
		cvSetImageROI( silh, comp_rect );
		cvSetImageROI( mhi, comp_rect );
		cvSetImageROI( orient, comp_rect );
		cvSetImageROI( mask, comp_rect );

		// calculate orientation
		//angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, MHI_DURATION);
		//angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, m_dMHIDuration);
		//angle = 360.0 - angle;  // adjust for images with top-left origin

		count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

		cvResetImageROI( mhi );
		cvResetImageROI( orient );
		cvResetImageROI( mask );
		cvResetImageROI( silh );

		// check for the case of little motion
		if( count < comp_rect.width*comp_rect.height * 0.05 )
			continue;

		angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, m_dMHIDuration);
		angle = 360.0 - angle;  // adjust for images with top-left origin

		// draw a clock with arrow indicating the direction
		center = cvPoint( (comp_rect.x + comp_rect.width/2),
			(comp_rect.y + comp_rect.height/2) );
		if(dst)
			{
			cvCircle( dst, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
			cvLine( dst, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
				cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );
			}

		if(nRequired==TRUE && i>=0)
			{
			m_ppstructArr[nStructCounter]=new structMovingTarget();
			m_ppstructArr[nStructCounter]->nTopLeftX=comp_rect.x*dRatioX;
			m_ppstructArr[nStructCounter]->nTopLeftY=comp_rect.y*dRatioY;
			m_ppstructArr[nStructCounter]->nBottomRightX=(comp_rect.x+comp_rect.width)*dRatioX;
			m_ppstructArr[nStructCounter]->nBottomRightY=(comp_rect.y+comp_rect.height)*dRatioY;
			m_ppstructArr[nStructCounter]->nCenterX = center.x * dRatioX;
			m_ppstructArr[nStructCounter]->nCenterY = center.y * dRatioY;

			
			if(dRatioY==dRatioX)
			{
				m_ppstructArr[nStructCounter]->angle=angle;
			}
			else
			{
				float dRadianAngle=0.0;
				dRadianAngle=angle*CV_PI/180;
				float dModifiedRadianAngle=0.0;
				float dModifiedAngle=0.0;
				float dModRatio02=1.0*m_Bih.biHeight/240;
				float dModRatio01=1.0*m_Bih.biWidth/320;
				float dModRatio=1.0*dModRatio02/dModRatio01;
				if((angle>0.0)&&(angle<=90.0))
				{
					dModifiedRadianAngle=asin(dModRatio*sin(dRadianAngle));
				}
				if((angle>90)&&(angle<=180))
				{
					dModifiedRadianAngle=CV_PI-asin(dModRatio*sin(dRadianAngle));
				}
				if((angle>180)&&(angle<=270))
				{
					dModifiedRadianAngle=CV_PI+asin(-1.0*dModRatio*sin(dRadianAngle));
				}
				if((angle>270)&&(angle<=360))
				{
					dModifiedRadianAngle=2*CV_PI-asin(-1.0*dModRatio*sin(dRadianAngle));
				}
				dModifiedAngle=dModifiedRadianAngle*180/CV_PI;
				
				m_ppstructArr[nStructCounter]->angle=360.; //dModifiedAngle;
			}
			nStructCounter++;
			}
		//if (i>-1) bAtLeastOneObject=TRUE;
	}
	this->m_nBigObjNum=nStructCounter;
}