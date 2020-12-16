#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <cerrno>
#include <fstream>
#include "../include/MyOpenssl.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

bool COpenssl::m_bLoadPublicObject = false;

COpenssl::COpenssl(bool bBase64LineFeed) : m_bLineFeed(bBase64LineFeed), m_eKeyType(E_KEY_NONE)
{
	memset(m_Key, 0, sizeof(m_Key));
	memset(m_IV, 0, sizeof(m_IV));

	LoadPublicObject();
}

COpenssl::~COpenssl()
{
}

/********************************************************************************
功能：	设置密钥和IV
参数：	eType 密钥类型
*		key 密钥
*		keylen 密钥长度
*		IV IV值
*		ivlen iv长度
*		szError 错误信息
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::SetKeyAndIV(EKeyType eKeyType, const char *key, int keylen, const char *iv, int ivlen, std::string &szError)
{
	bool bResult = false;
	m_eKeyType = E_KEY_NONE;

	if (nullptr == key || 8 > keylen)
	{
		szError = "Key is nullptr or length wrong !";
		return bResult;
	}

	if (nullptr != iv)
	{
		if (AES_BLOCK_SIZE != ivlen && 8 != ivlen)
		{
			szError = "IV is nullptr or length wrong !";
			return bResult;
		}
		else
			memcpy(m_IV, iv, ivlen);
	}
	else
		memset(m_IV, 0, sizeof(m_IV));

	switch (eKeyType)
	{
	case COpenssl::E_KEY_64: //des使用
	{
		if (8 == keylen)
		{
			memcpy(m_Key, key, 8);
			bResult = true;
			m_eKeyType = E_KEY_64;
		}
		else
			szError = "Key len must be 8 Bytes !";
	}
	break;
	case COpenssl::E_KEY_128:
	{
		if (16 == keylen)
		{
			memcpy(m_Key, key, 16);
			bResult = true;
			m_eKeyType = E_KEY_128;
		}
		else
			szError = "Key len must be 16 Bytes !";
	}
	break;
	case COpenssl::E_KEY_192:
	{
		if (24 == keylen)
		{
			memcpy(m_Key, key, 24);
			bResult = true;
			m_eKeyType = E_KEY_192;
		}
		else
			szError = "Key len must be 24 Bytes !";
	}
	break;
	case COpenssl::E_KEY_256:
	{
		if (32 == keylen)
		{
			memcpy(m_Key, key, 32);
			bResult = true;
			m_eKeyType = E_KEY_256;
		}
		else
			szError = "Key len must be 32 Bytes !";
	}
	break;
	default:
		szError = "Unknow key type !";
		break;
	}

	return bResult;
}

/********************************************************************************
功能：	aes加密
参数：	eCipher 算法类型
*		eType 密钥类型
*		szPlainText 要加密的字符串
*		szEncrypted 加密后的base64字符串
*		szError 错误信息
返回：	-2:参数错误，-1:加密处理失败，>0:返回加密后的字节数
修改：
********************************************************************************/
int COpenssl::Encrypt(ECipherType eCipher, EKeyType eKeyType, const std::string &szPlainText, std::string &szEncrypted, std::string &szError)
{
	int iEncryptedLen = 0, iRet = 0;
	unsigned char* pEncryptedData = nullptr;

	iRet = Encrypt(eCipher, eKeyType, szPlainText.c_str(), szPlainText.length(), pEncryptedData, iEncryptedLen, szError);

	if (0 <= iRet)
	{
		iRet = iEncryptedLen;
		szEncrypted = CCodeConvert::EncodeBase64(pEncryptedData, iEncryptedLen, (E_CIPHER_AES != eCipher ? m_bLineFeed : false));
	}

	if (nullptr != pEncryptedData)
	{
		delete[] pEncryptedData;
		pEncryptedData = nullptr;
	}
	
	return iRet;
}

/********************************************************************************
功能：	aes解密
参数：	eCipher 算法类型
*		eKeyType 密钥类型
*		szEncrypted 要解密的base64字符串
*		szPlainText 解密后的字符串
*		szError 错误信息
返回：	-2:参数错误，-1:加密处理失败，>0:返回解密后的字节数
修改：
********************************************************************************/
int COpenssl::Decrypt(ECipherType eCipher, EKeyType eKeyType, const std::string &szEncrypted, std::string &szPlainText, std::string &szError)
{
	if (szEncrypted.empty())
	{
		szError = "Decrypted data is empty !";
		return -2;
	}

	int iRet = 0;
	size_t uiInDataLen = (szEncrypted.length() / 4 + 1) * 3;
	unsigned char *pInData = new unsigned char[uiInDataLen + 1] {'\0'};

	if (nullptr != pInData)
	{
		uiInDataLen = CCodeConvert::DecodeBase64(szEncrypted, pInData, (uint32_t)uiInDataLen);
		int iDecryptedLen = 0;
		unsigned char* pDecryptedData = nullptr;

		iRet = Decrypt(eCipher, eKeyType, (const char*)pInData, (int)uiInDataLen, pDecryptedData, iDecryptedLen, szError);

		if (0 <= iRet)
		{
			iRet = iDecryptedLen;
			szPlainText = (const char*)pDecryptedData;
		}

		if (nullptr != pDecryptedData)
		{
			delete[] pDecryptedData;
			pDecryptedData = nullptr;
		}

		delete[] pInData;
		pInData = nullptr;
	}
	else
	{
		iRet = -1;
		szError = "Decode encrypted from base64 failed !";
	}

	return iRet;
}

/********************************************************************************
功能：	aes加密
参数：	eCipher 算法类型
*		eType 密钥类型
*		pPlainText 要加密的字符串
*		iPlainLen 要加密的字符串长度
*		pOut 加密后密文，外部释放
*		iOutLen 加密后密文长度
*		szError 错误信息
返回：	-2:参数错误，-1:加密处理失败，>0:返回加密后的字节数
修改：
********************************************************************************/
int COpenssl::Encrypt(ECipherType eCipher, EKeyType eKeyType, const char *pPlainText, int iPlainLen, unsigned char *&pOut, int &iOutLen,
	std::string &szError)
{
	if (nullptr == pPlainText || 0 >= iPlainLen)
	{
		szError = "Plaintext data is empty !";
		return -2;
	}

	int iRet = 0;
	const void *pCipher = GetCipher(eCipher, eKeyType, true, szError);

	if (nullptr != pCipher)
	{
		if (E_CIPHER_AES == eCipher)
		{
			iRet = EncryptHandle(pCipher, (const unsigned char*)pPlainText, iPlainLen, pOut, iOutLen, szError);

			//特殊处理
			delete (AES_KEY*)pCipher;
		}
		else
		{
			iRet = EncryptHandle(eCipher, pCipher, m_Key, m_IV, (const unsigned char*)pPlainText, iPlainLen,
				pOut, iOutLen, szError);
		}

		if (0 <= iRet)
			iRet = iOutLen;
	}
	else
		iRet = -1;

	return iRet;
}

