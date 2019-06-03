#include "../include/MySM4.hpp"
#include "../include/MyMD5.hpp"
#include <random>
#include <fstream>

/*
* 32-bit integer manipulation macros (big endian)
*/
#ifndef GET_ULONG_BE    
#define GET_ULONG_BE(n,b,i)                             \
{                                                       \
	(n) = ( (unsigned long) (b)[(i)    ] << 24 )        \
		| ( (unsigned long) (b)[(i) + 1] << 16 )        \
		| ( (unsigned long) (b)[(i) + 2] <<  8 )        \
		| ( (unsigned long) (b)[(i) + 3]       );       \
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

/*
*rotate shift left marco definition
*
*/
#define SHL(x,n) (((x) & 0xFFFFFFFF) << n)    
#define ROTL(x,n) (SHL((x),n) | ((x) >> (32 - n)))    
#define SWAP(a,b) { unsigned long t = (a); (a) = (b); (b) = t; t = 0; }    

/*
 * S 盒 
*/
static const unsigned char SboxTable[16][16] =
{
	{ 0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05 },
	{ 0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99 },
	{ 0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62 },
	{ 0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6 },
	{ 0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8 },
	{ 0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35 },
	{ 0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87 },
	{ 0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e },
	{ 0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1 },
	{ 0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3 },
	{ 0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f },
	{ 0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51 },
	{ 0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8 },
	{ 0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0 },
	{ 0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84 },
	{ 0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48 }
};

/*
 * 系统参数 FK 
*/
static const unsigned long FK[4] = { 0xa3b1bac6,0x56aa3350,0x677d9197,0xb27022dc };

/* 
 * 固定参数 CK 
*/
static const unsigned long CK[32] =
{
	0x00070e15,0x1c232a31,0x383f464d,0x545b6269,
	0x70777e85,0x8c939aa1,0xa8afb6bd,0xc4cbd2d9,
	0xe0e7eef5,0xfc030a11,0x181f262d,0x343b4249,
	0x50575e65,0x6c737a81,0x888f969d,0xa4abb2b9,
	0xc0c7ced5,0xdce3eaf1,0xf8ff060d,0x141b2229,
	0x30373e45,0x4c535a61,0x686f767d,0x848b9299,
	0xa0a7aeb5,0xbcc3cad1,0xd8dfe6ed,0xf4fb0209,
	0x10171e25,0x2c333a41,0x484f565d,0x646b7279
};

/*
 * S盒变换( 8byte输入, 8byte输出的置换)
*/
static unsigned char sm4Sbox(unsigned char inch)
{
	unsigned char *pTable = (unsigned char *)SboxTable;
	unsigned char retVal = (unsigned char)(pTable[inch]);
	return retVal;
}

/**
 * 合成置换T 
*/
static unsigned long sm4Lt(unsigned long ka)
{
	unsigned long bb = 0;
	unsigned long c = 0;
	unsigned char a[4];
	unsigned char b[4];
	PUT_ULONG_BE(ka, a, 0);
	b[0] = sm4Sbox(a[0]);
	b[1] = sm4Sbox(a[1]);
	b[2] = sm4Sbox(a[2]);
	b[3] = sm4Sbox(a[3]);
	GET_ULONG_BE(bb, b, 0);
	
	//线性L 变换
	c = bb ^ (ROTL(bb, 2)) ^ (ROTL(bb, 10)) ^ (ROTL(bb, 18)) ^ (ROTL(bb, 24));

	return c;
}

/**
 * 轮函数F 
*/
static unsigned long sm4F(unsigned long x0, unsigned long x1, unsigned long x2, unsigned long x3, unsigned long rk)
{
	return (x0^sm4Lt(x1^x2^x3^rk));
}

CSM4::CSM4()
{
	ClearKey();
}

CSM4::CSM4(const uChar8 *pKey, uInt32 iKeyLen, const uChar8* pIV, uInt32 iIVLen, ESM4_MODE eMode)
{
	m_eMode = eMode;
	SetKey(pKey, iKeyLen, pIV, iIVLen);
}

CSM4::~CSM4()
{
	ClearKey();
}

void CSM4::SetKey(const uChar8* pKey, uInt32 iKeyLen, const uChar8* pIV, uInt32 iIVLen)
{
	ClearKey();

	if (nullptr != pKey && iKeyLen > 0)
	{
		memcpy(m_KeyArr, pKey, (iKeyLen > SM4_BLOCK_SIZE ? SM4_BLOCK_SIZE : iKeyLen));
	}

	if (nullptr != pIV && iIVLen > 0)
	{
		memcpy(m_IvArr, pIV, (iIVLen > SM4_BLOCK_SIZE ? SM4_BLOCK_SIZE : iIVLen));
	}
}

void CSM4::ClearKey()
{
	memset(m_KeyArr, 0, sizeof(m_KeyArr));
	memset(m_IvArr, 0, sizeof(m_IvArr));
}

bool CSM4::Encrypt(const std::string &szInData, std::string &szOutEncryptData, std::string &szError, bool bLowercase) const
{
	if (szInData.empty())
	{
		szError = "待加密字符串为空！";
		return false;
	}

	//计算追加填充,保证为16 的整数倍	
	uChar8 paddingBuff[SM4_BLOCK_SIZE] = { 0 };
	int paddingSize = Padding(szInData.c_str(), szInData.length(), paddingBuff);

	char *in = (char *)szInData.c_str();
	int inLen = szInData.length();

	//设置输出缓冲
	uInt32 outBufLen = SM4_SALT_BYTES + inLen + paddingSize;
	char *out = new char[outBufLen + 1];
	memset(out, 0, outBufLen + 1);

	//设置随机盐值	
#if SM4_SALT_BYTES > 0
	uChar8 saltBuff[SM4_SALT_BYTES] = { 0 };
	GetRandBytes(SM4_SALT_BYTES,saltBuff);
	memcpy(out,saltBuff, SM4_SALT_BYTES);
	uChar8 *pSaltBuff = saltBuff;
#else
	uChar8 *pSaltBuff = nullptr;
#endif

	//设置密钥上下文
	SSM4Context SM4Context;
	SetKeyContext(SM4Context, pSaltBuff, true);
	uChar8 pIv[SM4_BLOCK_SIZE] = { 0 };
	memcpy(pIv, m_IvArr, SM4_BLOCK_SIZE);

	//处理
	int iRoundCounts = inLen / SM4_BLOCK_SIZE;
	uChar8 *pInPos = (uChar8 *)in; //明文处理位置
	uChar8 *pOutPos = (uChar8 *)(out + SM4_SALT_BYTES); //密文输出位置
	for (int i = 0;i < iRoundCounts;i++)
	{

		switch (m_eMode)
		{
		case ESM4_MODE_CBC:// CBC 模式
			CBCCipher(SM4Context, pIv, pInPos, pOutPos);

			break;
		case ESM4_MODE_EBC:// EBC 模式
			EBCCipher(SM4Context, pInPos, pOutPos);
			break;
		}

		pInPos += SM4_BLOCK_SIZE;
		pOutPos += SM4_BLOCK_SIZE;
	}

	//加密最后一块
	uChar8 dealBuff[SM4_BLOCK_SIZE] = { 0 };
	memcpy(dealBuff, (in + (SM4_BLOCK_SIZE * iRoundCounts)), inLen - (SM4_BLOCK_SIZE * iRoundCounts));
	memcpy(&dealBuff[inLen - (SM4_BLOCK_SIZE * iRoundCounts)], paddingBuff, paddingSize);
	switch (m_eMode)
	{
	case ESM4_MODE_CBC:// CBC 模式
		CBCCipher(SM4Context, pIv, dealBuff, pOutPos);

		break;
	case ESM4_MODE_EBC:// EBC 模式
		EBCCipher(SM4Context, dealBuff, pOutPos);
		break;
	}

	//需要对密文编码,防止出现不可识别的字符 16进制编码		
	szOutEncryptData = BytesToHexString((const uChar8*)out, outBufLen);

	// 释放内存 
	delete[] out;

	return true;
}

bool CSM4::Encrypt(const char *inData, uInt32 inDataLen, char *&outEnyData, uInt32 &outBufLen, std::string &szError) const
{
	if (nullptr == inData || inDataLen <= 0)
	{
		szError = "参数错误！";
		return false;
	}

	if (nullptr != outEnyData)
	{
		delete[] outEnyData;
		outEnyData = nullptr;
	}

	//计算追加填充,保证为16 的整数倍	
	uChar8 paddingBuff[SM4_BLOCK_SIZE] = { 0 };
	int paddingSize = Padding(inData, inDataLen, paddingBuff);

	//设置输出缓冲
	outBufLen = SM4_SALT_BYTES + inDataLen + paddingSize;
	outEnyData = new char[outBufLen + 1]{ '\0' };

	if (nullptr == outEnyData)
	{
		szError = "申请内存块失败！";
		return false;
	}

	//设置随机盐值	
#if SM4_SALT_BYTES > 0
	uChar8 saltBuff[SM4_SALT_BYTES] = { 0 };
	GetRandBytes(SM4_SALT_BYTES, saltBuff);
	memcpy(outEnyData, saltBuff, SM4_SALT_BYTES);
	uChar8 *pSaltBuff = saltBuff;
#else
	uChar8 *pSaltBuff = nullptr;
#endif

	//设置密钥上下文
	SSM4Context SM4Context;
	SetKeyContext(SM4Context, pSaltBuff, true);
	uChar8 pIv[SM4_BLOCK_SIZE] = { 0 };
	memcpy(pIv, m_IvArr, SM4_BLOCK_SIZE);

	//处理
	int iRoundCounts = inDataLen / SM4_BLOCK_SIZE;
	const uChar8 *pInPos = (const uChar8 *)inData; //明文处理位置
	uChar8 *pOutPos = (uChar8 *)(outEnyData + SM4_SALT_BYTES); //密文输出位置
	for (int i = 0; i < iRoundCounts; i++)
	{

		switch (m_eMode)
		{
		case ESM4_MODE_CBC:// CBC 模式
			CBCCipher(SM4Context, pIv, pInPos, pOutPos);
			break;
		case ESM4_MODE_EBC:// EBC 模式
			EBCCipher(SM4Context, pInPos, pOutPos);
			break;
		}

		pInPos += SM4_BLOCK_SIZE;
		pOutPos += SM4_BLOCK_SIZE;
	}

	//加密最后一块
	uChar8 dealBuff[SM4_BLOCK_SIZE] = { 0 };
	memcpy(dealBuff, (inData + (SM4_BLOCK_SIZE * iRoundCounts)), inDataLen - (SM4_BLOCK_SIZE * iRoundCounts));
	memcpy(&dealBuff[inDataLen - (SM4_BLOCK_SIZE * iRoundCounts)], paddingBuff, paddingSize);
	switch (m_eMode)
	{
	case ESM4_MODE_CBC:// CBC 模式
		CBCCipher(SM4Context, pIv, dealBuff, pOutPos);
		break;
	case ESM4_MODE_EBC:// EBC 模式
		EBCCipher(SM4Context, dealBuff, pOutPos);
		break;
	}

	return true;
}

bool CSM4::Decrypt(const std::string &szInEncryptData, std::string &szOutData, std::string & szError) const
{
	if (szInEncryptData.empty())
	{
		szError = "密文为空！";
		return false;
	}

	uChar8 *pInData = new uChar8[szInEncryptData.length() / 2 + 1]{ 0 };
	size_t iInDataLen = HexStringToBytes(szInEncryptData, pInData, szInEncryptData.length() / 2 + 1);

	//参数判断,不符合sm4 规范不用解码
	if (iInDataLen % SM4_BLOCK_SIZE != 0)
	{
		szError = "密文长度不符合SM4规范，长度:" + std::to_string(iInDataLen);
		return false;
	}

	char *in = (char*)pInData;

	//读出随机盐值
#if SM4_SALT_BYTES > 0	
	uChar8 saltBuff[SM4_SALT_BYTES] = { 0 };
	memcpy(saltBuff, in, SM4_SALT_BYTES);
	inLen -= SM4_SALT_BYTES;
	uChar8 *pSaltBuff = saltBuff;
#else
	uChar8 *pSaltBuff = nullptr;
#endif
	//设置输出缓冲
	char * out = new char[iInDataLen + 1 ];
	memset(out, 0, iInDataLen + 1);
	uChar8 pIv[SM4_BLOCK_SIZE] = {0};
	memcpy(pIv, m_IvArr, SM4_BLOCK_SIZE);
	//设置密钥上下文
	SSM4Context SM4Context;
	SetKeyContext(SM4Context, pSaltBuff, false);

	//处理
	int iRoundCounts = iInDataLen / SM4_BLOCK_SIZE;
	uChar8 *pInPos = (uChar8 *)(in + SM4_SALT_BYTES); //明文处理位置
	uChar8 *pOutPos = (uChar8 *)out; //密文输出位置
	for (int i = 0;i < iRoundCounts;i++)
	{
		switch (m_eMode)
		{
		case ESM4_MODE_CBC:// CBC 模式
			CBCCipher(SM4Context, pIv, pInPos, pOutPos);

			break;
		case ESM4_MODE_EBC:// EBC 模式
			EBCCipher(SM4Context, pInPos, pOutPos);
			break;
		}

		pInPos += SM4_BLOCK_SIZE;
		pOutPos += SM4_BLOCK_SIZE;
	}

	//校验填充
	size_t paddingValue = out[iInDataLen - 1];

	if (paddingValue > SM4_BLOCK_SIZE)
	{
		szError = "密文填充值错误,可能是密码错误!";
		delete[] out;

		return false;
	}

	for (size_t i = iInDataLen - paddingValue; i < iInDataLen; i++)
	{
		if (out[i] != paddingValue) 
		{
			szError = "密文填充值错误,可能是密文被损坏!";

			delete[]out;
			return false;
		}
	}

	// 明文
	szOutData = std::move(std::string(out, iInDataLen - paddingValue));

	// 释放内存
	delete[] out;

	return true;
}

bool CSM4::Decrypt(const char* inDeyData, uInt32 inDeyDataLen, char *&outData, uInt32 &outBufLen, std::string &szError) const
{
	//参数判断,不符合sm4 规范不用解码
	if (inDeyDataLen % SM4_BLOCK_SIZE != 0)
	{
		szError = "密文长度不符合SM4规范，长度:" + std::to_string(inDeyDataLen);
		return false;
	}

	if (nullptr != outData)
	{
		delete[] outData;
		outData = nullptr;
	}

	//读出随机盐值
#if SM4_SALT_BYTES > 0	
	uChar8 saltBuff[SM4_SALT_BYTES] = { 0 };
	memcpy(saltBuff, inDeyData, SM4_SALT_BYTES);
	inDeyDataLen -= SM4_SALT_BYTES;
	uChar8 *pSaltBuff = saltBuff;
#else
	uChar8 *pSaltBuff = nullptr;
#endif

	//设置输出缓冲
	outBufLen = inDeyDataLen;
	outData = new char[outBufLen + 1]{ '\0' };
	uChar8 pIv[SM4_BLOCK_SIZE] = { 0 };
	memcpy(pIv, m_IvArr, SM4_BLOCK_SIZE);

	if (nullptr == outData)
	{
		szError = "申请内存块失败！";
		return false;
	}

	//设置密钥上下文
	SSM4Context SM4Context;
	SetKeyContext(SM4Context, pSaltBuff, false);

	//处理
	int iRoundCounts = inDeyDataLen / SM4_BLOCK_SIZE;
	const uChar8 *pInPos = (const uChar8 *)(inDeyData + SM4_SALT_BYTES); //明文处理位置
	uChar8 *pOutPos = (uChar8 *)outData; //密文输出位置
	for (int i = 0; i < iRoundCounts; i++)
	{
		switch (m_eMode)
		{
		case ESM4_MODE_CBC:// CBC 模式
			CBCCipher(SM4Context, pIv, pInPos, pOutPos);
			break;
		case ESM4_MODE_EBC:// EBC 模式
			EBCCipher(SM4Context, pInPos, pOutPos);
			break;
		}

		pInPos += SM4_BLOCK_SIZE;
		pOutPos += SM4_BLOCK_SIZE;
	}

	//校验填充
	uInt32 paddingValue = outData[inDeyDataLen - 1];
	
	if (paddingValue > SM4_BLOCK_SIZE)
	{
		szError = "密文填充值错误,可能是密码错误!";

		delete[] outData;
		outData = nullptr;
		outBufLen = 0;

		return false;
	}

	for (uInt32 i = inDeyDataLen - paddingValue; i < inDeyDataLen; i++)
	{
		if (outData[i] != paddingValue)
		{
			szError = "密文填充值错误,可能是密文被损坏!";

			delete[] outData;
			outData = nullptr;
			outBufLen = 0;

			return false;
		}
	}

	//明文长度
	outBufLen -= paddingValue;

	return true;
}

bool CSM4::EncryptFile(const std::string &szInData, const std::string &szOutFileName, std::string &szError, bool bLowercase) const
{
	if (szInData.empty())
	{
		szError = "待加密的串为空！";
		return false;
	}

	if (szOutFileName.empty())
	{
		szError = "加密输出文件名称为空！";
		return false;
	}

	std::string szOutEncryptData;
	if (Encrypt(szInData, szOutEncryptData, szError))
	{
		std::ofstream outFile(szOutFileName.c_str());
		if (outFile.is_open() && outFile.good())
		{
			outFile << szOutEncryptData;
			outFile.close();

			return true;
		}
		else
		{
			szError = "打开加密输出文件：" + szOutFileName + "失败！";
			return false;
		}
	}
	else
		return false;
}

bool CSM4::DecryptFile(const std::string & szInFileName, std::string & szOutData, std::string & szError) const
{
	if (szInFileName.empty())
	{
		szError = "解密输入文件名称为空！";
		return false;
	}

	if (!CMD5::CheckFileExist(szInFileName))
	{
		szError = "解密输入文件不存在！";
		return false;
	}

	std::ifstream inFile(szInFileName.c_str());
	if (inFile.is_open() && inFile.good())
	{
		std::string szInEncryptData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
		inFile.close();

		return Decrypt(szInEncryptData, szOutData, szError);
	}
	else
	{
		szError = "打开解密输入文件：" + szInFileName + "失败！";
		return false;
	}
}

/**
* 将字节数组，处理成16进制的字串
* eg:{0xb1,0x94,0x6a,0xc9,0x24,0x92,0xd2,0x34,0x7c,0x62,0x35,0xb4,0xd2,0x61,0x11,0x84}
* 处理结果为"b1946ac92492d2347c6235b4d2611184"
*/
std::string CSM4::BytesToHexString(const uChar8 *in, size_t size, bool bLowercase)
{
	std::string str;
	int strLen = size * 2;
	char*  buf = new char[strLen + 3];
	memset(buf, 0, strLen + 3);

	for (size_t i = 0; i < size; i++)
	{
		snprintf(buf + i * 2, 3, (bLowercase ? "%02x" : "%02X"), in[i]);
	}

	str = buf;
	delete buf;

	return str;
}

/**
* 将16 进制字符串处理成字节数组
* eg: md5 "b1946ac92492d2347c6235b4d2611184"
* 处理结果为{0xb1,0x94,0x6a,0xc9,0x24,0x92,0xd2,0x34,0x7c,0x62,0x35,0xb4,0xd2,0x61,0x11,0x84}
*/
size_t CSM4::HexStringToBytes(const std::string &str, uChar8 *out, size_t out_buf_len)
{
	if (0 == out_buf_len)
		return 0;

	size_t out_len = 0;
	unsigned char h = 0, l = 0;

	for (size_t i = 0; i < str.size(); i += 2)
	{
		h = HexCharToInt(str[i]);
		l = HexCharToInt(str[i + 1]);
		out[out_len++] = (h << 4) | l;

		//防止越界
		if (out_len >= out_buf_len)
			break;
	}

	return out_len;
}
unsigned char CSM4::HexCharToInt(char hex)
{
	switch (hex)
	{
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'a':
	case 'A':
		return 10;
	case 'b':
	case 'B':
		return 11;
	case 'c':
	case 'C':
		return 12;
	case 'd':
	case 'D':
		return 13;
	case 'e':
	case 'E':
		return 14;
	default:
		break;
	}

	return 15;
}
char CSM4::IntToHexChar(unsigned char x, bool bLowercase)
{
	switch (x)
	{
	case 0:
		return '0';
	case 1:
		return '1';
	case 2:
		return '2';
	case 3:
		return '3';
	case 4:
		return '4';
	case 5:
		return '5';
	case 6:
		return '6';
	case 7:
		return '7';
	case 8:
		return '8';
	case 9:
		return '9';
	case 10:
		return (bLowercase ? 'a' : 'A');
	case 11:
		return (bLowercase ? 'b' : 'B');
	case 12:
		return (bLowercase ? 'c' : 'C');
	case 13:
		return (bLowercase ? 'd' : 'D');
	case 14:
		return (bLowercase ? 'e' : 'E');
	default:
		break;
	}

	return (bLowercase ? 'f' : 'F');
}

/**
* 获取长度为 iByteCounts 个字节的随机数,保存于pByteBuff 中
*/
void CSM4::GetRandBytes(uInt32 iByteCounts, uChar8 *pByteBuff)
{
	std::random_device rd;
	std::uniform_int_distribution<int> uni_dist(0, 255);

	for (uInt32 i = 0; i < iByteCounts; i++)
	{
		pByteBuff[i] = uni_dist(rd);
	}
}

/*
* 设置密钥上下文
*/
void CSM4::SetKeyContext(SSM4Context &SM4Context, uChar8 *pSalt, bool encrypted) const
{
	uChar8 keyArr[SM4_BLOCK_SIZE] = { 0 };
	//设置密钥
#if SM4_SALT_BYTES > 0	

	uChar8 keyBuff[SM4_BLOCK_SIZE + SM4_SALT_BYTES] = { 0 };
	memcpy(keyBuff, m_md5KeyArr, SM4_BLOCK_SIZE);
	memcpy(&keyBuff[SM4_BLOCK_SIZE], pSalt, SM4_SALT_BYTES);
	size_t size = HexStringToBytes(CMD5::StringToMD5((const char *)keyBuff, SM4_BLOCK_SIZE + SM4_SALT_BYTES), keyArr, SM4_BLOCK_SIZE);
#else
	memcpy(keyArr, m_KeyArr, SM4_BLOCK_SIZE);
#endif
	SetSubkey(SM4Context.sk, keyArr);

	SM4Context.encrypted = encrypted;

	if (!SM4Context.encrypted) //解密
	{
		//需要颠倒密钥
		for (int i = 0; i < SM4_BLOCK_SIZE; i++)
		{
			SWAP((SM4Context.sk[i]), (SM4Context.sk[31 - i]));
		}
	}
}

/*
* 密钥扩展算法 (轮密钥由加密密钥通过密钥扩展算法生成)
*/
void CSM4::SetSubkey(unsigned long SK[32], uChar8 key[SM4_BLOCK_SIZE]) const
{
	unsigned long MK[4];
	unsigned long k[36];
	unsigned long i = 0;


	GET_ULONG_BE(MK[0], key, 0);
	GET_ULONG_BE(MK[1], key, 4);
	GET_ULONG_BE(MK[2], key, 8);
	GET_ULONG_BE(MK[3], key, 12);
	k[0] = MK[0] ^ FK[0];
	k[1] = MK[1] ^ FK[1];
	k[2] = MK[2] ^ FK[2];
	k[3] = MK[3] ^ FK[3];

	for (; i < 32; i++)
	{
		k[i + 4] = k[i] ^ (SM4CalciRK(k[i + 1] ^ k[i + 2] ^ k[i + 3] ^ CK[i]));
		SK[i] = k[i + 4];
	}
}

/**
* T' 变换
*/
unsigned long CSM4::SM4CalciRK(unsigned long ka) const
{
	unsigned long bb = 0;
	unsigned long rk = 0;
	uChar8 a[4];
	uChar8 b[4];
	PUT_ULONG_BE(ka, a, 0)
		b[0] = sm4Sbox(a[0]);
	b[1] = sm4Sbox(a[1]);
	b[2] = sm4Sbox(a[2]);
	b[3] = sm4Sbox(a[3]);
	GET_ULONG_BE(bb, b, 0)
		rk = bb ^ (ROTL(bb, 13)) ^ (ROTL(bb, 23)); //L' 变换

	return rk;
}

void CSM4::SM4OneRound(unsigned long sk[32], uChar8 input[SM4_BLOCK_SIZE], uChar8 output[SM4_BLOCK_SIZE]) const
{
	unsigned long i = 0;
	unsigned long ulbuf[36];

	memset(ulbuf, 0, sizeof(ulbuf));
	GET_ULONG_BE(ulbuf[0], input, 0)
		GET_ULONG_BE(ulbuf[1], input, 4)
		GET_ULONG_BE(ulbuf[2], input, 8)
		GET_ULONG_BE(ulbuf[3], input, 12)
		while (i < 32)
		{
			ulbuf[i + 4] = sm4F(ulbuf[i], ulbuf[i + 1], ulbuf[i + 2], ulbuf[i + 3], sk[i]);

			i++;
		}
	PUT_ULONG_BE(ulbuf[35], output, 0);
	PUT_ULONG_BE(ulbuf[34], output, 4);
	PUT_ULONG_BE(ulbuf[33], output, 8);
	PUT_ULONG_BE(ulbuf[32], output, 12);
}

/**
 * 计算填充
*/
int CSM4::Padding(const char* pInData, int iInDataLen, uChar8 * paddingBuff) const
{
	uChar8 paddingSize = SM4_BLOCK_SIZE - iInDataLen % SM4_BLOCK_SIZE;

	for (int i = 0;i < paddingSize; i++)
	{
		paddingBuff[i] = paddingSize;
	}

	return paddingSize;
}

void CSM4::CBCCipher(SSM4Context & SM4Context, uChar8 *pIv, const uChar8 * pSrc, uChar8 * pDest) const
{
	uChar8 buff[SM4_BLOCK_SIZE] = { 0 };

	if (SM4Context.encrypted) //加密
	{
		//补入IV 值
		for (int i = 0; i < SM4_BLOCK_SIZE; i++)
		{
			buff[i] = (pSrc[i] ^ pIv[i]);
		}

		for (int i = 0;i < SM4_BLOCK_SIZE; i++)
		{			
			SM4OneRound(SM4Context.sk, buff, pDest);

			memcpy(buff, pDest, SM4_BLOCK_SIZE); //准备下一轮加密
		}

		//拷贝新的IV 值
		memcpy(pIv, pDest, SM4_BLOCK_SIZE);
	}
	else //解密
	{		
		uChar8 tmpIn[SM4_BLOCK_SIZE] = { 0 };
		memcpy(tmpIn, pSrc, SM4_BLOCK_SIZE);

		for (int i = 0;i < SM4_BLOCK_SIZE; i++)
		{
			SM4OneRound(SM4Context.sk, tmpIn, buff);

			memcpy(tmpIn, buff, SM4_BLOCK_SIZE); //准备下一轮解码
		}

		//除掉IV 值
		for (int i = 0; i < SM4_BLOCK_SIZE; i++)
		{
			pDest[i] = (buff[i] ^ pIv[i]);
		}

		//拷贝新的IV 值
		memcpy(pIv, pSrc, SM4_BLOCK_SIZE);
	}
}

void CSM4::EBCCipher(SSM4Context & SM4Context, const uChar8 * pSrc, uChar8 * pDest) const
{
	uChar8 buff[SM4_BLOCK_SIZE] = { 0 };
	memcpy(buff, pSrc, SM4_BLOCK_SIZE);

	for (int i = 0;i < SM4_BLOCK_SIZE; i++)
	{
		SM4OneRound(SM4Context.sk, buff, pDest);

		memcpy(buff, pDest, SM4_BLOCK_SIZE);
	}
}