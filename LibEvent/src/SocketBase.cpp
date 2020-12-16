#include "../include/SocketBase.hpp"
#include "event.h"
#include <cstdio>
#include <cerrno>
#include <regex>
#include <random>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif

CSocketBase::CSocketBase(const std::string &szIP, int iPort, int iTimeout) : m_bStatus(false), m_bInitWSA(false),
m_szServerIP(szIP), m_iServerPort(iPort), m_iTimeOut(iTimeout)
{
}

CSocketBase::CSocketBase(const CSocketBase &Other) : m_bStatus(Other.m_bStatus), m_bInitWSA(Other.m_bInitWSA),
m_szServerIP(Other.m_szServerIP), m_iServerPort(Other.m_iServerPort), m_iTimeOut(Other.m_iTimeOut)
{

}

CSocketBase::~CSocketBase()
{
}

CSocketBase& CSocketBase::operator=(const CSocketBase &Other)
{
	if (&Other != this)
	{
		m_bStatus = Other.m_bStatus;
		m_bInitWSA = Other.m_bInitWSA;
		m_szServerIP = Other.m_szServerIP;
		m_iServerPort = Other.m_iServerPort;
		m_iTimeOut = Other.m_iTimeOut;
	}

	return *this;
}

/*********************************************************************
功能：	获取随机数
参数：	min 最小值
*		max 最大值
返回：	随机数
修改：
*********************************************************************/
unsigned int CSocketBase::GetRandomNumber(unsigned int min, unsigned int max)
{
	unsigned int iNumber = 0;
	//std::random_device rd; //随机数生成器，种子

	//填充随机n个字节到buf中，当种子
	if (!GetRandomBytes(&iNumber, sizeof(iNumber)))
	{
		iNumber = rand();
	}

	//平均分布
	std::mt19937 eng(iNumber);

	//防止参数错误
	if (max == min)
		return min;
	else
	{
		//闭环[begin,end]
		std::uniform_int_distribution<unsigned int> dis((max > min ? min : max), (max > min ? max : min));
		return (unsigned int)dis(eng);
	}
}

/*********************************************************************
功能：	获取随机字节
参数：	buf 存放字节缓存
*		buf_len 获取的字节数
返回：	成功返回true
修改：
*********************************************************************/
bool CSocketBase::GetRandomBytes(void *buf, unsigned int buf_len)
{
	//填充随机n个字节到buf中
	evutil_secure_rng_get_bytes(buf, buf_len);

	return true;
}

/*********************************************************************
功能：	获取系统错误信息
参数：	sysErrCode 系统错误码
返回：	错误信息
修改：
*********************************************************************/
std::string CSocketBase::GetErrorMsg(int sysErrCode)
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
	return GetStrerror(sysErrCode);
#endif
}

/*********************************************************************
功能：	获取系统错误信息
参数：	sysErrCode 系统错误码
返回：	错误信息
修改：
*********************************************************************/
std::string CSocketBase::GetStrerror(int sysErrCode)
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

/*********************************************************************
功能：	获取日期时间字符串
参数：	nType 日期时间字符串类型：0：年-月-日 时:分:秒，
*								1：年-月-日，2：时:分:秒
*								3：时:分:秒.毫秒，
*								4：年-月-日 时:分:秒.毫秒
返回：	日期时间字符串
修改：
*********************************************************************/
std::string CSocketBase::DateTimeString(short nType)
{
	char buf[64] = { '\0' };
	time_t tt = time(NULL);
	struct tm tmS;
	int iMilliseconds = 0;

#ifdef _WIN32
	localtime_s(&tmS, &tt);

	if (3 == nType || 4 == nType)
	{
		SYSTEMTIME ctT;
		GetLocalTime(&ctT);
		iMilliseconds = ctT.wMilliseconds;
	}
#else
	localtime_r(&tt, &tmS);

	if (3 == nType || 4 == nType)
	{
		timeval tv;
		gettimeofday(&tv, NULL);
		iMilliseconds = (tv.tv_usec / 1000);
	}
#endif

	switch (nType)
	{
	case 1: //年-月-日
		strftime(buf, 16, "%Y-%m-%d", &tmS);
		break;
	case 2: //时:分:秒
		strftime(buf, 16, "%H:%M:%S", &tmS);
		break;
	case 3: //时:分:秒.毫秒
		snprintf(buf, 24, "%02d:%02d:%02d.%03d", tmS.tm_hour, tmS.tm_min, tmS.tm_sec, iMilliseconds);
		break;
	case 4: //年-月-日 时:分:秒.毫秒
		snprintf(buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%03d", tmS.tm_year + 1900, tmS.tm_mon + 1, tmS.tm_mday,
			tmS.tm_hour, tmS.tm_min, tmS.tm_sec, iMilliseconds);
		break;
	default: //年-月-日 时:分:秒
		strftime(buf, 40, "%Y-%m-%d %H:%M:%S", &tmS);
		break;
	}

	return std::string(buf);
}

