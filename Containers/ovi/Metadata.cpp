#include "..\ovi\metadata.h"


#include "zlib.h"

#include <iostream>

using namespace Meta_Data;

//===================================================================================================================================================
V_e_r MTDver = { 1,0,0,0 };

void MTD::Version(V_e_r *V)
{
	if (V != nullptr)
		memcpy(V, &MTDver, sizeof(V_e_r));
}


MTD::MTD(int Ver_OVMSML)
	{
	// �������� ���������� �������
	m_hFile					= nullptr;

	m_MetaDataBuff			= nullptr;
	m_MetaDataBufferSize	= MAXSIZEBLOCK2;

	m_LocalBuff				= nullptr;
	m_SizeLocalBuff			= 0;

	m_MetaDataIndexs		= nullptr;

	m_ZipMetaDataBuffSize	= MAXSIZEBLOCK2 + 12;
	m_ZipMetaDataBuff		= nullptr;

	m_SizeMetaDataFrames	= 0;

	// �����
	m_Crypto = 0;
	};


MTD::~MTD()
	{
	if (m_hFile != nullptr)  CloseHandle(m_hFile);

	if (m_MetaDataBuff != nullptr)		free(m_MetaDataBuff);

	if (m_MetaDataIndexs != nullptr)	free(m_MetaDataIndexs);

	if (m_ZipMetaDataBuff != nullptr)	free(m_ZipMetaDataBuff);
	};


//
//  �������� ����
//
int MTD::Create(LPCWSTR FileName, MetaDataFileInfo *FI)
	{
	if (m_hFile != nullptr)    return OVI_File;

	if (FI == nullptr)         return OVI_File;

	m_hFile = CreateFileW(FileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
		nullptr);

	// �������� ���������
	if (m_hFile == INVALID_HANDLE_VALUE)
		{
		return GetLastError();
		}

	// �������� ������ ����
	USHORT Comp = COMPRESSION_FORMAT_DEFAULT;
	DWORD res;

	DeviceIoControl(m_hFile,	// handle to file or directory
		FSCTL_SET_COMPRESSION,  // dwIoControlCode
		&Comp,					// input buffer
		sizeof(USHORT),			// size of input buffer
		NULL,                   // lpOutBuffer
		0,                      // nOutBufferSize
		&res,					// number of bytes returned
		NULL);					// OVERLAPPED structure


	// ������� ������
	m_Mod = 1;							// ����� ������

	m_Flags = FI->Mod;

	m_H_MTD.Mod = m_Flags;				// �������� ����� 

	m_Flush = FI->Mod  & CountFlush;	// ������� ����� ������ Flush
	c_Flush = 0;

	// �������� ������ ��� ����� �������
	memset((void *)&m_MC, 0, sizeof(MetaChank));

	// ������� ������ ��� �����
	if (m_MetaDataBuff==nullptr)
		if(CreateBuff(&m_MetaDataBuff, 1, 0, m_MetaDataBufferSize))												return OVI_NotAlloc;
	m_MetaDataIntoBuffers = m_MetaDataBuff;

	if (m_ZipMetaDataBuff==nullptr)
		if(CreateBuff(&m_ZipMetaDataBuff, 1, 0, m_ZipMetaDataBuffSize))											return OVI_NotAlloc;  // ��� ���������� ������

	// ������� ������ ��� �������
	if (m_MetaDataIndexs == nullptr)
		{
		m_MaxMetaDataIndex = 1024;
		if (CreateBuff((unsigned char **)&m_MetaDataIndexs, sizeof(ElementMetaIndex), 0, m_MaxMetaDataIndex))	return OVI_NotAlloc;
		}

	// �������� ��������� ������
	memset((void *)&m_H_MTD, 0, sizeof(Header_MTD));

	// �������� ���������
	m_H_MTD.Height = FI->Height;
	m_H_MTD.Width = FI->Width;
	m_H_MTD.MinObject = FI->MinObject;

	m_H_MTD.Ver = 2;

	m_H_MTD.CrcFile = 0;
	m_H_MTD.CrcHeader = 0;

	m_CountMataDataFrame = 0;

	m_CountMetaDataFrameIntoChunk = 0;
	m_CountMataDataFrame = 0;

	m_LastReadFrame = 0;

	m_PosMetaData = 0;

	GetFileTime(m_hFile, &m_H_MTD.Time, nullptr, nullptr);     // ��������

	return WriteHeader(false);
	}

