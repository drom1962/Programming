#include <windows.h>

#include <stdio.h>

#include <wchar.h>


#include "..\common.h"

#include "..\container.h"

#include "..\ovi\Ovi_container.h"


using namespace Archive_space;

//===================================================================================================================================================
V_e_r OviVer2={4,0,0,0};

void OVI2::Version(V_e_r *V)
	{
    if(V!=nullptr) 
	                memcpy(V,&OviVer2,sizeof(V_e_r));
    }


OVI2::OVI2(int Ver_OVMSML)
		{
		// �������� ���������� �������
		m_hFile=nullptr;

		m_Flush = 0;
		c_Flush = 0;

		m_VideoBufferSize		= VIDEOBUFFERSIZE;
		m_MaxVideoIndex			=30*60*60;
		m_VideoIndexs			=nullptr;
		m_VideoFramesIntoBuffers=nullptr;
		m_LocalFrame			=nullptr;
		m_SizeLocalFrame		=FRAMEBUFFERSIZE;
		
		m_AudioIndexs			=nullptr;
		m_MaxAudioIndex			=30*60*60;
		m_AudioSamplesIntoBuffers=nullptr;
		m_AudioBufferSize		=AUDIOBUFFERSIZE;

		m_LocalSample		=nullptr;
		m_SizeLocalSample	=AUDIOBUFFERSIZE;

		m_VerOVMSML				=Ver_OVMSML;

        m_average_frame_time	=1000000;

		AES_HEADER				= new AES(256);

		char ss[] = "ab32f8c395d0b07c7b612dcb56375e0383f85b05052b894c2f083f146c46f491";
		for (int i = 0; i < 8; i++)
			{
			sscanf_s(&ss[i * 8], "%8x", &m_CifKeyForHeader[i]);
			}

		AES_HEADER->Init(m_CifKeyForHeader);
		
		AES_STREAM = new AES(256);

		// �����
		m_Crypto = 0;
		m_WriteMetaData			= 0;
		};


OVI2::~OVI2()
		{
		if(m_VideoIndexs!=nullptr)		
			{
			// ������ ������ ��� �������
			free((void *)m_VideoBuff);
			free((void *)m_VideoIndexs);
			free(m_LocalFrame);
			}
				
		if(m_AudioIndexs!=nullptr)
			{
			// ������ ������ ��� �������
			free((void *)m_AudioBuff);
			free((void *)m_AudioIndexs);
			}

		if(m_hFile!=nullptr)  CloseHandle(m_hFile);
		};

//---------------------------------------------------------------------------
//  �������� ����������
//---------------------------------------------------------------------------
bool OVI2::CheckExtension(wchar_t * Ext)
	{	
    if (Ext == nullptr) return false;

	if(wmemcmp(Ext,L"OVI",3)==0) return true;

	return false;
	}

//---------------------------------------------------------------------------
//  �������� ����������
//---------------------------------------------------------------------------
Archiv * OVI2::CreateConteiner()
	{	
	return new OVI2(1);
	}


//---------------------------------------------------------------------------
//  �������� ����
//---------------------------------------------------------------------------
int OVI2::Create(LPCWSTR FileName,FileInfo *FI)
		{
		if(m_hFile!=nullptr)    return OVI_File;

		if(FI==nullptr)         return OVI_File;

		if(FI->VideoCodec==0)   return OVI_File;

		m_hFile=CreateFileW(FileName,
						 GENERIC_READ | GENERIC_WRITE,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 nullptr,
						 CREATE_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
						 nullptr);

		// �������� ���������
		if(m_hFile==INVALID_HANDLE_VALUE) 
			{
			return GetLastError();
			}

		// ������� ���������
		memset((void *)&m_H_OVI,0,sizeof(Header_OVI));

		// ������� ������
		m_Mod=1;							// ����� ������

		m_Flags		= FI->Mod;

		m_H_OVI.Mod = m_Flags;				// �������� ����� 

		if (m_Flags & WriteMetaData)	m_WriteMetaData = 1;   // �����
			
		if (m_Flags & Crypto)
			{
			AES_STREAM->Init(m_CifKeyForHeader);
			m_Crypto = 1;			//h.264 �������
			}

		m_Flush = FI->Mod  & CountFlush;	// ������� ����� ������ Flush
		c_Flush = 0;
				
		GetFileTime(m_hFile,&m_H_OVI.Time,nullptr,nullptr);
		
		// �������� ������ ��� ����� �������
		memset((void *)&m_VC,0,sizeof(VideoChank2));
		
		// ����� ��� �����
		m_VideoIndexs			=nullptr;
		m_VideoFramesIntoBuffers=nullptr;

		// ������� ������ ��� �����
		if(CreateBuff(&m_VideoBuff,1,0,m_VideoBufferSize))							return OVI_NotAlloc;
		
		if(CreateBuff(&m_VideoIndexs,sizeof(ElementVideoIndex2),0,m_MaxVideoIndex))	return OVI_NotAlloc;
		
		// ����� ��� ����
		m_AudioIndexs			=nullptr;
		m_AudioSamplesIntoBuffers=nullptr;
		
		if(FI->AudioCodec>0) 
			{
			// ������� ������ ��� �����
			if(CreateBuff(&m_AudioBuff,1,0,m_AudioBufferSize))							return OVI_NotAlloc;

			if(CreateBuff(&m_AudioIndexs,sizeof(ElementAudioIndex),0,m_MaxAudioIndex))	return OVI_NotAlloc;

			m_AudioSamplesIntoBuffers=m_AudioBuff;
			}
					
		m_FreeIndex=0;								// ����� ������ �����
		m_VideoFramesIntoBuffers=m_VideoBuff;

		m_SizeVideoFrames=0;
		m_CurrentVideoFrame=0;						
		m_CountVideoFrameIntoChunk=0;

		m_SizeAudioSamples=0;
		m_CurrentAudioSample=0;
		m_CountAudioSampleIntoChunk=0;

		GetFileTime(m_hFile,&m_H_OVI.Time,nullptr,nullptr);

		m_H_OVI.Ver			= 2;

		m_H_OVI.CrcFile		= 0;
		m_H_OVI.CrcHeader	= 0;

		SetFileInfo(FI);

		return WriteHeader(false);
	
		}

//---------------------------------------------------------------------------
//  �������� ����
//---------------------------------------------------------------------------
int OVI2::CreateEx(LPCWSTR FileName,LPCWSTR Pass,FileInfo *FI)
	{
	return OVI_NotSupport;
	}


