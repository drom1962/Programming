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
//    Создание декодера
//
int	Nvidia::Init(int Dev, DecoderInfo *DI)
	{

	
	// Заполним битмап хедер

	return S_OK;
	}


//
//		Декодирование кадра
//
int	Nvidia::Decode(Frame *Frame, FrameInfo *FI)
	{
	return S_OK;
	}


int	Nvidia::Destroy()
	{

	return S_OK;
	}
