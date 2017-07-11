/////////////////////////////////////////////////////////////////////////
//        Реализация криптоалгоритма AES
//            by SiAVoL
//
//        В криптоалгоритме AES длина входного и выходного блоков,
//        а так же состояния 128 бит. Это означает что Nb=4, что
//        отражает число 32-х битных слов (число столбцов) в состоянии.
//        В криптоалгоритме AES длина ключа шифра K может быть 128, 192 или
//        256 бит. Т.о. Nk = 4, 6 или 8, что покпзывает количество 32-х
//        битных слов (число столбцов) в ключе.
//        Следующая таблица показывает зависимость числа раундов шифра от
//        длины ключа
//
//                ! Длина    ! Длина    ! Число        !
//                ! ключа    ! блока    ! раундов    !
//                !  Nk    !   Nb    !   Nr        !
//        -------------------------------------
//        AES-128 !    4    !    4    !    10        !
//        AES-192 !    6    !    4    !    12        !
//        AES-256 !    8    !    4    !    14        !
//

//#include "stdafx.h"
#include "windows.h"
#include "stdlib.h"
#include <stdio.h>
#include <tchar.h>
//#include "AES.h"

// Циклический сдвиг байта влево на 1
#define ROTL(x)        (((x)>>7)|((x)<<1))
#define ROTL8(x)    (((x)<<8)|((x)>>24))
#define ROTL16(x)    (((x)<<16)|((x)>>16))
#define ROTL24(x)    (((x)<<24)|((x)>>8))

#define ij2n(i,j)    4*(j)+i
#define file_len(x) (unsigned long)x
#define pack(b)        *(WORD*)&*b

#define BYTE unsigned char
#define WORD unsigned long

static BYTE PowTab[256];        // Таблица степеней примитивного элемента поля
static BYTE LogTab[256];        // Обратная таблица (по значению можно найти степень)
static BYTE SubBytesTab[256];    // Таблица афинного преобразования SubBytes
static BYTE    InvSubBytesTab[256];// Таблица обратного преобразования SubBytes

static BYTE MixCo[4] = {0x03, 0x01, 0x01, 0x02};    // Коэффициенты для  преобразований
static BYTE InvMixCo[4] = {0x0B, 0x0D, 0x09, 0x0E};    // MixColumn и InvMixColumn

#define Nb 4
#define Nk 4
#define Nr 10


//#define LOGit // показывать промежуточные результаты
#define PERFOMANCE // показывать временные замеры


void log_it(char* msg, WORD* block)
{
    #ifdef LOGit
        printf(msg);
        for(int i=0; i<4; i++)
            printf("%08x\n", block[i]);
        printf("\n\n");
    #endif
}

//  Операция умножения элемента поля GF(2^8) a на x.
static BYTE xtime(BYTE a)
{
    BYTE b;
    if (a & 0x80)    // Проверка старшего бита на 1
        b = 0x1b;    // Маска возврата
    else
        b = 0;        // Маски возврата нет
    a<<=1;            // Поразрядный сдвиг
    a^=b;            
    return a;
}


// Перемножение элементов поля GF(2^8)
static BYTE bmul(BYTE x, BYTE y)
{
    if (x && y)
        return PowTab[(LogTab[x] + LogTab[y])%255];
    else
        return 0;
}

// Перемножает два многочлена над полем GF(2^8)
static BYTE product(WORD x, WORD y)
{
    BYTE* xb = (BYTE*)&x;
    BYTE* yb = (BYTE*)&y;
    return bmul(xb[0], yb[0]) ^ bmul(xb[1], yb[1]) ^
        bmul(xb[2], yb[2]) ^ bmul(xb[3], yb[3]);
}

