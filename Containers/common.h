#ifndef _COMMON_
#define _COMMON_

#include <Windows.h>

#include <stdint.h> 

struct Point
	{
	double	x;
	double  y;
	};


uint16_t Crc16(unsigned char * pcBlock, uint16_t len);

unsigned char Crc8(unsigned char *pcBlock, unsigned int len,unsigned char Old_crc);

void FillBitMap(void *BM, int Height, int Width);

enum 
	{
	AES_128_Nb= 4,
	AES_128_Nk= 4,
	AES_128_Nr= 10,

	AES_192_Nb = 4,
	AES_192_Nk = 6,
	AES_192_Nr = 12,

	AES_256_Nb = 4,
	AES_256_Nk = 8,
	AES_256_Nr = 14
	};


#define BYTE unsigned char
#define WORD unsigned long

class AES
	{
	private:
		int		m_level;
		int		m_Nb;
		int		m_Nk;
		int		m_Nr;

		WORD	m_ExpKey[AES_256_Nb*(AES_256_Nr + 1)];

		BYTE	PowTab[256];			// Таблица степеней примитивного элемента поля
		BYTE	LogTab[256];			// Обратная таблица (по значению можно найти степень)
		BYTE	SubBytesTab[256];		// Таблица афинного преобразования SubBytes
		BYTE	InvSubBytesTab[256];	// Таблица о братного преобразования SubBytes

		BYTE	MixCo[4];				// Коэффициенты для  преобразований  { 0x03, 0x01, 0x01, 0x02 };
		BYTE	InvMixCo[4];			// MixColumn и InvMixColumn          { 0x0B, 0x0D, 0x09, 0x0E }; 

	public:

		AES(int level);

		void Init(WORD *CifKey);

		void Crypt(WORD *in,DWORD size);

		void Decrypt(WORD *in,DWORD size);

		void Debug(void);

	private:
		WORD SubWord(WORD w);
		void GenSubBytesTab();
		BYTE SubBytes(BYTE x);
		void GenPowerTab();

		BYTE product(WORD x, WORD y);
		WORD MixCol(BYTE b[4]);
		WORD InvMixCol(BYTE b[4]);

		BYTE bmul(BYTE x, BYTE y);

		void MixColumn(WORD *in);
		void InvMixColumn(WORD *in);
		void KeyExpansion(WORD *key, WORD *ExKey);
		void AddRoundKey(WORD *in, WORD *key);
		void AES::Cipher(WORD *block, WORD *key);
		void InvCipher(WORD *block, WORD *key);
	};
#endif