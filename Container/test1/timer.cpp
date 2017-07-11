#include <iostream>
#include <windows.h>

using namespace std;

int main(int argc, char **argv)
{
	LARGE_INTEGER timerFrequency, timerStart, timerStop;

	QueryPerformanceFrequency(&timerFrequency);
	QueryPerformanceCounter(&timerStart);


	for (int i = 0; i < 1000000; i++)

		atan(1111.88);
	//
	//Здесь код, время работы которого замеряем
	//

	QueryPerformanceCounter(&timerStop);
	double const t(static_cast<double>(timerStop.QuadPart -
		timerStart.QuadPart) / timerFrequency.QuadPart);

	cout << "Time is " << t << " seconds." << endl;

	return 0;
}