/********************************************************************************
功能：	aes解密
参数：	eCipher 算法类型
*		eKeyType 密钥类型
*		pEncrypted 要解密的字符串
*		iEncryptedLen 要解密的字符串长度
*		pOut 解密后的字符串
*		iOutLen 解密后的字符串长度
*		szError 错误信息
返回：	-2:参数错误，-1:加密处理失败，>0:返回解密后的字节数
修改：
********************************************************************************/
int COpenssl::Decrypt(ECipherType eCipher, EKeyType eKeyType, const char *pEncrypted, int iEncryptedLen, unsigned char *&pOut, int &iOutLen,
	std::string &szError)
{
	if (nullptr == pEncrypted || 0 >= iEncryptedLen)
	{
		szError = "Decrypted data is null !";
		return -2;
	}

	int iRet = 0;
	const void *pCipher = GetCipher(eCipher, eKeyType, false, szError);

	if (nullptr != pCipher)
	{
		if (E_CIPHER_AES == eCipher)
		{
			iRet = DecryptHandle(pCipher, (const unsigned char*)pEncrypted, iEncryptedLen, pOut, iOutLen, szError);

			//特殊处理
			delete (AES_KEY*)pCipher;
		}
		else
		{
			iRet = DecryptHandle(eCipher, pCipher, m_Key, m_IV, (const unsigned char*)pEncrypted, iEncryptedLen,
				pOut, iOutLen, szError);
		}

		if (iRet >= 0)
			iRet = iOutLen;
	}
	else
		iRet = -1;

	return iRet;
}

