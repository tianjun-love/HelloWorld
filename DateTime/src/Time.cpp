#include "../include/Time.hpp"

CTime::CTime()
{
	m_lSeconds = 1329235200; // first day to work, 2012-02-15 08:30:00
}

CTime::CTime(unsigned int iHour, unsigned int iMin, unsigned int iSec)
{
	tm DateTime;

	DateTime.tm_year = 112;
	DateTime.tm_mon = 1;
	DateTime.tm_mday = 15;
	DateTime.tm_hour = iHour;
	DateTime.tm_min = iMin;
	DateTime.tm_sec = iSec;
	DateTime.tm_wday = 0;
	DateTime.tm_yday = 0;
	DateTime.tm_isdst = 0;

	m_lSeconds = mktime(&DateTime);
}

CTime::CTime(const std::string& szTime)
{
	CopyFromTime(szTime);
}

CTime::CTime(time_t lSeconds)
{
	m_lSeconds = lSeconds;
	truncateDate(m_lSeconds);
}

CTime::CTime(const CDateTime& DateTime)
{
	CopyFromDateTime(DateTime);
}

CTime::CTime(const CTime& Other)
{
	m_lSeconds = Other.m_lSeconds;
}

CTime::~CTime()
{
}

void CTime::InitCurrTime(time_t lSecond)
{
	InitCurrDateTime(lSecond);
	truncateDate(m_lSeconds);
}

void CTime::CopyFromTime(const std::string &szTime)
{
	//最少8个字节，"23:03:23"
	if (szTime.length() < 8)
		return;

	tm DateTime;

	char* pszStr = (char*)szTime.c_str();
	DateTime.tm_year = 112;
	DateTime.tm_mon = 1;
	DateTime.tm_mday = 15;
	DateTime.tm_hour = std::atoi(pszStr);
	DateTime.tm_min = std::atoi(pszStr + 3);
	DateTime.tm_sec = std::atoi(pszStr + 6);
	DateTime.tm_wday = 0;
	DateTime.tm_yday = 0;

	m_lSeconds = mktime(&DateTime);
}

void CTime::CopyFromDateTime(const CDateTime &DateTime)
{
	m_lSeconds = DateTime.m_lSeconds;
	truncateDate(m_lSeconds);
}

void CTime::truncateDate(time_t &tDateTime)
{
	struct tm tmTimeTemp;

	LocalTime(m_lSeconds, tmTimeTemp);

	tmTimeTemp.tm_year = 112;
	tmTimeTemp.tm_mon = 1;
	tmTimeTemp.tm_mday = 15;
	tmTimeTemp.tm_wday = 0;
	tmTimeTemp.tm_yday = 0;

	tDateTime = mktime(&tmTimeTemp);
}

CTime &CTime::operator=(const CTime &Other)
{
	m_lSeconds = Other.m_lSeconds;
	return *this;
}

CTime &CTime::operator=(const std::string &szTime)
{
	CopyFromTime(szTime);
	return *this;
}

CTime &CTime::operator=(time_t lSeconds)
{
	m_lSeconds = lSeconds;
	truncateDate(m_lSeconds);
	return *this;
}

CTime &CTime::operator=(const CDateTime &DateTime)
{
	CopyFromDateTime(DateTime);
	return *this;
}

CTime::operator std::string() const
{
	char strTime[16] = { '\0' };
	struct tm tmTimeTemp;

	LocalTime(m_lSeconds, tmTimeTemp);
	strftime(strTime, 10, "%X", &tmTimeTemp);

	return std::move(std::string(strTime));
}

std::ostream& operator<<(std::ostream &out, const CTime& Time)
{
	out << (std::string)Time;
	return out;
}