//
//  �������� ����
//
int MTD::CreateEx(LPCWSTR FileName, MetaDataFileInfo *FI,LPCWSTR Pass)
	{
	return S_OK;
	}

//
//  ������� �� ������ ��� ���������
//
int MTD::Open(LPCWSTR FileName, MetaDataFileInfo *FI)
{
	if (m_hFile != nullptr) return OVI_NotOpen;

	m_hFile = CreateFileW(FileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY,
		nullptr);

	// �������� ���������
	if (m_hFile == INVALID_HANDLE_VALUE)
		{
		m_hFile = nullptr;
		return OVI_NotOpen;
		}

	m_Mod = 0;		// ����� ������

	// �������� ������ ��� ����� �������
	memset((void *)&m_MC, 0, sizeof(MetaChank));

	//  ������ ���������
	int ret = ReadHeader(&m_H_MTD);

	if (ret)
		{
		CloseHandle(m_hFile);
		m_hFile = nullptr;
		return OVI_CrcHeader_FAIL;
		}

	FI->Height		= m_H_MTD.Height;
	FI->Width		= m_H_MTD.Width;
	FI->MinObject	= m_H_MTD.MinObject;

	// �������� ������
	if (m_MetaDataBuff==nullptr)
		if(CreateBuff(&m_MetaDataBuff, 1, 0, m_MetaDataBufferSize))		return OVI_NotAlloc;  // ��� ���������� ������


	if (m_ZipMetaDataBuff==nullptr)
		if(CreateBuff(&m_ZipMetaDataBuff, 1, 0, m_ZipMetaDataBuffSize))	return OVI_NotAlloc;  // ��� ���������� ������
	
	if (m_MetaDataIndexs==nullptr)
		if(CreateBuff((unsigned char **)&m_MetaDataIndexs, sizeof(ElementMetaIndex),0, m_H_MTD.CountMataDataFrame)) return OVI_NotAlloc;  // ��� ���������� ������


	//Refresh();  // ���������� �������

	int ff = 0;
	if (m_H_MTD.MainMetaDataIndex != 0L)
		{
		// ���� ������� ������ ��� �����
		if (MyRead((unsigned char *)m_MetaDataIndexs, m_H_MTD.CountMataDataFrame * sizeof(ElementMetaIndex), m_H_MTD.MainMetaDataIndex)) return OVI_InvalidIndex;
		}
	else
		{
		m_CountMataDataFrame = 0;
		Refresh(0);  // ���������� �������
		ff++;
		}
	m_CountMetaDataFrameIntoChunk = 0;
	m_CountMataDataFrame = 0;

	m_LastReadFrame = 0;

	m_PosMetaData = 0;

	if (FI != nullptr)
		{
		FI->CountMetadataFrame = m_H_MTD.CountMataDataFrame;
		}

	// ���� ������ - ��������
	if (ff>0) return OVI_NotClose;




	return S_OK;
	}

//
//  ������� ����
//
int MTD::OpenEx(LPCWSTR FileName, MetaDataFileInfo *FI, LPCWSTR Pass)
	{
	return S_OK;
	}


//
// ������ �����
//
int  MTD::IsOpen()
	{
	if (m_hFile == nullptr)				return OVI_NotOpen;

	return S_OK;
	}


//
// ������� ����� � ����
//
int	MTD::WriteLineAndZone(int CountLines, Line *, int CountZones, Zone *)
	{
	if (m_hFile == nullptr)				return OVI_NotOpen;

	return S_OK;
	}




















