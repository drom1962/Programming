#include <cuda.h>
#include <cuda_runtime_api.h>

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "nvcuvid.h"


#include <windows.h>
#include <memory.h>

#include "MyCuda.h"

#include <stdio.h>

#include "..\..\log\logs2.h"
using namespace My_LOG;

#include "..\..\containers\ovi\ovi_container.h"

Frames	*Fr;
DWORD	FrameCont;

LOG *MyLog;

int Ex(int Cod)
	{
	for(DWORD i=0;i<FrameCont;i++)
		{
		//printf("%d\n",i);
		if(Fr[i].Data!=nullptr) 	free(Fr[i].Data);
		}

	MyLog->Print(Always,L"Free memory");

	wchar_t Buff[129];
	swprintf(Buff,L"Cod = %d",Cod);

	MyLog->Print(Always,Buff);
	getchar();
	return Cod;
	}

#include <conio.h>

int main()
{
	int ret;
    VideoFrameInfo VFI;

	FileInfo FI;

	// Лог
	wchar_t Buff[1024];
	MyLog=new LOG(nullptr,L"Cuda.log");

	// Откроем контейнер
    OVI2    *F1=new OVI2(1);

	//CreateSurface 
	
	char Fn[128];
	wchar_t Con[128];
    
	do
		{
		printf("File name=");
		gets_s(Fn);
		if(swprintf(Con,127,L"%S.ovi",Fn)==0) continue;
		
		ret=F1->Open(Con,&FI);
		if(ret==0) break;
			
		printf("File     %s not opened\n",Con);	
		
		} while(1);

	swprintf(Buff,L"File = %s",Con);
	MyLog->Print(Always,Buff);

	swprintf(Buff,L"\nCodec = %1d Width = %d  Height = %d  FPS=%f\n\n",FI.VideoCodec,FI.Width,FI.Height,FI.FPS);
	MyLog->Print(Always,Buff);

	//FI.CountVideoFrame=100;
	
	Fr= new Frames[FI.CountVideoFrame];

	for(FrameCont=0;FrameCont<FI.CountVideoFrame;FrameCont++)
        {
		ret=F1->ReadVideoFrame(FrameCont,nullptr,0,&VFI);
		if(ret!=0)
			{
			swprintf(Buff,L"Error read %d",FrameCont);
			MyLog->Print(Always,Buff);
			return Ex(1);
			}

		Fr[FrameCont].Size=VFI.SizeFrame;
		Fr[FrameCont].Data=(unsigned char *)malloc(VFI.SizeFrame);
		if(Fr[FrameCont].Data==nullptr)
			{
			return Ex(99);
			}
		Fr[FrameCont].Type=VFI.TypeFrame;
		memcpy(Fr[FrameCont].Data,VFI.Data,VFI.SizeFrame);
		}

	swprintf(Buff,L"\nCount frames =%d",FrameCont);
	MyLog->Print(Always,Buff);

	printf("\n Type video proceddor (VP=1,CUDA=2) =");
	char type;
	do 
		{
		type=getch()-'0';
		} while(type!=1 && type !=2);

	printf("%c\n",type+'0');
	
	printf("\n Read memory (Read=1,No read=2) =");
	char ReadMemory;
	do 
		{
		ReadMemory=getch()-'0';
		} while(ReadMemory!=1 && ReadMemory !=2);

	printf("%c\n",ReadMemory+'0');

	CUresult result;
	
	// cuInit
	result=cuInit(0);
	if(result!=CUDA_SUCCESS)  return Ex(1);
	MyLog->Print(Always,L"\nCUDA Init success\n");

	// Создадим контекс
	CUcontext   	ctx;
	CUdevice  		dev=0;
	result=cuCtxCreate(&ctx,CU_CTX_SCHED_AUTO,dev);
	if(result!=CUDA_SUCCESS)  return Ex(2);
	MyLog->Print(Always,L"CUDA Create context success\n");

	MyCuda *m1=new MyCuda();

	// Распарсим I кадр
    CUVIDSOURCEDATAPACKET   Sdp;

    memset(&Sdp,0,sizeof(CUVIDSOURCEDATAPACKET));

    // Создадим парсер
    //------------------------------------------------
	ret=m1->CreateParser(FI.VideoCodec);

    // Создадим декодер
    //------------------------------------------------
    cuDecoderInfo DI;       // Контекст декодирования

    DI.Width		=FI.Width;
    DI.Height		=FI.Height;

	printf("Resize NewWidth= ");
	scanf("%d",&DI.NewWidth);
	if(DI.NewWidth==0)  DI.NewWidth	=FI.Width;

	printf("Resize NewHeight= ");
	scanf("%d",&DI.NewHeight);
	if(DI.NewHeight==0)  DI.NewHeight	=FI.Height;

	swprintf(Buff,L"\nNew size ====>  Width = %d  Height = %d",DI.NewWidth,DI.NewHeight);
	MyLog->Print(Always,Buff);

	switch(type)
		{ 
		case 1:
				DI.Flag			=VP;
				break;
		case 2:
				DI.Flag			=CUDA;
				break;
		default:
				DI.Flag			=0;
		}
	
	// Отпарсим первый ключевой
	Frames Frame;
	Frame.Data=Fr[0].Data;
	Frame.Size=Fr[0].Size;

	ret=m1->ParserFrame(Frame);
	if(ret) return Ex(10);
    
	DI.WorkPict=1;
	DI.OutPict=1;

	ret=m1->CreateDecoder(&DI);  // Создадим декодер
	if(ret) 
		{
		return Ex(20);
		}
	switch(DI.Flag)
		{
		case CUDA:
			swprintf(Buff,L"----------CUDA\n");
			break;

		case VP:
			swprintf(Buff,L"----------VP\n");
			break;
		case DXVA:
			swprintf(Buff,L"----------DXVA\n");
			break;

		default:
				return Ex(200);
		}
    MyLog->Print(Always,Buff);
	
DWORD t1,t2;

t1=GetTickCount();

void *Surface;
Surface=(void *)1;

for(int ii=0;ii<100;ii++)
	{
	int i;
	for(i=0;i<FI.CountVideoFrame;i++)
        {
		Frame.Data=Fr[i].Data;
		Frame.Size=Fr[i].Size;

		if(Fr[i].Type)  // Ключевой ?
			{
			ret=m1->ParserFrame(Frame);
			if(ret)	return Ex(30);
			}	
	
		// Декодируем
		ret=m1->DecoderFrame(0);
		if(ret)	return Ex(31);
			
		// Возьмем результат
		if(ReadMemory==1)
			{
		    ret=m1->GetD3D9Surface(0,Surface);
			ret=m1->GetDecodeFrame(0,nullptr,0,BlackWhite);
			if(ret)	return Ex(31);
			}
	
    	if(i>0 && ((i+1) % 1000) == 0) 
	    	{
    		t2=GetTickCount();
			double sec=(t2-t1)/1000.;
			double fps=1000./sec;
			swprintf(Buff,L"%5d time = %7.1f sec FPS = %7.1f   === %7.5f   =GPU for  FPS=> %7.2f",i+1,sec,fps,100./fps,(100./fps)*FI.FPS);
			MyLog->Print(Always,Buff);
		    t1=t2;
		    }
	
	//Sleep(40);
	ret=kbhit();
	if(ret!=0)
		{
		char ss=getch();
		switch(ss)
			{
			case 's':
					MyLog->Print(Always,L"Stop.");
					return Ex(1000);
			case 'p':
					MyLog->Print(Always,L"Sllep 5 sec. .... ");
					Sleep(5000);
					MyLog->Print(Always,L"Wake upc.");
			}
		}
	}

	t2=GetTickCount();
	
	double sec=(t2-t1)/1000.;
	double fps=(i % 1000)/sec;
	swprintf(Buff,L"%5d time = %7.1f sec FPS = %7.1f   === %7.5f   =GPU for  FPS=> %7.2f\n",i+1,sec,fps,100./fps,(100./fps)*FI.FPS);
	MyLog->Print(Always,Buff);
	t1=t2;


//	printf("%5d \n",ii);
	}
    //result=cuvidDestroyVideoParser(&obj);

	return Ex(0);
}
    // Обработаем ExtraData
    //------------------------------------------------
    //const char      NullFrame[6]={0,0,0,1,0x65,0x80};  // Пустой кадр
    //DWORD           sED=256+6;
    //unsigned char   *ED=new unsigned char [sED];
