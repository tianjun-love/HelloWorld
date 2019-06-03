/****************************************************
功能：基于openssl的加密
作者：田俊
时间：2019-05-28
修改：
****************************************************/
#ifndef __MY_OPENSSL_HPP__
#define __MY_OPENSSL_HPP__

#include <cstring>
#include "Base64Convert.hpp"

/*
加密算法：
openssl命令，命令及IV值为16进制串，加密后的base64串每76个字符会有换行，换行不属于密文：
echo -n "123456" | openssl enc -e -aes-128-cbc -base64 -K 01020304050607080900010203040506 -iv 01020304050607080900010203040506 -nosalt -p
echo "7//34D6QzBeeIjXXYs/5ig==" | openssl enc -d -base64 -aes-128-cbc -K 01020304050607080900010203040506 -iv 01020304050607080900010203040506 -nosalt -p

hash算法：
echo -n "123456" | openssl dgst -sha256/md5等
*/

class COpenssl
{
public:
	COpenssl(bool bBase64LineFeed = false);
	~COpenssl();

	//加密算法类型
	enum ECipherType
	{
		E_CIPHER_NONE    = 0, //未知

		E_CIPHER_AES     = 1, //openssl命令中的-aes128/192/256是使用cbc模式，这里直接使用原始方式，自定义填充
		E_CIPHER_AES_ECB = 2, //ecb模式不需要IV
		E_CIPHER_AES_CBC = 3,
		E_CIPHER_AES_CFB = 4,
		E_CIPHER_AES_OFB = 5,

		E_CIPHER_DES_ECB = 11, //ecb模式不需要IV
		E_CIPHER_DES_CBC = 12,
		E_CIPHER_DES_CFB = 13,
		E_CIPHER_DES_OFB = 14
	};

	//摘要算法类型
	enum EHashType
	{
		E_MD_MD4       = 1,
		E_MD_MD5       = 2,
		E_MD_MDC2      = 3,
		E_MD_SHA       = 4,
		E_MD_SHA1      = 5,
		E_MD_SHA224    = 6,
		E_MD_SHA256    = 7,
		E_MD_SHA384    = 8,
		E_MD_SHA512    = 9,
		E_MD_WHIRLPOOL = 10
	};

	//密钥类型
	enum EKeyType : int
	{
		E_KEY_64  = 64,  //des使用
		E_KEY_128 = 128, //aes128
		E_KEY_192 = 192, //aes192
		E_KEY_256 = 256  //aes256
	};

	bool SetKeyAndIV(EKeyType eKeyType, const char *key, int keylen, const char *iv, int ivlen, std::string &szError);
	int Encrypt(ECipherType eCipher, EKeyType eKeyType, const std::string &szPlainText, std::string &szEncrypted, std::string &szError);
	int Encrypt(ECipherType eCipher, EKeyType eKeyType, const char *pPlainText, int iPlainLen, unsigned char *&pOut, int &iOutLen, 
		std::string &szError);
	int Decrypt(ECipherType eCipher, EKeyType eKeyType, const std::string &szEncrypted, std::string &szPlainText, std::string &szError);
	int Decrypt(ECipherType eCipher, EKeyType eKeyType, const char *pEncrypted, int iEncryptedLen, unsigned char *&pOut, int &iOutLen,
		std::string &szError);

	bool Hash(EHashType eHashType, const std::string &szData, std::string &szHash, bool bIsFile = false, std::string *pError = nullptr, 
		bool bLowercase = false);
	bool Hash(EHashType eHashType, const char *pData, unsigned int uiLength, unsigned char *pHash, unsigned int &uiHashLen, bool bIsFile = false, 
		std::string *pError = nullptr, bool bLowercase = false);

private:
	int EncryptHandle(const void *pKey, const unsigned char *pIn, int iInLen, unsigned char *&pOut, int &iOutLen, std::string &szError);
	int DecryptHandle(const void *pKey, const unsigned char *pIn, int iInLen, unsigned char *&pOut, int &iOutLen, std::string &szError);
	int EncryptHandle(ECipherType eCipherType, const void *pCipher, const unsigned char *pKey, const unsigned char *pIV, const unsigned char *pIn,
		int iInLen, unsigned char *&pOut, int &iOutLen, std::string &szError);
	int DecryptHandle(ECipherType eCipherType, const void *pCipher, const unsigned char *pKey, const unsigned char *pIV, const unsigned char *pIn,
		int iInLen, unsigned char *&pOut, int &iOutLen, std::string &szError);

	const void* GetCipher(ECipherType eCipherType, EKeyType eKeyType, bool bEncrypt, std::string &szError);
	std::string GetOpensslErrorMsg();

	bool DigestInit(void *ctx, const void *type, void *impl, std::string *pError);
	bool DigestUpdate(void *ctx, const void *data, size_t cnt, std::string *pError);
	bool DigestFinal(void *ctx, unsigned char* md, unsigned int *s, std::string *pError);
	bool CipherInit(void *ctx, const void *cipher, void *impl, const unsigned char *key, const unsigned char *iv, bool bEncrypt,
		std::string &szError);
	bool CipherUpdate(void *ctx, unsigned char *out, int *outl, const unsigned char* in, int inl, std::string &szError);
	bool CipherFinal(void *ctx, int dl, unsigned char *outm, int *outl, std::string &szError);

	static int DealPadding(unsigned char *pData, size_t uiDataLength, bool bEncrypt, std::string &szError);
	static std::string GetErrorMsg(unsigned int sysErrCode);
	static char GetHexChar(const char &ch, bool isLowercase = false); //获取16进制的字符
	static char FromHexChar(const char &ch); //16进制的字符转数
	static std::string GetHexString(const unsigned char *data, int data_len, bool bLowercase = false);
	static int FromHexString(const std::string& szHexString, unsigned char* data, int data_len);
	static bool CheckFileExist(const char *pFileName);

private:
	bool          m_bLineFeed;           //base64编码是否换行
	bool          m_bSetKey;             //是否己设置key
	unsigned char m_Key[32];             //密码
	unsigned char m_IV[16];              //IV值

	static bool   m_bLoadErrorStrings;   //是否己加载错误信息

};

#endif