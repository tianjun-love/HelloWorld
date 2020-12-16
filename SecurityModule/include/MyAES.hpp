/********************************************************************
功能:	aes加解密功能类
作者:	田俊
时间:	2020-11-18
修改:
*********************************************************************/
#ifndef __MY_AES_HPP__
#define __MY_AES_HPP__

#include "EncryptionBase.hpp"

class CAES : public CEncryptionBase
{
public:
	CAES(EMODE_TYPE eMode = E_MODE_CBC, EKEY_TYPE eKeyType = E_KEY_128);
	CAES(const uint8_t *pKey, uint32_t iKeyLen, EMODE_TYPE eMode = E_MODE_CBC, EKEY_TYPE eKeyType = E_KEY_128);
	CAES(const uint8_t *pKey, uint32_t iKeyLen, const uint8_t *pIV, uint32_t iIVLen, EMODE_TYPE eMode = E_MODE_CBC,
		EKEY_TYPE eKeyType = E_KEY_128);
	CAES(const CAES &Other) = delete;
	virtual ~CAES();
	CAES& operator=(const CAES &Other) = delete;

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

};

#endif
