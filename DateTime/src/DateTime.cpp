#include "../include/DateTime.hpp"
#include <sstream>
#include <chrono>
#include <cmath>

#ifdef _WIN32
	#include <Windows.h>
#else
	#include <sys/time.h>
#endif

#ifdef _WIN32
	CDateTime::Snprintf CDateTime::m_snprintf = _snprintf;
#else
	CDateTime::Snprintf CDateTime::m_snprintf = snprintf;
#endif

CDateTime::CDateTime() : m_lSeconds(1329265800) //fisrt day to work, 2012-02-15 08:30:00
{}

CDateTime::CDateTime(unsigned int iYear, unsigned int iMon, unsigned int iDay, unsigned int iHour, unsigned int iMin, unsigned int iSec)
{
	tm DateTime;

	DateTime.tm_year = iYear - 1900;
	DateTime.tm_mon = iMon - 1;
	DateTime.tm_mday = iDay;
	DateTime.tm_hour = iHour;
	DateTime.tm_min = iMin;
	DateTime.tm_sec = iSec;
	DateTime.tm_wday = 0;
	DateTime.tm_yday = 0;
	DateTime.tm_isdst = 0;

	m_lSeconds = mktime(&DateTime);
}

CDateTime::CDateTime(const CDateTime& Other) : m_lSeconds(Other.m_lSeconds)
{}

CDateTime::CDateTime(time_t lSeconds)
{
	if (lSeconds <= 1329265800)
	{
		time(&m_lSeconds);
	}
	else
	{
		m_lSeconds = lSeconds;
	}
}

CDateTime::CDateTime(const std::string& szDateTime)
{
	CopyFromDateTimeStr(m_lSeconds, szDateTime);
}

CDateTime::~CDateTime()
{}

clock_t CDateTime::GetRunMs()
{
	return clock();
}

time_t CDateTime::GetSeconds()
{
	return time(0);
}

long long CDateTime::GetMilliSeconds()
{
	auto t = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();
}

long long CDateTime::GetMicroSeconds()
{
	auto t = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count();
}

bool CDateTime::CheckIsLeapYear(unsigned int iYear)
{
	if (0 == iYear)
	{
		return false; //年份必须大于0
	}

	if (0 == (iYear%400))
	{
		return true; //能被400整除，是闰年
	}
	else
	{
		if (0 == (iYear % 4) && 0 != (iYear % 100))
		{
			return true; //能被4整除但不能被100整除，是闰年
		}
		else
		{
			return false; //不是闰年
		}
	}
}

void CDateTime::InitCurrDateTime(time_t lSecond)
{
	if (lSecond <= 1329265800)
		time(&m_lSeconds);
	else
		m_lSeconds = lSecond;
}

time_t CDateTime::GetDateTimeSeconds() const
{
	return m_lSeconds;
}

void CDateTime::SetDateTimeSeconds(time_t lSecond)
{
	if (lSecond <= 1329265800)
		m_lSeconds = 1329265800;
	else
		m_lSeconds = lSecond;
}

clock_t CDateTime::GetDateTimePart(const std::string& szPart, clock_t iAdd) const
{
	clock_t iPart = -1;
	time_t tTemp;
	struct tm tmTimeTemp;

	switch (GetEnum(szPart))
	{
	case e_YEAR:
	{
		tTemp = m_lSeconds;
		LocalTime(tTemp, tmTimeTemp);
		iPart = tmTimeTemp.tm_year + 1900 + iAdd;
	}
		break;
	case e_MON:
	{
		tTemp = m_lSeconds;
		LocalTime(tTemp, tmTimeTemp);
		iPart = std::abs((tmTimeTemp.tm_mon + iAdd) % 12) + 1;
	}
		break;
	case e_DAY:
	{
		tTemp = m_lSeconds + iAdd * 24 * 60 * 60;
		LocalTime(tTemp, tmTimeTemp);
		iPart = tmTimeTemp.tm_mday;
	}
		break;
	case e_HOUR:
	{
		tTemp = m_lSeconds + iAdd * 60 * 60;
		LocalTime(tTemp, tmTimeTemp);
		iPart = tmTimeTemp.tm_hour;
	}
		break;
	case e_MIN:
	{
		tTemp = m_lSeconds + iAdd * 60;
		LocalTime(tTemp, tmTimeTemp);
		iPart = tmTimeTemp.tm_min;
	}
		break;
	case e_SEC:
	{
		tTemp = m_lSeconds + iAdd;
		LocalTime(tTemp, tmTimeTemp);
		iPart = tmTimeTemp.tm_sec;
	}
		break;
	case e_WEEK:
	{
		tTemp = m_lSeconds;
		LocalTime(tTemp, tmTimeTemp);
		iPart = tmTimeTemp.tm_wday;
	}
		break;
	case e_NONE:
		break;
	default:
		break;
	}

	return iPart;
}

