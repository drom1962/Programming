#include "..\decoder.h"

Decoder::~Decoder() {};

//
//
//
Nvidia::Nvidia()
	{
	DecoderFeature = DecodResize || CopyDevToHost || Surface;
	SizeBuff = 0;
	LocalBuff = nullptr;
	}

Nvidia::~Nvidia()
	{
	}


int Nvidia::Feature()
	{
	return DecoderFeature;
	}

//
//    �������� ��������
//
int	Nvidia::Init(int Dev, DecoderInfo *DI)
	{

	
	// �������� ������ �����

	return S_OK;
	}


//
//		������������� �����
//
int	Nvidia::Decode(Frame *Frame, FrameInfo *FI)
	{
	return S_OK;
	}


int	Nvidia::Destroy()
	{

	return S_OK;
	}
