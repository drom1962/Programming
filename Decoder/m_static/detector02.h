#ifndef _DETECTOR_02_H_
#define _DETECTOR_02_H_

#include "globals.h"

/*! function GetBMIH
/param Ширина картинки
/param Высота картинки
/result Указатель на структуру внутри целочисленной переменной
*/
__int64 __stdcall GetBMIH(int nWidth,int nHeight);

/*! function ReleaseBMIH
/param Указатель на структуру внутри целочисленной переменной
/result 1 в случае успешного освобождения ресурсов или -1 во всех остальных случаях
*/
int __stdcall ReleaseBMIH(__int64 nObject);

/*! function GetNumberOfMovingObjects
/param Указатель на объект в виде целочисленной переменной
/result Длина массива в штуках
*/
int __stdcall GetNumberOfMovingObjects(__int64 nObject);

/*! function GetArrayOfMovingObjects
/param Указатель на объект в виде целочисленной переменной
/result Указатель на массив структур в виде целочисленной переменной
*/
__int64 __stdcall GetArrayOfMovingObjects(__int64 nObject);

/*! function ReleaseArrayOfMovingObjects
/param Указатель на массив структур
/result 1 в случае успешного освобождения ресурсов или -1 во всех остальных случаях
*/
int __stdcall ReleaseArrayOfMovingObjects(__int64 nObject);

/*! function GetArrayOfByte
/param Размер массива в байтах
/result Указатель на массив
*/
BYTE* __stdcall GetArrayOfByte(int nSize);

/*! function ReleaseArrayOfByte
/param Указатель на массив
/result 1 в случае успешного освобождения ресурсов или -1 во всех остальных случаях
*/
int __stdcall ReleaseArrayOfByte(BYTE* pbArray);

/*! function CreateDetector02
/result Копия указателя внутри целочисленной переменной
*/
__int64 __stdcall CreateDetector02();

/*! function InitDetector02
/param Копия указателя внутри целочисленной переменной
/param Продолжительность истории статистики движения, с
/param Минимальное время движения, на которое реагирует детектор, с
/param Максимальное время движения, на которое реагирует детектор, с
/param Количество кадров, используемое для сегментации на объекты
/result 1 в случае успешного выполнения или -1 во всех остальных случаях
*/
int __stdcall InitDetector02(__int64 nObject, double dDuration, double dMin,double dMax,int nStatLength);

/*! function UpdateDetector02InverseMetaResize
/param Копия указателя внутри целочисленной переменной
/param Указатель на 24-битный битмап с исходным кадром
/param Указатель на 24-битный битмап с кадром для карты двигательной активности
/param Целочисленная переменная со значением порога (по умолчанию 30)
/param Копия указателя на структуру BITMAPINFOHEADER внутри целочисленной переменной
/param 1, если необходимо формирование массива метаданных текущего кадра, 0 в обратном случае
/param переменная типа double для передачи времени, прошедшего с момента запуска процесса (миллисекунды)
/result 1 в случае успешного выполнения или -1 во всех остальных случаях
Оба массива - DIB, в первом 14 байт отведены на структуру BITMAPFILEHEADER и 40 байт отвеедены под BITMAPINFOHEADER, а все следующие под байты картинки (последствия использования библиотеки JPEGConverter)
Во втором массиве 40 байт отвеедены под BITMAPINFOHEADER, а все следующие под байты картинки
*/

int __stdcall UpdateDetector02InverseMetaResize(__int64 nDetector,BYTE* pbSource,BYTE* pbDestination,int nTreashold,__int64 nStruct,BOOL nMetaRequired,double dTimeStamp);

/*! function ReleaseDetector02
/param Копия указателя внутри целочисленной переменной
/result 1 в случае успешного освобождения ресурсов или -1 во всех остальных случаях
*/
int __stdcall ReleaseDetector02(__int64 nDetector);

/*! function IsMovingDetector02
/param Копия указателя внутри целочисленной переменной
/result 1 в случае наличия двигательной активности или 0 во всех остальных случаях
*/

int __stdcall IsMovingDetector02(__int64 nDetector);

/*! function SetMinObjSizeDetector02
/param Копия указателя внутри целочисленной переменной
/param Минимальный размер значимого объекта внутри переменной типа double (доля от площади кадра, выраженная в процентах)
/result 1 в случае наличия двигательной активности или 0 во всех остальных случаях
*/

int __stdcall SetMinObjSizeDetector02(__int64 nDetector,double dSize);

/*! function GetMinObjSizeDetector02
/param Копия указателя внутри целочисленной переменной
/result Минимальный размер значимого объекта внутри переменной типа double (доля от площади кадра, выраженная в процентах), или -1 во всех остальных случаях
*/

double __stdcall GetMinObjSizeDetector02(__int64 nDetector);

/*! function GetNumberOfMovingObjects
/param Указатель на объект в виде целочисленной переменной
/param 1 - формат 16:9, 0 - формат 4:3
/result 0 в случае успеха, -1 во всех остальных случаях
*/

int __stdcall ResetStatisticsDetector02(__int64 nObject,int nIsWideScreen);

#endif //_DETECTOR_02_H_