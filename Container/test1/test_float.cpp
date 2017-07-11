#include <windows.h>

#include <stdio.h>

void main()

{
DWORD t0 = GetTickCount(); 

float xx, a = 67890.0, b = 78687.89789; 

for (int ii = 0; ii < 1000; ii++) 
  { 
  for (int i = 0; i < 1000000; i++) 
     { 
	 xx = a / b;
     //xx = a*b; 
     } 
  } 

DWORD t00 = GetTickCount(); 
printf("Time = %5.2f\n", (t00 - t0) / 1000.);

getchar();

}