//
//  ������� ���������
//
int MTD::Close(MetaDataFileInfo *FI)
	{
	if (m_hFile == nullptr)				return OVI_NotOpen;

	if (m_Mod == 0)
		{
		// �� �� ������ � ��� �������
		CloseHandle(m_hFile);
		m_hFile = nullptr;

		return S_OK;
		}

	if (m_CountMataDataFrame == 0)	return OVI_Err1;

	// �������, ��� �������� � ������
	WrireGroupMetaData();

	// ������� �������
	DWORD Pos = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);

	if (MyWrite(m_MetaDataIndexs, sizeof(ElementMetaIndex)*(m_CountMataDataFrame), 0))
		{
		CloseHandle(m_hFile);
		m_hFile = nullptr;

		return OVI_NotWrite;
		}

	// ������� �����
	m_H_MTD.MainMetaDataIndex = Pos;
	m_H_MTD.CountMataDataFrame = m_CountMataDataFrame;

	WriteHeader(false);

	// ������� ����
	CloseHandle(m_hFile);
	m_hFile = nullptr;

	if (FI != nullptr)
		{
		FI->CountMetadataFrame = m_CountMataDataFrame;
		}

	return S_OK;
	}



//
// ������� ������ �� ����
//
int MTD::Flush()
{
	if (m_hFile == nullptr)	return OVI_NotOpen;

	if (!m_Mod)				return S_OK;

	m_H_MTD.CountMataDataFrame = m_CountMataDataFrame;

	c_Flush++;

	// ������� ����������
	if (c_Flush == m_Flush)
		{
		//WriteHeader(true);

		FlushFileBuffers(m_hFile);
		c_Flush = 0;
		}

	return S_OK;
}




//
//  ������� ����� ���� � ����� + ���������������� ������
//
int MTD::WriteMetaData(unsigned char  *MetaData, uint32_t Size, uint64_t Time)
	{
	if (m_hFile == nullptr)		return OVI_NotOpen;

	if (m_Mod == 0)	            return OVI_ReadOnly;

	ElementMetaIndex *EMI;

	// ������ �������� ��������� �� ������ - �� 1�
	if (m_CountMetaDataFrameIntoChunk == MAXFRAMESINTOCHANC2 || m_SizeMetaDataFrames>MAXSIZEBLOCK2)
		{
		int res = WrireGroupMetaData();
		if (res) return res;
		}

	if (m_CountMataDataFrame == m_MaxMetaDataIndex)
		{
		CreateBuff((unsigned char **)&m_MetaDataIndexs, sizeof(ElementMetaIndex), m_MaxMetaDataIndex, m_MaxMetaDataIndex + 1024);
		m_MaxMetaDataIndex += 1024;
		}

	// ������� ���� ������
	int c = Size / sizeof(MovingTarget);

	MovingTarget2	ToDisk[1024], *pToDisk = ToDisk;

	// ��������� � ����� ���������� ��������� ��� ������ �� ����
	for (int i = 0; i < c; i++)
		{
		pToDisk->nBottom	= ((MovingTarget *)MetaData)->nBottom;
		pToDisk->nTop		= ((MovingTarget *)MetaData)->nTop;

		pToDisk->nLeft		= ((MovingTarget *)MetaData)->nLeft;
		pToDisk->nRight		= ((MovingTarget *)MetaData)->nRight;

		pToDisk->fMovAngle  = ((MovingTarget *)MetaData)->fMovAngle;

		if (pToDisk->fMovAngle < 0. || pToDisk->fMovAngle > 360.)
			{
			int i = 0;
			}

		pToDisk++;
		MetaData += sizeof(MovingTarget);
		}

	Size = c * sizeof(MovingTarget2);
	
	//  ��������� ��������� ������
	EMI = &m_MC.IndexFrame[m_CountMetaDataFrameIntoChunk]; 
	EMI->Position	= m_SizeMetaDataFrames;
	EMI->Size		= Size;
	EMI->TimeFrame	= Time;

	//  ��������� ���������� ������
	EMI = &((ElementMetaIndex *)m_MetaDataIndexs)[m_CountMataDataFrame];
	EMI->Position	= m_SizeMetaDataFrames;
	EMI->Size		= Size;
	EMI->TimeFrame	= Time;


	// �������� ������ � ������
	if (m_MetaDataBufferSize < m_SizeMetaDataFrames + Size)
		{
		DWORD xx = m_SizeMetaDataFrames + Size;

		CreateBuff(&m_MetaDataBuff, 1, m_MetaDataBufferSize, xx);

		m_MetaDataBufferSize = xx;

		m_MetaDataIntoBuffers = m_MetaDataBuff + m_SizeMetaDataFrames;
		}
	
	memmove(m_MetaDataIntoBuffers, (const void *)(ToDisk), m_MC.IndexFrame[m_CountMetaDataFrameIntoChunk].Size);

	m_MetaDataIntoBuffers += Size;

	m_SizeMetaDataFrames  += Size;

	m_CountMataDataFrame++;

	m_CountMetaDataFrameIntoChunk++;

	return S_OK;
	}




