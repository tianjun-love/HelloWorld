/******************************************************
功能：	SM4加密算法
作者：	田俊
时间：	2019-06-03
修改：
******************************************************/
#ifndef __MY_SM4_HPP__
#define __MY_SM4_HPP__

#include <string>
#include <cstring>

class CSM4
{
public:
	typedef unsigned char uChar8;
	typedef unsigned int uInt32; //需要保证为32位整型

	#define SM4_SALT_BYTES (0)
    #define SM4_BLOCK_SIZE (16)

	enum ESM4_MODE
	{
		ESM4_MODE_CBC = 1, // CBC 模式
		ESM4_MODE_EBC = 2  // EBC 模式
	};

public:
	CSM4();
	CSM4(const uChar8* pKey, uInt32 iKeyLen, const uChar8* pIV, uInt32 iIVLen, ESM4_MODE eMode = ESM4_MODE_CBC);
	CSM4(const CSM4& Other) = delete;
	~CSM4();
	CSM4& operator=(const CSM4& Other) = delete;

	void SetKey(const uChar8* pKey, uInt32 iKeyLen, const uChar8* pIV, uInt32 iIVLen); //设置密钥
	void ClearKey(); //清除密钥信息
	bool Encrypt(const std::string &szInData, std::string &szOutEncryptData, std::string &szError, bool bLowercase = false) const; //加密后的串为16进制串
	bool Encrypt(const char *inData, uInt32 inDataLen, char *&outEnyData, uInt32 &outBufLen, std::string &szError) const;
	bool Decrypt(const std::string &szInEncryptData, std::string &szOutData, std::string &szError) const; //解密前的串为16进制串
	bool Decrypt(const char *inDeyData, uInt32 inDeyDataLen, char *&outData, uInt32 &outBufLen, std::string &szError) const;
	bool EncryptFile(const std::string &szInData, const std::string &szOutFileName, std::string &szError, bool bLowercase = false) const; //加密后的串为16进制串
	bool DecryptFile(const std::string &szInFileName, std::string &szOutData, std::string &szError) const; //解密前的串为16进制串

	static std::string BytesToHexString(const uChar8 *in, size_t size, bool bLowercase = false);
	static size_t HexStringToBytes(const std::string &str, uChar8 *out, size_t out_buf_len);
	static unsigned char HexCharToInt(char hex);
	static char IntToHexChar(unsigned char x, bool bLowercase);
	static void GetRandBytes(uInt32 iByteCounts, uChar8 *pByteBuff);

private:
	typedef struct sm4_context
	{
		bool          encrypted;    //true:加密，false:解密 
		unsigned long sk[32];       //SM4 子密钥
	}SSM4Context;

	void SetKeyContext(SSM4Context &SM4Context, uChar8* pSalt, bool encrypted) const; //设置密钥内容
	void SetSubkey(unsigned long SK[32], uChar8 key[SM4_BLOCK_SIZE]) const;
	unsigned long SM4CalciRK(unsigned long ka) const;
	void SM4OneRound(unsigned long sk[32], uChar8 input[SM4_BLOCK_SIZE], uChar8 output[SM4_BLOCK_SIZE]) const;
	int Padding(const char* pInData, int iInDataLen ,uChar8 *paddingBuff) const;
	void CBCCipher(SSM4Context &SM4Context,uChar8 *pIv,const uChar8 *pSrc, uChar8 *pDest) const;
	void EBCCipher(SSM4Context &SM4Context, const uChar8 *pSrc, uChar8 *pDest) const;
private:
	ESM4_MODE m_eMode;                   //模式
	uChar8    m_KeyArr[SM4_BLOCK_SIZE];  //密钥
	uChar8    m_IvArr[SM4_BLOCK_SIZE];   //IV值
};




#endif//__QUANTUM_SM4_HPP__