/*********************************************************************
功能：	检查IP格式是否正确
参数：	szIP IP串
*		bIsIPv4 是否是IPV4
返回：	格式正确返回true
修改：
*********************************************************************/
bool CSocketBase::CheckIPFormat(const std::string &szIP, bool bIsIPv4)
{
	//为空不处理
	if (szIP.empty())
		return false;

	if (bIsIPv4)
	{
		try
		{
			//IP format 4字节:192.168.0.11
			std::regex patten("(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})");
			if (std::regex_match(szIP, patten))
			{
				//判断数字是否是合法
				int a = 0, b = 0, c = 0, d = 0;
				sscanf(szIP.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d);

				if (a > 255 || b > 255 || c > 255 || d > 254)
					return false;
				else
					return true;
			}
			else
				return false;
		}
		catch (...)
		{
			return false;
		}
	}
	else
	{
		//IP format 16字节，由16进制表示，前面的0可以省略，连续两个0可以用::表示，但只能使用一次，如下：
		//1、FEDC:BA98:7654:4210:FEDC:BA98:7642:3210
		//2、FF01:0:0:0:0:0:0:101 可表示为 FF01::101
		//3、0:0:0:0:0:0:0:1 可表示为 ::1
		//4、2001:0000:3238:DFE1:0063:0000:0000:FEFB 表示为 2001:0:3238:DFE1:0063::FEFB
		//5、IPv4和IPv6混和使用时，IPv4地址192.168.1.1 表示为 ::192.168.1.1


		//暂时不支持
		return false;
	}
}

/*********************************************************************
功能：	char*数据转16进制字符串
参数：	data 待转数据，输入
*		data_len 数据长度，输入
*		bLowercase 结果是否小写
返回：	16进制字符串
修改：
*********************************************************************/
std::string CSocketBase::GetHexString(const unsigned char *data, unsigned int data_len, bool bLowercase)
{
	std::string szReturnHex;

	if (nullptr == data || 0 >= data_len)
		return std::move(szReturnHex);

	int n = 0;
	char *pStrTemp = new char[data_len * 2 + 1];

	if (nullptr != pStrTemp)
	{
		for (unsigned int i = 0; i < data_len; ++i)
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
功能：	char*数据转16进制字符串
参数：	data 待转数据，输入
*		data_len 数据长度，输入
*		bLowercase 结果是否小写
返回：	16进制字符串
修改：
*********************************************************************/
std::string CSocketBase::GetHexString(const char *data, unsigned int data_len, bool bLowercase)
{
	return GetHexString((const unsigned char*)data, data_len, bLowercase);
}

/*********************************************************************
功能：	16进制字符串转char*
参数：	szHexString 待转字节串，输入
*		data 转后的缓存，输出
*		data_len 缓存大小，输入
返回：	成功转后的字节数
修改：
*********************************************************************/
unsigned int CSocketBase::FromHexString(const std::string &szHexString, char* data, unsigned int data_len)
{
	if (szHexString.empty() || nullptr == data || 0 == data_len)
		return 0;

	unsigned int out_len = 0;
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
功能：	获取16进制的字符
参数：	ch 1字节前或后4位的值
*		isLowercase 是否小写字母
返回：	转后的字符
修改：
*********************************************************************/
char CSocketBase::GetHexChar(const char &ch, bool isLowercase)
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
char CSocketBase::FromHexChar(const char &ch)
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