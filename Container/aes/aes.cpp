#include <windows.h>

#include <stdio.h>

#include "..\..\containers\common.h"

int main()
	{
	BYTE* in = new BYTE[4 * AES_256_Nb+100];

	memset(in, '1', 4 * AES_256_Nb + 100);
	memcpy(in,"01234567890123456789012345678901-",33);
	
	WORD CifKey[AES_256_Nk] = { 0x00010203,    0x04050607,	0x08090a0b,    0x0c0d0e0f };

	AES  *MyAES =  new AES(256);

	MyAES->Init(CifKey);

	DWORD t1=GetTickCount();

	for (int i = 0; i < 100000;i++)
		{

		MyAES->Crypt((WORD *)in,32+2);

		MyAES->Decrypt((WORD *)in,32+2);
		}
	DWORD t2 = GetTickCount();

	printf("%f",(t2-t1)/1000.);

	getchar();
    return 0;
	}

