#include "../include/MySM4.hpp"
#include "../include/MyMD5.hpp"
#include "../include/CodeConvert.hpp"

#include <cstring>

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

CSM4::CSM4(EMODE_TYPE eMode) : CEncryptionBase(eMode, E_KEY_128)
{
}

CSM4::CSM4(const uint8_t *pKey, uint32_t iKeyLen, EMODE_TYPE eMode) : CEncryptionBase(eMode, E_KEY_128)
{
	SetKey(pKey, iKeyLen);
}

CSM4::CSM4(const uint8_t *pKey, uint32_t iKeyLen, const uint8_t *pIV, uint32_t iIVLen, EMODE_TYPE eMode) : 
	CEncryptionBase(eMode, E_KEY_128)
{
	SetKey(pKey, iKeyLen);
	SetIV(pIV, iIVLen);
}

CSM4::~CSM4()
{
}

/*********************************************************************
功能：	设置密钥，自定义处理，IV跟着设置
参数：	pKey 密钥
*		iKeyLen 密钥长度
返回：	无
修改：
*********************************************************************/
void CSM4::SetKeyAndIvCustom(const uint8_t* pKey, uint32_t iKeyLen)
{
	ClearKey();

	if (nullptr != pKey && iKeyLen > 0)
	{
		//密码
		CMD5::StringToMD5((const char*)pKey, iKeyLen, m_KeyArray, E_KEY_LENGTH);

		//计算IV值
		uint8_t *pTemp = new uint8_t[iKeyLen + 1]{ 0 };
		for (uint32_t i = 0; i < iKeyLen; ++i)
		{
			//反转密码串
			pTemp[i] = pKey[iKeyLen - i - 1];
		}

		CMD5::StringToMD5((const char*)pTemp, iKeyLen, m_IVArray, E_IV_LENGTH);

		delete[] pTemp;
	}
}


