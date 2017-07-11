#ifndef _METADATA_
#define _METADATA_

#include <windows.h>
#include <stdint.h>


#include "..\container.h"

#include "..\common.h"

#pragma pack(push,1)

struct Header_MTD								    // ��������� ��������� �����
	{
    unsigned char		Ver;						// ������ ����������
    unsigned char		VerOVSML;					// ������ �������

	unsigned int		Mod;

	FILETIME			Time;

	// ��������� ������
	int					Height;						// ������
	int					Width;						// ������
	int					MinObject;					// ����������� ������ �������

	// �������� ����� �����������
	int					CountLines;					// ���������� �����
	int					PointLines;					// ������ � �����

	// �������� ��� ��������
	int					CountZome;					// ���������� ���
	int					PointZone;					// ������ ��� � �����

	int					FirstMetaDataFrame;			// ������ ���� ������

	// �������
	int					PointMetaData;				// ������ ������� ����������
	
	int					CountMataDataFrame;
	
	int					MainMetaDataIndex;			// ������� ������
                                                    
    DWORD				CrcFile;					// ����������� ����� �����
    unsigned char		CrcHeader;					// ����������� ����� ���������
	};


struct ElementMetaIndex
	{
	uint64_t			TimeFrame;					// ��������� �����

    DWORD				Position;					//  
    uint16_t			Size;						// ������ 

    DWORD				MetaDataChank;
	};
	

struct MetaChank
	{
    DWORD					NextChank;				            // ��������� ���� � �����
    DWORD					CountFrameIntoChank;	            // ���������� ������� � �����
	DWORD					Origanal;
	DWORD					Zip;
    ElementMetaIndex		IndexFrame[200];					// ������ ����� ��� ������� 
                                                                // � ����� �������� ���� �����
	};
#pragma pack(pop)


struct Line
	{
	int		x;
	int		y;
	int		angle;
	int		direction;
	};

struct Zone
	{
	int		Status;
	Point	xy1;
	Point	xy2;
	};

	
enum
	{
	MAXFRAMESINTOCHANC2	= 200,
	MAXSIZEBLOCK2		= 1024 * 32
	};

struct MovingTarget  // �� ����
	{
	DWORD		dwZoneID;
	DWORD		dwObjID;
	LONG		nLeft;		// y1
	LONG		nTop;		// x1
	LONG		nRight;		// y2
	LONG		nBottom;	// x2
	double		fMovAngle;
	};



struct MovingTarget2  // �� ����
	{
	uint16_t	nTop;		// x1
	uint16_t	nLeft;		// y1
	
	uint16_t	nRight;		// y2
	uint16_t	nBottom;	// x2
	
	float		fMovAngle;
	};



using namespace Meta_Data;

class   MTD : public MetaData
{
private: 
	int					m_VerOVMSML;
	HANDLE		 		m_hLog;						// 

	HANDLE				m_hFile;

	unsigned int		m_SizeLocalBuff;
	unsigned char		*m_LocalBuff;
	
	Header_MTD			m_H_MTD;

	int					m_Crypto;

	int					m_Flags;
	int					m_Flush,c_Flush;
	int					m_Mod;

	MetaChank			m_MC;
	int					m_SizeMetaDataFrames;

	int					m_CountMataDataFrame;		// ����� ������ � �����������

	unsigned char		*m_MetaDataBuff;			// ��������� �����
	int					m_MetaDataBufferSize;		// ��� ������
	                    
	unsigned char		*m_ZipMetaDataBuff;			// ������������ �����
	int					m_ZipMetaDataBuffSize;		// ��� �����

	unsigned char		*m_MetaDataIntoBuffers;		// ������� ������� � ������

	int					m_CurrentSizeMetaData;
	int					m_MetaDataLastNext;

	int					m_PosMetaData;

	
	int					m_CountMetaDataFrameIntoChunk;
	int					m_CurrentMetaDataChunk;
	int					m_LastReadFrame;

	ElementMetaIndex	*m_MetaDataIndexs;
	int					m_MaxMetaDataIndex;

	
public:
	MTD(int VerOVSML);
	~MTD();

	void 				Version(V_e_r*);

	int					Create(LPCWSTR FileName, MetaDataFileInfo *FileInfo);

	int					CreateEx(LPCWSTR FileName, MetaDataFileInfo *FI, LPCWSTR Pass);

	int					Open(LPCWSTR FileName, MetaDataFileInfo *FI);

	int					OpenEx(LPCWSTR FileName, MetaDataFileInfo *FI, LPCWSTR Pass);

	int					IsOpen();

	int					WriteLineAndZone(int CountLines, Line *,int CountZones, Zone *);

	// ������������� ��������� �� ������ ��� �� ���������� (200 ������ � �����)
	//
	int					WriteMetaData(unsigned char  *MetaData, DWORD Size, uint64_t Time);

	int					ReadMetaData(long IndexFrame,unsigned char *BuffFrame,DWORD BuffSize, MetaDataInfo *MDI);

	int					SeekMetaDataByTime(uint64_t Time,DWORD *IndexFrame);

	int					Refresh(int Count);
	
	int					Flush();
	
	int					Close(MetaDataFileInfo *FI);

	char *				Errors(int Cod);

	int					Recovery(LPCWSTR FileName);

	unsigned char *		GetLocalBufer();


	int					ParserZone(unsigned char *Zones, int Size, MovingTarget2 **);


private: 
	int		WrireGroupMetaData();

	int		ReadGroupMetadata(DWORD MetaDataChunk);
	
	int		CreateBuff(unsigned char **Buff, int SizeAtom, DWORD SizeBuff, DWORD NewSize);

	int		WriteHeader(bool Flag);
	
	int		ReadHeader(Header_MTD *HD);

	DWORD	MyRead(unsigned char  *Buff,DWORD SizeBuff,DWORD Position);
	
	DWORD	MyWrite(void *Buff,DWORD SizeBuff,DWORD Position);

	int		Logs(void *Buff,DWORD Size);

	int		Encrypt(unsigned char *Buff,DWORD Size);

	int		Decrypt(unsigned char *Buff,DWORD Size);
};

#endif