// Генерит таблицы степеней примитивного элемента поля
// и обратную таблицу
void GenPowerTab()
{
    LogTab[0] = 0;
    PowTab[0] = 1;    // в нулевой степени 1
    PowTab[1] = 3;    // сам примитивный элемент
    LogTab[1] = 0;    // стазу же генерим обратную таблицу
    LogTab[3] = 1;
    for (int i = 2; i < 256; i++)
    {
        PowTab[i] = PowTab[i-1]^xtime(PowTab[i-1]);
        LogTab[PowTab[i]] = i;
    }
}

BYTE SubBytes(BYTE x)
{
    BYTE y = PowTab[255 - LogTab[x]]; // Нахождение обратного элемента
    x = y;    x = ROTL(x);
    y ^= x;    x = ROTL(x);
    y ^= x;    x = ROTL(x);
    y ^= x;    x = ROTL(x);
    y ^= x;    y ^= 0x63;
    return y;
}

// Генерит прямую и обратную таблицы преобразования Subbytes
void GenSubBytesTab()
{
    SubBytesTab[0] = 0x63; // инициализирующий элемент
    InvSubBytesTab[0x63] = 0;
    for (int i = 1; i < 256; i++)
    {
        BYTE y = SubBytes((BYTE) i);
        SubBytesTab[i] = y;
        InvSubBytesTab[y] = i;
    }
}

// Преобразование сдвига рядов
void ShiftRows(WORD in[Nb])
{
    ROTL24(in[1]);
    ROTL16(in[2]);
    ROTL8(in[3]);
}

// Обратное преобразование сдвига рядов
void InvShiftRows(WORD in[Nb])
{
    ROTL8(in[1]);
    ROTL16(in[2]);
    ROTL24(in[3]);
}

static WORD MixCol(BYTE b[4])
{
    BYTE s[4];
    s[0] = bmul(0x2, b[0]) ^ bmul(0x3, b[1]) ^ b[2] ^ b[3];
    s[1] = b[0] ^ bmul(0x2, b[1]) ^ bmul(0x3, b[2]) ^ b[3];
    s[2] = b[0] ^ b[1] ^ bmul(0x2, b[2]) ^ bmul(0x3, b[3]);
    s[3] = bmul(0x3, b[0]) ^ b[1] ^ b[2] ^ bmul(0x2, b[3]);
    return pack(s);
}

static WORD InvMixCol(BYTE b[4])
{
    BYTE s[4];
    s[0] = bmul(0xe, b[0]) ^ bmul(0xb, b[1]) ^ bmul(0xd, b[2]) ^ bmul(0x9, b[3]);
    s[1] = bmul(0x9, b[0]) ^ bmul(0xe, b[1]) ^ bmul(0xb, b[2]) ^ bmul(0xd, b[3]);
    s[2] = bmul(0xd, b[0]) ^ bmul(0x9, b[1]) ^ bmul(0xe, b[2]) ^ bmul(0xb, b[3]);
    s[3] = bmul(0xb, b[0]) ^ bmul(0xd, b[1]) ^ bmul(0x9, b[2]) ^ bmul(0xe, b[3]);
    return pack(s);
}

void MixColumn(WORD in[Nb])
{
    for(int i = 0; i < Nb; i++)
        in[i] = MixCol((BYTE*)&in[i]);
}

void InvMixColumn(WORD in[Nb])
{
    for(int i = 0; i < Nb; i++)
        in[i] = InvMixCol((BYTE*)&in[i]);
}

WORD SubWord(WORD w)
{
    BYTE* b = (BYTE*)&w;
    for (int i = 0; i < 4; i++)
        b[i] = SubBytesTab[b[i]];
    return w;
}

// Выполняет расширение ключа
void KeyExpansion(WORD key[Nk], WORD ExKey[Nb*(Nr+1)])
{
    for(int i=0; i<Nk; i++)
        ExKey[i] = key[i];

    BYTE xi = 0x01;
    WORD temp;
    for(int i=Nk; i<Nb*(Nr+1); i++)
    {
        temp = ExKey[i-1];
        if (i % Nk == 0)
        {
            WORD w = xi;
            w = ROTL24(xi);
            temp = SubWord(ROTL8(temp)) ^ w;
            xi = xtime(xi);
        }
        //else
        //    temp = SubWord(temp);
        ExKey[i] = ExKey[i-Nk] ^ temp;
    }
}

