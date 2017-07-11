#include <math.h> 

#include "..\metadata\mymath.h"

MyMath::MyMath(Point p1, Point p2)
	{
	m_p1 = p1;
	m_p2 = p2;

	m_a = (m_p2.y - m_p1.y) / (m_p2.x - m_p1.x);
	m_b = m_p2.y - m_a * m_p2.x;

	m_angle = (atan((m_p2.y - m_p1.y) / (m_p2.x - m_p1.x))*180.0)/ 3.14159265358979323846;
	}

//
//  ������� ������� ������
//
void MyMath::Box(Point p1, Point p2)
	{
	m_Box.P1 = p1;
	m_Box.P2 = p2;
	}


//
//		�������� �� �����
//
double MyMath::func(int x)
	{
	return m_a*x + m_b;
	}

//  ��������� ���� ������ �� ���� ������
//
// https://ru.wikipedia.org/wiki/%D0%9F%D0%B5%D1%80%D0%B5%D1%81%D0%B5%D1%87%D0%B5%D0%BD%D0%B8%D0%B5_%D0%BF%D1%80%D1%8F%D0%BC%D1%8B%D1%85
//
Point MyMath::intersection(Point p1, Point p2,Point p3,Point p4)
	{
	Point res;

	double	d1 = p1.x*p2.y - p1.y*p2.x,

			d2 = p3.x*p4.y - p3.y*p4.x,

			ss = (p1.x - p2.x)*(p3.y - p4.y) - (p1.y - p2.y)*(p3.x - p4.x);

	res.x = (d1*(p3.x - p4.x) - (p1.x - p2.x)*d2) / ss;

	res.y = (d1*(p3.y - p4.y) - (p1.y - p2.y)*d2) / ss;

	return res;
	}

Point MyMath::intersection(Point p1, Point p2)
	{
	return intersection(p1,p2,m_p1,m_p2);
	}


double MyMath::S(Point p1, Point p2, Point p3)
	{

	double s = 0.5*((p1.x - p3.x)*(p2.y - p3.y) - (p2.x - p3.x)*(p1.y - p3.y));

	return s;


	}

//
//		������������ ��������������
//   (P11,P12) - �������� �������
//   (P21,P22) - ����������� �������
//

template <typename Type> double Association2(Type P11, Type P12, Type P21, Type P22)
	{
	Type XY1, XY2;

	if (P21.x < P11.x)	    XY1.x = P11.x;   // ����� �������
	else					XY1.x = P21.x;

	if (P22.x > P12.x)		XY2.x = P12.x;	// ������ �������		
	else					XY2.x = P22.x;

	if (P21.y < P11.y)		XY1.y = P11.y;	// ������ �������
	else					XY1.y = P21.y;

	if (P22.y > P12.y)      XY2.y = P12.y;	// ������� ������
	else	   				XY2.y = P22.y;

	return (XY2.x - XY1.x)*(XY2.y - XY1.y);	// �������� �������
	}

template <typename Type> double Association3(Type P11, Type P12, MovingTarget2 *MT)
    {
	Type XY1, XY2;

	if (MT->nTop < P11.x)	        XY1.x = P11.x;			 // ����� �������
	else							XY1.x = MT->nTop;

	if (MT->nBottom    > P12.x)		XY2.x = P12.x;	         // ������ �������		
	else			         		XY2.x = MT->nBottom;

	if (MT->nLeft   < P11.y)		XY1.y = P11.y;	         // ������ �������
	else			        		XY1.y = MT->nLeft;

	if (MT->nRight  > P12.y)        XY2.y = P12.y;	// ������� ������
	else	   			        	XY2.y = MT->nRight;

	return (XY2.x - XY1.x)*(XY2.y - XY1.y);	// �������� �������
    }



double MyMath::Association(Point P11, Point P12, Point P21, Point P22)
	{
	if (P21.x > P12.x ||	// ������ ������ �������
		P22.x < P11.x ||	// ������ ����� ������� 
		P21.y > P12.y ||	// ������ ���� ������� 
		P22.y < P11.y)		// ������ ���� ������� 
			{
			// �� ������������
			return 0.0;
			}
		
	return Association2<Point>(P11, P12, P21, P22);
	}


//
//		������������ ��������������
//
double MyMath::Association(Point P21, Point P22)
	{
	if (P21.x > m_Box.P2.x ||	// ������ ������ �������
		P22.x < m_Box.P1.x ||	// ������ ����� ������� 
		P21.y > m_Box.P2.y ||	// ������ ���� ������� 
		P22.y < m_Box.P1.y)		// ������ ���� ������� 
			{
			// �� ������������
			return 0.0;
			}

	// ������������ ��� ���������
	return Association2<Point>(m_Box.P1, m_Box.P2, P21, P22);
	}


//
//		������������ ��������������
//
double MyMath::Association(Point P11, Point P12, MovingTarget2 *MT)
    {
	if (MT->nLeft    > P12.x    ||	// ������ ������ �������
		MT->nRight   < P11.x    ||	// ������ ����� ������� 
		MT->nBottom  > P12.y    ||	// ������ ���� ������� 
		MT->nTop     < P11.y)		// ������ ���� ������� 
			{
			// �� ������������
			return 0.0;
			}
	
	// ������������ ��� ���������
	return Association3<Point>(P11, P12, MT);
    }


//
//		������������ ��������������
//
double MyMath::Association(MovingTarget2 *MT)
    {
	if (MT->nLeft    > m_Box.P2.x	||	// ������ ������ �������
		MT->nRight   < m_Box.P1.x	||	// ������ ����� ������� 
		MT->nBottom  > m_Box.P2.y	||	// ������ ���� ������� 
		MT->nTop     < m_Box.P1.y)		// ������ ���� ������� 
			{
			// �� ������������
			return 0.0;
			}
	
	// ������������ ��� ���������
	return Association3<Point>(m_Box.P1, m_Box.P2, MT);
    }

