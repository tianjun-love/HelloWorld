#include "../include/EncryptionBase.hpp"
#include "../include/MyMD5.hpp"
#include <cstring>
#include <regex>
#include <random>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif // WIN32

CEncryptionBase::CEncryptionBase(EMODE_TYPE eMode, EKEY_TYPE eKeyType) : m_eMode(eMode), m_eKeyType(eKeyType)
{
	ClearKey();
}

CEncryptionBase::~CEncryptionBase()
{
	ClearKey();
}

/*********************************************************************
功能：	设置密钥
参数：	pKey 密钥
*		iKeyLen 密钥长度
*		bConvertMD5 是否转MD5，一但转MD5，密钥长度就是128
返回：	无
修改：
*********************************************************************/
void CEncryptionBase::SetKey(const uint8_t *pKey, uint32_t iKeyLen, bool bConvertMD5)
{
	if (nullptr == pKey || 0 == iKeyLen)
	{
		memset(m_KeyArray, 0, E_KEY_MAX_LENGTH);
		return;
	}

	if (bConvertMD5)
	{
		m_eKeyType = E_KEY_128;
		CMD5::StringToMD5((const char*)pKey, iKeyLen, m_KeyArray, E_KEY_MAX_LENGTH);
	}
	else
	{
		for (uint32_t i = 0; i < iKeyLen && i < E_KEY_MAX_LENGTH; ++i)
		{
			m_KeyArray[i] = pKey[i];
		}
	}
}

/*********************************************************************
功能：	设置IV
参数：	pIV IV值
*		iIVLen IV长度
*		bConvertMD5 是否转MD5
返回：	无
修改：
*********************************************************************/
void CEncryptionBase::SetIV(const uint8_t *pIV, uint32_t iIVLen, bool bConvertMD5)
{
	if (nullptr == pIV || 0 == iIVLen)
	{
		memset(m_IVArray, 0, E_IV_LENGTH);
		return;
	}

	if (bConvertMD5)
	{
		CMD5::StringToMD5((const char*)pIV, iIVLen, m_IVArray, E_IV_LENGTH);
	}
	else
	{
		for (uint32_t i = 0; i < iIVLen && i < E_IV_LENGTH; ++i)
		{
			m_IVArray[i] = pIV[i];
		}
	}
}

/*********************************************************************
功能：	密钥信息清零
参数：	无
返回：	无
修改：
*********************************************************************/
void CEncryptionBase::ClearKey()
{
	memset(m_KeyArray, 0, E_KEY_MAX_LENGTH);
	memset(m_IVArray, 0, E_IV_LENGTH);
}

/*********************************************************************
功能：	获取随机字节
参数：	iByteCounts 获取数量
*		pByteBuff 存放buff
返回：	无
修改：
*********************************************************************/
void CEncryptionBase::GetRandBytes(uint32_t iByteCounts, uint8_t *pByteBuff)
{
	std::random_device rd;
	std::uniform_int_distribution<int> uni_dist(0, 255);

	for (uint32_t i = 0; i < iByteCounts; i++)
	{
		pByteBuff[i] = uni_dist(rd);
	}
}

/*********************************************************************
功能：	检查文件是否存在
参数：	szFileName 文件名称
返回：	存在返回true
修改：
*********************************************************************/
bool CEncryptionBase::CheckFileExist(const std::string &szFileName)
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

/*********************************************************************
功能：	检查加密字符串格式
参数：	szEncryptedData 加密字符串
*		eCodeType 编码类型
*		szError 错误信息
返回：	匹配返回true
修改：
*********************************************************************/
bool CEncryptionBase::CheckEncryptedData(const std::string &szEncryptedData, EENCODE_TYPE eCodeType, std::string &szError)
{
	if (szEncryptedData.empty())
	{
		szError = "加密字符串为空！";
		return false;
	}

	bool bRet = true;

	switch (eCodeType)
	{
	case CEncryptionBase::E_CODE_NONE:
	{
		//长度必须是16的整倍数
		if (szEncryptedData.length() % 16 != 0)
		{
			bRet = false;
			szError = "加密字节串长度错误，必须是16的整倍数！";
		}
	}
		break;
	case CEncryptionBase::E_CODE_HEX:
	{
		//长度必须是2的整倍数
		if (szEncryptedData.length() % 2 != 0)
		{
			bRet = false;
			szError = "HEX串长度错误，必须是2的整倍数！";
		}
		else
		{
			//检查字符
			const std::regex pattern1("^[0-9a-f]+$"), pattern2("^[0-9A-F]+$");

			if (!std::regex_match(szEncryptedData, pattern1) && !std::regex_match(szEncryptedData, pattern2))
			{
				bRet = false;
				szError = "不能包含非HEX字符！";
			}
		}
	}
		break;
	case CEncryptionBase::E_CODE_BASE64:
	{
		//长度必须是4的整倍数
		if (szEncryptedData.length() % 4 != 0)
		{
			bRet = false;
			szError = "Base64串长度错误，必须是4的整倍数！";
		}
		else
		{
			//检查字符
			const std::regex pattern("^[0-9a-zA-Z+/=]+$");

			if (!std::regex_match(szEncryptedData, pattern))
			{
				bRet = false;
				szError = "不能包含非Base64字符！";
			}
		}
	}
		break;
	default:
		bRet = false;
		szError = "未知编码类型：" + std::to_string(eCodeType);
		break;
	}

	return bRet;
}