/*********************************************************************
功能：	加密
参数：	szIn 待加密串
*		szOut 加密串
*		eEncodeType 加密串编码类型
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CSM4::Encrypt(const std::string &szIn, std::string &szOut, EENCODE_TYPE eEncodeType, std::string &szError) const
{
	if (szIn.empty())
	{
		szError = "待加密字符串为空！";
		return false;
	}

	uint32_t outLen = 0;
	uint8_t *out = nullptr;

	if (!Encrypt(szIn.c_str(), (uint32_t)szIn.length(), out, outLen, szError))
	{
		return false;
	}
	else
	{
		//编码
		if (E_CODE_BASE64 == eEncodeType)
			szOut = CCodeConvert::EncodeBase64(out, outLen);
		else if (E_CODE_HEX == eEncodeType)
			szOut = CCodeConvert::EncodeHex(out, outLen);
		else
			szOut = std::string((const char*)out, outLen);

		delete[] out;
	}

	return true;
}

/*********************************************************************
功能：	加密
参数：	in 待加密串
*		inLen 待加密串长度
*		out 加密串，调用者释放
*		outLen 加密串长度
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CSM4::Encrypt(const char *in, uint32_t inLen, uint8_t *&out, uint32_t &outLen, std::string &szError) const
{
	if (nullptr == in || inLen <= 0)
	{
		szError = "待加密串为空或长度错误！";
		return false;
	}
	else
	{
		if (inLen > E_MAX_LENGTH)
		{
			szError = "暂时处理数据最大长度为" + std::to_string(E_MAX_LENGTH / 1048576) + "M！";
			return false;
		}
	}

	if (nullptr != out)
	{
		outLen = 0;
		delete[] out;
		out = nullptr;
	}

	//计算追加填充,保证为16 的整数倍	
	uint8_t paddingBuff[E_BLOCK_SIZE] = { 0 };
	int paddingSize = Padding(in, inLen, paddingBuff);

	//设置输出缓冲
	outLen = E_SALT_LENGTH + inLen + paddingSize;
	out = new uint8_t[outLen + 1]{ '\0' };

	if (nullptr == out)
	{
		szError = "申请输出内存缓存失败！";
		return false;
	}

	//设置随机盐值	
#if E_SALT_LENGTH > 0
	uint8_t saltBuff[E_SALT_LENGTH] = { 0 };
	GetRandBytes(E_SALT_LENGTH, saltBuff);
	memcpy(out, saltBuff, E_SALT_LENGTH);
	uint8_t *pSaltBuff = saltBuff;
#else
	uint8_t *pSaltBuff = nullptr;
#endif

	//设置密钥上下文
	SSM4Context SM4Context;
	SetKeyContext(SM4Context, pSaltBuff, true);
	uint8_t pIv[E_IV_LENGTH] = { 0 };
	memcpy(pIv, m_IVArray, E_IV_LENGTH);

	//处理
	int iRoundCounts = inLen / E_BLOCK_SIZE;
	const uint8_t *pInPos = (const uint8_t*)in; //明文处理位置
	uint8_t *pOutPos = (uint8_t *)(out + E_SALT_LENGTH); //密文输出位置

	for (int i = 0; i < iRoundCounts; i++)
	{

		switch (m_eMode)
		{
		case E_MODE_CBC:// CBC模式
			CBCCipher(SM4Context, pIv, pInPos, pOutPos);
			break;
		case E_MODE_ECB:// ECB模式
			ECBCipher(SM4Context, pInPos, pOutPos);
			break;
		}

		pInPos += E_BLOCK_SIZE;
		pOutPos += E_BLOCK_SIZE;
	}

	//加密最后一块
	uint8_t dealBuff[E_BLOCK_SIZE] = { 0 };
	memcpy(dealBuff, (in + (E_BLOCK_SIZE * iRoundCounts)), inLen - (E_BLOCK_SIZE * iRoundCounts));
	memcpy(&dealBuff[inLen - (E_BLOCK_SIZE * iRoundCounts)], paddingBuff, paddingSize);

	switch (m_eMode)
	{
	case E_MODE_CBC:// CBC模式
		CBCCipher(SM4Context, pIv, dealBuff, pOutPos);
		break;
	case E_MODE_ECB:// ECB模式
		ECBCipher(SM4Context, dealBuff, pOutPos);
		break;
	}

	return true;
}

/*********************************************************************
功能：	加密数据到文件
参数：	in 待加密串
*		inLen 待加密串长度
*		szOutFileName 输出文件名称
*		eEncodeType 加密串编码类型
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CSM4::EncryptToFile(const char *in, uint32_t inLen, const std::string &szOutFileName, EENCODE_TYPE eEncodeType,
	std::string &szError) const
{
	if (szOutFileName.empty())
	{
		szError = "加密后存放数据文件：" + szOutFileName + " 名称为空！";
		return false;
	}

	//加密
	std::string szInTemp(in, inLen), szOutTemp;

	if (!Encrypt(szInTemp, szOutTemp, eEncodeType, szError))
	{
		return false;
	}

	return WriteDataToFile(szOutFileName, szOutTemp.c_str(), (int64_t)szOutTemp.length(), szError);
}

/*********************************************************************
功能：	解密
参数：	in 待解密串
*		inLen 待解密串长度
*		out 解密串，调用者释放
*		outLen 解密串长度
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CSM4::Decrypt(const std::string &szIn, EENCODE_TYPE eDecodeType, std::string &szOut, std::string & szError) const
{
	uint32_t iTempLen = 0;

	if (szIn.empty())
	{
		szError = "待解密串为空！";
		return false;
	}
	else
	{
		//检查格式
		if (E_CODE_BASE64 == eDecodeType)
		{
			if (CheckEncryptedData(szIn, eDecodeType, szError))
				iTempLen = (uint32_t)(szIn.length() / 4 + 1) * 3;
			else
				return false;
		}
		else if (E_CODE_HEX == eDecodeType)
		{
			if (CheckEncryptedData(szIn, eDecodeType, szError))
				iTempLen = (uint32_t)szIn.length() / 2;
			else
				return false;
		}
		else
		{
			if (CheckEncryptedData(szIn, eDecodeType, szError))
				iTempLen = (uint32_t)szIn.length();
			else
				return false;
		}
	}

	bool bRet = true;
	uint8_t *pTemp = new uint8_t[iTempLen + 1]{ '\0' };
	uint32_t outLen = 0;
	char *out = nullptr;

	//转char*
	if (nullptr == pTemp)
	{
		szError = "申请内存缓存失败！";
		return false;
	}
	else
	{
		if (E_CODE_BASE64 == eDecodeType)
			iTempLen = CCodeConvert::DecodeBase64(szIn, pTemp, iTempLen);
		else if (E_CODE_HEX == eDecodeType)
			iTempLen = CCodeConvert::DecodeHex(szIn, pTemp, iTempLen);
		else
			memcpy(pTemp, szIn.c_str(), iTempLen);
	}

	//解密
	if (!Decrypt(pTemp, iTempLen, out, outLen, szError))
	{
		bRet = false;
	}
	else
	{
		szOut = std::string(out, outLen);
		delete[] out;
	}

	// 释放内存
	delete[] pTemp;

	return bRet;
}

/*********************************************************************
功能：	解密
参数：	in 待解密串
*		inLen 待解密串长度
*		out 解密串，调用者释放
*		outLen 解密串长度
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CSM4::Decrypt(const uint8_t *in, uint32_t inLen, char *&out, uint32_t &outLen, std::string &szError) const
{
	if (nullptr == in || inLen <= 0 || inLen % E_BLOCK_SIZE != 0)
	{
		szError = "待解密串为空或长度错误（必须是16的整倍数）！";
		return false;
	}
	else if (inLen > (E_MAX_LENGTH + E_BLOCK_SIZE))
	{
		szError = "暂时处理数据最大长度为" + std::to_string(E_MAX_LENGTH / 1048576) + "M！";
		return false;
	}

	if (nullptr != out)
	{
		outLen = 0;
		delete[] out;
		out = nullptr;
	}

	//读出随机盐值
#if E_SALT_LENGTH > 0	
	uint8_t saltBuff[E_SALT_LENGTH] = { 0 };
	memcpy(saltBuff, in, E_SALT_LENGTH);
	inLen -= E_SALT_LENGTH;
	uint8_t *pSaltBuff = saltBuff;
#else
	uint8_t *pSaltBuff = nullptr;
#endif

	//设置输出缓冲
	outLen = inLen;
	out = new char[outLen + 1]{ '\0' };
	uint8_t pIv[E_IV_LENGTH] = { 0 };
	memcpy(pIv, m_IVArray, E_IV_LENGTH);

	if (nullptr == out)
	{
		outLen = 0;
		szError = "申请输出内存缓存失败！";
		return false;
	}

	//设置密钥上下文
	SSM4Context SM4Context;
	SetKeyContext(SM4Context, pSaltBuff, false);

	//处理
	bool bRet = true;
	int iRoundCounts = inLen / E_BLOCK_SIZE;
	const uint8_t *pInPos = in + E_SALT_LENGTH; //明文处理位置
	uint8_t *pOutPos = (uint8_t*)out; //密文输出位置

	for (int i = 0; i < iRoundCounts; i++)
	{
		switch (m_eMode)
		{
		case E_MODE_CBC: //CBC模式
			CBCCipher(SM4Context, pIv, pInPos, pOutPos);
			break;
		case E_MODE_ECB: //ECB模式
			ECBCipher(SM4Context, pInPos, pOutPos);
			break;
		}

		pInPos += E_BLOCK_SIZE;
		pOutPos += E_BLOCK_SIZE;
	}

	//校验填充
	uint32_t paddingValue = out[inLen - 1];
	
	if (paddingValue <= E_BLOCK_SIZE)
	{
		for (uint32_t i = inLen - paddingValue; i < inLen; i++)
		{
			if (out[i] != paddingValue)
			{
				bRet = false;
				szError = "密文填充值错误，可能是密文被损坏！";
				break;
			}
		}
	}
	else
	{
		bRet = false;
		szError = "密文填充值错误，可能是密码错误！";
	}
	

	//明文长度
	if (bRet)
		outLen -= paddingValue;
	else
	{
		outLen = 0;
		delete[] out;
		out = nullptr;
	}

	return bRet;
}

/*********************************************************************
功能：	解密文件中的内容
参数：	szInFileName 待解密文件名称
*		eDecodeType 文件中的解码类型
*		out 解密串，调用都释放
*		outLen 解密串长度
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CSM4::DecryptFromFile(const std::string &szInFileName, EENCODE_TYPE eDecodeType, char *&out, uint32_t &outLen,
	std::string &szError) const
{
	if (szInFileName.empty())
	{
		szError = "待解密文件名称为空！";
		return false;
	}
	else
	{
		if (!CheckFileExist(szInFileName))
		{
			szError = "待解密文件：" + szInFileName + " 不存在！";
			return false;
		}
	}

	if (nullptr != out)
	{
		outLen = 0;
		delete[] out;
		out = nullptr;
	}

	bool bRet = true;
	int64_t fileSize = 0;
	char *fileData = nullptr;

	//读取文件内容
	if (ReadDataFromFile(szInFileName, fileData, fileSize, szError))
	{
		if (fileSize <= 0)
		{
			bRet = false;
			szError = "待解密文件为空！";
		}
		else
		{
			std::string szInTemp(fileData, fileSize), szOutTemp;

			//解密
			bRet = Decrypt(szInTemp, eDecodeType, szOutTemp, szError);
			if (bRet)
			{
				outLen = (uint32_t)szOutTemp.length();
				out = new char[outLen + 1]{ '\0' };

				if (nullptr == out)
				{
					bRet = false;
					outLen = 0;
					szError = "申请输出内存缓存失败！";
				}
				else
					memcpy(out, szOutTemp.c_str(), outLen);
			}

			delete[] fileData;
			fileData = nullptr;
			fileSize = 0;
		}
	}
	else
		bRet = false;

	return bRet;
}

/*********************************************************************
功能：	设置密钥上下文
参数：	SM4Context 密钥上下文结构体
*		pSalt 盐值
*		bEncrypted true:加密，false:解密
返回：	无
修改：
*********************************************************************/
void CSM4::SetKeyContext(SSM4Context &SM4Context, uint8_t *pSalt, bool bEncrypted) const
{
	uint8_t keyArr[E_KEY_LENGTH] = { 0 };

	//设置密钥
#if E_SALT_LENGTH > 0
	uint8_t keyBuff[E_KEY_LENGTH + E_SALT_LENGTH] = { 0 };
	memcpy(keyBuff, m_KeyArray, E_KEY_LENGTH);
	memcpy(&keyBuff[E_KEY_LENGTH], pSalt, E_SALT_LENGTH);
	CMD5::StringToMD5((const char*)keyBuff, E_BLOCK_SIZE + E_SALT_LENGTH, (char*)keyArr, E_KEY_LENGTH);
#else
	memcpy(keyArr, m_KeyArray, E_KEY_LENGTH);
#endif

	SetSubkey(SM4Context.sk, keyArr);
	SM4Context.encrypted = bEncrypted;

	if (!SM4Context.encrypted) //解密
	{
		//需要颠倒密钥
		for (int i = 0; i < E_KEY_LENGTH; i++)
		{
			SWAP((SM4Context.sk[i]), (SM4Context.sk[31 - i]));
		}
	}
}