std::string CDateTime::GetDateTimeStr(bool bNeedMilliseconds) const
{
	char strDateTime[32] = { '\0' };
	struct tm tmTimeTemp;
	size_t strLen = 0;

	LocalTime(m_lSeconds, tmTimeTemp);
	strLen = strftime(strDateTime, 24, "%Y-%m-%d %X", &tmTimeTemp);

	//添加毫秒数
	if (bNeedMilliseconds)
		m_snprintf(strDateTime + strLen, 5, ".%3d", GetCurrentMillisecond());

	return std::move(std::string(strDateTime));
}

std::string CDateTime::GetDateStr(const std::string& szDecollator)  const
{
	char strDate[16] = { '\0' };
	struct tm tmTimeTemp;

	LocalTime(m_lSeconds, tmTimeTemp);
	m_snprintf(strDate, 16, "%d%s%02d%s%02d", tmTimeTemp.tm_year + 1900, szDecollator.c_str(), tmTimeTemp.tm_mon + 1,
		szDecollator.c_str(), tmTimeTemp.tm_mday);

	return std::move(std::string(strDate));
}

std::string CDateTime::GetDateMonStr(const std::string& szDecollator)  const
{
	char strDate[16] = { '\0' };
	struct tm tmTimeTemp;

	LocalTime(m_lSeconds, tmTimeTemp);
	m_snprintf(strDate, 16, "%d%s%02d", tmTimeTemp.tm_year + 1900, szDecollator.c_str(), tmTimeTemp.tm_mon + 1);

	return std::move(std::string(strDate));
}

std::string CDateTime::GetDateHourStr(const std::string& szDecollator)  const
{
	char strDate[32] = { '\0' };
	struct tm tmTimeTemp;

	LocalTime(m_lSeconds, tmTimeTemp);
	m_snprintf(strDate, 32, "%d%s%02d%s%02d%s%02d", tmTimeTemp.tm_year + 1900, szDecollator.c_str(), tmTimeTemp.tm_mon + 1,
		szDecollator.c_str(), tmTimeTemp.tm_mday, szDecollator.c_str(), tmTimeTemp.tm_hour);

	return std::move(std::string(strDate));
}

std::string CDateTime::GetDateMinStr(const std::string& szDecollator)  const
{
	char strDate[32] = { '\0' };
	struct tm tmTimeTemp;

	LocalTime(m_lSeconds, tmTimeTemp);
	m_snprintf(strDate, 32, "%d%s%02d%s%02d%s%02d%s%02d", tmTimeTemp.tm_year + 1900, szDecollator.c_str(),
		tmTimeTemp.tm_mon + 1, szDecollator.c_str(), tmTimeTemp.tm_mday, szDecollator.c_str(),
		tmTimeTemp.tm_hour, szDecollator.c_str(), tmTimeTemp.tm_min);

	return std::move(std::string(strDate));
}

std::string CDateTime::GetTimeStr(const std::string& szDecollator, bool bNeedMilliseconds)  const
{
	char strTime[24] = { '\0' };
	struct tm tmTimeTemp;

	LocalTime(m_lSeconds, tmTimeTemp);

	if (bNeedMilliseconds)
		m_snprintf(strTime, 24, "%02d%s%02d%s%02d.%3d", tmTimeTemp.tm_hour, szDecollator.c_str(), tmTimeTemp.tm_min, szDecollator.c_str(),
			tmTimeTemp.tm_sec, GetCurrentMillisecond());
	else
		m_snprintf(strTime, 24, "%02d%s%02d%s%02d", tmTimeTemp.tm_hour, szDecollator.c_str(), tmTimeTemp.tm_min, szDecollator.c_str(),
			tmTimeTemp.tm_sec);

	return std::move(std::string(strTime));
}

CDateTime::ETIMEPART CDateTime::GetEnum(const std::string& szPart)
{
	if ("y" == szPart || "Y" == szPart)
	{
		return e_YEAR;
	}
	else if ("m" == szPart || "M" == szPart)
	{
		return e_MON;
	}
	else if ("d" == szPart || "D" == szPart)
	{
		return e_DAY;
	}
	else if ("h" == szPart || "H" == szPart)
	{
		return e_HOUR;
	}
	else if ("m" == szPart || "M" == szPart)
	{
		return e_MIN;
	}
	else if ("s" == szPart || "S" == szPart)
	{
		return e_SEC;
	}
	else if ("w" == szPart || "W" == szPart)
	{
		return e_WEEK;
	}
	else
	{
		return e_NONE;
	}
}

