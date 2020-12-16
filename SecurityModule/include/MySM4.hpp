/********************************************************************
功能:	sm4加解密功能类
作者:	田俊
时间:	2020-11-19
修改:
*********************************************************************/
#ifndef __MY_SM4_HPP__
#define __MY_SM4_HPP__

#include "EncryptionBase.hpp"

class CSM4 : public CEncryptionBase
{
public:
	CSM4(EMODE_TYPE eMode = E_MODE_CBC);
	CSM4(const uint8_t *pKey, uint32_t iKeyLen, EMODE_TYPE eMode = E_MODE_CBC);
	CSM4(const uint8_t *pKey, uint32_t iKeyLen, const uint8_t *pIV, uint32_t iIVLen, EMODE_TYPE eMode = E_MODE_CBC);
	CSM4(const CSM4& Other) = delete;
	virtual ~CSM4();
	CSM4& operator=(const CSM4& Other) = delete;

	//设置密钥，自定义处理
	void SetKeyAndIvCustom(const uint8_t *pKey, uint32_t iKeyLen);

	//加解密实现
	bool Encrypt(const std::string &szIn, std::string &szOut, EENCODE_TYPE eEncodeType, std::string &szError) const override;
	bool Encrypt(const char *in, uint32_t inLen, uint8_t *&out, uint32_t &outLen, std::string &szError) const override;
	bool EncryptToFile(const char *in, uint32_t inLen, const std::string &szOutFileName, EENCODE_TYPE eEncodeType,
		std::string &szError) const override;
	bool Decrypt(const std::string &szIn, EENCODE_TYPE eDecodeType, std::string &szOut, std::string &szError) const override;
	bool Decrypt(const uint8_t *in, uint32_t inLen, char *&out, uint32_t &outLen, std::string &szError) const override;
	bool DecryptFromFile(const std::string &szInFileName, EENCODE_TYPE eDecodeType, char *&out, uint32_t &outLen,
		std::string &szError) const override;

private:
	//SM4相应定义
	enum ESM4_LENGTH_TYPES : uint32_t
	{
		E_KEY_LENGTH       = 16,       //SM4密钥固定128bits
		E_ROUND_KEY_LENGTH = 32,       //轮密钥长度
		E_SALT_LENGTH      = 0,        //盐值长度
		E_REPEAT_ROUND     = 1         //对同一块密钥加密轮数
	};

	//密钥上下文对象
	struct SSM4Context
	{
		bool          encrypted;              //true:加密，false:解密 
		unsigned long sk[E_ROUND_KEY_LENGTH]; //加密轮密钥
	};

	void SetKeyContext(SSM4Context &SM4Context, uint8_t* pSalt, bool bEncrypted) const; //设置密钥内容
	void SetSubkey(unsigned long *sk, uint8_t *key) const;
	unsigned long SM4CalciRK(unsigned long ka) const;
	void SM4OneRound(unsigned long *sk, const uint8_t *input, uint8_t *output) const;
	int Padding(const char *pInData, int iInDataLen , uint8_t *paddingBuff) const;
	void CBCCipher(SSM4Context &SM4Context, uint8_t *pIv,const uint8_t *pSrc, uint8_t *pDest) const;
	void ECBCipher(SSM4Context &SM4Context, const uint8_t *pSrc, uint8_t *pDest) const;

};

#endif