/*********************************************************************
功能：	密钥扩展算法 (轮密钥由加密密钥通过密钥扩展算法生成)
参数：	sk 轮密钥存放buff
*		key 加密密钥
返回：	无
修改：
*********************************************************************/
void CSM4::SetSubkey(unsigned long *sk, uint8_t *key) const
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

	for (; i < E_ROUND_KEY_LENGTH; i++)
	{
		k[i + 4] = k[i] ^ (SM4CalciRK(k[i + 1] ^ k[i + 2] ^ k[i + 3] ^ CK[i]));
		sk[i] = k[i + 4];
	}
}

/*********************************************************************
功能：	T' 变换
参数：	ka 值
返回：	变换后的值
修改：
*********************************************************************/
unsigned long CSM4::SM4CalciRK(unsigned long ka) const
{
	unsigned long bb = 0;
	unsigned long rk = 0;
	uint8_t a[4];
	uint8_t b[4];
	PUT_ULONG_BE(ka, a, 0)
		b[0] = sm4Sbox(a[0]);
	b[1] = sm4Sbox(a[1]);
	b[2] = sm4Sbox(a[2]);
	b[3] = sm4Sbox(a[3]);
	GET_ULONG_BE(bb, b, 0)
		rk = bb ^ (ROTL(bb, 13)) ^ (ROTL(bb, 23)); //L' 变换

	return rk;
}

