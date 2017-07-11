#include <windows.h>

#include <stdint.h>

#include "decoder.h"

#include "common.h"

const uint16_t Crc16Table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

uint16_t Crc16(unsigned char * pcBlock, uint16_t len)
	{
    uint16_t crc = 0xFFFF;

    while (len--)
        crc = (crc << 8) ^ Crc16Table[(crc >> 8) ^ *pcBlock++];

    return crc;
	}

//
//  CRC 8 бит
//
unsigned char Crc8(unsigned char *pcBlock, unsigned int len, unsigned char Old_crc)
{
    unsigned char crc;		//0xFF;
    unsigned int i;

    crc = Old_crc;

    while (len--)
    {
        crc ^= *pcBlock++;

        for (i = 0; i < 8; i++)
            crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
    }

    return crc;
}


//
//    Создадим AES 128 192 или 256
//
AES::AES(int level)
	{
	m_level = level;

	switch (level)
		{
		case 128:
			m_Nb =AES_128_Nb;
			m_Nk =AES_128_Nk;
			m_Nr =AES_128_Nr;
			break;

		case 192:
			m_Nb = AES_192_Nb;
			m_Nk = AES_192_Nk;
			m_Nr = AES_192_Nr;
			break;

		case 256:
			m_Nb = AES_256_Nb;
			m_Nk = AES_256_Nk;
			m_Nr = AES_256_Nr;
			break;

		default:
			m_level = 0;
		}

	MixCo[0] = 0x03;
	MixCo[1] = 0x01;
	MixCo[2] = 0x01;
	MixCo[3] = 0x02;

	InvMixCo[0] = 0x0B;
	InvMixCo[1] = 0x0D;
	InvMixCo[2] = 0x09;
	InvMixCo[3] = 0x0E;

	}	

//
//		Инициализация
//
void AES::Init(WORD *CifKey)
	{
	GenPowerTab();
	GenSubBytesTab();

	KeyExpansion(CifKey,m_ExpKey);
	}


//
//		Зашифруем
//
void AES::Crypt(WORD *in, DWORD size)
	{
	while (size >= m_Nb*4)
		{

		AddRoundKey(in, m_ExpKey);

		Cipher(in, m_ExpKey);

		size -= (m_Nb*4);
		in   += m_Nb;
		}
	}

//
//		Дешифруем
//
void AES::Decrypt(WORD *in,DWORD size)
	{

	while (size >= m_Nb*4)
		{
		AddRoundKey(in, &m_ExpKey[4 * m_Nr]);

		InvCipher(in, m_ExpKey);

		size -= (m_Nb*4);
		in += m_Nb;
		}
	}


// Циклический сдвиг байта влево на 1
#define ROTL(x)			(((x)>>7)|((x)<<1))
#define ROTL8(x)		(((x)<<8)|((x)>>24))
#define ROTL16(x)		(((x)<<16)|((x)>>16))
#define ROTL24(x)		(((x)<<24)|((x)>>8))

#define ij2n(i,j)		4*(j)+i
#define pack(b)			*(WORD*)&*b

//  Операция умножения элемента поля GF(2^8) a на x.
//
BYTE xtime(BYTE a)
	{
	BYTE b;

	if (a & 0x80)    // Проверка старшего бита на 1
		b = 0x1b;    // Маска возврата
	else
		b = 0;        // Маски возврата нет
	a <<= 1;            // Поразрядный сдвиг
	a ^= b;

	return a;
	}


// Перемножение элементов поля GF(2^8)
//
BYTE AES::bmul(BYTE x, BYTE y)
	{
	if (x && y)
		return PowTab[(LogTab[x] + LogTab[y]) % 255];
	else
		return 0;
	}

// Перемножает два многочлена над полем GF(2^8)
//
BYTE AES::product(WORD x, WORD y)
	{
	BYTE* xb = (BYTE*)&x;
	BYTE* yb = (BYTE*)&y;
	return bmul(xb[0], yb[0]) ^ bmul(xb[1], yb[1]) ^
		bmul(xb[2], yb[2]) ^ bmul(xb[3], yb[3]);
	}

