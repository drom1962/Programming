#include "StgFileBase.h"

using namespace Writer;

//========================================
// Base for storage recording version
//========================================

MediaContainerBase::MediaContainerBase()
{
	codec_ = NONE;
	ex_data.data = NULL;
	ex_data.size = 0;
}


MediaContainerBase::~MediaContainerBase()
{
	if (ex_data.data != NULL)
	{
		delete[] ex_data.data;
		ex_data.size = 0;
	}
}


bool MediaContainerBase::CreateWriter(VideoCodecType vc)
{
	codec_ = vc;
	firstFrame_ = true;
	isBegan_ = false;
	return true;
}


HRESULT MediaContainerBase::SaveExtraData(const char *buffer, unsigned int size)
{
	if(ex_data.data != NULL)
	{
		delete[] ex_data.data;
	}
	ex_data.data = new unsigned char[size];
	ex_data.size = size;
	memcpy(ex_data.data, buffer, size);
	return S_OK;
}


bool MediaContainerBase::IsNeedExtraData() const
{
	return true;
}