/*********************************************************************
功能：	一轮处理
参数：	input 输入数据，块大小
*		output 输出数据，块大小
返回：	无
修改：
*********************************************************************/
void CSM4::SM4OneRound(unsigned long *sk, const uint8_t *input, uint8_t *output) const
{
	unsigned long i = 0;
	unsigned long ulbuf[36];

	memset(ulbuf, 0, sizeof(ulbuf));
	GET_ULONG_BE(ulbuf[0], input, 0)
		GET_ULONG_BE(ulbuf[1], input, 4)
		GET_ULONG_BE(ulbuf[2], input, 8)
		GET_ULONG_BE(ulbuf[3], input, 12)
		while (i < E_ROUND_KEY_LENGTH)
		{
			ulbuf[i + 4] = sm4F(ulbuf[i], ulbuf[i + 1], ulbuf[i + 2], ulbuf[i + 3], sk[i]);

			i++;
		}
	PUT_ULONG_BE(ulbuf[35], output, 0);
	PUT_ULONG_BE(ulbuf[34], output, 4);
	PUT_ULONG_BE(ulbuf[33], output, 8);
	PUT_ULONG_BE(ulbuf[32], output, 12);
}

/*********************************************************************
功能：	计算填充
参数：	pInData 待加密串
*		iInDataLen 待加密串长度
*		paddingBuff 填充buff，填充字符为填充的字节数转char
返回：	填充的字节数
修改：
*********************************************************************/
int CSM4::Padding(const char* pInData, int iInDataLen, uint8_t *paddingBuff) const
{
	uint8_t paddingSize = E_BLOCK_SIZE - iInDataLen % E_BLOCK_SIZE;

	for (int i = 0;i < paddingSize; i++)
	{
		paddingBuff[i] = paddingSize;
	}

	return paddingSize;
}