//
//  ����� ������ ������.
//
int MTD::WrireGroupMetaData()
	{
	int ret;

	if (m_hFile == nullptr) return OVI_NotOpen;

	if (m_Mod == 0)		return OVI_ReadOnly;

	if (m_CountMetaDataFrameIntoChunk == 0) return S_OK;

	// �������� ������ ��������� ����� �������
	int SizeHeaderChunc = ((char *)&m_MC.IndexFrame[m_CountMetaDataFrameIntoChunk]) - (char *)&m_MC;

	// ������� Next + Count
	m_MC.CountFrameIntoChank = m_CountMetaDataFrameIntoChunk;

	DWORD Pos = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);  // ������� �������

	if (m_H_MTD.FirstMetaDataFrame == 0)
		{
		m_H_MTD.FirstMetaDataFrame = sizeof(m_H_MTD) + m_H_MTD.CountZome *sizeof(Line) + m_H_MTD.CountZome*sizeof(Zone);
		WriteHeader(true);
		}

	// �������
	uLongf destLen = m_ZipMetaDataBuffSize;

	compress(m_ZipMetaDataBuff, &destLen, m_MetaDataBuff, m_SizeMetaDataFrames);

	m_MC.Zip		= destLen;
	m_MC.Origanal	= m_SizeMetaDataFrames;

	m_MC.NextChank  = Pos + SizeHeaderChunc + m_MC.Zip; //+ m_SizeMetaDataFrames;
	
	m_MetaDataLastNext = Pos;			// ���� �������� ������ ����, �� ����� ���������������
										
	Pos += SizeHeaderChunc;				// ������� ����� ���������

	DWORD MetaDataChank = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);  // ������� �������

	for (DWORD i = 0; i<m_CountMetaDataFrameIntoChunk; i++)
		{
		m_MC.IndexFrame[i].Position += Pos;
		                                       
		((ElementMetaIndex *)m_MetaDataIndexs)[m_CountMataDataFrame - m_CountMetaDataFrameIntoChunk + i].Position += Pos;

		m_MC.IndexFrame[i].MetaDataChank = MetaDataChank;
		((ElementMetaIndex *)m_MetaDataIndexs)[m_CountMataDataFrame - m_CountMetaDataFrameIntoChunk + i].MetaDataChank = MetaDataChank;
		}

	// ������� �������
	ret = MyWrite((void *)&m_MC, SizeHeaderChunc, 0);

	// ������� �����
	//ret = MyWrite(m_MetaDataBuff, m_SizeMetaDataFrames, 0);
	ret = MyWrite(m_ZipMetaDataBuff, m_MC.Zip, 0);

	// ������� ����� ������� � �������� �� �� ������
	memset(&m_MC, 0, sizeof(MetaDataChank));
	ret = MyWrite((void *)&m_MC, 16, 0);
	Pos = SetFilePointer(m_hFile, -16L, nullptr, FILE_CURRENT);  // ����������������� � ������ �����������

	// ������� ����������
	m_MetaDataIntoBuffers = m_MetaDataBuff;
	m_CountMetaDataFrameIntoChunk = 0;
	m_SizeMetaDataFrames = 0;

	Flush();
	return S_OK;
}




