#include "../include/Public.hpp"
#include <ctime>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#endif

CPublic::CPublic()
{
}

CPublic::~CPublic()
{
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
std::string CPublic::DateTimeString(short nType)
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