/*********************************************************************
功能：	从文件读取数据
参数：	szFileName 文件名称
*		data 数据缓存，输出，调用者释放
*		dataLen 数据大小，输出
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CEncryptionBase::ReadDataFromFile(const std::string &szFileName, char *&data, int64_t &dataLen, std::string &szError)
{
	//检查文件名称
	if (szFileName.empty())
	{
		szError = "文件名称为空！";
		return false;
	}

	if (!CheckFileExist(szFileName))
	{
		szError = "文件：" + szFileName + " 不存在！";
		return false;
	}

	//清空输出变量
	if (nullptr != data)
	{
		delete[] data;
		data = nullptr;
	}

	dataLen = 0;

	bool bRet = true;
	std::ifstream ifile;

	//打开文件
	ifile.open(szFileName, std::ios::in | std::ios::binary);
	if (ifile.is_open() && ifile.good())
	{
		std::streampos fileSize = 0;

		//获取文件大小
		ifile.seekg(0, std::ios::end);
		fileSize = ifile.tellg();
		ifile.seekg(0, std::ios::beg);

		//判断大小
		if (fileSize >= 1)
		{
			if (fileSize <= E_MAX_LENGTH)
			{
				dataLen = fileSize;
				data = new char[dataLen + 1]{ '\0' };

				if (nullptr != data)
				{
					//读取文件
					ifile.read(data, dataLen);
					const std::streamsize readSize = ifile.gcount();

					if (readSize != dataLen)
					{
						bRet = false;
						szError = "读取文件内容失败，总大小：" + std::to_string(dataLen) + "，读取大小：" + std::to_string(readSize);
						dataLen = 0;
						delete[] data;
						data = nullptr;
					}
				}
				else
				{
					dataLen = 0;
					bRet = false;
					szError = "申请输出内存缓存失败：" + GetStrerror(errno);
				}
			}
			else
			{
				bRet = false;
				szError = "暂不支持大于" + std::to_string(E_MAX_LENGTH / 1024) + "KB的文件直接读取！";
			}
		}

		//关闭文件
		ifile.close();
	}
	else
	{
		bRet = false;
		szError = "打开文件：" + szFileName + " 失败：" + GetStrerror(errno);
	}

	return bRet;
}

/*********************************************************************
功能：	将数据写入文件
参数：	szFileName 文件名称
*		data 数据缓存，输入
*		dataLen 数据大小，输入
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CEncryptionBase::WriteDataToFile(const std::string &szFileName, const char *data, int64_t dataLen, std::string &szError)
{
	//检查文件名称
	if (szFileName.empty())
	{
		szError = "文件名称为空！";
		return false;
	}

	bool bRet = true;
	std::ofstream ofile;

	//打开文件
	ofile.open(szFileName, std::ios::out | std::ios::binary | std::ios::trunc);
	if (ofile.is_open() && ofile.good())
	{
		//写入文件
		if (nullptr != data && dataLen > 0)
		{
			ofile.write(data, dataLen);
		}

		//关闭文件
		ofile.close();
	}
	else
	{
		bRet = false;
		szError = "打开文件：" + szFileName + " 失败：" + GetStrerror(errno);
	}

	return bRet;
}

/*********************************************************************
功能：	获取系统错误信息
参数：	sysErrCode 系统错误码
返回：	错误信息
修改：
*********************************************************************/
std::string CEncryptionBase::GetStrerror(int sysErrCode)
{
	std::string szRet;
	char buf[2048] = { '\0' };

#ifdef _WIN32
	if (0 == strerror_s(buf, 2047, sysErrCode))
		szRet = buf;
	else
		szRet = "strerror_s work wrong, source error code:" + std::to_string(sysErrCode);
#else
	if (nullptr != strerror_r(sysErrCode, buf, 2047))
		szRet = buf;
	else
		szRet = "strerror_r work wrong, source error code:" + std::to_string(sysErrCode);
#endif

	return std::move(szRet);
}