// ���������� ���������� �����. ���� 0 �� ���� �� ���������� � �����
//
int MTD::ReadMetaData(long IndexFrame, unsigned char *MetaData, uint32_t MetaDataSize, MetaDataInfo *MDI)
	{
	if (m_hFile == nullptr)			return OVI_NotOpen;

	if (IndexFrame<0)  IndexFrame = m_LastReadFrame;

	ElementMetaIndex *EVI2;

	EVI2 = &((ElementMetaIndex *)m_MetaDataIndexs)[IndexFrame];

	DWORD TotalSize = EVI2->Size;

	if (MetaData != nullptr && MetaDataSize <= TotalSize) return 555;  // ���� ���� ���

	if (EVI2->MetaDataChank != m_CurrentMetaDataChunk)
		{
		// ������� ��� ������
		ReadGroupMetadata(EVI2->MetaDataChank);

		// �������� �� �������
		m_CurrentMetaDataChunk = EVI2->MetaDataChank;
		}

 	DWORD Pos = EVI2->Position - m_PosMetaData;

	if (MetaData == nullptr)
		{
		// ��������� �� ���������� �����
		if (m_SizeLocalBuff < TotalSize)
			{
			// �������� ����� �� 120 ���������
			DWORD xx = static_cast<DWORD>(TotalSize*1.2);

			if (CreateBuff(&m_LocalBuff, 1, m_SizeLocalBuff, xx)) return OVI_NotAlloc;  // ��� ������ ����� �� ���������� �����

			m_SizeLocalBuff = xx;
			}

		MetaData = m_LocalBuff;
		}

	// �������  ���� �� ������ ������
	memcpy(MetaData, m_MetaDataBuff + Pos, TotalSize);

	if (MDI != nullptr)
		{
		// ������ ���������� � ������
		MDI->Size		= EVI2->Size;
		MDI->Time		= EVI2->TimeFrame;// / 10;   // ����������� �������
		MDI->Data		= MetaData;
		}
		
	return S_OK;
	}


//
// ��������� ������ ������
//
int MTD::ReadGroupMetadata(DWORD VideoChunk)
{
	MetaChank			m_MC;

	if (m_hFile == nullptr)			return OVI_NotOpen;

	if (VideoChunk == 0)				return OVI_NotOpen;

	// ������� ��������� �����
	if (MyRead((unsigned char *)&m_MC, 16, VideoChunk) != S_OK) return -1;

	// ��������� ��������� �������
	if (MyRead((unsigned char *)&m_MC.IndexFrame, sizeof(ElementMetaIndex) * m_MC.CountFrameIntoChank, 0) != S_OK) return -2;

	if (m_MetaDataBufferSize<m_MC.Origanal)
		{
		// �������� ����� ���� ����
		CreateBuff(&m_MetaDataBuff, 1, m_MetaDataBufferSize, m_MC.Origanal);
		m_MetaDataBufferSize = m_MC.Origanal;
		}

	// �������� ������ ������
	m_PosMetaData = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);

	// ������� ������ ������ �����
	if (MyRead(m_ZipMetaDataBuff, m_MC.Zip, 0) != S_OK) return -3;
	
	uLongf xx = MAXSIZEBLOCK2;
	
	int ret=uncompress(m_MetaDataBuff, &xx, m_ZipMetaDataBuff, m_MC.Zip);

	if (ret!= Z_OK || m_MC.Origanal != xx)		return -3;

	return S_OK;
}




