#include "globals.h"
#include "detector02.h"
#include "Bradski_Devis.h"
#include <windows.h>
#include <tracking.hpp>


__int64 __stdcall GetBMIH(int nWidth,int nHeight)
{
	__int64 nResult=0;
	BITMAPINFOHEADER* pBih=NULL;
	pBih=new BITMAPINFOHEADER();
	ZeroMemory(pBih, sizeof(BITMAPINFOHEADER));
	//
	pBih->biWidth=nWidth;
	pBih->biHeight=nHeight;
	//

	pBih->biBitCount=24;    //24 bit per pixel
	pBih->biClrImportant=0;
	pBih->biClrUsed = 0;
	pBih->biCompression = BI_RGB;
	pBih->biPlanes = 1;
	pBih->biSize = 40;
	pBih->biXPelsPerMeter = 0;
	pBih->biYPelsPerMeter = 0;
	//calculate total size of RGBQUAD scanlines (DWORD aligned)
	pBih->biSizeImage = (((pBih->biWidth * 3) + 3) & 0xFFFC) * pBih->biHeight ;
	nResult=(__int64) pBih;
	return nResult;
}


int __stdcall ReleaseBMIH(__int64 nObject)
{
	int nResult=0;
	BITMAPINFOHEADER* pBih=NULL;
	pBih=(BITMAPINFOHEADER*)nObject;
	try
	{
		DELETE_OBJECT(pBih);
	}
	catch(...)
	{
			nResult=-1;
	}
	return nResult;
}


int __stdcall GetNumberOfMovingObjects(__int64 nObject)
{
	int nResult=0;
	Bradski_Devis* pDetector=NULL;
	pDetector=(Bradski_Devis*)nObject;
	nResult=pDetector->m_nBigObjNum;
	return nResult;
}


__int64 __stdcall GetArrayOfMovingObjects(__int64 nObject)
{
	__int64 nResult=0;
	structMovingTarget** ppstructArr=NULL;
	Bradski_Devis* pDetector=NULL;
	pDetector=(Bradski_Devis*)nObject;
	ppstructArr=pDetector->m_ppstructArr;
	nResult=(__int64)ppstructArr;
	return nResult;
}


int __stdcall ReleaseArrayOfMovingObjects(__int64 nObject)
{
	int nResult=0;
	Bradski_Devis* pDetector=NULL;
	pDetector=(Bradski_Devis*)nObject;
	try
	{
		pDetector->ReleaseArrayOfStructures();
	}
	catch(...)
	{
			nResult=-1;
	}
	return nResult;
}


BYTE* __stdcall GetArrayOfByte(int nSize)
{
	BYTE* pbArray=NULL;
	pbArray=new BYTE[nSize];
	return pbArray;
}


int __stdcall ReleaseArrayOfByte(BYTE* pbArray)
{
	int nResult=0;
	if(pbArray!=NULL)
	{
		DELETE_OBJECT(pbArray);
	}
	else nResult=-1;
	return nResult;
}


__int64 __stdcall CreateDetector02()
{
	Bradski_Devis* pDetector=NULL;
	pDetector=new Bradski_Devis();
	__int64 nResult=(__int64)pDetector;
	return nResult;
}


int __stdcall InitDetector02(__int64 nObject, double dDuration, double dMin,double dMax,int nStatLength)
{
	Bradski_Devis* pDetector=NULL;
	pDetector=(Bradski_Devis*)nObject;
	int nResult=0;
	try
	{
		pDetector->m_dMHIDuration=dDuration;
		pDetector->m_dMHIMinTimeDelta=dMin;
		pDetector->m_dMHIMaxTimeDelta=dMax;
		pDetector->m_nStatLength=nStatLength;
	}
	catch(...)
	{
		nResult=-1;
	}

	memset(&pDetector->m_Bih , 0, sizeof(BITMAPINFOHEADER));
	return nResult;
}


int __stdcall UpdateDetector02InverseMetaResize(__int64 nDetector,BYTE* pbSource,BYTE* pbDestination,int nTreashold,__int64 nStruct,BOOL nMetaRequired,double dTimeStamp)
{
	int nResult=0;
	Bradski_Devis* pDetector=NULL;
	pDetector=(Bradski_Devis*)nDetector;
	pDetector->Update_IplInvMetaResize(pbSource,pbDestination,nTreashold,nStruct,nMetaRequired,dTimeStamp);
	return nResult;
}


int __stdcall ReleaseDetector02(__int64 nDetector)
{
	Bradski_Devis* pDetector=NULL;
	pDetector=(Bradski_Devis*)nDetector;
	int nResult=0;
	try
	{
		DELETE_OBJECT(pDetector);
	}
	catch(...)
	{
		nResult=-1;
	}
	return nResult;
}


int __stdcall IsMovingDetector02(__int64 nDetector)
{
	int nResult=0;
	Bradski_Devis* pDetector=NULL;
	pDetector=(Bradski_Devis*)nDetector;
	if(pDetector->bAtLeastOneObject==TRUE) nResult=1;
	return nResult;
}


int __stdcall SetMinObjSizeDetector02(__int64 nDetector,double dSize)
	{
	int nResult = 0;
	Bradski_Devis* pDetector=pDetector=(Bradski_Devis*)nDetector;

	double dActualRatio=0.0001*dSize*dSize;
	try
		{
		pDetector->SetMinObjSize(dActualRatio);
		}
	catch(...)
		{
		return -1;
		}

	return 0;
	}


double __stdcall GetMinObjSizeDetector02(__int64 nDetector)
{
	double dResult=0;
	Bradski_Devis* pDetector=NULL;
	pDetector=(Bradski_Devis*)nDetector;
	try
	{
		dResult=pDetector->GetMinObjSize();
		dResult=dResult*10000;
		dResult=sqrt(dResult);
	}
	catch(...)
	{
		dResult=-1.0;
	}
	return dResult;
}


int __stdcall ResetStatisticsDetector02(__int64 nObject,int nIsWideScreen)
{
	int nResult=0;
	Bradski_Devis* pDetector=NULL;
	pDetector=(Bradski_Devis*)nObject;
	nResult=pDetector->ResetStatistics(NULL, nIsWideScreen);
	return nResult;
}