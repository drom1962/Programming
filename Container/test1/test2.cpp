#include <wchar.h>

#include <string>

#include "..\..\Containers\ovi\ovi_container.h"

#include "..\..\Containers\ovi\metadata.h"

#include "..\..\Containers\metadata\mymath.h"

#include <stdio.h>


using namespace Meta_Data;

int wmain(int argc, wchar_t* argv[])
{


	











	int ret;

	MTD  *mtd1 = new MTD(1);

	MetaDataFileInfo MDFI;

	MovingTarget	*MT = new MovingTarget[1024];

	ret = mtd1->Create(L"xx.mtd", &MDFI);
	if (ret != 0) return 2;


	for (int i = 0; i < 100; i++)
		{
		MT[i].dwObjID = 0;
		MT[i].dwZoneID = 0;
		MT[i].nTop = 1;
		MT[i].nBottom = 100;
		MT[i].nLeft = 1;
		MT[i].nRight = 1000;
		MT[i].fMovAngle = 90*i;
		}
	
	int zz = 1, zz1 = 2;
	for (int i = 0; i < 10000; i++)
		{
		ret=mtd1->WriteMetaData((unsigned char *)MT, sizeof(MovingTarget)*zz1, i);

		if (ret != 0) return -1;

		if (zz == 1)
		{
			// идем вверх
			if (zz1 == 11) zz = -1;
		}
		else
		{
			// идем вниз
			if (zz1 == 2) zz = 1;
		}

		zz1 += zz;
	}

	mtd1->Close(&MDFI);
	

	ret = mtd1->Open(L"xx.mtd", &MDFI);


	MovingTarget2	*MT2 = new MovingTarget2[1024];

	MetaDataInfo MDI;
	
	DWORD t1 = GetTickCount();

	for (int i = 0; i < MDFI.CountMetadataFrame; i++)
		{
		ret = mtd1->ReadMetaData(i, (unsigned char *)MT2, sizeof(MovingTarget2) * 100, &MDI);

		if (ret != 0) return -2;



		ret = mtd1->ParserZone(MDI.Data, MDI.Size, &MT2);

		printf(" MD = %6d  Zon = %3d Angle =", MDI.Size, ret);
		double ff, ss, ss1, ss2, ss3;

		Point pp1, pp2;

		for (int i = 0; i < ret; i++)
			{
			printf(" %3.0f", MT[i].fMovAngle);
			}
		printf("\n");
		}

	ret=mtd1->Close(&MDFI);

	DWORD t2 = GetTickCount();

	printf("Time = %5.2f\n", (t2 - t1) / 1000.);

	return 0;
}