//
//  ������ ����� ����� �� �������
//
int MTD::SeekMetaDataByTime(uint64_t Time, uint32_t *IndexFrame)
	{
	if (m_hFile == nullptr) return OVI_NotOpen;

	if (m_H_MTD.MainMetaDataIndex == 0) Refresh(5);

	DWORD First = 0, End = m_H_MTD.CountMataDataFrame;
	
	DWORD ss;
	// ������� ����� ����� ������� �������� (�������� � ��� ����)

	ss = End / 2;
	do
		{
		if (Time  < ((ElementMetaIndex *)m_MetaDataIndexs)[ss].TimeFrame)
			{
			// ������ ��������
			End = ss;
			ss = End / 2;
			}
		else
			{
			// ������� ��������
			First = ss;
			ss = (End - First) / 2 + First;
			}
	} while ((End - First) > 5);

	ElementMetaIndex *EMI;

	EMI=&((ElementMetaIndex *)m_MetaDataIndexs)[First];

	for (; First<End; First++,EMI++)
		{
		if (Time == EMI->TimeFrame)
			{
			// ����� ���� � �������� ������ ���������
			if (IndexFrame != nullptr) *IndexFrame = First;

			return S_OK;
			}
		}

	return OVI_FrameNone;
	}




unsigned char *	MTD::GetLocalBufer()
	{
	return m_MetaDataBuff;
	}


//
//  ��������� ����
//
int	MTD::ParserZone(unsigned char *Zones, int Size, MovingTarget2 **MT)
	{
	DWORD CountZone = Size / sizeof(MovingTarget2);
	*MT = (MovingTarget2 *)Zones;
	return CountZone;
	}








//
// ���������� ������
//
int MTD::Refresh(int Count)
	{
	if (m_hFile == nullptr)	return OVI_NotOpen;

	if (m_Mod)				return S_OK;

	return S_OK;
	}






//
//   ��������� ������
//
char * MTD::Errors(int Cod)
	{
	return "";
	}


//
//   ���������� ����
//
int	MTD::Recovery(LPCWSTR FileName)
	{
			
	return 0;
	}








//
//  ������� ��������� �����
//
int MTD::WriteHeader(bool Flag)
	{
	DWORD Size, Pos;

	//delete m_VideoIndexs;

	// �������� � ������ �����
	if (Flag) Pos = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);

	// �������� � ������ �����
	Size = SetFilePointer(m_hFile, 0L, nullptr, FILE_BEGIN);
	if (Size != 0) return GetLastError();

	// �������� ����������� ����� ���������
	m_H_MTD.CrcHeader = Crc8((unsigned char *)&m_H_MTD, sizeof(Header_MTD) - 1, 0x25);

	// ������� ���������

	Header_MTD ss;
	memcpy(&ss, &m_H_MTD, sizeof(Header_MTD));

	
	if (MyWrite((void *)&ss, sizeof(Header_MTD), 0))
		{
		return GetLastError();
		}

	if (Flag) Size = SetFilePointer(m_hFile, Pos, nullptr, FILE_BEGIN);

	return S_OK;
	}


//
//  ��������� ��������� �����
//
int  MTD::ReadHeader(Header_MTD *HD)
	{
	DWORD Size;

	if (m_hFile == nullptr)		return OVI_NotOpen;

	if (HD == nullptr)				return OVI_InvalidParam;

	// �������� � ������ �����
	Size = SetFilePointer(m_hFile, 0L, nullptr, FILE_BEGIN);
	if (Size != 0) return GetLastError();

	// ��������� ������ ����������
	if (!ReadFile(m_hFile, &HD->Ver, sizeof(HD->Ver), &Size, nullptr))
		{
		return GetLastError();
		}

	// ������� ���������
	if (!ReadFile(m_hFile, (Header_MTD *)&HD->VerOVSML, sizeof(Header_MTD) - sizeof(HD->Ver), &Size, nullptr))
		{
		return GetLastError();
		}

	if (Size != sizeof(m_H_MTD) - sizeof(HD->Ver)) return OVI_E_FAIL;

	// �������� ����������� ����� ���������
	unsigned char Crc = Crc8((unsigned char *)HD, sizeof(Header_MTD) - 1, 0x25);

	if (Crc != HD->CrcHeader) return OVI_CrcHeader_FAIL;

	return S_OK;
	}




