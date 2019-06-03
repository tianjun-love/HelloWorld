#include "../include/MySM3.hpp"
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#else
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#endif // WIN32

/*
* 32-bit integer manipulation macros (big endian)
*/
#ifndef GET_ULONG_BE
#define GET_ULONG_BE(n,b,i)                             \
{                                                       \
    (n) = ( (uInt32) (b)[(i)    ] << 24 )        \
        | ( (uInt32) (b)[(i) + 1] << 16 )        \
        | ( (uInt32) (b)[(i) + 2] <<  8 )        \
        | ( (uInt32) (b)[(i) + 3]       );       \
}
#endif

#ifndef PUT_ULONG_BE
#define PUT_ULONG_BE(n,b,i)                             \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

static const unsigned char sm3_padding[64] =
{
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


CSM3::CSM3()
{
}

CSM3::~CSM3()
{
}

std::string CSM3::StringToSM3(const char* pSrc, uInt32 iLen, bool bLowercase)
{
	if (nullptr == pSrc || iLen <= 0)
		return "";

	SSM3Context SM3Context;

	SM3Init(&SM3Context);
	SM3Update(&SM3Context, pSrc, iLen);
	SM3Final(&SM3Context);

	//转字符串
	return BytesToHexString(SM3Context.digest, resultsize, bLowercase);
}

std::string CSM3::StringToSM3(const std::string& szSrc, bool bLowercase)
{
	return StringToSM3(szSrc.c_str(), szSrc.length(), bLowercase);
}

bool CSM3::StringToSM3(const char* pSrc, uInt32 iLen, char* pDst, uInt32 iDstBufLen)
{
	if (nullptr == pSrc || iLen <= 0 || nullptr == pDst || iDstBufLen <= 0)
		return false;

	SSM3Context SM3Context;

	SM3Init(&SM3Context);
	SM3Update(&SM3Context, pSrc, iLen);
	SM3Final(&SM3Context);

	//复制数据
	memcpy(pDst, SM3Context.digest, (iDstBufLen <= resultsize ? iDstBufLen : resultsize));

	return true;
}

bool CSM3::StringToSM3(const std::string& szSrc, char* pDst, uInt32 iDstBufLen)
{
	return StringToSM3(szSrc.c_str(), szSrc.length(), pDst, iDstBufLen);
}

std::string CSM3::FileToSM3(const std::string& szFileName, bool bLowercase)
{
	if (szFileName.empty() || !CheckFileExist(szFileName))
		return "";

	std::ifstream FileIn(szFileName.c_str(), std::ios::binary);
	if (FileIn.fail())
		return "";

	std::string szInputData((std::istreambuf_iterator<char>(FileIn)), std::istreambuf_iterator<char>());
	FileIn.close();

	return StringToSM3(szInputData.c_str(), szInputData.length());
}

bool CSM3::FileToSM3(const std::string& szFileName, char* pDst, uInt32 iDstBufLen)
{
	if (szFileName.empty() || !CheckFileExist(szFileName) || nullptr == pDst || iDstBufLen <= 0)
		return false;

	std::ifstream FileIn(szFileName.c_str(), std::ios::binary);
	if (FileIn.fail())
		return false;

	std::string szInputData((std::istreambuf_iterator<char>(FileIn)), std::istreambuf_iterator<char>());
	FileIn.close();

	return StringToSM3(szInputData.c_str(), szInputData.length(), pDst, iDstBufLen);
}

bool CSM3::CheckFileExist(const std::string &szFileName)
{
	if (szFileName.empty())
		return false;

	int iRet = 0;

#ifdef _WIN32
	iRet = _access(szFileName.c_str(), 0);
#else
	iRet = access(szFileName.c_str(), F_OK);
#endif // WIN32

	return (0 == iRet);
}

std::string CSM3::BytesToHexString(const unsigned char *pIn, size_t iSize, bool bLowercase)
{
	std::string str;
	int strLen = iSize * 2;
	char*  buf = new char[strLen + 3];
	memset(buf, 0, strLen + 3);

	for (size_t i = 0; i < iSize; i++)
	{
		snprintf(buf + i * 2, 3, (bLowercase ? "%02x" : "%02X"), pIn[i]);
	}

	str = buf;
	delete buf;

	return std::move(str);
}


void CSM3::SM3Init(SSM3Context * pCtx)
{
	pCtx->total[0] = 0;
	pCtx->total[1] = 0;

	pCtx->state[0] = 0x7380166FUL;
	pCtx->state[1] = 0x4914B2B9UL;
	pCtx->state[2] = 0x172442D7UL;
	pCtx->state[3] = 0xDA8A0600UL;
	pCtx->state[4] = 0xA96F30BCUL;
	pCtx->state[5] = 0x163138AAUL;
	pCtx->state[6] = 0xE38DEE4DUL;
	pCtx->state[7] = 0xB0FB0E4EUL;

    memset(pCtx->digest, 0, resultsize);
}

void CSM3::SM3Update(SSM3Context * pCtx, const void * pVdata, uInt32 iDataLen)
{
	int fill;
	uInt32 left;

	unsigned char *input = (unsigned char *)pVdata;
	int ilen = iDataLen;

	left = pCtx->total[0] & 0x3F;
	fill = blocksize - left;

	pCtx->total[0] += ilen;
	pCtx->total[0] &= 0xFFFFFFFF;

	if (pCtx->total[0] < (uInt32)ilen)
		pCtx->total[1]++;

	if (left && ilen >= fill)
	{
		memcpy((void *)(pCtx->buffer + left),(void *)input, fill);
		SM3Guts(pCtx, pCtx->buffer);
		input += fill;
		ilen -= fill;
		left = 0;
	}

	while (ilen >= blocksize)
	{
		SM3Guts(pCtx, input);
		input += blocksize;
		ilen -= blocksize;
	}

	if (ilen > 0)
	{
		memcpy((void *)(pCtx->buffer + left),
			(void *)input, ilen);
	}
}


void CSM3::SM3Guts(SSM3Context * pCtx, unsigned char data[blocksize])
{
	uInt32 SS1, SS2, TT1, TT2, W[68], W1[64];
	uInt32 A, B, C, D, E, F, G, H;
	uInt32 T[64];
	uInt32 Temp1, Temp2, Temp3, Temp4, Temp5;
	int j;

	for (j = 0; j < 16; j++)
		T[j] = 0x79CC4519UL;
	for (j = 16; j < 64; j++)
		T[j] = 0x7A879D8AUL;

	GET_ULONG_BE(W[0], data, 0);
	GET_ULONG_BE(W[1], data, 4);
	GET_ULONG_BE(W[2], data, 8);
	GET_ULONG_BE(W[3], data, 12);
	GET_ULONG_BE(W[4], data, 16);
	GET_ULONG_BE(W[5], data, 20);
	GET_ULONG_BE(W[6], data, 24);
	GET_ULONG_BE(W[7], data, 28);
	GET_ULONG_BE(W[8], data, 32);
	GET_ULONG_BE(W[9], data, 36);
	GET_ULONG_BE(W[10], data, 40);
	GET_ULONG_BE(W[11], data, 44);
	GET_ULONG_BE(W[12], data, 48);
	GET_ULONG_BE(W[13], data, 52);
	GET_ULONG_BE(W[14], data, 56);
	GET_ULONG_BE(W[15], data, 60);


#define FF0(x,y,z) ( (x) ^ (y) ^ (z)) 
#define FF1(x,y,z) (((x) & (y)) | ( (x) & (z)) | ( (y) & (z)))

#define GG0(x,y,z) ( (x) ^ (y) ^ (z)) 
#define GG1(x,y,z) (((x) & (y)) | ( (~(x)) & (z)) )


#define  SHL(x,n) (((x) & 0xFFFFFFFF) << n)
#define ROTL(x,n) (SHL((x),n) | ((x) >> (32 - n)))

#define P0(x) ((x) ^  ROTL((x),9) ^ ROTL((x),17)) 
#define P1(x) ((x) ^  ROTL((x),15) ^ ROTL((x),23)) 

	for (j = 16; j < 68; j++)
	{
		//W[j] = P1( W[j-16] ^ W[j-9] ^ ROTL(W[j-3],15)) ^ ROTL(W[j - 13],7 ) ^ W[j-6];
		//Why thd release's result is different with the debug's ?
		//Below is okay. Interesting, Perhaps VC6 has a bug of Optimizaiton.

		Temp1 = W[j - 16] ^ W[j - 9];
		Temp2 = ROTL(W[j - 3], 15);
		Temp3 = Temp1 ^ Temp2;
		Temp4 = P1(Temp3);
		Temp5 = ROTL(W[j - 13], 7) ^ W[j - 6];
		W[j] = Temp4 ^ Temp5;
	}

	for (j = 0; j < blocksize; j++)
	{
		W1[j] = W[j] ^ W[j + 4];
	}

	A = pCtx->state[0];
	B = pCtx->state[1];
	C = pCtx->state[2];
	D = pCtx->state[3];
	E = pCtx->state[4];
	F = pCtx->state[5];
	G = pCtx->state[6];
	H = pCtx->state[7];

	for (j = 0; j < 16; j++)
	{
		SS1 = ROTL((ROTL(A, 12) + E + ROTL(T[j], j)), 7);
		SS2 = SS1 ^ ROTL(A, 12);
		TT1 = FF0(A, B, C) + D + SS2 + W1[j];
		TT2 = GG0(E, F, G) + H + SS1 + W[j];
		D = C;
		C = ROTL(B, 9);
		B = A;
		A = TT1;
		H = G;
		G = ROTL(F, 19);
		F = E;
		E = P0(TT2);

	}

	for (j = 16; j < 64; j++)
	{
		SS1 = ROTL((ROTL(A, 12) + E + ROTL(T[j], j)), 7);
		SS2 = SS1 ^ ROTL(A, 12);
		TT1 = FF1(A, B, C) + D + SS2 + W1[j];
		TT2 = GG1(E, F, G) + H + SS1 + W[j];
		D = C;
		C = ROTL(B, 9);
		B = A;
		A = TT1;
		H = G;
		G = ROTL(F, 19);
		F = E;
		E = P0(TT2);
	
	}

	pCtx->state[0] ^= A;
	pCtx->state[1] ^= B;
	pCtx->state[2] ^= C;
	pCtx->state[3] ^= D;
	pCtx->state[4] ^= E;
	pCtx->state[5] ^= F;
	pCtx->state[6] ^= G;
	pCtx->state[7] ^= H;

}

void CSM3::SM3Final(SSM3Context * pCtx)
{
	uInt32 last, padn;
	uInt32 high, low;
	unsigned char msglen[8];

	high = (pCtx->total[0] >> 29)
		| (pCtx->total[1] << 3);
	low = (pCtx->total[0] << 3);

	PUT_ULONG_BE(high, msglen, 0);
	PUT_ULONG_BE(low, msglen, 4);

	last = pCtx->total[0] & 0x3F;
	padn = (last < 56) ? (56 - last) : (120 - last);

	SM3Update(pCtx, (unsigned char *)sm3_padding, padn);
	SM3Update(pCtx, msglen, 8);

	PUT_ULONG_BE(pCtx->state[0], pCtx->digest, 0);
	PUT_ULONG_BE(pCtx->state[1], pCtx->digest, 4);
	PUT_ULONG_BE(pCtx->state[2], pCtx->digest, 8);
	PUT_ULONG_BE(pCtx->state[3], pCtx->digest, 12);
	PUT_ULONG_BE(pCtx->state[4], pCtx->digest, 16);
	PUT_ULONG_BE(pCtx->state[5], pCtx->digest, 20);
	PUT_ULONG_BE(pCtx->state[6], pCtx->digest, 24);
	PUT_ULONG_BE(pCtx->state[7], pCtx->digest, 28);


}

///////////////////////////////////////////////////////////////////////////////////////////////
void CSM3::QtSM3Init(SSM3Context *pCtx)
{
	return SM3Init(pCtx);
}

void CSM3::QtSM3Update(SSM3Context *pCtx, const void *pVdata, uInt32 iDataLen)
{
	return SM3Update(pCtx, pVdata, iDataLen);
}

void CSM3::QtSM3Final(SSM3Context *pCtx, unsigned char hash[resultsize])
{
	SM3Final(pCtx);
	memcpy(hash , pCtx->digest, resultsize);
}

std::string CSM3::StringToHMAC(const std::string& szSrc, const char* pKey, uInt32 iKeyLen, bool bLowercase)
{
	return StringToHMAC(szSrc.c_str(), szSrc.length(), pKey, iKeyLen, bLowercase);
}

std::string CSM3::StringToHMAC(const char *pSrc, uInt32 iLen, const char* pKey, uInt32 iKeyLen, bool bLowercase)
{
	if (nullptr == pSrc || iLen <= 0 || pKey == nullptr || iKeyLen <= 0)
		return "";

	SHMACContext HMACContext;

	HMACInit(&HMACContext, pKey, iKeyLen);
	HMACUpdate(&HMACContext, pSrc, iLen);
	HMACFinal(&HMACContext);

	//转字符串
	return BytesToHexString(HMACContext.SM3Context.digest, resultsize, bLowercase);
}

bool CSM3::StringToHMAC(const char *pSrc, uInt32 iLen, const char* pKey, uInt32 iKeyLen, char* pDst, uInt32 iDstBufLen)
{
	if (nullptr == pSrc || iLen <= 0 || pKey == nullptr || iKeyLen <= 0 || nullptr == pDst || iDstBufLen <= 0)
		return false;

	SHMACContext HMACContext;

	HMACInit(&HMACContext, pKey, iKeyLen);
	HMACUpdate(&HMACContext, pSrc, iLen);
	HMACFinal(&HMACContext);

	//复制数据
	memcpy(pDst, HMACContext.SM3Context.digest, (iDstBufLen <= resultsize ? iDstBufLen : resultsize));

	return true;
}

bool CSM3::StringToHMAC(const std::string& szSrc, const char* pKey, uInt32 iKeyLen, char* pDst, uInt32 iDstBufLen)
{
	return StringToHMAC(szSrc.c_str(), szSrc.length(), pKey, iKeyLen, pDst, iDstBufLen);
}

std::string CSM3::FileToHMAC(const std::string& szFileName, const char* pKey, uInt32 iKeyLen, bool bLowercase)
{
	if (szFileName.empty() || !CheckFileExist(szFileName) || nullptr == pKey || iKeyLen <= 0)
		return "";

	std::ifstream FileIn(szFileName.c_str(), std::ios::binary);
	if (FileIn.fail())
		return "";

	std::string szInputData((std::istreambuf_iterator<char>(FileIn)), std::istreambuf_iterator<char>());

	return StringToHMAC(szInputData, pKey, iKeyLen, bLowercase);
}

bool CSM3::FileToHMAC(const std::string& szFileName, const char* pKey, uInt32 iKeyLen, char* pDst, uInt32 iDstBufLen)
{
	if (szFileName.empty() || !CheckFileExist(szFileName) || nullptr == pKey || iKeyLen <= 0 || nullptr == pDst || iDstBufLen <= 0)
		return false;

	std::ifstream FileIn(szFileName.c_str(), std::ios::binary);
	if (FileIn.fail())
		return false;

	std::string szInputData((std::istreambuf_iterator<char>(FileIn)), std::istreambuf_iterator<char>());
	FileIn.close();

	return StringToHMAC(szInputData.c_str(), szInputData.length(), pKey, iKeyLen, pDst, iDstBufLen);
}

void CSM3::HMACInit(SHMACContext *pCtx, const char *pKey, uInt32 iKeyLen)
{
	pCtx->iIpad = 0x36; //Inner padding
	pCtx->iOpad = 0x5C; //Outer padding 
	//0. HMAC 算法对于秘钥长度不够1块,则用0填充。因此此处直接初始化为0.
	memset(pCtx->pKeyHash, 0, blocksize);

	if (iKeyLen > blocksize)
	{
		// 秘钥太长, 使用SM3 算法计算hash
		SM3Init(&pCtx->SM3Context);
		SM3Update(&pCtx->SM3Context, pKey, iKeyLen);
		SM3Final(&pCtx->SM3Context); //用0填充
		memcpy(pCtx->pKeyHash, pCtx->SM3Context.digest, resultsize);
	}
	else
	{
		memcpy(pCtx->pKeyHash, pKey, iKeyLen);
	}

	//2. 秘钥异或 Inner pad
	for (int i = 0; i < blocksize; i++)
	{
		pCtx->pKeyHash[i] ^= pCtx->iIpad;
	}

	//3. 初始化
	SM3Init(&pCtx->SM3Context);
	SM3Update(&pCtx->SM3Context, pCtx->pKeyHash, blocksize);
}

void CSM3::HMACUpdate(SHMACContext *pCtx, const void *pVdata, uInt32 iDataLen)
{
	SM3Update(&pCtx->SM3Context, pVdata, iDataLen);
}

void CSM3::HMACFinal(SHMACContext *pCtx)
{
	for (int i = 0; i < blocksize; i++)
	{
		pCtx->pKeyHash[i] ^= (pCtx->iIpad ^ pCtx->iOpad);
	}

	SM3Final(&pCtx->SM3Context);

	char hash[32]{ 0 };
	memcpy(hash ,pCtx->SM3Context.digest, resultsize);

	SM3Init(&pCtx->SM3Context);
	SM3Update(&pCtx->SM3Context, pCtx->pKeyHash, blocksize);
	SM3Update(&pCtx->SM3Context, hash, 32);
	SM3Final(&pCtx->SM3Context);

}