// Генерит таблицы степеней примитивного элемента поля и обратную таблицу
//
void AES::GenPowerTab()
	{
	LogTab[0] = 0;
	PowTab[0] = 1;    // в нулевой степени 1
	PowTab[1] = 3;    // сам примитивный элемент
	LogTab[1] = 0;    // стазу же генерим обратную таблицу
	LogTab[3] = 1;
	for (int i = 2; i < 256; i++)
		{
		PowTab[i] = PowTab[i - 1] ^ xtime(PowTab[i - 1]);
		LogTab[PowTab[i]] = i;
		}
	}

//
//
BYTE AES::SubBytes(BYTE x)
	{
	BYTE y = PowTab[255 - LogTab[x]]; // Нахождение обратного элемента
	x = y;     x = ROTL(x);
	y ^= x;    x = ROTL(x);
	y ^= x;    x = ROTL(x);
	y ^= x;    x = ROTL(x);
	y ^= x;    y ^= 0x63;
	return y;
	}

// Генерит прямую и обратную таблицы преобразования Subbytes
//
void AES::GenSubBytesTab()
	{
	SubBytesTab[0] = 0x63; // инициализирующий элемент
	InvSubBytesTab[0x63] = 0;
	for (int i = 1; i < 256; i++)
		{
		BYTE y = SubBytes((BYTE)i);
		SubBytesTab[i] = y;
		InvSubBytesTab[y] = i;
		}
	}

// Преобразование сдвига рядов
//
void ShiftRows(WORD *in)
	{
	ROTL24(in[1]);
	ROTL16(in[2]);
	ROTL8(in[3]);
	}

// Обратное преобразование сдвига рядов
//
void InvShiftRows(WORD *in)
	{
	ROTL8(in[1]);
	ROTL16(in[2]);
	ROTL24(in[3]);
	}

//
//
WORD AES::MixCol(BYTE b[4])
	{
	BYTE s[4];
	s[0] = bmul(0x2, b[0]) ^ bmul(0x3, b[1]) ^ b[2] ^ b[3];
	s[1] = b[0] ^ bmul(0x2, b[1]) ^ bmul(0x3, b[2]) ^ b[3];
	s[2] = b[0] ^ b[1] ^ bmul(0x2, b[2]) ^ bmul(0x3, b[3]);
	s[3] = bmul(0x3, b[0]) ^ b[1] ^ b[2] ^ bmul(0x2, b[3]);
	return pack(s);
	}

//
//
WORD AES::InvMixCol(BYTE b[4])
	{
	BYTE s[4];
	
	s[0] = bmul(0xe, b[0]) ^ bmul(0xb, b[1]) ^ bmul(0xd, b[2]) ^ bmul(0x9, b[3]);
	s[1] = bmul(0x9, b[0]) ^ bmul(0xe, b[1]) ^ bmul(0xb, b[2]) ^ bmul(0xd, b[3]);
	s[2] = bmul(0xd, b[0]) ^ bmul(0x9, b[1]) ^ bmul(0xe, b[2]) ^ bmul(0xb, b[3]);
	s[3] = bmul(0xb, b[0]) ^ bmul(0xd, b[1]) ^ bmul(0x9, b[2]) ^ bmul(0xe, b[3]);

	return pack(s);
	}

//
//
void AES::MixColumn(WORD *in)
	{
	for (int i = 0; i < m_Nb; i++)
		in[i] = MixCol((BYTE*)&in[i]);
	}

//
//
void AES::InvMixColumn(WORD *in)
	{
	for (int i = 0; i < m_Nb; i++)
		in[i] = InvMixCol((BYTE*)&in[i]);
	}