void CDateTime::CopyFromDateTimeStr(time_t& tSeconds, const std::string& szDateTime)
{
	//最少19个字节，"2019-03-23 23:44:22"
	if (szDateTime.length() < 19)
		return;

	struct tm DateTime;

	char* pszStr = (char*)szDateTime.c_str();
	DateTime.tm_year = std::atoi(pszStr);
	DateTime.tm_mon = std::atoi(pszStr + 5);
	DateTime.tm_mday = std::atoi(pszStr + 8);
	DateTime.tm_hour = std::atoi(pszStr + 11);
	DateTime.tm_min = std::atoi(pszStr + 14);
	DateTime.tm_sec = std::atoi(pszStr + 17);

	DateTime.tm_year -= 1900;
	DateTime.tm_mon -= 1;
	DateTime.tm_wday = 0;
	DateTime.tm_yday = 0;
	DateTime.tm_isdst = 0;

	tSeconds = mktime(&DateTime);
}

void CDateTime::LocalTime(const time_t& tSeconds, tm& tmTemp)
{
	//localtime函数不可重入，即不是线程安全的，所以用下面的版本
#ifdef _WIN32
	localtime_s(&tmTemp, &tSeconds);
#else
	localtime_r(&tSeconds, &tmTemp);
#endif
}

int CDateTime::GetCurrentMillisecond()
{
	//获取当前时间的毫秒数
#ifdef _WIN32
		SYSTEMTIME ctT;
		GetLocalTime(&ctT);
		return ctT.wMilliseconds;
#else
		timeval tv;
		gettimeofday(&tv, NULL);
		return (tv.tv_usec / 1000);
#endif
}

CDateTime& CDateTime::operator =(const CDateTime& Other)
{
	m_lSeconds = Other.m_lSeconds;
	return *this;
}

CDateTime& CDateTime::operator =(time_t lSeconds)
{
	m_lSeconds = lSeconds;
	return *this;
}

CDateTime& CDateTime::operator =(const std::string& szDateTime)
{
	CopyFromDateTimeStr(m_lSeconds, szDateTime);
	return *this;
}

bool CDateTime::operator ==(const CDateTime& Other) const
{
	return m_lSeconds == Other.m_lSeconds;
}

bool CDateTime::operator !=(const CDateTime& Other) const
{
	return m_lSeconds != Other.m_lSeconds;
}

bool CDateTime::operator <(const CDateTime& Other) const
{
	return m_lSeconds < Other.m_lSeconds;
}

bool CDateTime::operator >(const CDateTime& Other) const
{
	return m_lSeconds > Other.m_lSeconds;
}

bool CDateTime::operator <=(const CDateTime& Other) const
{
	return m_lSeconds <= Other.m_lSeconds;
}

bool CDateTime::operator >=(const CDateTime& Other) const
{
	return m_lSeconds >= Other.m_lSeconds;
}

time_t CDateTime::operator -(const CDateTime& Other) const
{
	return (m_lSeconds - Other.m_lSeconds);
}

CDateTime::operator std::string() const
{
	return GetDateTimeStr();
}

std::ostream& operator <<(std::ostream& out, const CDateTime& Other)
{
	out << (std::string)Other;
	return out;
}

void CDateTime::AddYear(int iYear)
{
	struct tm tmTimeTemp;

	LocalTime(m_lSeconds, tmTimeTemp);
	tmTimeTemp.tm_year += iYear;
	if (tmTimeTemp.tm_year > 0)
	{
		if (1 == tmTimeTemp.tm_mon && 29 == tmTimeTemp.tm_mday) //2月
		{
			if (!CheckIsLeapYear(tmTimeTemp.tm_year + 1900))
			{
				tmTimeTemp.tm_mday = 28;
			}
		}

		m_lSeconds = mktime(&tmTimeTemp);
	}
}

void CDateTime::AddMonth(int iMonth)
{
	struct tm tmTimeTemp;

	LocalTime(m_lSeconds, tmTimeTemp);
	tmTimeTemp.tm_mon = abs((tmTimeTemp.tm_mon + iMonth) % 12);
	if (1 == tmTimeTemp.tm_mon)
	{
		//2月
		if (CheckIsLeapYear(tmTimeTemp.tm_year + 1900))
		{
			if (29 < tmTimeTemp.tm_mday)
				tmTimeTemp.tm_mday = 29;
		}
		else
		{
			if (29 <= tmTimeTemp.tm_mday)
				tmTimeTemp.tm_mday = 28;
		}
	}
	else if (3 == tmTimeTemp.tm_mon || 5 == tmTimeTemp.tm_mon || 8 == tmTimeTemp.tm_mon || 10 == tmTimeTemp.tm_mon)
	{
		//4、6、9、11月只有30天
		if (31 == tmTimeTemp.tm_mday)
		{
			tmTimeTemp.tm_mday = 30;
		}
	}

	m_lSeconds = mktime(&tmTimeTemp);
}

