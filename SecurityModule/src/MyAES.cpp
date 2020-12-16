#include "../include/MyAES.hpp"
#include "../include/CodeConvert.hpp"

#include <cstring>
#include <openssl/aes.h>

CAES::CAES(EMODE_TYPE eMode, EKEY_TYPE eKeyType) : CEncryptionBase(eMode, eKeyType)
{
}

CAES::CAES(const uint8_t *pKey, uint32_t iKeyLen, EMODE_TYPE eMode, EKEY_TYPE eKeyType) : CEncryptionBase(eMode, eKeyType)
{
	SetKey(pKey, iKeyLen);
}

CAES::CAES(const uint8_t *pKey, uint32_t iKeyLen, const uint8_t *pIV, uint32_t iIVLen, EMODE_TYPE eMode, EKEY_TYPE eKeyType) :
	CEncryptionBase(eMode, eKeyType)
{
	SetKey(pKey, iKeyLen);
	SetIV(pIV, iIVLen);
}

CAES::~CAES()
{
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
bool CAES::Encrypt(const std::string &szIn, std::string &szOut, EENCODE_TYPE eEncodeType, std::string &szError) const
{
	uint32_t outLen = 0;
	uint8_t *out = nullptr;

	if (szIn.empty())
	{
		szError = "待加密字符串为空！";
		return false;
	}

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
bool CAES::Encrypt(const char *in, uint32_t inLen, uint8_t *&out, uint32_t &outLen, std::string &szError) const
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

	char *pTemp = nullptr;
	uint32_t iTempLen = E_BLOCK_SIZE;

	//计算填充
	if (inLen > E_BLOCK_SIZE)
	{
		if (inLen % E_BLOCK_SIZE == 0)
			iTempLen = inLen;
		else
			iTempLen = (inLen / E_BLOCK_SIZE + 1) * E_BLOCK_SIZE;
	}

	bool bRet = true;
	pTemp = new char[iTempLen + 1]{ '\0' };

	if (nullptr == pTemp)
	{
		szError = "申请内存缓存失败！";
		return false;
	}
	else
	{
		//后面填充0
		memcpy(pTemp, in, inLen);

		outLen = iTempLen;
		out = new unsigned char[outLen + 1]{ '\0' };

		if (nullptr == out)
		{
			outLen = 0;
			szError = "申请输出内存缓存失败！";
			delete[] pTemp;

			return false;
		}
	}

	AES_KEY aes;

	//设置密钥
	if (AES_set_encrypt_key(m_KeyArray, m_eKeyType, &aes) < 0)
	{
		bRet = false;
		szError = "设置密钥失败！";
	}
	else
	{
		//加密
		if (E_MODE_CBC == m_eMode)
		{
			uint8_t iv[E_IV_LENGTH];

			memcpy(iv, m_IVArray, E_IV_LENGTH);
			AES_cbc_encrypt((const uint8_t*)pTemp, out, iTempLen, &aes, iv, AES_ENCRYPT);
		}
		else
		{
			uint32_t counts = iTempLen / E_BLOCK_SIZE;

			for (uint32_t i = 0; i < counts; ++i)
			{
				AES_ecb_encrypt((const uint8_t*)pTemp + i * E_BLOCK_SIZE, out + i * E_BLOCK_SIZE, &aes, AES_ENCRYPT);
			}
		}
	}

	//清除缓存
	delete[] pTemp;

	if (!bRet)
	{
		outLen = 0;
		delete[] out;
		out = nullptr;
	}

	return bRet;
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
bool CAES::EncryptToFile(const char *in, uint32_t inLen, const std::string &szOutFileName, EENCODE_TYPE eEncodeType, 
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
参数：	szIn 待解密编码串
*		eDecodeType 待解密串解码类型
*		szOut 解密串
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CAES::Decrypt(const std::string &szIn, EENCODE_TYPE eDecodeType, std::string &szOut, std::string &szError) const
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

	//释放内存
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
bool CAES::Decrypt(const uint8_t *in, uint32_t inLen, char *&out, uint32_t &outLen, std::string &szError) const
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

	outLen = inLen;
	out = new char[outLen + 1]{ '\0' };

	if (nullptr == out)
	{
		outLen = 0;
		szError = "申请输出内存缓存失败！";

		return false;
	}

	bool bRet = true;
	AES_KEY aes;

	//设置密钥
	if (AES_set_decrypt_key(m_KeyArray, m_eKeyType, &aes) < 0)
	{
		bRet = false;
		szError = "设置密钥失败！";
	}
	else
	{
		//解密
		if (E_MODE_CBC == m_eMode)
		{
			uint8_t iv[E_IV_LENGTH];

			memcpy(iv, m_IVArray, E_IV_LENGTH);
			AES_cbc_encrypt(in, (unsigned char*)out, inLen, &aes, iv, AES_DECRYPT);
		}
		else
		{
			uint32_t counts = inLen / E_BLOCK_SIZE;

			for (uint32_t i = 0; i < counts; ++i)
			{
				AES_ecb_encrypt(in + i * E_BLOCK_SIZE, (unsigned char*)out + i * E_BLOCK_SIZE, &aes, AES_DECRYPT);
			}
		}
	}

	if (!bRet)
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
bool CAES::DecryptFromFile(const std::string &szInFileName, EENCODE_TYPE eDecodeType, char *&out, uint32_t &outLen, 
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