/********************************************************************************
功能：	生成rsa公私钥文件
参数：	szPubKeyPath 公钥文件路径
*		szPriKeyPath 私钥文件路径
*		szPassword 私钥文件加密密码
*		iKeyBits 密钥长度，1024/2048/3072/7680/15360
*		szError 错误信息
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::RSAGenerateKeyPair(const std::string &szPubKeyPath, const std::string &szPriKeyPath, const std::string &szPassword, int iKeyBits,
	std::string &szError)
{
	if (szPubKeyPath.empty())
	{
		szError = "Public key file path can't be null !";
		return false;
	}

	if (szPriKeyPath.empty())
	{
		szError = "Private key file path can't be null !";
		return false;
	}

	if (szPassword.empty() || 4 > szPassword.length())
	{
		szError = "Private key file encrypt key can't be null and length must greater than or equal 4 !";
		return false;
	}

	if (1024 != iKeyBits && 2048 != iKeyBits && 3072 != iKeyBits && 7680 != iKeyBits && 15360 != iKeyBits)
	{
		szError = "Public key bits length must be 1024/2048/3072/7680/15360 !";
		return false;
	}

	bool bResult = true;
	RSA *r = NULL;
	BIGNUM *e = NULL;

	r = RSA_new();
	if (NULL == r)
	{
		szError = "RSA_new error:" + GetOpensslErrorMsg();
		bResult = false;
	}

	if (bResult)
	{
		e = BN_new();
		if (NULL == e)
		{
			szError = "BN_new error:" + GetOpensslErrorMsg();
			bResult = false;
		}
		else
			BN_set_word(e, RSA_F4);
	}

	if (bResult)
	{
		//生成密钥对
		if (1 == RSA_generate_key_ex(r, iKeyBits, e, NULL))
		{
			//公钥写文件
			BIO *bioPtr = BIO_new(BIO_s_file());

			if (0 < BIO_write_filename(bioPtr, (void*)szPubKeyPath.c_str()))
			{
				if (1 != PEM_write_bio_RSAPublicKey(bioPtr, r))
				{
					bResult = false;
				}
			}
			else
				bResult = false;

			BIO_free_all(bioPtr);

			if (bResult)
			{
				//私钥写文件
				bioPtr = BIO_new_file(szPriKeyPath.c_str(), "w+");

				if (1 != PEM_write_bio_RSAPrivateKey(bioPtr, r, EVP_des_ede3_cbc(), (unsigned char *)szPassword.c_str(), szPassword.length(),
					NULL, NULL))
				{
					bResult = false;
				}

				BIO_free_all(bioPtr);
			}
		}
		else
			bResult = false;
	}

	//获取错误信息
	if (!bResult)
		szError = GetOpensslErrorMsg();

	//清理
	if (NULL != e)
		BN_free(e);

	if (NULL != r)
		RSA_free(r);

	return bResult;
}

/********************************************************************************
功能：	公钥加密
参数：	szPubKeyFile 公钥文件名称
*		szIn 输入数据
*		szOut 输出数据
*		szError 错误信息
*		bEncrypt true:加密，输出数据为base64编码，false:解密
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::RSAPublicKey(const std::string &szPubKeyFile, const std::string &szIn, std::string &szOut, std::string &szError, bool bEncrypt)
{
	bool bResult = true;
	unsigned char* pOut = nullptr;
	size_t uiOutLen = 0;

	szOut.clear();

	if (bEncrypt)
	{
		if (RSAPublicKey(szPubKeyFile, szIn.c_str(), szIn.length(), pOut, uiOutLen, szError, bEncrypt))
			szOut = CCodeConvert::EncodeBase64(pOut, uiOutLen);
		else
			bResult = false;
	}
	else
	{
		if (!szIn.empty())
		{
			size_t uiInLen = (szIn.length() / 4 + 1) * 3;
			char *pInData = new char[uiInLen + 1]{ '\0' };

			if (nullptr != pInData)
			{
				uiInLen = CCodeConvert::DecodeBase64(szIn, (unsigned char*)pInData, (uint32_t)uiInLen);

				if (RSAPublicKey(szPubKeyFile, pInData, uiInLen, pOut, uiOutLen, szError, bEncrypt))
					szOut = (const char*)pOut;
				else
					bResult = false;

				delete[] pInData;
				pInData = nullptr;
			}
			else
			{
				szError = "Encrypted data(base64 string) convert to char* failed !";
				bResult = false;
			}
		}
		else
		{
			szError = "Encrypted data(base64 string) can't be null !";
			bResult = false;
		}
	}

	if (nullptr != pOut)
	{
		delete[] pOut;
		pOut = nullptr;
	}

	return bResult;
}

/********************************************************************************
功能：	私钥解密
参数：	szPriKeyFile 私钥文件名称
*		szPassword 私钥文件加密密码
*		szIn 输入数据
*		szOut 输出数据
*		szError 错误信息
*		bEncrypt true:加密，输出数据为base64编码，false:解密，注意私钥加密只能私钥解
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::RSAPrivateKey(const std::string &szPriKeyFile, const std::string &szPassword, const std::string &szIn, std::string &szOut,
	std::string &szError, bool bEncrypt)
{
	bool bResult = true;
	unsigned char* pOut = nullptr;
	size_t uiOutLen = 0;

	szOut.clear();

	if (bEncrypt)
	{
		if (RSAPrivateKey(szPriKeyFile, szPassword.c_str(), szIn.c_str(), szIn.length(), pOut, uiOutLen, szError, bEncrypt))
			szOut = CCodeConvert::EncodeBase64(pOut, uiOutLen);
		else
			bResult = false;
	}
	else
	{
		if (!szIn.empty())
		{
			size_t uiInLen = (szIn.length() / 4 + 1) * 3;
			char *pInData = new char[uiInLen + 1]{ '\0' };

			if (nullptr != pInData)
			{
				uiInLen = CCodeConvert::DecodeBase64(szIn, (unsigned char*)pInData, (uint32_t)uiInLen);

				if (RSAPrivateKey(szPriKeyFile, szPassword.c_str(), pInData, uiInLen, pOut, uiOutLen, szError, bEncrypt))
					szOut = (const char*)pOut;
				else
					bResult = false;

				delete[] pInData;
				pInData = nullptr;
			}
			else
			{
				szError = "Encrypted data(base64 string) convert to char* failed !";
				bResult = false;
			}
		}
		else
		{
			szError = "Encrypted data(base64 string) can't be null !";
			bResult = false;
		}
	}

	if (nullptr != pOut)
	{
		delete[] pOut;
		pOut = nullptr;
	}

	return bResult;
}

/********************************************************************************
功能：	公钥加密
参数：	szPubKeyFile 公钥文件名称
*		pIn 输入
*		uiInLen 输入长度
*		pOut 输出
*		uiOutLen 输出长度
*		szError 错误信息
*		bEncrypt true:加密，false:解密，注意私钥加密只能私钥解
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::RSAPublicKey(const std::string &szPubKeyFile, const char *pIn, size_t uiInLen, unsigned char *&pOut, size_t &uiOutLen,
	std::string &szError, bool bEncrypt)
{
	bool bResult = true;

	if (szPubKeyFile.empty())
	{
		szError = "Public key file name is null !";
		return false;
	}
	else
	{
		if (!CheckFileExist(szPubKeyFile.c_str()))
		{
			szError = "Public key file is not exist !";
			return false;
		}
	}

	if (nullptr == pIn || 0 == uiInLen)
	{
		szError = "In treatment data is null !";
		return false;
	}

	if (nullptr != pOut)
	{
		delete[] pOut;
		pOut = nullptr;
	}

	uiOutLen = 0;

	//读取公钥文件
	std::ifstream PubFile(szPubKeyFile.c_str(), std::ios::in | std::ios::binary);

	if (PubFile.is_open() && PubFile.good())
	{
		std::string szPubKey((std::istreambuf_iterator<char>(PubFile)), std::istreambuf_iterator<char>());
		PubFile.close();

		//打开公钥文件
		BIO *keybio = BIO_new_mem_buf(szPubKey.c_str(), szPubKey.length());

		if (nullptr != keybio)
		{
			//读取公钥文件
			RSA *rsa = NULL;
			const std::string pkcs1_header = "-----BEGIN RSA PUBLIC KEY-----";
			const std::string pkcs8_header = "-----BEGIN PUNLIC KEY-----";

			if (0 == strncmp(pkcs1_header.c_str(), szPubKey.c_str(), pkcs1_header.length()))
				rsa = PEM_read_bio_RSAPublicKey(keybio, NULL, NULL, NULL);
			else if (0 == strncmp(pkcs8_header.c_str(), szPubKey.c_str(), pkcs8_header.length()))
				rsa = PEM_read_bio_RSA_PUBKEY(keybio, NULL, NULL, NULL);
			else
			{
				bResult = false;
				szError = "Unknow public key file format !";
			}

			if (bResult && NULL != rsa)
			{
				//生成密钥对
				EVP_PKEY *keypair = EVP_PKEY_new();
				EVP_PKEY_set1_RSA(keypair, rsa);
				uiOutLen = uiInLen + 15360;
				pOut = new unsigned char[uiOutLen + 1]{ 0 };

				if (nullptr != pOut)
				{
					EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(keypair, NULL);

					if (NULL != ctx)
					{
						if (bEncrypt) //加密
						{
							if (1 == EVP_PKEY_encrypt_init(ctx))
							{
								EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);

								if (1 != EVP_PKEY_encrypt(ctx, pOut, &uiOutLen, (const unsigned char*)pIn, uiInLen))
								{
									bResult = false;
									szError = "EVP_PKEY_encrypt error:" + GetOpensslErrorMsg();
								}
							}
							else
							{
								bResult = false;
								szError = "EVP_PKEY_encrypt_init error:" + GetOpensslErrorMsg();
							}
						}
						else //解密
						{
							if (1 == EVP_PKEY_decrypt_init(ctx))
							{
								EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);

								if (1 != EVP_PKEY_decrypt(ctx, pOut, &uiOutLen, (const unsigned char*)pIn, uiInLen))
								{
									bResult = false;
									szError = "EVP_PKEY_decrypt error:" + GetOpensslErrorMsg();
								}
							}
							else
							{
								bResult = false;
								szError = "EVP_PKEY_decrypt_init error:" + GetOpensslErrorMsg();
							}
						}

						EVP_PKEY_CTX_free(ctx);
					}
					else
					{
						bResult = false;
						szError = "EVP_PKEY_CTX_new error:" + GetOpensslErrorMsg();
					}

					if (!bResult)
					{
						delete[] pOut;
						pOut = nullptr;
						uiOutLen = 0;
					}
				}
				else
				{
					bResult = false;
					uiOutLen = 0;
					szError = "New out data memory return nullptr:" + GetErrorMsg(errno);
				}

				EVP_PKEY_free(keypair);
				RSA_free(rsa);
			}
			else
			{
				bResult = false;
				if (szError.empty())
					szError = "PEM read bio key error:" + GetOpensslErrorMsg();
			}

			BIO_free_all(keybio);
		}
		else
		{
			bResult = false;
			szError = "BIO_new_mem_buf error:" + GetOpensslErrorMsg();
		}
	}
	else
	{
		bResult = false;
		szError = "Open public key file:" + szPubKeyFile + " error:" + GetErrorMsg(errno);
	}

	return bResult;
}

/********************************************************************************
功能：	私钥解密
参数：	szPriKeyFile 私钥文件名称
*		pPassword 私钥文件加密密码
*		pIn 输入
*		uiInLen 输入长度
*		pOut 输出
*		uiOutLen 输出长度
*		szError 错误信息
*		bEncrypt true:加密，false:解密，注意私钥加密只能私钥解
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::RSAPrivateKey(const std::string &szPriKeyFile, const char *pPassword, const char *pIn, size_t uiInLen, unsigned char *&pOut,
	size_t &uiOutLen, std::string &szError, bool bEncrypt)
{
	bool bResult = true;

	if (szPriKeyFile.empty())
	{
		szError = "Private key file name is null !";
		return false;
	}
	else
	{
		if (!CheckFileExist(szPriKeyFile.c_str()))
		{
			szError = "Private key file is not exist !";
			return false;
		}
	}

	if (nullptr == pIn || 0 == uiInLen)
	{
		szError = "In treatment data is null !";
		return false;
	}

	if (nullptr != pOut)
	{
		delete[] pOut;
		pOut = nullptr;
	}

	uiOutLen = 0;

	//读取私钥文件
	std::ifstream PriFile(szPriKeyFile.c_str(), std::ios::in | std::ios::binary);

	if (PriFile.is_open() && PriFile.good())
	{
		std::string szPriKey((std::istreambuf_iterator<char>(PriFile)), std::istreambuf_iterator<char>());
		PriFile.close();

		//打开私钥文件
		BIO *keybio = BIO_new_mem_buf(szPriKey.c_str(), szPriKey.length());

		if (nullptr != keybio)
		{
			//读取私钥文件
			RSA *rsa = NULL;
			const std::string pk_header_enc = "-----BEGIN RSA PRIVATE KEY-----\nProc-Type:"; //私钥文件有加密

			if (0 == strncmp(pk_header_enc.c_str(), szPriKey.c_str(), pk_header_enc.length()))
			{
				if (nullptr == pPassword || 4 > std::strlen(pPassword))
				{
					bResult = false;
					szError = "Private key file encrypt password can't be null and length must greater than or equal 4 !";
				}
				else
					rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL, NULL, (void*)pPassword);
			}
			else
				rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL, NULL, NULL);

			if (bResult && NULL != rsa)
			{
				//生成密钥对
				EVP_PKEY *keypair = EVP_PKEY_new();
				EVP_PKEY_set1_RSA(keypair, rsa);
				uiOutLen = uiInLen + 15360;
				pOut = new unsigned char[uiOutLen + 1]{ 0 };

				if (nullptr != pOut)
				{
					EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(keypair, NULL);

					if (nullptr != ctx)
					{
						if (bEncrypt) //加密
						{
							if (1 == EVP_PKEY_encrypt_init(ctx))
							{
								EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);

								if (1 != EVP_PKEY_encrypt(ctx, pOut, &uiOutLen, (const unsigned char*)pIn, uiInLen))
								{
									bResult = false;
									szError = "EVP_PKEY_encrypt error:" + GetOpensslErrorMsg();
								}
							}
							else
							{
								bResult = false;
								szError = "EVP_PKEY_encrypt_init error:" + GetOpensslErrorMsg();
							}
						}
						else //解密，注意私钥加密的数据只能私钥解，不然这里会崩溃
						{
							if (1 == EVP_PKEY_decrypt_init(ctx))
							{
								EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);

								if (1 != EVP_PKEY_decrypt(ctx, pOut, &uiOutLen, (const unsigned char*)pIn, uiInLen))
								{
									bResult = false;
									szError = "EVP_PKEY_decrypt error:" + GetOpensslErrorMsg();
								}
							}
							else
							{
								bResult = false;
								szError = "EVP_PKEY_decrypt_init error:" + GetOpensslErrorMsg();
							}
						}

						EVP_PKEY_CTX_free(ctx);
					}
					else
					{
						bResult = false;
						szError = "EVP_PKEY_CTX_new error:" + GetOpensslErrorMsg();
					}

					if (!bResult)
					{
						delete[] pOut;
						pOut = nullptr;
						uiOutLen = 0;
					}
				}
				else
				{
					bResult = false;
					uiOutLen = 0;
					szError = "New out data memory return nullptr:" + GetErrorMsg(errno);
				}

				EVP_PKEY_free(keypair);
				RSA_free(rsa);
			}
			else
			{
				bResult = false;
				if (szError.empty())
					szError = "PEM read bio key error:" + GetOpensslErrorMsg();
			}

			BIO_free_all(keybio);
		}
		else
		{
			bResult = false;
			szError = "BIO_new_mem_buf error:" + GetOpensslErrorMsg();
		}
	}
	else
	{
		bResult = false;
		szError = "Open public key file:" + szPriKeyFile + " error:" + GetErrorMsg(errno);
	}

	return bResult;
}

/********************************************************************************
功能：	签名
参数：	szPriKeyFile 私钥文件名称
*		pPassword 私钥文件加密密码
*		eHashType 摘要算法类型
*		szFileName 要签名的文件
*		pOut 输出
*		uiOutLen 输出长度
*		szError 错误信息
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::RSASignFile(const std::string &szPriKeyFile, const char *pPassword, EHashType eHashType, const std::string &szFileName, unsigned char *&pOut,
	unsigned int &uiOutLen, std::string &szError)
{
	bool bResult = true;

	if (szPriKeyFile.empty())
	{
		szError = "Private key file name is null !";
		return false;
	}
	else
	{
		if (!CheckFileExist(szPriKeyFile.c_str()))
		{
			szError = "Private key file is not exist !";
			return false;
		}
	}

	if (nullptr == pPassword || 4 > std::strlen(pPassword))
	{
		szError = "Private key file encrypt password can't be null and length must greater than or equal 4 !";
		return false;
	}

	if (szFileName.empty())
	{
		szError = "Sign file name is null !";
		return false;
	}
	else
	{
		if (!CheckFileExist(szFileName.c_str()))
		{
			szError = "Sign file is not exist !";
			return false;
		}
	}

	if (nullptr != pOut)
	{
		delete[] pOut;
		pOut = nullptr;
	}

	uiOutLen = 0;

	EVP_MD_CTX *ctx = EVP_MD_CTX_create();
	if (nullptr == ctx)
	{
		szError = "EVP_MD_CTX_create error:" + GetOpensslErrorMsg();
		return false;
	}
	else
		EVP_MD_CTX_init(ctx);

	if (1 != EVP_SignInit_ex(ctx, (const EVP_MD*)GetHashAlgorithm(eHashType), nullptr))
	{
		bResult = false;
		szError = "EVP_SignInit_ex error:" + GetOpensslErrorMsg();
	}
	else
	{
		std::ifstream file;

		file.open(szFileName.c_str(), std::ios::binary | std::ios::in);
		if (file.is_open() && file.good())
		{
			const std::streamsize iBufLen = 1024;
			char buf[iBufLen + 1] = { '\0' };
			std::streamsize readLen = 0;

			while (!file.eof())
			{
				file.read(buf, iBufLen);
				readLen = file.gcount(); //实际读取长度

				if (readLen >= 0)
				{
					if (1 != EVP_SignUpdate(ctx, buf, (size_t)readLen, pError))
					{
						bResult = false;
						szError = "EVP_SignUpdate error:" + GetOpensslErrorMsg();
						break;
					}
				}
			}

			file.close();
		}
		else
		{
			bResult = false;
			szError = "Open file fail:" + GetErrorMsg(errno);
		}
	}

	if (bResult)
	{
		//读取私钥文件
		std::ifstream PriFile(szPriKeyFile.c_str(), std::ios::in | std::ios::binary);

		if (PriFile.is_open() && PriFile.good())
		{
			std::string szPriKey((std::istreambuf_iterator<char>(PriFile)), std::istreambuf_iterator<char>());
			PriFile.close();

			//打开私钥文件
			BIO *keybio = BIO_new_mem_buf(szPriKey.c_str(), szPriKey.length());

			if (nullptr != keybio)
			{
				//读取私钥文件
				RSA *rsa = NULL;
				const std::string pk_header_enc = "-----BEGIN RSA PRIVATE KEY-----\nProc-Type:"; //私钥文件有加密

				if (0 == strncmp(pk_header_enc.c_str(), szPriKey.c_str(), pk_header_enc.length()))
				{
					if (nullptr == pPassword || 4 > std::strlen(pPassword))
					{
						bResult = false;
						szError = "Private key file encrypt password can't be null and length must greater than or equal 4 !";
					}
					else
						rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL, NULL, (void*)pPassword);
				}
				else
					rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL, NULL, NULL);

				if (bResult && NULL != rsa)
				{
					//生成密钥对
					EVP_PKEY *keypair = EVP_PKEY_new();
					EVP_PKEY_set1_RSA(keypair, rsa);

					pOut = new unsigned char[2048]{ 0 };
					if (nullptr != pOut)
					{
						if (1 != EVP_SignFinal(ctx, pOut, &uiOutLen, keypair))
						{
							bResult = false;
							delete[] pOut;
							pOut = nullptr;
							szError = "EVP_SignFinal error:" + GetOpensslErrorMsg();
						}
					}
					else
					{
						bResult = false;
						szError = "New sign data memory return nullptr:" + GetErrorMsg(errno);
					}

					EVP_PKEY_free(keypair);
					RSA_free(rsa);
				}
				else
				{
					bResult = false;
					if (szError.empty())
						szError = "PEM read bio key error:" + GetOpensslErrorMsg();
				}

				BIO_free_all(keybio);
			}
			else
			{
				bResult = false;
				szError = "BIO_new_mem_buf error:" + GetOpensslErrorMsg();
			}
		}
		else
		{
			bResult = false;
			szError = "Open public key file:" + szPriKeyFile + " error:" + GetErrorMsg(errno);
		}
	}

	EVP_MD_CTX_cleanup(ctx);
	EVP_MD_CTX_destroy(ctx);

	return bResult;
}

/********************************************************************************
功能：	验证签名
参数：	szPubKeyFile 公钥文件名称
*		eHashType 摘要算法类型
*		szFileName 要签名的文件
*		pSign 签名数据
*		uiSignLen 签名长度
*		szError 错误信息
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::RSAVerifySign(const std::string &szPubKeyFile, EHashType eHashType, const std::string &szFileName, const unsigned char *pSign,
	unsigned int uiSignLen, std::string &szError)
{
	bool bResult = true;

	if (szPubKeyFile.empty())
	{
		szError = "Public key file name is null !";
		return false;
	}
	else
	{
		if (!CheckFileExist(szPubKeyFile.c_str()))
		{
			szError = "Public key file is not exist !";
			return false;
		}
	}

	if (szFileName.empty())
	{
		szError = "Sign file name is null !";
		return false;
	}
	else
	{
		if (!CheckFileExist(szFileName.c_str()))
		{
			szError = "Sign file is not exist !";
			return false;
		}
	}

	if (nullptr == pSign || 0 == uiSignLen)
	{
		szError = "Sign data is null !";
		return false;
	}

	EVP_MD_CTX *ctx = EVP_MD_CTX_create();
	if (nullptr == ctx)
	{
		szError = "EVP_MD_CTX_create error:" + GetOpensslErrorMsg();
		return false;
	}
	else
		EVP_MD_CTX_init(ctx);

	if (1 != EVP_VerifyInit_ex(ctx, (const EVP_MD*)GetHashAlgorithm(eHashType), nullptr))
	{
		bResult = false;
		szError = "EVP_VerifyInit_ex error:" + GetOpensslErrorMsg();
	}
	else
	{
		std::ifstream file;

		file.open(szFileName.c_str(), std::ios::binary | std::ios::in);
		if (file.is_open() && file.good())
		{
			const std::streamsize iBufLen = 1024;
			char buf[iBufLen + 1] = { '\0' };
			std::streamsize readLen = 0;

			while (!file.eof())
			{
				file.read(buf, iBufLen);
				readLen = file.gcount(); //实际读取长度

				if (readLen >= 0)
				{
					if (1 != EVP_VerifyUpdate(ctx, buf, (size_t)readLen, pError))
					{
						bResult = false;
						szError = "EVP_VerifyUpdate error:" + GetOpensslErrorMsg();
						break;
					}
				}
			}

			file.close();
		}
		else
		{
			bResult = false;
			szError = "Open file fail:" + GetErrorMsg(errno);
		}
	}

	if (bResult)
	{
		//读取公钥文件
		std::ifstream PubFile(szPubKeyFile.c_str(), std::ios::in | std::ios::binary);

		if (PubFile.is_open() && PubFile.good())
		{
			std::string szPubKey((std::istreambuf_iterator<char>(PubFile)), std::istreambuf_iterator<char>());
			PubFile.close();

			//打开公钥文件
			BIO *keybio = BIO_new_mem_buf(szPubKey.c_str(), szPubKey.length());

			if (nullptr != keybio)
			{
				//读取公钥文件
				RSA *rsa = NULL;
				const std::string pkcs1_header = "-----BEGIN RSA PUBLIC KEY-----";
				const std::string pkcs8_header = "-----BEGIN PUNLIC KEY-----";

				if (0 == strncmp(pkcs1_header.c_str(), szPubKey.c_str(), pkcs1_header.length()))
					rsa = PEM_read_bio_RSAPublicKey(keybio, NULL, NULL, NULL);
				else if (0 == strncmp(pkcs8_header.c_str(), szPubKey.c_str(), pkcs8_header.length()))
					rsa = PEM_read_bio_RSA_PUBKEY(keybio, NULL, NULL, NULL);
				else
				{
					bResult = false;
					szError = "Unknow public key file format !";
				}

				if (bResult && NULL != rsa)
				{
					//生成密钥对
					EVP_PKEY *keypair = EVP_PKEY_new();
					EVP_PKEY_set1_RSA(keypair, rsa);

					if (1 != EVP_VerifyFinal(ctx, pSign, uiSignLen, keypair))
					{
						bResult = false;
						szError = "EVP_VerifyFinal error:" + GetOpensslErrorMsg();
					}

					EVP_PKEY_free(keypair);
					RSA_free(rsa);
				}
				else
				{
					bResult = false;
					if (szError.empty())
						szError = "PEM read bio key error:" + GetOpensslErrorMsg();
				}

				BIO_free_all(keybio);
			}
			else
			{
				bResult = false;
				szError = "BIO_new_mem_buf error:" + GetOpensslErrorMsg();
			}
		}
		else
		{
			bResult = false;
			szError = "Open private key file:" + szPubKeyFile + " error:" + GetErrorMsg(errno);
		}
	}

	EVP_MD_CTX_cleanup(ctx);
	EVP_MD_CTX_destroy(ctx);

	return bResult;
}

/********************************************************************************
功能：	字符串转HASH
参数：	eHashType hash类型
*		szData 字符串
*		szHash 16进制的MD5串
*		bIsFile 是否是文件，如果是文件则szData是文件名
*		pError 错误信息
*		bLowercase true:小写，false:大写
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::Hash(EHashType eHashType, const std::string &szData, std::string &szHash, bool bIsFile, std::string *pError, bool bLowercase)
{
	if (szData.empty())
	{
		if (nullptr != pError)
		{
			if (bIsFile)
				*pError = "Hash treatment file name is null !";
			else
				*pError = "Hash treatment data is empty !";	
		}

		return false;
	}

	unsigned char md[EVP_MAX_MD_SIZE]{ '\0' };
	unsigned int md_len = 0;

	if (Hash(eHashType, szData.c_str(), szData.length(), md, md_len, bIsFile, pError, bLowercase))
	{
		szHash = GetHexString(md, md_len, bLowercase);
	}
	else
		return false;

	return true;
}

/********************************************************************************
功能：	字符串转HASH
参数：	eHashType hash类型
*		pData 字符串
*		uiLength 字符串长度
*		pHash hash串
*		uiHashLen hash串长度
*		bIsFile 是否是文件，如果是文件则szData是文件名
*		pError 错误信息
*		bLowercase true:小写，false:大写
返回：	成功返回true
修改：
********************************************************************************/
bool COpenssl::Hash(EHashType eHashType, const char *pData, unsigned int uiLength, unsigned char *pHash, unsigned int &uiHashLen, bool bIsFile,
	std::string *pError, bool bLowercase)
{
	if (nullptr == pData || 0 == uiLength)
	{
		if (nullptr != pError)
		{
			if (bIsFile)
				*pError = "Hash treatment file name is null !";
			else
				*pError = "Hash treatment data is empty !";
		}

		return false;
	}

	if (bIsFile && !CheckFileExist(pData))
	{
		if (nullptr != pError)
		{
			*pError = "Hash treatment file is not exist !";
		}

		return false;
	}

	bool bResult = false;
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();

	if (nullptr != ctx)
	{
		const EVP_MD *pmd = (const EVP_MD*)GetHashAlgorithm(eHashType);
		EVP_MD_CTX_init(ctx);

		if (DigestInit(ctx, pmd, NULL, pError))
		{
			if (bIsFile)
			{
				std::ifstream file;

				file.open(pData, std::ios::binary | std::ios::in);
				if (file.is_open() && file.good())
				{
					bool bUpdateFlag = true;
					const std::streamsize iBufLen = 1024;
					char buf[iBufLen + 1] = { '\0' };
					std::streamsize readLen = 0;

					while (!file.eof())
					{
						file.read(buf, iBufLen);
						readLen = file.gcount(); //实际读取长度

						if (readLen >= 0)
						{
							if (!DigestUpdate(ctx, buf, (size_t)readLen, pError))
							{
								bUpdateFlag = false;
								break;
							}
						}
					}

					file.close();

					if (bUpdateFlag && DigestFinal(ctx, pHash, &uiHashLen, pError))
					{
						bResult = true;
					}
				}
				else
				{
					if (nullptr != pError)
						*pError = "Open file fail:" + GetErrorMsg(errno);
				}
			}
			else
			{
				if (DigestUpdate(ctx, pData, uiLength, pError))
				{
					if (DigestFinal(ctx, pHash, &uiHashLen, pError))
					{
						bResult = true;
					}
				}
			}
		}

		EVP_MD_CTX_destroy(ctx);
	}

	return bResult;
}