//---------------------------------------------------------------------------
//  ������� �� ������ ��� ���������
//---------------------------------------------------------------------------
int OVI2::Open(LPCWSTR FileName,FileInfo *FI)
		{
		if(m_hFile!=nullptr) return OVI_NotOpen;	
		
		m_hFile=CreateFileW(FileName,
						 GENERIC_READ,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 nullptr,
						 OPEN_EXISTING,
						 FILE_ATTRIBUTE_READONLY,
						 nullptr);

		// �������� ���������
		if(m_hFile==INVALID_HANDLE_VALUE) 
			{
			m_hFile=nullptr;
			return OVI_NotOpen;
			}

		m_SizeOviFile=GetFileSize(m_hFile,NULL);

		m_Mod=0;		// ����� ������

		int i = sizeof(VideoChank2);
		// �������� ������ ��� ����� �������
		memset((void *)&m_VC,0,sizeof(VideoChank2));

		//  ������ ���������
		int ret=ReadHeader(&m_H_OVI);

		if(ret)	
			{
			CloseHandle(m_hFile);
			m_hFile=nullptr;
			return OVI_CrcHeader_FAIL;
			}

		if (m_H_OVI.Mod & Crypto)
			{
			AES_STREAM->Init(m_CifKeyForHeader);
			m_Crypto = 1;			// ��� �������
			}


		GetFileTime(m_hFile,&m_H_OVI.Time,nullptr,nullptr);     // ��������

		if(FI!=nullptr) GetFileInfo(FI);

		// �������� ������
		if(CreateBuff(&m_VideoBuff,1,0,m_VideoBufferSize)) return OVI_NotAlloc;  // ��� ���������� ������

		if(CreateBuff(&m_LocalFrame,1,0,m_SizeLocalFrame)) return OVI_NotAlloc;  // ��� ������ ����� �� ���������� �����

		if(m_H_OVI.CountVideoFrame>0) m_MaxVideoIndex=m_H_OVI.CountVideoFrame;
		if(CreateBuff((unsigned char **)&m_VideoIndexs,sizeof(ElementVideoIndex2),0,m_MaxVideoIndex)) return OVI_NotAlloc;

		m_LastVideoRefresh=m_H_OVI.FirstVideoFrame;
		m_LastAudioRefresh=m_H_OVI.FirstAudioSample;

		//Refresh();  // ���������� �������
		int ff=0;
		if(m_H_OVI.MainVideoIndex!=0L)
			{
			// ���� ������� ������ ��� �����
			if(MyRead((unsigned char *)m_VideoIndexs,m_H_OVI.CountVideoFrame*sizeof(ElementVideoIndex2),m_H_OVI.MainVideoIndex)) return OVI_InvalidIndex; 
			}
		else 
			{
			m_CurrentVideoFrame=0;
			Refresh(0);  // ���������� �������
			ff++;
			}
				
		// �������� ������������ ������ �����
		m_MaxVideoFrame=0;
		for(DWORD i=0;i<m_H_OVI.CountVideoFrame;i++)
			{
			if(m_MaxVideoFrame<((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame) m_MaxVideoFrame=((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame;
			}
		
		if(m_H_OVI.AudioCodec>0)
			{
			// ���� ����� �����
			if(m_H_OVI.CountAudioSample>0) m_MaxAudioIndex=m_H_OVI.CountAudioSample;

			if(CreateBuff(&m_AudioBuff,1,0,m_AudioBufferSize)) return OVI_NotAlloc;  // ��� ���������� ������

			if(CreateBuff((unsigned char **)&m_AudioIndexs,sizeof(ElementAudioIndex),0,m_MaxAudioIndex)) return OVI_NotAlloc;

			if(CreateBuff(&m_LocalSample,1,0,m_SizeLocalSample)) return OVI_NotAlloc;  // ��� ������ ������ �� ���������� �����


			if(m_H_OVI.MainAudioIndex!=0L)
				{
				// ���� ������� ������ ��� �����
				if(MyRead((unsigned char *)m_AudioIndexs,m_H_OVI.CountAudioSample*sizeof(ElementAudioIndex),m_H_OVI.MainAudioIndex)) return OVI_InvalidIndex; 
				}
			else 
				{
				Refresh(0);	// ���������� �������
			    //ff++;
				}
			}

		m_CurrentVideoChunk=0;
		m_LastReadFrame=0;
		m_VideoPosFrames=0;
	
		if(m_H_OVI.MaxSizeVideoFrame==0)
			{
			// �������� ������������ ������ �����

			for(DWORD i=0;i<m_H_OVI.CountVideoFrame;i++)
				{
				if(m_H_OVI.MaxSizeVideoFrame<((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame)
					
					m_H_OVI.MaxSizeVideoFrame=((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame;
				}
			}


		// ���� ������ - ��������
		if(ff>0) return OVI_NotClose;
				
		return S_OK;
		}


//---------------------------------------------------------------------------
//  ������� �� ������ ��� ���������
//---------------------------------------------------------------------------
int OVI2::OpenEx(LPCWSTR FileName,LPCWSTR Pass,FileInfo *FI)
	{
	return OVI_NotSupport;
	}


//---------------------------------------------------------------------------
// ������ �����
//---------------------------------------------------------------------------
int  OVI2::IsOpen()
	{
	if(m_hFile==nullptr)				return OVI_NotOpen;	

	return S_OK;
	}



//---------------------------------------------------------------------------
//  ������� ���������
//---------------------------------------------------------------------------
int OVI2::Close(FileInfo *FI)
		{
		if(m_hFile==nullptr)				return OVI_NotOpen;	

		if(m_Mod==0)		
			{
			// �� �� ������ � ��� �������
			CloseHandle(m_hFile);
			m_hFile=nullptr;

			return S_OK;
			}

		if(m_CurrentVideoFrame==0)	return OVI_Err1;
		
		// �������, ��� �������� � �������
		WrireGroupVideoFrames();
		WrireGroupAudioSamples();

		// ������� �������� ����
		m_VC.NextChank=0xFFFFFFFF;
		MyWrite((void *)&m_VC,12,0);

		// ������� �������
		DWORD Pos=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

		if(MyWrite(m_VideoIndexs,sizeof(ElementVideoIndex2)*(m_CurrentVideoFrame),0))
			{
			CloseHandle(m_hFile);
			m_hFile=nullptr;

			return OVI_NotWrite;
			}

		// ������� �����
		m_H_OVI.MainVideoIndex=Pos;
		m_H_OVI.CountVideoFrame=m_CurrentVideoFrame;
		
		m_H_OVI.Duration =static_cast<UINT>(((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-1].TimeFrame/m_average_frame_time);
		
		// �������� GOP
		m_H_OVI.GOP = 1;
		for (; m_H_OVI.GOP < m_H_OVI.CountVideoFrame; m_H_OVI.GOP++)
			{
			if (((ElementVideoIndex2 *)m_VideoIndexs)[m_H_OVI.GOP].TypeFrame == 1) break;
			}

		if(m_H_OVI.AudioCodec>0)
			{
			m_H_OVI.CountAudioSample=m_CurrentAudioSample;

			m_H_OVI.MainAudioIndex=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

			if(MyWrite(m_AudioIndexs,sizeof(ElementAudioIndex)*(m_CurrentAudioSample),0))
				{
				CloseHandle(m_hFile);
				m_hFile=nullptr;

				return OVI_NotWrite;
				}
			}

		WriteHeader(false);

		if(FI!=nullptr) GetFileInfo(FI);

		// ������� ����
		CloseHandle(m_hFile);
		m_hFile=nullptr;

		return S_OK;
		}


//---------------------------------------------------------------------------
//  ������� ExtarData
//---------------------------------------------------------------------------
int OVI2::SetExtraData(unsigned char *ExtraData,uint32_t Size)
		{
		if(m_hFile==nullptr)					return  OVI_NotOpen;	

		// � ���� ��� ������� ?
		if(Size<=m_H_OVI.SizeExtraData)			return 999;  // ���� ��� ��������� � ������

		if(m_H_OVI.SizeExtraData>0)				return 888;  // ���  �� ��������
		if(m_H_OVI.FirstVideoFrame>0)			return 888;  // ���  �� ��������
		if(m_H_OVI.FirstAudioSample>0)			return 888;  // ���  �� ��������
		
		m_H_OVI.ExtraData=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);  // ����������� ������ ExtraData

		if(MyWrite(ExtraData,Size,0))		return 999;

		m_H_OVI.SizeExtraData=Size;
				
 		WriteHeader(true);

		Flush();

		return S_OK;
		}

//---------------------------------------------------------------------------
//  ������� ExtraData
//---------------------------------------------------------------------------
int OVI2::GetExtraData(unsigned char *ExtraData,uint32_t Size,uint32_t *SizeExtraData)
		{
		if(m_hFile==nullptr)				return OVI_NotOpen;	

		if(ExtraData==nullptr)				return S_OK;

		if(m_H_OVI.ExtraData==0)			return OVI_ExtraNone;

		if(Size<m_H_OVI.SizeExtraData)		return 999;
						
		if(SizeExtraData!=nullptr)			*SizeExtraData=m_H_OVI.SizeExtraData;
		
		return MyRead(ExtraData,m_H_OVI.SizeExtraData,m_H_OVI.ExtraData);
		}

//---------------------------------------------------------------------------
//  ������� ����� ���� � ����� + ���������������� ������
//---------------------------------------------------------------------------
int OVI2::WriteVideoFrame(unsigned char  *Frame,uint32_t SizeFrame,int KeyFlag,uint64_t Time,unsigned char  *UserData,uint32_t SizeUserData)
		{
		if(m_hFile==nullptr)		return OVI_NotOpen;
        
        if(m_Mod==0)	            return OVI_ReadOnly;
		
		ElementVideoIndex2 *EVI2;

		//if( !(((FrameInfoHeader *)Frame)->mType & mtVideoFrame) )	return E_FAIL;  // �� ����� ��������

		// ������ �������� ��������� �� ������ - �� 1�
		if(m_CountVideoFrameIntoChunk==VIDEOMAXFRAMESINTOCHANC || m_SizeVideoFrames>VIDEOMAXSIZEBLOCK)
			{
			int res=WrireGroupVideoFrames();
			if(res) return res;
			}

        if(UserData==nullptr) SizeUserData=0;

        DWORD TotalSizeFrame=SizeFrame+SizeUserData;

		//  �������� ������������ ������ �����
		if(m_H_OVI.MaxSizeVideoFrame<TotalSizeFrame) m_H_OVI.MaxSizeVideoFrame=TotalSizeFrame;

        if (m_CurrentVideoFrame == m_MaxVideoIndex)
            {
            CreateBuff(&m_VideoIndexs, sizeof(ElementVideoIndex2), m_MaxVideoIndex, m_MaxVideoIndex + 1024);
            m_MaxVideoIndex += 1024;
            }

		//  ��������� ��������� ������
		EVI2 = &m_VC.IndexFrame[m_CountVideoFrameIntoChunk];
		EVI2->TypeFrame		= KeyFlag;
		EVI2->Position		= m_SizeVideoFrames;
		EVI2->SizeFrame		= SizeFrame;
		EVI2->SizeUserData	= SizeUserData;
		EVI2->TimeFrame		= Time;

		//  ��������� ���������� ������
		EVI2 = &((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame];
		EVI2->TypeFrame		= KeyFlag;
		EVI2->Position		= m_SizeVideoFrames;
		EVI2->SizeFrame		= SizeFrame;
		EVI2->SizeUserData	= SizeUserData;
		EVI2->TimeFrame		= Time;

		// �������� ���� � ������
		if(m_VideoBufferSize < m_SizeVideoFrames+TotalSizeFrame)
			{
			DWORD xx=m_SizeVideoFrames+TotalSizeFrame;

			CreateBuff(&m_VideoBuff,1,m_VideoBufferSize,xx);

			m_VideoBufferSize=xx;

			m_VideoFramesIntoBuffers=m_VideoBuff+m_SizeVideoFrames;

			}

        // ������� ��� ����
		memmove(m_VideoFramesIntoBuffers,(const void *)(Frame),m_VC.IndexFrame[m_CountVideoFrameIntoChunk].SizeFrame);
		int ret;
		if(m_Crypto && KeyFlag)
				ret=Encrypt(m_VideoFramesIntoBuffers, SizeFrame);
		
        // ������� ���������������� ������
		if(m_WriteMetaData)
			{	
			if (UserData != nullptr)
				{
				memmove(m_VideoFramesIntoBuffers + SizeFrame, (const void *)UserData, SizeUserData);
				}
			}

		// ���������� ������ � ���������
		Frame+=sizeof(ElementVideoIndex2);

		m_VideoFramesIntoBuffers+=TotalSizeFrame;

		m_SizeVideoFrames       +=TotalSizeFrame;

		m_CurrentVideoFrame++;

		m_CountVideoFrameIntoChunk++;

		return S_OK;
		}



//---------------------------------------------------------------------------
// ���������� ���������� �����. ���� 0 �� ���� �� ���������� � �����
//---------------------------------------------------------------------------
int OVI2::ReadVideoFrame(long IndexFrame,unsigned char *BuffFrame,uint32_t BuffSize,VideoFrameInfo *VFI)  
		{
		if(m_hFile==nullptr)			return OVI_NotOpen;

		if(IndexFrame<0)  IndexFrame=m_LastReadFrame;

		ElementVideoIndex2 *EVI2;

		EVI2 = &((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame];

        DWORD TotalSizeFrame=EVI2->SizeFrame + EVI2->SizeUserData;

		if(BuffFrame!=nullptr && BuffSize<=TotalSizeFrame) return 555;  // ���� ���� ���

		if(EVI2->VideoChank!=m_CurrentVideoChunk)
			{
			// ������� ��� ������
			ReadGroupVideoFrame(EVI2->VideoChank);
			
			// �������� �� �������
			m_CurrentVideoChunk= EVI2->VideoChank;

			}
		
		DWORD Pos= EVI2->Position-m_VideoPosFrames;

		if(BuffFrame!=nullptr)
			{
			// �������  ���� �� ������ ������
			memcpy(BuffFrame,m_VideoBuff+Pos,TotalSizeFrame);
			if (m_Crypto && EVI2->TypeFrame)
						Decrypt(BuffFrame, EVI2->SizeFrame);
			}
		else
			{
			// ��������� �� ���������� �����
			if(m_SizeLocalFrame<TotalSizeFrame) 
				{
				// �������� ����� �� 120 ���������
				DWORD xx=static_cast<DWORD>(TotalSizeFrame*1.2);

				if(CreateBuff(&m_LocalFrame,1,m_SizeLocalFrame,xx)) return OVI_NotAlloc;  // ��� ������ ����� �� ���������� �����

				m_SizeLocalFrame=xx;
				}

			memcpy(m_LocalFrame,m_VideoBuff+Pos,TotalSizeFrame); 

			if (m_Crypto && EVI2->TypeFrame)
						Decrypt(m_LocalFrame, EVI2->SizeFrame);
			}

		if(VFI!=nullptr)
			{
			// ������ ���������� � ������
			VFI->Codec       = m_H_OVI.VideoCodec;
			VFI->TypeFrame   = EVI2->TypeFrame;
			VFI->SizeFrame   = EVI2->SizeFrame;
            VFI->SizeUserData= EVI2->SizeUserData;
			VFI->TimeFrame = EVI2->TimeFrame; // 10;  // ����� ��� �������, � ����� �� �����
            VFI->Data        = m_LocalFrame;
			}

		return S_OK;
		}


//---------------------------------------------------------------------------
// ��������� ��������� �������� ����
//---------------------------------------------------------------------------
//int OVI2::ReadNextKeyVideoFrame(uint32_t IndexFrame,unsigned char *BuffFrame,uint32_t BuffSize,VideoFrameInfo *FI)
//		{
//		if(m_hFile==nullptr)		return OVI_NotOpen;
//
//		if(BuffFrame==nullptr)		return OVI_InvalidBuffer;
//
//		for(DWORD i=IndexFrame;i<m_H_OVI.CountVideoFrame;i++)
//			{
//			if(((ElementVideoIndex2 *)m_VideoIndexs)[i].TypeFrame) // �������� ?
//				{
//				// ��
//				if(FI!=nullptr)
//					{
//					// ������ ���������� � ������
//					FI->TypeFrame       =((ElementVideoIndex2 *)m_VideoIndexs)[i].TypeFrame;
//					FI->SizeFrame       =((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame;
//                    FI->SizeUserData    =((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeUserData;
//					FI->TimeFrame       =((ElementVideoIndex2 *)m_VideoIndexs)[i].TimeFrame/10;
//					}
//
//				return ReadVideoFrame(i,BuffFrame,BuffSize,FI);
//				}
//			}
//
//		return OVI_FrameNone;
//		}



//---------------------------------------------------------------------------
// ��������� ��������� ����
//---------------------------------------------------------------------------
//int OVI2::ReadNextVideoFrame(unsigned char *BuffFrame,uint32_t BuffSize,VideoFrameInfo *FI)
//		{
//		if(m_hFile==nullptr)		return OVI_NotOpen;
//
//		if((m_LastReadFrame+1)==(long)m_H_OVI.CountVideoFrame) 
//			{
//			if(m_H_OVI.MainVideoIndex==0) 	
//				{
//				//ov_util::TRACE("Refreh");
//				Refresh(5);  // ���� ��� �������
//				}
//			if((long)m_H_OVI.CountVideoFrame==(m_LastReadFrame+1))  return OVI_InvalidIndex; 
//			}
//
//		// �������� �� ���������
//		m_LastReadFrame++;
//
//		// ������ ����
//		return ReadVideoFrame(m_LastReadFrame,BuffFrame,BuffSize,FI);
//		}




//---------------------------------------------------------------------------
// ��������� ���������� � �����
//---------------------------------------------------------------------------
int OVI2::GetInfoVideoFrame(uint32_t IndexFrame,VideoFrameInfo *VFI)
	{
	if(m_hFile==nullptr) return OVI_NotOpen;
	
	if (IndexFrame<0 || IndexFrame>m_H_OVI.CountVideoFrame)  return -1;

	if (VFI != nullptr)
		{
		// ������ ���������� � ������
		VFI->Codec			= m_H_OVI.VideoCodec;
		VFI->TypeFrame		= ((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].TypeFrame;
		VFI->SizeFrame		= ((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].SizeFrame;
		VFI->Data			= nullptr;
		VFI->SizeUserData	= ((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].SizeUserData;
		VFI->TimeFrame		= ((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].TimeFrame / 10;
		}

	return S_OK;
	}



//---------------------------------------------------------------------------
//  ������ ����� ����� �� �������
//---------------------------------------------------------------------------
int OVI2::SeekVideoFrameByTime(uint64_t Time,uint32_t *IndexFrame)
	{
	if(m_hFile==nullptr) return OVI_NotOpen;

	if(m_H_OVI.MainVideoIndex==0) Refresh(5);


	DWORD	First=1,
			End= m_H_OVI.CountVideoFrame,
			ss;

	// ������� ����� ����� ������� �������� (�������� � ��� ����)
	ss = (End / 2)+1;
	do
		{
		if (Time  < ((ElementVideoIndex2 *)m_VideoIndexs)[ss].TimeFrame)
			{
			// ������ ��������
			End = ss+1;
			}
		else
			{
			if (Time == ((ElementVideoIndex2 *)m_VideoIndexs)[ss].TimeFrame) 
				{
				*IndexFrame = ss;
				return S_OK;
				}
			// ������� ��������
			First = ss+1;
			}
		ss = (End - First) / 2 + First;
			
		} while ((End - First) > 10);
		
	for(First--;First<End;First++)
		{
		if(Time <= ((ElementVideoIndex2 *)m_VideoIndexs)[First].TimeFrame)
			{
			// ����� ���� � �������� ������ ���������
			if(IndexFrame!=nullptr) *IndexFrame= First;
				
			return S_OK;
			}
		}

	return OVI_FrameNone;
	}

//---------------------------------------------------------------------------
//  ������ ���������� ��������
//---------------------------------------------------------------------------
long OVI2::SeekPreviosKeyVideoFrame(long IndexFrame)
	{
	if(m_hFile==nullptr)	return OVI_NotOpen;

	if(m_Mod==1)			return OVI_NotOpen;  // �������� ������ ��� �������� ������    
	// ... m_Mod=0

	if(IndexFrame<0)		IndexFrame=m_LastReadFrame;

	for(;IndexFrame>=0;IndexFrame--)
		{
		if(((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].TypeFrame==1) // �������� ?
			{
			return IndexFrame;
			}
		}

	return IndexFrame;
	}


//---------------------------------------------------------------------------
//  ������ ��������� ��������
//---------------------------------------------------------------------------
long OVI2::SeekNextKeyVideoFrame(long IndexFrame)
	{
	DWORD Index;

	if(m_hFile==nullptr)	return OVI_NotOpen;

	if(m_Mod==1)			return OVI_NotOpen;  // �������� ������ ��� �������� ������    
	// ... m_Mod=0

	if(IndexFrame<0)		Index=m_LastReadFrame+1;
	else					Index=IndexFrame+1;

    for(;Index<m_H_OVI.CountVideoFrame;Index++)
		{
		if(((ElementVideoIndex2 *)m_VideoIndexs)[Index].TypeFrame==1) // �������� ?
			{
			if(IndexFrame<0) m_LastReadFrame=Index-1;
			return Index;
			}
		}

	return Index;
	}


//---------------------------------------------------------------------------
//  ������� ����� ����� 
//---------------------------------------------------------------------------
int	OVI2::WriteAudioSample(unsigned char *AudioSample,uint32_t SizeSample,uint64_t Time)
	{
	if(m_hFile==nullptr)		return OVI_NotOpen;
        
    if(m_Mod==0)	            return OVI_ReadOnly;
		
	ElementAudioIndex *EAU;


	// ������ �������� ��������� �� ������ - �� 1�
	if(m_CountAudioSampleIntoChunk==AUDIOMAXFRAMESINTOCHANC || m_SizeAudioSamples>AUDIOMAXSIZEBLOCK)
		{
		int res=WrireGroupAudioSamples();
		if(res) return res;
		}
		//  �������� ������������ ������ �����
		if(m_H_OVI.MaxSizeAudioSample<SizeSample) m_H_OVI.MaxSizeAudioSample=SizeSample;

        if (m_CurrentAudioSample == m_MaxAudioIndex)
            {
            CreateBuff(&m_AudioIndexs, sizeof(ElementAudioIndex), m_MaxAudioIndex, m_MaxAudioIndex + 1024);
            m_MaxAudioIndex += 1024;
            }

		//  ��������� ��������� ������
		EAU = &m_AC.IndexSample[m_CountAudioSampleIntoChunk];
		EAU->Position		= m_SizeAudioSamples;
		EAU->SizeSample		= SizeSample;
		EAU->TimeSample		= Time;

		//  ��������� ���������� ������
		EAU = &((ElementAudioIndex *)m_AudioIndexs)[m_CurrentAudioSample];
		EAU->Position		= m_SizeAudioSamples;
		EAU->SizeSample		= SizeSample;
		EAU->TimeSample		= Time;

		// �������� ���� � ������
		if(m_AudioBufferSize < m_SizeAudioSamples+SizeSample)
			{
			DWORD xx=m_SizeAudioSamples+SizeSample;

			CreateBuff(&m_AudioBuff,1,m_AudioBufferSize,xx);

			m_AudioBufferSize=xx;

			m_AudioSamplesIntoBuffers=m_AudioBuff+m_SizeAudioSamples;

			}

        // ������� ��� ����
		memmove(m_AudioSamplesIntoBuffers,(const void *)(AudioSample),m_AC.IndexSample[m_CountAudioSampleIntoChunk].SizeSample);

		// ���������� ������ � ���������
		AudioSample+=sizeof(ElementAudioIndex);

		m_AudioSamplesIntoBuffers+=SizeSample;

		m_SizeAudioSamples       +=SizeSample;

		m_CurrentAudioSample++;

		m_CountAudioSampleIntoChunk++;

		return S_OK;
		}


//---------------------------------------------------------------------------
//  ������� ����� ����� 
//---------------------------------------------------------------------------
int OVI2::ReadAudioSample(uint32_t IndexSample,unsigned char *AudioSample,uint32_t SizeSample,AudioSampleInfo *ASI)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		ElementAudioIndex *AUI;

		AUI = &((ElementAudioIndex *)m_AudioIndexs)[IndexSample];

		if(AudioSample!=nullptr && SizeSample<=AUI->SizeSample) return 555;  // ���� ���� ���

		if(AUI->AudioChank!=m_CurrentAudioChunk)
			{
			// ������� ��� ������
			ReadGroupAudioSamples(AUI->AudioChank);
			
			// �������� �� �������
			m_CurrentAudioChunk= AUI->AudioChank;

			}
		
		DWORD Pos= AUI->Position-m_AudioPosSamples;  

		if(AudioSample!=nullptr)
			{
			// �������  ���� �� ������ ������
			memcpy(AudioSample,m_AudioBuff+Pos,SizeSample);
			}
		else
			{
			// ��������� �� ���������� �����
			if(m_SizeLocalSample<SizeSample) 
				{
				// �������� ����� �� 120 ���������
				DWORD xx=static_cast<DWORD>(SizeSample*1.2);

				if(CreateBuff(&m_LocalSample,1,m_SizeLocalSample,xx)) return OVI_NotAlloc;  // ��� ������ ����� �� ���������� �����

				m_SizeLocalSample=xx;
				}

			memcpy(m_LocalSample,m_AudioBuff+Pos,SizeSample); 
			}

		if(ASI!=nullptr)
			{
			// ������ ���������� � ������
			ASI->SizeFrame   = AUI->SizeSample;
			ASI->TimeFrame   = AUI->TimeSample; // 10;  // ����� ��� �������, � ����� �� �����
            ASI->Data        = m_LocalSample;
			}

		return OVI_S_OK;
		}


//---------------------------------------------------------------------------
//  ������ ����� ����� �� �������
//---------------------------------------------------------------------------
long OVI2::SeekAudioSampleByTime(uint64_t Time,uint32_t *IndexSample)
	{
	if(m_hFile==nullptr) return OVI_NotOpen;
	
	if(m_hFile==nullptr) return OVI_NotOpen;

	if(m_H_OVI.MainAudioIndex==0) Refresh(5);
	
	DWORD	First=1,
			End= m_H_OVI.CountAudioSample,
			ss;

	// ������� ����� ����� ������� �������� (�������� � ��� ����)
	ss = (End / 2)+1;
	do
		{
		if (Time  < ((ElementAudioIndex *)m_AudioIndexs)[ss].TimeSample)
			{
			// ������ ��������
			End = ss+1;
			}
		else
			{
			if (Time == ((ElementAudioIndex *)m_AudioIndexs)[ss].TimeSample) 
				{
				*IndexSample = ss;
				return S_OK;
				}
			// ������� ��������
			First = ss+1;
			}
		ss = (End - First) / 2 + First;
			
		} while ((End - First) > 10);
		
	for(First--;First<End;First++)
		{
		if(Time <= ((ElementAudioIndex *)m_AudioIndexs)[First].TimeSample)
			{
			// ����� ���� � �������� ������ ���������
			if(IndexSample!=nullptr) *IndexSample= First;
				
			return S_OK;
			}
		}

	return OVI_FrameNone;
	}




//---------------------------------------------------------------------------
//  ��������� ������ � ����� �� ���������
//---------------------------------------------------------------------------
int OVI2::SetFileInfo(FileInfo *FileInfo)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		if(FileInfo!=nullptr)
			{
			m_H_OVI.Mod				=FileInfo->Mod;

			// ��������� ������ ������ � �����
			m_H_OVI.VerOVSML		=FileInfo->VerOVSML;
            m_H_OVI.VideoCodec		=FileInfo->VideoCodec;
			m_H_OVI.Width			=FileInfo->Width;
			m_H_OVI.Height			=FileInfo->Height;
			m_H_OVI.VideoBitRate	=FileInfo->VideoBitRate;
			m_H_OVI.FPS				=FileInfo->FPS;

			m_H_OVI.Time			=FileInfo->Time;

			m_H_OVI.GOP				=FileInfo->GOP;
			m_H_OVI.Duration		=FileInfo->Duration;
			m_H_OVI.CountVideoFrame	=FileInfo->CountVideoFrame;

			m_H_OVI.AudioCodec		=FileInfo->AudioCodec;
			m_H_OVI.BitsPerSample	=FileInfo->AudioCodec;
			m_H_OVI.SamplesPerSec	=FileInfo->SamplesPerSec;
			m_H_OVI.CountAudioSample=FileInfo->CountAudioFrame;

			return WriteHeader(true);
			}

		return S_OK;
		}

//---------------------------------------------------------------------------
//  ��������� �������� ������ � ����� �� ���������
//---------------------------------------------------------------------------
int OVI2::GetFileInfo(FileInfo *FileInfo)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		if(FileInfo!=nullptr)
			{
			FileInfo->Mod				=m_Mod;

			FileInfo->Time				=m_H_OVI.Time;

			// ��������� ������ ������ � �����
			FileInfo->Ver				=m_H_OVI.Ver;
			FileInfo->VerOVSML			=m_H_OVI.VerOVSML;

			FileInfo->VideoCodec		=m_H_OVI.VideoCodec;
			FileInfo->Width				=m_H_OVI.Width;
			FileInfo->Height			=m_H_OVI.Height;
			FileInfo->VideoBitRate		=m_H_OVI.VideoBitRate;
			FileInfo->FPS				=m_H_OVI.FPS;

			FileInfo->GOP				=m_H_OVI.GOP;
			FileInfo->Duration			=m_H_OVI.Duration;
			FileInfo->CountVideoFrame	=m_H_OVI.CountVideoFrame;

			FileInfo->AudioCodec		=m_H_OVI.AudioCodec;
			FileInfo->BitsPerSample		=m_H_OVI.BitsPerSample;
			FileInfo->SamplesPerSec		=m_H_OVI.SamplesPerSec;
			FileInfo->CountAudioFrame	=m_H_OVI.CountAudioSample;

			FileInfo->SizeOviFile		=m_SizeOviFile;
			}

		return S_OK;
		}




//======================================================================================
//		� � � � � � � � �     � � � � � � � 
//======================================================================================
int OVI2::Flush()  
		{
		if(m_hFile==nullptr)	return OVI_NotOpen;

		if(!m_Mod)				return S_OK;

		m_H_OVI.CountVideoFrame=m_CurrentVideoFrame;

		c_Flush++;

		// ������� ����������
		if (c_Flush==m_Flush) 
			{
			//WriteHeader(true);

			FlushFileBuffers(m_hFile); 
			c_Flush = 0;
			}

		return S_OK;
		}


//
// ���������� ������
//
int OVI2::Refresh(int Count)  
		{
		if(m_hFile==nullptr)	return OVI_NotOpen;

		if(m_Mod)				return S_OK;

		VideoChank2 *p;
		
		p=&m_VC;

		//m_H_OVI.MainVideoIndex=0L;
		//m_CurrentVideoFrame=0;
		
		// �������� ������� �����
		if(m_H_OVI.MainVideoIndex==0)
			{
			ReadHeader(&m_H_OVI);
			if(Count==0) Count=9999;

            //  ��������� �������. 
			while(Count>0)
				{
				Count--;

				int ret=MyRead((unsigned char  *)&m_VC,8,m_LastVideoRefresh); //  ���� �� ��������� ������� 
				if(ret!=0)
					{
					int i=0;  // ��� ���� �� ���� ������
					return 0;
					}
				
				if(m_VC.CountFrameIntoChank>100)
					{
					int i=0;   // ��� ���� �� ���� ������
					}

				if(m_VC.NextChank==0)
					{
					int i=0;
					// ����������� ��������� �������� �������
					//m_H_OVI.FirstVideoFrame=Pos;
					break;
					}

				MyRead((unsigned char *)m_VC.IndexFrame,sizeof(ElementVideoIndex2)*m_VC.CountFrameIntoChank,0);
			
				// �������� �� ������������ ������
				if(m_MaxVideoIndex<m_CurrentVideoFrame+m_VC.CountFrameIntoChank)
					{
					// ����� ��������� ������
					int i=0;
					if(CreateBuff((unsigned char **)&m_VideoIndexs,sizeof(ElementVideoIndex2),m_MaxVideoIndex,m_MaxVideoIndex+1024)) return OVI_NotAlloc;
					m_MaxVideoIndex+=1024;
					}


				// ������� � ���������� ������
				ElementVideoIndex2 *EVI2;

				EVI2 = ((ElementVideoIndex2 *)m_VideoIndexs);

				for(DWORD i=0;i<m_VC.CountFrameIntoChank;i++) 
					{
					EVI2->TypeFrame		=m_VC.IndexFrame[i].TypeFrame;
					EVI2->TimeFrame		=m_VC.IndexFrame[i].TimeFrame;
					EVI2->Position		=m_VC.IndexFrame[i].Position;
					EVI2->SizeFrame		=m_VC.IndexFrame[i].SizeFrame;
					EVI2->VideoChank	=m_VC.IndexFrame[i].VideoChank;

					m_CurrentVideoFrame++;
					
					if(m_CurrentVideoFrame==m_MaxVideoIndex)
						{
						// �������� ������ ��� �������
						CreateBuff(&m_VideoIndexs,sizeof(ElementVideoIndex2),m_MaxVideoIndex,m_MaxVideoIndex+1024);
						m_MaxVideoIndex+=1024;
						}

					EVI2++;
					}

				m_LastVideoRefresh=m_VC.NextChank;
				memset(&m_VC,0,sizeof(VideoChank2));  // ������
				}
			m_H_OVI.CountVideoFrame=m_CurrentVideoFrame;

			return OVI_NextRefresh;
			}


		// �������� ������� �����
		if(m_H_OVI.MainAudioIndex==0)
			{
			// ���� ������ �������

			}

		return OVI_S_OK;
			
		}

//
//  ���������� ��� ������ ����� �� ������� �������
//
int OVI2::Read_Video_Frame(int Index,unsigned char *BuffFrame,DWORD BuffSize)
	{
	if(BuffFrame!=nullptr) 	
		{
		BuffSize=((ElementVideoIndex2 *)m_VideoIndexs)[Index].SizeFrame;
		BuffFrame=m_LocalFrame;

		if(m_SizeLocalFrame<BuffSize) 
			{
			// �������� �����
			if(CreateBuff(&m_LocalFrame,1,m_SizeLocalFrame,BuffSize)) return OVI_NotAlloc;  // ��� ������ ����� �� ���������� �����
			}
		}
	else 
		{
		if(BuffSize<((ElementVideoIndex2 *)m_VideoIndexs)[Index].SizeFrame) return OVI_Err2;
		}
	
	return ReadVideoFrame(Index,BuffFrame,BuffSize,nullptr);

	}



//======================================================================================
//		� � � � � � � � �     � � � � � � � 
//======================================================================================

//
// ��������� ������ ������
//
int OVI2::ReadGroupVideoFrame(uint32_t VideoChunk)  
		{
        VideoChank2			V_C;

		if(m_hFile==nullptr)			return OVI_NotOpen;

		if(VideoChunk==0)				return OVI_NotOpen;

		// ������� ��������� �����
		if(MyRead((unsigned char *)&V_C,12,VideoChunk)!=S_OK) return -1;

		// ��������� ��������� �������
		if(MyRead((unsigned char *)&V_C.IndexFrame,sizeof(ElementVideoIndex2) * V_C.CountFrameIntoChank,0)!=S_OK) return -2;

		// �������� ������ ���� ������
		DWORD Size=0;
        
    	for(DWORD i=0;i<V_C.CountFrameIntoChank;i++)
			{
            Size+=(V_C.IndexFrame[i].SizeFrame+V_C.IndexFrame[i].SizeUserData);
			}
       
		if(m_VideoBufferSize<Size) 
			{
			// �������� ����� ���� ����
			CreateBuff(&m_VideoBuff,1,m_VideoBufferSize,Size);
			m_VideoBufferSize=Size;
			}

		// �������� ������ ������
		m_VideoPosFrames=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

		// ������� ������ �����
		if(MyRead(m_VideoBuff,Size,0)!=S_OK) return -3;
				
		return S_OK;
		}


//
// ��������� ������ ����� ������
//
int OVI2::ReadGroupAudioSamples(uint32_t AudioChunk)  
		{
		AudioChank			A_C;  // ��������� �������

		if(m_hFile==nullptr)			return OVI_NotOpen;

		if(AudioChunk==0)				return OVI_NotOpen;

		// ������� ��������� �����
		if(MyRead((unsigned char *)&A_C,12,AudioChunk)!=S_OK) return -1;

		// ��������� ��������� �������
		if(MyRead((unsigned char *)&A_C.IndexSample,sizeof(ElementAudioIndex) * A_C.CountFrameIntoChank,0)!=S_OK) return -2;  

		// �������� ������ ���� ������
		DWORD Size=0;
        
		ElementAudioIndex *AUI=&A_C.IndexSample[0];

		// ��������� ������ ���� ������ � �����
    	for(DWORD i=1;i<A_C.CountFrameIntoChank;i++)
			{
			Size+=AUI->SizeSample;
			AUI++;
			}
       
		// � �������� ���
		if(m_AudioBufferSize<Size) 
			{
			// �������� ����� ���� ����
			CreateBuff(&m_AudioBuff,1,m_AudioBufferSize,Size);
			m_VideoBufferSize=Size;
			}

		// �������� ������ ������
		m_AudioPosSamples=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

		// ������� ������ �����
		if(MyRead(m_AudioBuff,Size,0)!=S_OK) return -3;

		return S_OK;
		}

//
//  ����� ������ ������.
//
int OVI2::WrireGroupVideoFrames()
		{
		int ret;

		if(m_hFile==nullptr) return OVI_NotOpen;

		if(m_Mod==0)		return OVI_ReadOnly;

		if(m_CountVideoFrameIntoChunk==0) return S_OK;

		// �������� ������ ��������� ����� �������
		int SizeHeaderChunc=((char *)&m_VC.IndexFrame[m_CountVideoFrameIntoChunk])-(char *)&m_VC;

		// ������� Next + Count
		m_VC.CountFrameIntoChank=m_CountVideoFrameIntoChunk;

		DWORD Pos=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);  // ������� �������

		if(m_H_OVI.FirstVideoFrame==0) 
			{
			m_H_OVI.FirstVideoFrame=sizeof(m_H_OVI)+m_H_OVI.SizeExtraData;
			WriteHeader(true);
			}

		m_VC.NextChank=Pos+SizeHeaderChunc+m_SizeVideoFrames;
		m_VideoLastNext=Pos;			// ���� �������� ������ ����, �� ����� ���������������

		// ������� ����� ���������
		Pos+=SizeHeaderChunc;
		
		DWORD VideoChank=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);  // ������� �������

		for(DWORD i=0;i<m_CountVideoFrameIntoChunk;i++)
			{
			m_VC.IndexFrame[i].Position+=Pos;

			((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-m_CountVideoFrameIntoChunk+i].Position+=Pos;

			m_VC.IndexFrame[i].VideoChank=VideoChank;
			((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-m_CountVideoFrameIntoChunk+i].VideoChank=VideoChank;
			}

		// ������� �������
		m_VC.SizeAllFrames=m_SizeVideoFrames;
		ret=MyWrite((void *)&m_VC,SizeHeaderChunc,0);
		
		// ������� �����
		ret=MyWrite(m_VideoBuff,m_SizeVideoFrames,0);

		// ������� ����� ������� � �������� �� �� ������
		memset(&m_VC,0,12);  // ������� ������ ���������

		ret=MyWrite((void *)&m_VC,12,0);
		Pos=SetFilePointer(m_hFile,-12L,nullptr,FILE_CURRENT);  // ����������������� � ������ �����������

		m_H_OVI.Duration =static_cast<DWORD>(m_VC.IndexFrame[m_CountVideoFrameIntoChunk-1].TimeFrame/m_average_frame_time);
		
		// ������� ����������
		m_VideoFramesIntoBuffers=m_VideoBuff;
		m_CountVideoFrameIntoChunk=0;
		m_SizeVideoFrames=0;
		
		Flush();
		return S_OK;
		}


//---------------------------------------------------------------------------
//  ������� ������ ����� ������� �� ����
//---------------------------------------------------------------------------
int OVI2::WrireGroupAudioSamples()
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		if(m_Mod==0)		return OVI_ReadOnly;

		if(m_CountAudioSampleIntoChunk==0) return S_OK;

		// �������� ������ ��������� ����� �������
		int SizeHeaderChunc=((char *)&m_AC.IndexSample[m_CountAudioSampleIntoChunk])-(char *)&m_VC;

		// ������� Next + Count
		m_AC.CountFrameIntoChank=m_CountAudioSampleIntoChunk;

		DWORD Pos=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);  // ������� �������

		if(m_H_OVI.FirstAudioSample==0 && m_H_OVI.FirstVideoFrame==0) 
			{
			m_H_OVI.FirstAudioSample=sizeof(m_H_OVI)+m_H_OVI.SizeExtraData;
			WriteHeader(true);
			}
		else
			{
			m_H_OVI.FirstAudioSample=Pos;
			}

		m_AC.NextChank=Pos+SizeHeaderChunc+m_SizeAudioSamples;
		m_AudioLastNext=Pos;			// ���� �������� ������ ����, �� ����� ���������������

		// ������� ����� ���������
		Pos+=SizeHeaderChunc;
		
		DWORD AudioChank=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);  // ������� �������

		for(DWORD i=0;i<m_CountAudioSampleIntoChunk;i++)
			{
			m_AC.IndexSample[i].Position+=Pos;

			((ElementAudioIndex *)m_AudioIndexs)[m_CurrentAudioSample-m_CountAudioSampleIntoChunk+i].Position+=Pos;

			m_AC.IndexSample[i].AudioChank=AudioChank;
			((ElementAudioIndex *)m_AudioIndexs)[m_CurrentAudioSample-m_CountAudioSampleIntoChunk+i].AudioChank=AudioChank;
			}

		// ������� �������
		m_AC.SizeAllSample=m_SizeAudioSamples;
		int ret=MyWrite((void *)&m_AC,SizeHeaderChunc,0);
		
		// ������� ������
		ret=MyWrite(m_AudioBuff,m_SizeAudioSamples,0);

		// ������� ����� ������� � �������� �� �� ������
		memset(&m_AC,0,12);  // ������� ������ ���������

		ret=MyWrite((void *)&m_AC,12,0);
		Pos=SetFilePointer(m_hFile,-12L,nullptr,FILE_CURRENT);  // ����������������� � ������ �����������
		
		// ������� ����������
		m_AudioSamplesIntoBuffers=m_AudioBuff;
		m_CountAudioSampleIntoChunk=0;
		m_SizeAudioSamples=0;
		
		Flush();

		return OVI_S_OK;
		}


//
//  ��� ������
//
DWORD OVI2::MyRead(unsigned char *Buff,DWORD SizeRead,DWORD Position)
		{
		DWORD Pos;
		
		if(Position>0)
			{
			// �������� � �������
			Pos=SetFilePointer(m_hFile,Position,nullptr,FILE_BEGIN);
			if(Pos!=Position) return GetLastError();
			}

		// �������
		if(!ReadFile(m_hFile,(void *)Buff,SizeRead,&Pos,nullptr))
			{
			return GetLastError();
			}

		if(Pos!=SizeRead) return OVI_Err3;

		return S_OK;
		}


//
//  ��� ������
//
DWORD OVI2::MyWrite(void *Buff,DWORD SizeBuff,DWORD Position)
		{
		DWORD Size;

		// �������� � ������ �����
		if(Position>0)
			{
			Size=SetFilePointer(m_hFile,Position,nullptr,FILE_BEGIN);
			if(Size!=0) return GetLastError();
			}

		// ������� ����
		if(!WriteFile(m_hFile,Buff,SizeBuff,&Size,nullptr))
			{
			return GetLastError();
			}

		if(Size!=SizeBuff) return OVI_Err4;

		return S_OK;
		}

//
//  ������� ��������� �����
//
int OVI2::WriteHeader(bool Flag)
		{
		DWORD Size,Pos;

		//delete m_VideoIndexs;

		// �������� � ������ �����
		if(Flag) Pos=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

		// �������� � ������ �����
		Size=SetFilePointer(m_hFile,0L,nullptr,FILE_BEGIN);
		if(Size!=0) return GetLastError();

		// �������� ����������� ����� ���������
		m_H_OVI.CrcHeader=Crc8((unsigned char *)&m_H_OVI,sizeof(Header_OVI)-1,0x25);
						
		// ������� ���������

		Header_OVI ss;
		memcpy(&ss, &m_H_OVI, sizeof(Header_OVI));

		AES_HEADER->Crypt((unsigned long *)&ss, sizeof(m_H_OVI));

		if(MyWrite((void *)&ss,sizeof(Header_OVI),0))
			{
			return GetLastError();
			}

		if(Flag) Size=SetFilePointer(m_hFile,Pos,nullptr,FILE_BEGIN);

		return S_OK; 
		}


//
//  ��������� ��������� �����
//
int  OVI2::ReadHeader(Header_OVI *HD)
		{
		DWORD Size;

		if(m_hFile==nullptr)		return OVI_NotOpen;	

		if(HD==nullptr)				return OVI_InvalidParam;

		AES_HEADER->Debug();

		// �������� � ������ �����
		Size=SetFilePointer(m_hFile,0L,nullptr,FILE_BEGIN);
		if(Size!=0) return GetLastError();

		// ��������� ������ ����������
		if(!ReadFile(m_hFile,&HD->Ver,sizeof(HD->Ver),&Size,nullptr))
			{
			return GetLastError();
			}

		// ������� ���������
		if(!ReadFile(m_hFile,(Header_OVI *)&HD->VerOVSML,sizeof(Header_OVI)-sizeof(HD->Ver),&Size,nullptr))
			{
			return GetLastError();
			}

		if(Size!=sizeof(m_H_OVI)-sizeof(HD->Ver)) return OVI_E_FAIL;

		AES_HEADER->Decrypt((unsigned long *)HD, sizeof(Header_OVI));
		
		// �������� ����������� ����� ���������
		unsigned char Crc=Crc8((unsigned char *)HD,sizeof(Header_OVI)-1,0x25);
		
		if(Crc != HD->CrcHeader) return OVI_CrcHeader_FAIL;

		return S_OK; 
		}



//
//  ������ ������ ��� �������� ������
//
int OVI2::CreateBuff(unsigned char **Buff,int SizeAtom,DWORD SizeBuff,DWORD NewSize)
		{

		if(SizeBuff==0)
			{
			// ������ �������
			//*Buff= new unsigned char [NewSize*SizeAtom];
			*Buff=(	unsigned char *)malloc(NewSize*SizeAtom);
			if(*Buff==nullptr) return OVI_NotAlloc;
			}
		else
			{
			// ���� ��������
			if(SizeBuff<NewSize)
				{
				// ������� ����� �����
				//unsigned char *NewBuff= new unsigned char[NewSize*SizeAtom];
				unsigned char *NewBuff= (unsigned char *)malloc(NewSize*SizeAtom);
				if(NewBuff==nullptr) return OVI_NotAlloc;

				// �������
				memset((void *)NewBuff,0,NewSize*SizeAtom);

				// ��������� ������
				memcpy(NewBuff,*Buff,SizeBuff*SizeAtom);

				//delete [] *Buff;
				free((void *)*Buff);
				
				// ������� �����
				*Buff=NewBuff;
				
				}
			}
		return S_OK;
		}


//
//   ��������� ������
//
char * OVI2::Errors(int Cod)
		{
		if(Cod==OVI_NotOpen)		return "���� �� ������";
				
		if(Cod==OVI_NotReadHeder)	return "�� ������ ��������� ���������";
				
		if(Cod==OVI_NotClose)		return "�� ������ ������� ����";

		return "None";
		}



//
//  ���������� ����. (�� ����������)
//
int OVI2::Recovery(LPCWSTR FileName)
	{
	if(m_hFile!=nullptr)		return OVI_NotOpen;

	// ������� �� ������
	m_hFile=CreateFileW(FileName,
						 GENERIC_READ | GENERIC_WRITE,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 nullptr,
						 CREATE_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
						 nullptr);

	// �������� ���������
	if(m_hFile==INVALID_HANDLE_VALUE) 
		{
		return OVI_Err4;
		}

	//  ������ ���������
	if(ReadHeader(&m_H_OVI)) return OVI_NotReadHeder;

	if(m_H_OVI.MainVideoIndex>0) return S_OK;

	// �������� ������ ��� ����� �������
	memset((void *)&m_VC,0,sizeof(VideoChank2));

	// ������� ����� ��� �������
	if(CreateBuff(&m_VideoIndexs,sizeof(ElementVideoIndex2),0,m_MaxVideoIndex))	return OVI_NotAlloc;

	//if(CreateBuff(&m_AudioIndexs,sizeof(ElementAudioIndex),0,m_MaxAudioIndex))	return OVI_NotAlloc;

	// ����� �������������� ������� ������
	Refresh(0);

	// ������� �������
	m_H_OVI.MainVideoIndex=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

	if(MyWrite(m_VideoIndexs,sizeof(ElementVideoIndex2)*(m_CurrentVideoFrame),0))
		{
		return OVI_NotWrite;
		}

	// ������� �����
	m_H_OVI.CountVideoFrame=m_CurrentVideoFrame;
		
	m_H_OVI.Duration =static_cast<UINT>(((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-1].TimeFrame/m_average_frame_time);
			
	WriteHeader(false);

	return S_OK;
	}


//
//  ���������
//
int OVI2::Encrypt(unsigned char *Buff,DWORD Size)
	{
	if(Size>128) Size = 128;

	AES_STREAM->Crypt((unsigned long *)Buff, Size);

	long *ss = (long *)(Buff + 2);
	int   xx = (Size / 4) - 2;

	for (int i = 0; i<xx; i=i + 2)
		ss[i] = ~ ss[i];  // ������� ��������
	
	return 0;
	}


//
//  ����������
//
int OVI2::Decrypt(unsigned char *Buff,DWORD Size)
	{
	if (Size>128) Size = 128;

	long *ss = (long *)(Buff + 2);
	int   xx = (Size / 4) - 2;

	for(int i=0;i<xx;i=i+2)
	      ss[i] = ~ ss[i];  // ������� ���������
	
	AES_STREAM->Decrypt((unsigned long *)Buff, Size);

	return 0;
	}

//
//  ����������
//
void OVI2::Debug(void)
{
}
