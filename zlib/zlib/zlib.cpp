// zlib.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"

#include <windows.h>

#include <malloc.h>


#include "zlib.h"


int main()
{
	uLongf destLen = 1024 * 1024,datalen=128*1024;

	Bytef *dest = (Bytef *)malloc(destLen);

	Bytef *data = (Bytef *)malloc(datalen);

	FILE *ff;
	fopen_s(&ff,"1.mtd","r");

	fread(data, 1, 1024 * 54, ff);

	int x;

	DWORD t1=GetTickCount();


	for (int i = 0; i < 10000; i++)
		{
		x = compress(dest, &destLen, data, 1024 * 54);
		}
	DWORD t2 = GetTickCount();

	printf("%d",(t2-t1)/1000);

	getchar();

    return 0;
}