void AddRoundKey(WORD in[Nb], WORD key[Nb])
{
    #ifdef LOGit
    printf("Round key:\n");
    printf("%08x %08x %08x %08x\n", key[0], key[1],
            key[2], key[3]);
    printf("\n\n");
    #endif

    for(int i = 0; i < Nb; i++)
        in[i] ^= key[i];
}

void Cipher(WORD block[Nb], WORD key[Nb*(Nr+1)])
{
    for(int round=1; round<Nr; round++)
    {
        log_it("Round %d started with:\n", block);

        // ByteSub
        for(int i=0; i<Nb; i++)
        {
            BYTE* temp = (BYTE*)&block[i];
            for (int j = 0; j < 4; j++)
                temp[j] = SubBytesTab[temp[j]];
            block[i] = pack(temp);
        }
        log_it("After ByteSub\n", block);

        // ShiftRows
        ShiftRows(block);
        log_it("After ShiftRows\n", block);

        // MixColumn
        MixColumn(block);
        log_it("After MixColumn\n", block);

        // AddRoundKey
        AddRoundKey(block, &key[4*round]);
        log_it("After AddRoundKey\n", block);
    }

    for(int i=0; i<Nb; i++)
    {
        BYTE* temp = (BYTE*)&block[i];
        for (int j = 0; j < 4; j++)
            temp[j] = SubBytesTab[temp[j]];
        block[i] = pack(temp);
    }

    ShiftRows(block);

    AddRoundKey(block, &key[4*Nr]);
    log_it("After AddRoundKey\n", block);
}

void InvCipher(WORD block[Nb], WORD key[Nb*(Nr+1)])
{
    for(int round=Nr-1; round>0; round--)
    {
        log_it("Round %d started with:\n", block);

        // InvShiftRows
        InvShiftRows(block);
        log_it("After InvShiftRows\n", block);

        // InvByteSub
        for(int i=0; i<Nb; i++)
        {
            BYTE* temp = (BYTE*)&block[i];
            for (int j = 0; j < 4; j++)
                temp[j] = InvSubBytesTab[temp[j]];
            block[i] = pack(temp);
        }
        log_it("After InvByteSub\n", block);

        // AddRoundKey
        AddRoundKey(block, &key[4*round]);
        log_it("After AddRoundKey\n", block);

        // InvMixColumn
        InvMixColumn(block);
        log_it("After InvMixColumn\n", block);

    }
    // InvShiftRows
    InvShiftRows(block);
    log_it("After InvShiftRows\n", block);

    // InvByteSub
    for(int i=0; i<Nb; i++)
    {
        BYTE* temp = (BYTE*)&block[i];
        for (int j = 0; j < 4; j++)
            temp[j] = InvSubBytesTab[temp[j]];
        block[i] = pack(temp);
    }
    log_it("After InvByteSub\n", block);

    // AddRoundKey
    AddRoundKey(block, &key[0]);
    log_it("After AddRoundKey\n", block);
}

