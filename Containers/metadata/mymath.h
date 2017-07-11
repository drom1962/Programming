#ifndef _MyMath_
#define _MyMath_

#include "..\..\Containers\ovi\metadata.h"

struct Rectangl
	{
	Point	P1;
	Point   P2;
	};


class MyMath
{
private:
	Point		m_p1,
				m_p2;

	Rectangl	m_Box;


	double		m_a,
				m_b;

	double		m_angle;

public:
	MyMath(Point p1, Point p2);

	void Box(Point p1, Point p2);
	
	Point intersection(Point p1, Point p2, Point p3, Point p4);

	Point intersection(Point p1, Point p2);

	double S(Point p1, Point p2, Point p3);

	double Association(Point P11, Point P12, Point P21, Point P22);

	double Association(Point P21, Point P22);
	
	double Association(Point P11, Point P12, MovingTarget2 *);

	double Association(MovingTarget2 *);

	double func(int x);


};

#endif