//
//
WORD AES::SubWord(WORD w)
	{
	BYTE* b = (BYTE*)&w;
	for (int i = 0; i < 4; i++)
		b[i] = SubBytesTab[b[i]];
	return w;
	}

//
// Выполняет расширение ключа
//
void AES::KeyExpansion(WORD *key, WORD *ExKey)
	{
	for (int i = 0; i<m_Nk; i++)
		ExKey[i] = key[i];

	BYTE xi = 0x01;
	WORD temp;
	for (int i = m_Nk; i<m_Nb*(m_Nr + 1); i++)
		{
		temp = ExKey[i - 1];
		if (i % m_Nk == 0)
			{
			WORD w = xi;
			w = ROTL24(xi);
			temp = SubWord(ROTL8(temp)) ^ w;
			xi = xtime(xi);
			}
		//else
		//    temp = SubWord(temp);
		ExKey[i] = ExKey[i - m_Nk] ^ temp;
		}
	}

//
//
void AES::AddRoundKey(WORD *in, WORD *key)
{
	for (int i = 0; i < m_Nb; i++)
		in[i] ^= key[i];
}

//
//		Кодируем
//
void AES::Cipher(WORD *block, WORD *key)
	{
	for (int round = 1; round<m_Nr; round++)
		{
		// ByteSub
		for (int i = 0; i<m_Nb; i++)
			{
			BYTE* temp = (BYTE*)&block[i];
			for (int j = 0; j < 4; j++)
				temp[j] = SubBytesTab[temp[j]];
			block[i] = pack(temp);
			}

		// ShiftRows
		ShiftRows(block);

		// MixColumn
		MixColumn(block);

		// AddRoundKey
		AddRoundKey(block, &key[4 * round]);
		}

	for (int i = 0; i<m_Nb; i++)
		{
		BYTE* temp = (BYTE*)&block[i];
		for (int j = 0; j < 4; j++)
			temp[j] = SubBytesTab[temp[j]];
		block[i] = pack(temp);
		}

	ShiftRows(block);

	AddRoundKey(block, &key[4 * m_Nr]);
	}

//
//		Декодируем
//
void AES::InvCipher(WORD *block, WORD *key)
{
	for (int round = m_Nr - 1; round>0; round--)
		{
		// InvShiftRows
		InvShiftRows(block);

		// InvByteSub
		for (int i = 0; i<m_Nb; i++)
			{
			BYTE* temp = (BYTE*)&block[i];
			
			for (int j = 0; j < 4; j++)
				temp[j] = InvSubBytesTab[temp[j]];

			block[i] = pack(temp);
			}

		// AddRoundKey
		AddRoundKey(block, &key[4 * round]);

		// InvMixColumn
		InvMixColumn(block);
		}

	// InvShiftRows
	InvShiftRows(block);

	// InvByteSub
	for (int i = 0; i<m_Nb; i++)
		{
		BYTE* temp = (BYTE*)&block[i];
		for (int j = 0; j < 4; j++)
			temp[j] = InvSubBytesTab[temp[j]];
		
		block[i] = pack(temp);
		}

	// AddRoundKey
	AddRoundKey(block, &key[0]);
}

void AES::Debug(void)
	{
	int i = 0;
	}

//
//	  Заполнение header + info бит мап
//
void FillBitMap(void *BM,int Height, int Width)
	{
	if (BM == nullptr) return;

	memset(BM, 0, sizeof(BMP));

    ((BMP *)BM)->BM_Heade.bfType = 19778;
	((BMP *)BM)->BM_Heade.bfOffBits = 54;
	((BMP *)BM)->BM_Heade.bfSize = Height*Width * 3 + sizeof(BMP);

	((BMP *)BM)->BM_Info.biBitCount = 24;
	((BMP *)BM)->BM_Info.biCompression = BI_RGB;
	((BMP *)BM)->BM_Info.biPlanes = 1;
	((BMP *)BM)->BM_Info.biSize = sizeof(BITMAPINFOHEADER);
	}