int _tmain(int argc, char* argv[])
{
    #ifdef PERFOMANCE
        DWORD dwTime = GetTickCount();
    #endif

    BYTE* in = new BYTE[4*Nb];

    if (toupper(*argv[2]) == 'E')
    {
        printf("Encoding...\n");
        GenPowerTab();
        GenSubBytesTab();

        #ifdef PERFOMANCE
            printf("After tables generating %d\n",
                GetTickCount() - dwTime);
            dwTime = GetTickCount();
        #endif

        FILE* stream_in;
        FILE* stream_out;
        if ( !(stream_in = fopen(argv[1], "rb")))
        {
            printf("File in: %s cannot be read", argv[1]);
            return -1;
        }
        if ( !(stream_out = fopen(argv[3], "wb")) )
        {
            printf("File out: %s cannot be read", argv[3]);
            return -1;
        }

        fpos_t flen;
        // определение длины файла
        fseek(stream_in, 0, SEEK_END);
        fgetpos(stream_in, &flen); 
        unsigned long rlen = file_len(flen);
        // переход на начало
        fseek(stream_in, 0, SEEK_SET);
        
        unsigned long len;

        WORD CifKey[Nk] = { 0x00010203,    0x04050607,
            0x08090a0b,    0x0c0d0e0f};
        WORD ExpKey[Nb*(Nr+1)]; 
        KeyExpansion(CifKey, ExpKey);

        while(rlen > 0 && !feof(stream_in))
        {  
            len =
                (unsigned long)fread(in, 1, 4*Nb, stream_in);
            if (rlen < 4*Nb)
            {
                for (int i = rlen; i < 4*Nb; i++)
                    in[i] = 0;
            }
            rlen -= len;


            #ifdef LOGit
            printf("\nNew block\n");
            for(int i=0; i<4; i++)
            {
                printf("%02x %02x %02x %02x\n", in[i], in[4+i],
                    in[8+i], in[12+i]);
            }
            #endif

            AddRoundKey((WORD*)in, ExpKey);
            Cipher((WORD*)in, ExpKey);
            
            int nWritten = fwrite(in, sizeof(BYTE), 4*Nb, stream_out);
        }

        in[0] = len;
        int nWritten = fwrite(in, sizeof(BYTE), 1, stream_out);

        fclose(stream_out);
    }

    if (toupper(*argv[2]) == 'D')
    {
        printf("Decoding...\n");
        GenPowerTab();
        GenSubBytesTab();
        
        #ifdef PERFOMANCE
            printf("After tables generating %d\n",
                GetTickCount() - dwTime);
            dwTime = GetTickCount();
        #endif

        FILE* stream_in;
        FILE* stream_out;
        if ( !(stream_in = fopen(argv[1], "rb")))
        {
            printf("File in: %s cannot be read", argv[1]);
            return -1;
        }
        if ( !(stream_out = fopen(argv[3], "wb")) )
        {
            printf("File out: %s cannot be read", argv[3]);
            return -1;
        }

        fpos_t flen;
        // определение длины файла
        fseek(stream_in, 0, SEEK_END);
        fgetpos(stream_in, &flen); 
        unsigned long rlen = file_len(flen);
        // переход на начало
        fseek(stream_in, 0, SEEK_SET);

        WORD ExpKey[Nb*(Nr+1)]; 
        WORD CifKey[Nk] = { 0x00010203,    0x04050607,
            0x08090a0b,    0x0c0d0e0f};
        KeyExpansion(CifKey, ExpKey);

        while(rlen > 0 && !feof(stream_in))
        {  
            unsigned long len =
                (unsigned long)fread(in, 1, 4*Nb, stream_in);
            if (rlen < 4*Nb)
                for (int i = rlen; i < 4*Nb; i++)
                    in[i] = 0;
            rlen -= len;
            //if (len != 4*Nb)

            #ifdef LOGit
            printf("\nNew block\n");
            for(int i=0; i<4; i++)
            {
                printf("%02x %02x %02x %02x\n", in[i], in[4+i],
                    in[8+i], in[12+i]);
            }
            #endif

            AddRoundKey((WORD*)in, &ExpKey[4*Nr]);
            InvCipher((WORD*)in, ExpKey);

            if (rlen == 1)
            {
                BYTE* out = new BYTE[1];
                fread(out, sizeof(BYTE), 1, stream_in);
                len = out[0];
                rlen = 0;
            }

            int nWritten = fwrite(in, sizeof(BYTE), len, stream_out);
        }

        fclose(stream_out);
    }
        
    #ifdef PERFOMANCE
        printf("Finished %d\n",  
            GetTickCount() - dwTime);
    #endif

    //cin >> argc;

    return 0;
}