void CDateTime::AddDay(int iDay)
{
	m_lSeconds += iDay * 24 * 60 * 60;
}

void CDateTime::AddHour(int iHour)
{
	m_lSeconds += iHour * 60 * 60;
}

void CDateTime::AddMinute(long iMin)
{
	m_lSeconds += iMin * 60;
}

void CDateTime::AddSecond(long long iSecond)
{
	m_lSeconds += iSecond;
}

int CDateTime::GetYear() const
{
	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	return tmTimeTemp.tm_year + 1900;
}

int CDateTime::GetMonth() const
{
	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	return tmTimeTemp.tm_mon + 1;
}

int CDateTime::GetDay() const
{
	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	return tmTimeTemp.tm_mday;
}

int CDateTime::GetHour() const
{
	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	return tmTimeTemp.tm_hour;
}

int CDateTime::GetMinute() const
{
	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	return tmTimeTemp.tm_min;
}

int CDateTime::GetSecond() const
{
	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	return tmTimeTemp.tm_sec;
}

int CDateTime::GetWDay() const
{
	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	return tmTimeTemp.tm_wday;
}

void CDateTime::SetYear(int iValue)
{
	if (iValue < 1970)
		return;

	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	tmTimeTemp.tm_year = iValue - 1900;
	if (1 == tmTimeTemp.tm_mon && 29 == tmTimeTemp.tm_mday) //2月
	{
		if (!CheckIsLeapYear(iValue))
		{
			tmTimeTemp.tm_mday = 28;
		}
	}
	m_lSeconds = mktime(&tmTimeTemp);
}

void CDateTime::SetMonth(int iValue)
{
	if (iValue < 1 || iValue > 12)
		return;

	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	tmTimeTemp.tm_mon = iValue - 1;
	if (1 == tmTimeTemp.tm_mon)
	{
		//2月
		if (CheckIsLeapYear(tmTimeTemp.tm_year + 1900))
		{
			if (29 < tmTimeTemp.tm_mday)
				tmTimeTemp.tm_mday = 29;
		}
		else
		{
			if (29 <= tmTimeTemp.tm_mday)
				tmTimeTemp.tm_mday = 28;
		}
	}
	else if (3 == tmTimeTemp.tm_mon || 5 == tmTimeTemp.tm_mon || 8 == tmTimeTemp.tm_mon || 10 == tmTimeTemp.tm_mon)
	{
		//4、6、9、11月只有30天
		if (31 == tmTimeTemp.tm_mday)
		{
			tmTimeTemp.tm_mday = 30;
		}
	}
	m_lSeconds = mktime(&tmTimeTemp);
}

void CDateTime::SetDay(int iValue)
{
	if (iValue < 0 || iValue > 31) //2月需要处理
		return;

	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	tmTimeTemp.tm_mday = iValue;
	if (1 == tmTimeTemp.tm_mon)
	{
		//2月
		if (CheckIsLeapYear(tmTimeTemp.tm_year + 1900))
		{
			if (29 < tmTimeTemp.tm_mday)
				tmTimeTemp.tm_mday = 29;
		}
		else
		{
			if (29 <= tmTimeTemp.tm_mday)
				tmTimeTemp.tm_mday = 28;
		}
	}
	else if (3 == tmTimeTemp.tm_mon || 5 == tmTimeTemp.tm_mon || 8 == tmTimeTemp.tm_mon || 10 == tmTimeTemp.tm_mon)
	{
		//4、6、9、11月只有30天
		if (31 == tmTimeTemp.tm_mday)
		{
			tmTimeTemp.tm_mday = 30;
		}
	}
	m_lSeconds = mktime(&tmTimeTemp);
}

void CDateTime::SetHour(int iValue)
{
	if (iValue < 0 || iValue > 23)
		return;

	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	tmTimeTemp.tm_hour = iValue;
	m_lSeconds = mktime(&tmTimeTemp);
}

void CDateTime::SetMinute(int iValue)
{
	if (iValue < 0 || iValue > 59)
		return;

	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	tmTimeTemp.tm_min = iValue;
	m_lSeconds = mktime(&tmTimeTemp);
}

void CDateTime::SetSecond(int iValue)
{
	if (iValue < 0 || iValue > 59)
		return;

	struct tm tmTimeTemp;
	LocalTime(m_lSeconds, tmTimeTemp);
	tmTimeTemp.tm_sec = iValue;
	m_lSeconds = mktime(&tmTimeTemp);
}