/*********************************************************************
功能：	CBC加解密处理
参数：	SM4Context 密钥上下文
*		pIv IV值
*		pSrc 待处理串
*		pDest 输出串
返回：	填充的字节数
修改：
*********************************************************************/
void CSM4::CBCCipher(SSM4Context &SM4Context, uint8_t *pIv, const uint8_t *pSrc, uint8_t *pDest) const
{
	uint8_t buff[E_BLOCK_SIZE] = { 0 };

	if (SM4Context.encrypted) //加密
	{
		//补入IV 值
		for (int i = 0; i < E_BLOCK_SIZE; i++)
		{
			buff[i] = (pSrc[i] ^ pIv[i]);
		}

		for (int i = 0; i < E_REPEAT_ROUND; i++)
		{			
			SM4OneRound(SM4Context.sk, buff, pDest);
			memcpy(buff, pDest, E_BLOCK_SIZE); //准备下一轮加密
		}

		//拷贝新的IV 值
		memcpy(pIv, pDest, E_IV_LENGTH);
	}
	else //解密
	{		
		uint8_t tmpIn[E_BLOCK_SIZE] = { 0 };
		memcpy(tmpIn, pSrc, E_BLOCK_SIZE);

		for (int i = 0; i < E_REPEAT_ROUND; i++)
		{
			SM4OneRound(SM4Context.sk, tmpIn, buff);

			memcpy(tmpIn, buff, E_BLOCK_SIZE); //准备下一轮解码
		}

		//除掉IV 值
		for (int i = 0; i < E_BLOCK_SIZE; i++)
		{
			pDest[i] = (buff[i] ^ pIv[i]);
		}

		//拷贝新的IV 值
		memcpy(pIv, pSrc, E_IV_LENGTH);
	}
}

/*********************************************************************
功能：	ECB加解密处理
参数：	SM4Context 密钥上下文
*		pSrc 待处理串
*		pDest 输出串
返回：	填充的字节数
修改：
*********************************************************************/
void CSM4::ECBCipher(SSM4Context &SM4Context, const uint8_t *pSrc, uint8_t *pDest) const
{
	uint8_t buff[E_BLOCK_SIZE] = { 0 };
	memcpy(buff, pSrc, E_BLOCK_SIZE);

	for (int i = 0; i < E_REPEAT_ROUND; i++)
	{
		SM4OneRound(SM4Context.sk, buff, pDest);
		memcpy(buff, pDest, E_BLOCK_SIZE);
	}
}