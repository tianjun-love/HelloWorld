#include "../include/HashBase.hpp"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif // WIN32

CHashBase::CHashBase()
{
}

CHashBase::~CHashBase()
{
}

/*********************************************************************
功能：	检查文件是否存在
参数：	szFileName 文件名称
返回：	存在返回true
修改：
*********************************************************************/
bool CHashBase::CheckFileExist(const std::string &szFileName)
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
功能：	char*转16进制串，不加0x
参数：	in char*串
*		length char*串长度
*		bLowercase 是否小写字母
返回：	转后的串
修改：
*********************************************************************/
std::string CHashBase::BytesToHexString(const uint8_t *in, size_t length, bool bLowercase)
{
	std::string szReturn;

	if (nullptr == in || 0 == length)
		return std::move(szReturn);

	int n = 0;
	char *pStrTemp = new char[length * 2 + 1]{ '\0' };

	if (nullptr != pStrTemp)
	{
		for (unsigned int i = 0; i < length; ++i)
		{
			n += snprintf(pStrTemp + n, 3, (bLowercase ? "%02x" : "%02X"), in[i]);
		}

		pStrTemp[n] = '\0';
		szReturn = pStrTemp;
		delete[] pStrTemp;
	}

	return std::move(szReturn);
}