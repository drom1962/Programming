#ifndef _DETECTOR_02_H_
#define _DETECTOR_02_H_

#include "globals.h"

/*! function GetBMIH
/param ������ ��������
/param ������ ��������
/result ��������� �� ��������� ������ ������������� ����������
*/
__int64 __stdcall GetBMIH(int nWidth,int nHeight);

/*! function ReleaseBMIH
/param ��������� �� ��������� ������ ������������� ����������
/result 1 � ������ ��������� ������������ �������� ��� -1 �� ���� ��������� �������
*/
int __stdcall ReleaseBMIH(__int64 nObject);

/*! function GetNumberOfMovingObjects
/param ��������� �� ������ � ���� ������������� ����������
/result ����� ������� � ������
*/
int __stdcall GetNumberOfMovingObjects(__int64 nObject);

/*! function GetArrayOfMovingObjects
/param ��������� �� ������ � ���� ������������� ����������
/result ��������� �� ������ �������� � ���� ������������� ����������
*/
__int64 __stdcall GetArrayOfMovingObjects(__int64 nObject);

/*! function ReleaseArrayOfMovingObjects
/param ��������� �� ������ ��������
/result 1 � ������ ��������� ������������ �������� ��� -1 �� ���� ��������� �������
*/
int __stdcall ReleaseArrayOfMovingObjects(__int64 nObject);

/*! function GetArrayOfByte
/param ������ ������� � ������
/result ��������� �� ������
*/
BYTE* __stdcall GetArrayOfByte(int nSize);

/*! function ReleaseArrayOfByte
/param ��������� �� ������
/result 1 � ������ ��������� ������������ �������� ��� -1 �� ���� ��������� �������
*/
int __stdcall ReleaseArrayOfByte(BYTE* pbArray);

/*! function CreateDetector02
/result ����� ��������� ������ ������������� ����������
*/
__int64 __stdcall CreateDetector02();

/*! function InitDetector02
/param ����� ��������� ������ ������������� ����������
/param ����������������� ������� ���������� ��������, �
/param ����������� ����� ��������, �� ������� ��������� ��������, �
/param ������������ ����� ��������, �� ������� ��������� ��������, �
/param ���������� ������, ������������ ��� ����������� �� �������
/result 1 � ������ ��������� ���������� ��� -1 �� ���� ��������� �������
*/
int __stdcall InitDetector02(__int64 nObject, double dDuration, double dMin,double dMax,int nStatLength);

/*! function UpdateDetector02InverseMetaResize
/param ����� ��������� ������ ������������� ����������
/param ��������� �� 24-������ ������ � �������� ������
/param ��������� �� 24-������ ������ � ������ ��� ����� ������������ ����������
/param ������������� ���������� �� ��������� ������ (�� ��������� 30)
/param ����� ��������� �� ��������� BITMAPINFOHEADER ������ ������������� ����������
/param 1, ���� ���������� ������������ ������� ���������� �������� �����, 0 � �������� ������
/param ���������� ���� double ��� �������� �������, ���������� � ������� ������� �������� (������������)
/result 1 � ������ ��������� ���������� ��� -1 �� ���� ��������� �������
��� ������� - DIB, � ������ 14 ���� �������� �� ��������� BITMAPFILEHEADER � 40 ���� ��������� ��� BITMAPINFOHEADER, � ��� ��������� ��� ����� �������� (����������� ������������� ���������� JPEGConverter)
�� ������ ������� 40 ���� ��������� ��� BITMAPINFOHEADER, � ��� ��������� ��� ����� ��������
*/

int __stdcall UpdateDetector02InverseMetaResize(__int64 nDetector,BYTE* pbSource,BYTE* pbDestination,int nTreashold,__int64 nStruct,BOOL nMetaRequired,double dTimeStamp);

/*! function ReleaseDetector02
/param ����� ��������� ������ ������������� ����������
/result 1 � ������ ��������� ������������ �������� ��� -1 �� ���� ��������� �������
*/
int __stdcall ReleaseDetector02(__int64 nDetector);

/*! function IsMovingDetector02
/param ����� ��������� ������ ������������� ����������
/result 1 � ������ ������� ������������ ���������� ��� 0 �� ���� ��������� �������
*/

int __stdcall IsMovingDetector02(__int64 nDetector);

/*! function SetMinObjSizeDetector02
/param ����� ��������� ������ ������������� ����������
/param ����������� ������ ��������� ������� ������ ���������� ���� double (���� �� ������� �����, ���������� � ���������)
/result 1 � ������ ������� ������������ ���������� ��� 0 �� ���� ��������� �������
*/

int __stdcall SetMinObjSizeDetector02(__int64 nDetector,double dSize);

/*! function GetMinObjSizeDetector02
/param ����� ��������� ������ ������������� ����������
/result ����������� ������ ��������� ������� ������ ���������� ���� double (���� �� ������� �����, ���������� � ���������), ��� -1 �� ���� ��������� �������
*/

double __stdcall GetMinObjSizeDetector02(__int64 nDetector);

/*! function GetNumberOfMovingObjects
/param ��������� �� ������ � ���� ������������� ����������
/param 1 - ������ 16:9, 0 - ������ 4:3
/result 0 � ������ ������, -1 �� ���� ��������� �������
*/

int __stdcall ResetStatisticsDetector02(__int64 nObject,int nIsWideScreen);

#endif //_DETECTOR_02_H_