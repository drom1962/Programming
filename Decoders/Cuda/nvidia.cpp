#include "..\decoder.h"

Decoder::~Decoder() {};

//
//
//
Nvidia::Nvidia()
	{
	m_Feature = DecodResize || CopyDevToHost || Surface;
	}

Nvidia::~Nvidia()
	{
	}


int Nvidia::Feature()
	{
	return m_Feature;
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
