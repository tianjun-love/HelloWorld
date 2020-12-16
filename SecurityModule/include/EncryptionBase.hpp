/********************************************************************
功能:	自定义加解密基类
作者:	田俊
时间:	2020-11-19
修改:
*********************************************************************/
#ifndef __ENCRYPTION_BASE_HPP__
#define __ENCRYPTION_BASE_HPP__

#include <string>

class CEncryptionBase
{
public:
	//加解密模式
	enum EMODE_TYPE : uint8_t
	{
		E_MODE_CBC = 1, //CBC模式
		E_MODE_ECB = 2  //ECB模式
	};

	//密钥长度
	enum EKEY_TYPE : uint16_t
	{
		E_KEY_128 = 128, //128 bits
		E_KEY_192 = 192, //192 bits
		E_KEY_256 = 256  //256 bits
	};

	//编码类型
	enum EENCODE_TYPE : uint8_t
	{
		E_CODE_NONE,  //原始串
		E_CODE_HEX,   //HEX
		E_CODE_BASE64 //Base64
	};

protected:
	//其它长度定义
	enum ELENGTH_TYPE : int32_t
	{
		E_BLOCK_SIZE       = 16,       //处理块大小
		E_KEY_MAX_LENGTH   = 32,       //密钥最大32字节
		E_IV_LENGTH        = 16,       //IV固定16字节
		E_MAX_LENGTH       = 10485760  //最大处理数据长度，10M
	};

public:
	CEncryptionBase(EMODE_TYPE eMode, EKEY_TYPE eKeyType);
	CEncryptionBase(const CEncryptionBase &Other) = delete;
	virtual ~CEncryptionBase();
	CEncryptionBase& operator=(const CEncryptionBase &Other) = delete;

	//加解密
	virtual bool Encrypt(const std::string &szIn, std::string &szOut, EENCODE_TYPE eEncodeType, std::string &szError) const = 0;
	virtual bool Encrypt(const char *in, uint32_t inLen, uint8_t *&out, uint32_t &outLen, std::string &szError) const = 0;
	virtual bool EncryptToFile(const char *in, uint32_t inLen, const std::string &szOutFileName, EENCODE_TYPE eEncodeType, 
		std::string &szError) const = 0;
	virtual bool Decrypt(const std::string &szIn, EENCODE_TYPE eDecodeType, std::string &szOut, std::string &szError) const = 0;
	virtual bool Decrypt(const uint8_t *in, uint32_t inLen, char *&out, uint32_t &outLen, std::string &szError) const = 0;
	virtual bool DecryptFromFile(const std::string &szInFileName, EENCODE_TYPE eDecodeType, char *&out, uint32_t &outLen,
		std::string &szError) const = 0;

	//设置密钥信息
	void SetKey(const uint8_t *pKey, uint32_t iKeyLen, bool bConvertMD5 = true);
	void SetIV(const uint8_t *pIV, uint32_t iIVLen, bool bConvertMD5 = true);
	void ClearKey();

	//获取随机字节
	static void GetRandBytes(uint32_t iByteCounts, uint8_t *pByteBuff);

	//检查文件是否存在
	static bool CheckFileExist(const std::string &szFileName);

	//检查加密字符串格式
	static bool CheckEncryptedData(const std::string &szEncryptedData, EENCODE_TYPE eCodeType, std::string &szError);

	//读取文件数据
	static bool ReadDataFromFile(const std::string &szFileName, char *&data, int64_t &dataLen, std::string &szError);

	//将数据写入文件
	static bool WriteDataToFile(const std::string &szFileName, const char *data, int64_t dataLen, std::string &szError);

	//获取系统错误信息
	static std::string GetStrerror(int sysErrCode);

protected:
	EMODE_TYPE m_eMode;                      //加解密模式
	EKEY_TYPE  m_eKeyType;                   //密钥长度类型，bits
	uint8_t    m_KeyArray[E_KEY_MAX_LENGTH]; //密钥
	uint8_t    m_IVArray[E_IV_LENGTH];       //IV

};

#endif
