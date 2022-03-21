#include "../include/BaseAttributes.hpp"
#include <cstring>
#include <ctime>
#include <cstdarg>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#endif // _WIN32

CBaseAttributes::CBaseAttributes(const std::string &szServerIP, unsigned short nServerPort) : m_szServerIP(szServerIP),
m_nServerPort(nServerPort), m_bInitWSA(false), m_bState(false)
{
}

CBaseAttributes::~CBaseAttributes()
{
}

/*****************************************************
功能：	输出日志
参数：	level 日志级别
*		fmt 格式
*		... 参数
返回：	无
*****************************************************/
void CBaseAttributes::PrintfLog(int level, const char *fmt, ...)
{
	char buff[102400] = { '\0' };
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	printf("%s >> [%d] %s\n", GetTimeString(E_TIME_MILLISECOND).c_str(), level, buff);

	return;
}

/*****************************************************
功能：	设置socket句柄阻塞状态
参数：	socket socket句柄
*		block true:阻塞，false:不阻塞
返回：	无
*****************************************************/
void CBaseAttributes::SetSocketBolckState(MY_SOCKET_TYPE socket, bool block)
{
	unsigned long ulBlock = 0; //设置是否阻塞，默认阻塞，0：阻塞，1：非阻塞

	if (!block)
		ulBlock = 1;

#ifdef _WIN32
	ioctlsocket(socket, FIONBIO, &ulBlock);
#else
	ioctl(socket, FIONBIO, &ulBlock);
#endif
}

/*****************************************************
功能：	关闭socket句柄
参数：	socket socket句柄
返回：	无
*****************************************************/
void CBaseAttributes::CloseSocket(MY_SOCKET_TYPE socket)
{
#ifdef _WIN32
	closesocket(socket);
#else
	close(socket);
#endif

	return;
}

/*****************************************************
功能：	获取日期时间字符串
参数：	eType 日期时间字符串类型
返回：	日期时间字符串
修改：
*****************************************************/
std::string CBaseAttributes::GetTimeString(ETimeStringType eType)
{
	char buf[64] = { '\0' };
	time_t tt = time(NULL);
	struct tm tmS;
	int iMilliseconds = 0;

#ifdef _WIN32
	localtime_s(&tmS, &tt);

	if (E_TIME_MILLISECOND == eType || E_DATE_TIME_MILLISECOND == eType)
	{
		SYSTEMTIME ctT;
		GetLocalTime(&ctT);
		iMilliseconds = ctT.wMilliseconds;
	}
#else
	localtime_r(&tt, &tmS);

	if (E_TIME_MILLISECOND == eType || E_DATE_TIME_MILLISECOND == eType)
	{
		timeval tv;
		gettimeofday(&tv, NULL);
		iMilliseconds = (tv.tv_usec / 1000);
	}
#endif

	switch (eType)
	{
	case E_DATE: //年-月-日
		strftime(buf, 16, "%Y-%m-%d", &tmS);
		break;
	case E_TIME: //时:分:秒
		strftime(buf, 16, "%H:%M:%S", &tmS);
		break;
	case E_TIME_MILLISECOND: //时:分:秒.毫秒
		snprintf(buf, 24, "%02d:%02d:%02d.%03d", tmS.tm_hour, tmS.tm_min, tmS.tm_sec, iMilliseconds);
		break;
	case E_DATE_TIME_MILLISECOND: //年-月-日 时:分:秒.毫秒
		snprintf(buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%03d", tmS.tm_year + 1900, tmS.tm_mon + 1, tmS.tm_mday,
			tmS.tm_hour, tmS.tm_min, tmS.tm_sec, iMilliseconds);
		break;
	default: //年-月-日 时:分:秒
		strftime(buf, 40, "%Y-%m-%d %H:%M:%S", &tmS);
		break;
	}

	return std::string(buf);
}

/*****************************************************
功能：	获取系统错误信息
参数：	sysErrCode 系统错误码
返回：	错误信息
修改：
*****************************************************/
std::string CBaseAttributes::GetErrorMsg(int errCode)
{
	std::string szError;

#ifdef _WIN32
	LPVOID lpMsgBuf = NULL;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errCode,
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
			+ std::to_string(errCode);
		return std::move(szError);
	}
#else
	char buff[2048] = { '\0' };

	char *pError = strerror_r(errCode, buff, sizeof(buff));
	if (nullptr != pError)
		szError = pError;
	else
		szError = "strerror_r work wrong, source error code:" + std::to_string(errCode);

#endif

	return std::move(szError);
}