/*********************************************************************
功能：	加密处理
参数：	pKey 加密密钥对象
*		pIn 要加密的数据
*		iInLen 要加密的数据长度
*		pOut 加密后的数据，内存由调用都释放
*		iOutLen 加密后的数据长度
*		szError 错误信息
返回：	0:成功，-1:加密处理失败，-2:参数错误
修改：
*********************************************************************/
int COpenssl::EncryptHandle(const void *pKey, const unsigned char *pIn, int iInLen, unsigned char *&pOut, int &iOutLen,
	std::string &szError)
{
	if (E_KEY_NONE == m_eKeyType)
	{
		szError = "Encryption key is unset !";
		return -2;
	}

	if (nullptr == pKey)
	{
		szError = "Encrypted AES_KEY is null !";
		return -2;
	}

	iOutLen = 0;
	szError.clear();

	if (nullptr != pOut)
	{
		delete[] pOut;
		pOut = nullptr;
	}

	int iRet = 0;
	int iEncryptLen = 0;
	const int iDealLen = (iInLen / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;
	unsigned char *pInTemp = new unsigned char[iDealLen + 1]{ '\0' };

	if (nullptr != pInTemp)
	{
		memcpy(pInTemp, pIn, iInLen);
		DealPadding(pInTemp, iInLen, true, szError);
		pOut = new unsigned char[iDealLen + 1]{ '\0' };

		if (nullptr != pOut)
		{
			while (iEncryptLen < iDealLen)
			{
				AES_encrypt(pInTemp + iEncryptLen, pOut + iEncryptLen, (const AES_KEY*)pKey);
				iEncryptLen += AES_BLOCK_SIZE;
			}

			iOutLen = iDealLen;
		}
		else
		{
			szError = "New encrypted data memory failed:" + GetErrorMsg(errno);
			iRet = -1;
		}

		if (nullptr != pInTemp)
		{
			delete[] pInTemp;
			pInTemp = nullptr;
		}
	}
	else
	{
		szError = "New plaintext data memory failed:" + GetErrorMsg(errno);
		iRet = -1;
	}

	return iRet;
}

/*********************************************************************
功能：	解密处理
参数：	pKey 解密密钥对象
*		pIn 要解密的数据
*		iInLen 要解密的数据长度
*		pOut 解密后的数据，内存由调用都释放
*		iOutLen 解密后的数据长度
*		szError 错误信息
返回：	0:成功，-1:解密处理失败，-2:参数错误
修改：
*********************************************************************/
int COpenssl::DecryptHandle(const void *pKey, const unsigned char *pIn, int iInLen, unsigned char *&pOut, int &iOutLen,
	std::string &szError)
{
	if (E_KEY_NONE == m_eKeyType)
	{
		szError = "Decryption key is unset !";
		return -2;
	}

	if (nullptr == pKey)
	{
		szError = "Decrypted AES_KEY is null !";
		return -2;
	}

	if (nullptr == pIn || 0 >= iInLen || (0 != iInLen % AES_BLOCK_SIZE))
	{
		szError = "Decrypted data is null or wrong length !";
		return -2;
	}

	iOutLen = 0;
	szError.clear();

	if (nullptr != pOut)
	{
		delete[] pOut;
		pOut = nullptr;
	}

	int iRet = 0, uiDecryptLen = 0;
	pOut = new unsigned char[iInLen + 1]{ '\0' };

	if (nullptr != pOut)
	{
		while (uiDecryptLen < iInLen)
		{
			AES_decrypt(pIn + uiDecryptLen, pOut + uiDecryptLen, (const AES_KEY*)pKey);
			uiDecryptLen += AES_BLOCK_SIZE;
		}

		iRet = DealPadding(pOut, iInLen, false, szError);

		if (iRet >= 1)
		{
			iOutLen = iInLen - iRet;
			iRet = 0;
		}
		else
		{
			iRet = -1;
			delete[] pOut;
			pOut = nullptr;
		}
	}
	else
	{
		szError = "New decrypted data memory failed:" + GetErrorMsg(errno);
		iRet = -1;
	}

	return iRet;
}

/*********************************************************************
功能：	加密处理
参数：	eCipherType 算法类型
*		pCipher 加密方法
*		pKey 加密密钥
*		pIV iv值
*		pIn 要加密的数据
*		iInLen 要加密的数据长度
*		pOut 加密后的数据，内存由调用都释放
*		iOutLen 加密后的数据长度
*		szError 错误信息
返回：	0:成功，-1:加密处理失败，-2:参数错误
修改：
*********************************************************************/
int COpenssl::EncryptHandle(ECipherType eCipherType, const void *pCipher, const unsigned char *pKey, const unsigned char *pIV, const unsigned char *pIn,
	int iInLen, unsigned char *&pOut, int &iOutLen, std::string &szError)
{
	if (E_KEY_NONE == m_eKeyType)
	{
		szError = "Encryption key and iv is unset !";
		return -2;
	}

	if (nullptr == pCipher)
	{
		szError = "Encrypted cipher is null !";
		return -2;
	}

	iOutLen = 0;
	szError.clear();

	if (nullptr != pOut)
	{
		delete[] pOut;
		pOut = nullptr;
	}

	int iRet = 0;

	//new加密对象
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if (nullptr != ctx)
	{
		//初始化
		if (CipherInit(ctx, pCipher, NULL, pKey, pIV, true, szError))
		{
			//申请存放加密数据的内存
			pOut = new unsigned char[(iInLen / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE + 1]{ '\0' };

			if (nullptr != pOut)
			{
				int iLenTemp = 0;

				//加密
				if (CipherUpdate(ctx, pOut, &iLenTemp, pIn, iInLen, szError))
				{
					iOutLen = iLenTemp;
					iLenTemp = 0;

					//结束
					if (1 == CipherFinal(ctx, iOutLen, pOut + iOutLen, &iLenTemp, szError))
					{
						iOutLen += iLenTemp;
						pOut[iOutLen] = '\0';
					}
					else
						iRet = -1;
				}
				else
					iRet = -1;
			}
			else
			{
				iRet = -1;
				szError = "New encrypted data memory failed:" + GetErrorMsg(errno);
			}
		}
		else
			iRet = -1;

		//获取错误信息
		if (iRet < 0)
		{
			if (nullptr != pOut)
			{
				delete[] pOut;
				pOut = nullptr;
			}
		}

		EVP_CIPHER_CTX_free(ctx);
		ctx = nullptr;
	}
	else
	{
		iRet = -1;
		szError = "New EVP_CIPHER_CTX handle failed:" + GetErrorMsg(errno);
	}

	return iRet;
}

/*********************************************************************
功能：	解密处理
参数：	eCipherType 算法类型
*		pCipher 解密方法
*		pKey 解密密钥
*		pIV iv值
*		pIn 要解密的数据
*		iInLen 要解密的数据长度
*		pOut 解密后的数据，内存由调用都释放
*		iOutLen 解密后的数据长度
*		szError 错误信息
返回：	0:成功，-1:解密处理失败，-2:参数错误
修改：
*********************************************************************/
int COpenssl::DecryptHandle(ECipherType eCipherType, const void *pCipher, const unsigned char *pKey, const unsigned char *pIV, const unsigned char *pIn,
	int iInLen, unsigned char *&pOut, int &iOutLen, std::string &szError)
{
	if (E_KEY_NONE == m_eKeyType)
	{
		szError = "Decryption key and iv is unset !";
		return -2;
	}

	if (nullptr == pCipher)
	{
		szError = "Decrypted cipher is null !";
		return -2;
	}

	//长度校验
	if (E_CIPHER_AES_CFB != eCipherType && E_CIPHER_AES_OFB != eCipherType
		&& E_CIPHER_DES_CFB != eCipherType && E_CIPHER_DES_OFB != eCipherType)
	{
		if (0 != iInLen % AES_BLOCK_SIZE)
		{
			szError = "Decrypted data length must multiple of 16 !";
			return -2;
		}
	}

	iOutLen = 0;
	szError.clear();

	if (nullptr != pOut)
	{
		delete[] pOut;
		pOut = nullptr;
	}

	int iRet = 0;

	//new解密对象
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if (nullptr != ctx)
	{
		//初始化
		if (CipherInit(ctx, pCipher, nullptr, pKey, pIV, false, szError))
		{
			//申请存放解密数据的内存
			pOut = new unsigned char[iInLen + 1]{ '\0' };

			if (nullptr != pOut)
			{
				int iLenTemp = 0;

				//加密
				if (CipherUpdate(ctx, pOut, &iLenTemp, pIn, iInLen, szError))
				{
					iOutLen = iLenTemp;
					iLenTemp = 0;

					//结束
					if (CipherFinal(ctx, iOutLen, pOut + iOutLen, &iLenTemp, szError))
					{
						iOutLen += iLenTemp;
						pOut[iOutLen] = '\0';
					}
					else
						iRet = -1;
				}
				else
					iRet = -1;
			}
			else
			{
				iRet = -1;
				szError = "New decrypted data memory failed:" + GetErrorMsg(errno);
			}
		}
		else
			iRet = -1;

		//获取错误信息
		if (iRet < 0)
		{
			if (nullptr != pOut)
			{
				delete[] pOut;
				pOut = nullptr;
			}
		}

		EVP_CIPHER_CTX_free(ctx);
		ctx = nullptr;
	}
	else
	{
		iRet = -1;
		szError = "New EVP_CIPHER_CTX handle failed:" + GetErrorMsg(errno);
	}

	return iRet;
}

/*********************************************************************
功能：	获取加密算法指针
参数：	eCipherType 算法类型
*		eKeyType 密钥长度
*		bEncrypt true:加密，false:解密
*		szError 错误信息
返回：	算法指针，nullptr:失败
修改：
*********************************************************************/
const void* COpenssl::GetCipher(ECipherType eCipherType, EKeyType eKeyType, bool bEncrypt, std::string &szError)
{
	const void *pCipher = nullptr;

	if (E_KEY_NONE == eKeyType)
	{
		szError = "Unset key and iv value !";
		return pCipher;
	}

	switch (eCipherType)
	{
	case COpenssl::E_CIPHER_AES:
	{
		int iRet = 0;
		AES_KEY *p = new AES_KEY();

		if (nullptr != p)
		{
			if (E_KEY_128 == eKeyType)
			{
				if (bEncrypt)
					iRet = AES_set_encrypt_key(m_Key, E_KEY_128, p);
				else
					iRet = AES_set_decrypt_key(m_Key, E_KEY_128, p);
			}
			else if (E_KEY_192 == eKeyType)
			{
				if (bEncrypt)
					iRet = AES_set_encrypt_key(m_Key, E_KEY_192, p);
				else
					iRet = AES_set_decrypt_key(m_Key, E_KEY_192, p);
			}
			else
			{
				if (bEncrypt)
					iRet = AES_set_encrypt_key(m_Key, E_KEY_256, p);
				else
					iRet = AES_set_decrypt_key(m_Key, E_KEY_256, p);
			}

			if (0 != iRet)
			{
				szError = "Set cipher key failed:" + GetOpensslErrorMsg();
				delete p;
			}
			else
				pCipher = p;
		}
		else
			szError = "New AES_KEY return nullptr !";
	}
	break;
	case COpenssl::E_CIPHER_AES_ECB:
	{
		if (E_KEY_128 == eKeyType)
			pCipher = EVP_aes_128_ecb();
		else if (E_KEY_192 == eKeyType)
			pCipher = EVP_aes_192_ecb();
		else
			pCipher = EVP_aes_256_ecb();
	}
	break;
	case COpenssl::E_CIPHER_AES_CBC:
	{
		if (E_KEY_128 == eKeyType)
			pCipher = EVP_aes_128_cbc();
		else if (E_KEY_192 == eKeyType)
			pCipher = EVP_aes_192_cbc();
		else
			pCipher = EVP_aes_256_cbc();
	}
	break;
	case COpenssl::E_CIPHER_AES_CFB:
	{
		if (E_KEY_128 == eKeyType)
			pCipher = EVP_aes_128_cfb();
		else if (E_KEY_192 == eKeyType)
			pCipher = EVP_aes_192_cfb();
		else
			pCipher = EVP_aes_256_cfb();
	}
	break;
	case COpenssl::E_CIPHER_AES_OFB:
	{
		if (E_KEY_128 == eKeyType)
			pCipher = EVP_aes_128_ofb();
		else if (E_KEY_192 == eKeyType)
			pCipher = EVP_aes_192_ofb();
		else
			pCipher = EVP_aes_256_ofb();
	}
	break;
	case COpenssl::E_CIPHER_DES_ECB:
		pCipher = EVP_des_ecb();
		break;
	case COpenssl::E_CIPHER_DES_CBC:
		pCipher = EVP_des_cbc();
		break;
	case COpenssl::E_CIPHER_DES_CFB:
		pCipher = EVP_des_cfb();
		break;
	case COpenssl::E_CIPHER_DES_OFB:
		pCipher = EVP_des_ofb();
		break;
	default:
		szError = "Unknow cipher type !";
		break;
	}

	return pCipher;
}

/*********************************************************************
功能：	获取hash算法
参数：	eHashType 算法类型
返回：	算法指针
修改：
*********************************************************************/
const void* COpenssl::GetHashAlgorithm(EHashType eHashType)
{
	const EVP_MD *pmd = nullptr;

	switch (eHashType)
	{
	case COpenssl::E_MD_MD4:
		pmd = EVP_md4();
		break;
	case COpenssl::E_MD_MD5:
		pmd = EVP_md5();
		break;
	case COpenssl::E_MD_MDC2:
		pmd = EVP_mdc2();
		break;
	case COpenssl::E_MD_SHA:
		pmd = EVP_sha();
		break;
	case COpenssl::E_MD_SHA1:
		pmd = EVP_sha1();
		break;
	case COpenssl::E_MD_SHA224:
		pmd = EVP_sha224();
		break;
	case COpenssl::E_MD_SHA256:
		pmd = EVP_sha256();
		break;
	case COpenssl::E_MD_SHA384:
		pmd = EVP_sha384();
		break;
	case COpenssl::E_MD_SHA512:
		pmd = EVP_sha512();
		break;
	case COpenssl::E_MD_WHIRLPOOL:
		pmd = EVP_whirlpool();
		break;
	default:
		pmd = EVP_md5(); //默认MD5
		break;
	}

	return pmd;
}

/*********************************************************************
功能：	获取openssl错误信息
参数：	无
返回：	错误信息
修改：
*********************************************************************/
std::string COpenssl::GetOpensslErrorMsg()
{
	std::string szError;
	const size_t uiErrorBufLen = 4096;
	char ErrorBuf[uiErrorBufLen + 1];

	memset(ErrorBuf, 0, uiErrorBufLen + 1);
	ERR_error_string_n(ERR_get_error(), ErrorBuf, uiErrorBufLen);
	szError = ErrorBuf;

	return std::move(szError);
}

bool COpenssl::DigestInit(void *ctx, const void *type, void *impl, std::string *pError)
{
	if (1 != EVP_DigestInit_ex((EVP_MD_CTX*)ctx, (const EVP_MD*)type, NULL))
	{
		if (nullptr != pError)
			*pError = GetOpensslErrorMsg();

		return false;
	}

	return true;
}
bool COpenssl::DigestUpdate(void *ctx, const void *data, size_t cnt, std::string *pError)
{
	if (1 != EVP_DigestUpdate((EVP_MD_CTX*)ctx, data, cnt))
	{
		if (nullptr != pError)
			*pError = GetOpensslErrorMsg();

		return false;
	}

	return true;
}
bool COpenssl::DigestFinal(void *ctx, unsigned char* md, unsigned int *s, std::string *pError)
{
	if (1 != EVP_DigestFinal_ex((EVP_MD_CTX*)ctx, md, s))
	{
		if (nullptr != pError)
			*pError = GetOpensslErrorMsg();

		return false;
	}

	return true;
}

bool COpenssl::CipherInit(void *ctx, const void *cipher, void *impl, const unsigned char *key, const unsigned char *iv, bool bEncrypt,
	std::string &szError)
{
	if (1 != EVP_CipherInit_ex((EVP_CIPHER_CTX*)ctx, (EVP_CIPHER*)cipher, NULL, key, iv, (bEncrypt ? 1 : 0)))
	{
		szError = GetOpensslErrorMsg();
		return false;
	}

	return true;
}
bool COpenssl::CipherUpdate(void *ctx, unsigned char *out, int *outl, const unsigned char* in, int inl, std::string &szError)
{
	if (1 != EVP_CipherUpdate((EVP_CIPHER_CTX*)ctx, out, outl, in, inl))
	{
		szError = GetOpensslErrorMsg();
		return false;
	}

	return true;
}
bool COpenssl::CipherFinal(void *ctx, int dl, unsigned char *outm, int *outl, std::string &szError)
{
	if (1 != EVP_CipherFinal_ex((EVP_CIPHER_CTX*)ctx, outm + dl, outl))
	{
		szError = GetOpensslErrorMsg();
		return false;
	}

	return true;
}

/*********************************************************************
功能：	加载公共信息
参数：	无
返回：	无
修改：
*********************************************************************/
void COpenssl::LoadPublicObject()
{
	//加载公共信息
	if (!m_bLoadPublicObject)
	{
		m_bLoadPublicObject = true;

		//错误信息
		ERR_load_ERR_strings();
		ERR_load_crypto_strings();
		ERR_load_RSA_strings();
		ERR_load_PEM_strings();
		ERR_load_BIO_strings();

		//rsa私钥文件有加密必须调用
		OpenSSL_add_all_algorithms();
	}
}

/*********************************************************************
功能：	处理加解密填充
参数：	pData 要处理的数据
*		uiDataLength 加密时表示要加密的明文长度，解密时表示解密后的数据（包括填充）长度
*		bEncrypt true:加密，false:解密
*		szError 错误信息
返回：	成功返回填充长度，失败返回-1
修改：
*********************************************************************/
int COpenssl::DealPadding(unsigned char *pData, size_t uiDataLength, bool bEncrypt, std::string &szError)
{
	int iRet = -1;

	if (nullptr == pData || 0 == uiDataLength)
	{
		szError = "Deal padding data is null !";
		return iRet;
	}

	if (bEncrypt)
	{
		iRet = AES_BLOCK_SIZE - uiDataLength % AES_BLOCK_SIZE;
		unsigned char chTemp = (unsigned char)iRet;

		//设置填充
		for (int i = 0; i < iRet; ++i)
		{
			pData[uiDataLength + i] = chTemp;
		}
	}
	else
	{
		iRet = (pData[uiDataLength - 1]);

		//最少填充1字节，最多填充16字节
		if (iRet >= 1 && iRet <= 16)
		{
			//验证填充
			unsigned char chTemp = (unsigned char)iRet;

			for (int i = 1; i <= iRet; ++i)
			{
				if (pData[uiDataLength - i] != chTemp)
				{
					iRet = -1;
					break;
				}
			}
		}
		else
			iRet = -1;

		if (iRet <= 0)
			szError = "Decrypted failed, data padding wrong !";
		else
			pData[uiDataLength - iRet] = '\0';
	}

	return iRet;
}

/*********************************************************************
功能：	获取系统错误信息
参数：	sysErrCode 系统错误码
返回：	错误信息
修改：
*********************************************************************/
std::string COpenssl::GetErrorMsg(unsigned int sysErrCode)
{
#ifdef _WIN32
	std::string szError;
	LPVOID lpMsgBuf = NULL;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, sysErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);

	if (NULL != lpMsgBuf)
	{
		szError = (LPSTR)lpMsgBuf;
		LocalFree(lpMsgBuf);
		return std::move(szError);
	}
	else
	{
		szError = "FormatMessage failed, error code:" + std::to_string(GetLastError()) + " original code:"
			+ std::to_string(sysErrCode);
		return std::move(szError);
	}
#else
	return std::move(std::string(strerror(sysErrCode)));
#endif
}

/*********************************************************************
功能：	获取16进制的字符
参数：	ch 1字节前或后4位的值
*		isLowercase 是否小写字母
返回：	转后的字符
修改：
*********************************************************************/
char COpenssl::GetHexChar(const char &ch, bool isLowercase)
{
	switch (ch)
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
		return (isLowercase ? 'a' : 'A');
	case 11:
		return (isLowercase ? 'b' : 'B');
	case 12:
		return (isLowercase ? 'c' : 'C');
	case 13:
		return (isLowercase ? 'd' : 'D');
	case 14:
		return (isLowercase ? 'e' : 'E');
	default:
		break;
	}

	return (isLowercase ? 'f' : 'F');
}