//
//  ��� ������
//
DWORD MTD::MyRead(unsigned char *Buff, DWORD SizeRead, DWORD Position)
{
	DWORD Pos;

	if (Position>0)
	{
		// �������� � �������
		Pos = SetFilePointer(m_hFile, Position, nullptr, FILE_BEGIN);
		if (Pos != Position) return GetLastError();
	}

	// �������
	if (!ReadFile(m_hFile, (void *)Buff, SizeRead, &Pos, nullptr))
	{
		return GetLastError();
	}

	if (Pos != SizeRead) return OVI_Err3;

	return S_OK;
}


//
//  ��� ������
//
DWORD MTD::MyWrite(void *Buff, DWORD SizeBuff, DWORD Position)
{
	DWORD Size;

	// �������� � ������ �����
	if (Position>0)
	{
		Size = SetFilePointer(m_hFile, Position, nullptr, FILE_BEGIN);
		if (Size != 0) return GetLastError();
	}

	// ������� ����
	if (!WriteFile(m_hFile, Buff, SizeBuff, &Size, nullptr))
	{
		return GetLastError();
	}

	if (Size != SizeBuff) return OVI_Err4;

	return S_OK;
}


//
//  ������ ������ ��� �������� ������
//
int MTD::CreateBuff(unsigned char **Buff, int SizeAtom, DWORD SizeBuff, DWORD NewSize)
	{

	if (SizeBuff == 0)
		{
		// ������ �������
		//*Buff= new unsigned char [NewSize*SizeAtom];
		*Buff = (unsigned char *)malloc(NewSize*SizeAtom);
		if (*Buff == nullptr) return OVI_NotAlloc;
		}
	else
		{
		// ���� ��������
		if (SizeBuff<NewSize)
			{
			// ������� ����� �����
			//unsigned char *NewBuff= new unsigned char[NewSize*SizeAtom];
			unsigned char *NewBuff = (unsigned char *)malloc(NewSize*SizeAtom);
			if (NewBuff == nullptr) return OVI_NotAlloc;

			// �������
			memset((void *)NewBuff, 0, NewSize*SizeAtom);

			// ��������� ������
			memcpy(NewBuff, *Buff, SizeBuff*SizeAtom);

			//delete [] *Buff;
			free((void *)*Buff);

			// ������� �����
			*Buff = NewBuff;

			}
		}
	return S_OK;
	}


int	MTD::Logs(void *Buff, DWORD Size)
	{
	if (m_hLog == nullptr)		return OVI_NotOpen;

	DWORD rSize;

	WriteFile(m_hLog, Buff, Size, &rSize, nullptr);

	WriteFile(m_hLog, "\n", 3, &rSize, nullptr);

	return S_OK;
	}



//
//  ���������
//
int MTD::Encrypt(unsigned char *Buff, DWORD Size)
{
	if (Size>128) Size = 128;

	//AES_STREAM->Crypt((unsigned long *)Buff, Size);

	long *ss = (long *)(Buff + 2);
	int   xx = (Size / 4) - 2;

	for (int i = 0; i<xx; i = i + 2)
		ss[i] = ~ss[i];  // ������� ��������

	return 0;
}


//
//  ����������
//
int MTD::Decrypt(unsigned char *Buff, DWORD Size)
{
	if (Size>128) Size = 128;

	long *ss = (long *)(Buff + 2);
	int   xx = (Size / 4) - 2;

	for (int i = 0; i<xx; i = i + 2)
		ss[i] = ~ss[i];  // ������� ���������

	//AES_STREAM->Decrypt((unsigned long *)Buff, Size);

	return 0;
}

