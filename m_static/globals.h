#pragma once
#include <Windows.h>

// workaround от дублирования structMovingTarget из-за детектора цвета
#ifndef _DEFINED_DETECTOR_MOVING_TARGET_
#define _DEFINED_DETECTOR_MOVING_TARGET_
struct structMovingTarget
{
	int nCenterX;
	int nCenterY;
	int nTopLeftX;
	int nTopLeftY;
	int nBottomRightX;
	int nBottomRightY;
	double angle;
};
#endif //_DEFINED_DETECTOR_MOVING_TARGET_


#define DELETE_ARRAY(x)  if (x != NULL) {delete[] x; x = NULL;}
#define DELETE_OBJECT(x) if (x != NULL) {delete x;x=NULL;}