/*********************************************************************
功能：	16进制的字符转数
参数：	ch 1字节16进制字符
返回：	转后的数
修改：
*********************************************************************/
char COpenssl::FromHexChar(const char &ch)
{
	switch (ch)
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

/*********************************************************************
功能：	char*数据转16进制字符串
参数：	data 待转数据，输入
*		data_len 数据长度，输入
*		bLowercase 结果是否小写
返回：	16进制字符串
修改：
*********************************************************************/
std::string COpenssl::GetHexString(const unsigned char *data, int data_len, bool bLowercase)
{
	std::string szReturnHex;

	if (nullptr == data || 0 >= data_len)
		return std::move(szReturnHex);

	int n = 0;
	char *pStrTemp = new char[data_len * 2 + 1];

	if (nullptr != pStrTemp)
	{
		for (int i = 0; i < data_len; ++i)
		{
			n += snprintf(pStrTemp + n, 3, (bLowercase ? "%02x" : "%02X"), data[i]);
		}

		pStrTemp[n] = '\0';
		szReturnHex = pStrTemp;
		delete[] pStrTemp;
	}

	return std::move(szReturnHex);
}

/*********************************************************************
功能：	16进制字符串转char*
参数：	szHexString 待转字节串，输入
*		data 转后的缓存，输出
*		data_len 缓存大小，输入
返回：	成功转后的字节数
修改：
*********************************************************************/
int COpenssl::FromHexString(const std::string &szHexString, unsigned char* data, int data_len)
{
	if (szHexString.empty() || nullptr == data || 0 == data_len)
		return 0;

	int out_len = 0;
	char h = 0, l = 0;

	for (size_t i = 0; i < szHexString.length(); i += 2)
	{
		h = FromHexChar(szHexString[i]);
		l = FromHexChar(szHexString[i + 1]);
		data[out_len++] = (h << 4) | l;

		//防止越界
		if (out_len >= data_len)
			break;
	}

	return out_len;
}

/*********************************************************************
功能：	检查文件是否存在
参数：	szHexString 待转字节串，输入
*		data 转后的缓存，输出
*		data_len 缓存大小，输入
返回：	成功转后的字节数
修改：
*********************************************************************/
bool COpenssl::CheckFileExist(const char *pFileName)
{
	if (nullptr == pFileName)
		return false;

	int iRet = 0;

#ifdef _WIN32
	iRet = _access(pFileName, _A_NORMAL);
#else
	iRet = access(pFileName, F_OK);
#endif // WIN32

	return (0 == iRet);
}