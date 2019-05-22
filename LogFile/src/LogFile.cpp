#include "../include/LogFile.hpp"
#include <stdarg.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#endif

CLogFile::CLogFile() : m_bLogInit(false)
{
	Init();
	CreateLogFile();
}

CLogFile::CLogFile(const string& szLogFileName, const string& szLogLevel, const string& szDecollator)
{
	m_LogMember.m_szFileName = szLogFileName;
	m_LogMember.m_eLogDeco = GetDecollator(szDecollator);
	m_LogMember.m_eLogLevel = GetLogLevel(szLogLevel);

	Init();
	CreateLogFile();
}

CLogFile::~CLogFile()
{
	Close();
}

void CLogFile::Init()
{
	m_LogMember.m_szDate = GetDateTimeStr(false);
	m_bLogInit = true;
}

eLOGFILE_DECOLLATOR CLogFile::GetDecollator(const string& szDecollator)
{
	F_COMPARE compare;

#ifdef _WIN32
	compare = _stricmp;
#else
	compare = strcasecmp;
#endif

	if (!compare("minute", szDecollator.c_str()))
	{
		return E_LOG_DECO_MINUTE;
	}
	else if (!compare("hour", szDecollator.c_str()))
	{
		return E_LOG_DECO_HOUR;
	}
	else if (!compare("day", szDecollator.c_str()))
	{
		return E_LOG_DECO_DAY;
	}
	else if (!compare("month", szDecollator.c_str()))
	{
		return E_LOG_DECO_MONTH;
	}
	else
	{
		return E_LOG_DECO_HOUR;
	}
}

eLOG_FILE_LEVEL CLogFile::GetLogLevel(const string& szLevel)
{
	F_COMPARE compare;

#ifdef _WIN32
	compare = _stricmp;
#else
	compare = strcasecmp;
#endif

	if (!compare("all", szLevel.c_str()))
	{
		return E_LOG_LEVEL_ALL;
	}
	else if (!compare("serious", szLevel.c_str()))
	{
		return E_LOG_LEVEL_SERIOUS;
	}
	else if (!compare("warn", szLevel.c_str()))
	{
		return E_LOG_LEVEL_WARN;
	}
	else if (!compare("prompt", szLevel.c_str()))
	{
		return E_LOG_LEVEL_PROMPT;
	}
	else if (!compare("debug", szLevel.c_str()))
	{
		return E_LOG_LEVEL_DEBUG;
	}

	return E_LOG_LEVEL_ALL;
}

const eLOG_FILE_LEVEL& CLogFile::GetLogLevel() const
{
	return m_LogMember.m_eLogLevel;
}

void CLogFile::SetLogLevel(const string& szLevel)
{
	m_LogMember.m_LevelLock.lock();
	m_LogMember.m_eLogLevel = GetLogLevel(szLevel);
	m_LogMember.m_LevelLock.unlock();
}

void CLogFile::Close()
{
	if (m_bLogInit)
	{
		m_bLogInit = false;

		if (m_LogMember.m_bOpen && m_LogMember.m_Ofstream.is_open())
		{
			m_LogMember.m_bOpen = false;
			m_LogMember.m_Ofstream.close();
		}
	}
}

void CLogFile::CheckLogFile()
{
	if (m_bLogInit)
	{
		string szFileDate, szCurrDate;

		m_Lock.lock();

		szFileDate = m_LogMember.m_szDate;
		szCurrDate = GetDateTimeStr(false);

		if (szFileDate != szCurrDate)
		{
			m_LogMember.m_szDate = szCurrDate;
			CreateLogFile();
		}

		m_Lock.unlock();
	}
}

void CLogFile::CreateLogFile()
{
	string szFileName = m_LogMember.m_szFileName + "_" + m_LogMember.m_szDate + ".log";

	if (m_LogMember.m_bOpen)
	{
		if (m_LogMember.m_Ofstream.is_open() && m_LogMember.m_Ofstream.good())
		{
			m_LogMember.m_Ofstream.flush();
			m_LogMember.m_Ofstream.close();
		}

		m_LogMember.m_bOpen = false;
		m_LogMember.m_Ofstream.clear();
	}

	m_LogMember.m_Ofstream.open(szFileName.c_str(), ios::out | ios::app);
	if (m_LogMember.m_Ofstream.is_open() && m_LogMember.m_Ofstream.good())
	{
		m_LogMember.m_bOpen = true;
		std::cout << GetDateTimeStr() << CLogFileMember::GetLogLevelStr(E_LOG_LEVEL_ALL) << "Open log file:" 
			<< szFileName << " success !" << std::endl;
	}
	else
	{
		std::cerr << GetDateTimeStr() << CLogFileMember::GetLogLevelStr(E_LOG_LEVEL_ALL) << "Open log file:" 
			<< szFileName << " failed !" << std::endl;
	}
}

CLogOutObject CLogFile::operator << (const string& Str)
{
	CheckLogFile();

#ifdef _DEBUG
		std::cout << GetDateTimeStr() << Str;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << Str;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const char* pStr)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << pStr;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << pStr;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const char& cChar)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << cChar;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << cChar;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const signed char& cChar)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << cChar;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << cChar;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const unsigned char& ucChar)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << ucChar;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << ucChar;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const short& nNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << nNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << nNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const unsigned short& unNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << unNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << unNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const int& iNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << iNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << iNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const unsigned int& uiNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << uiNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << uiNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const long& lNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << lNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << lNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const unsigned long& ulNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << ulNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << ulNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const float& fNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << fNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << fNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const double& dNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << dNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << dNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const long long& NNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << NNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << NNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const unsigned long long& uNNumber)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << uNNumber;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << uNNumber;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (std::ostream& (*_F)(std::ostream&))
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << std::endl;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << std::endl;

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const CLogEnd& LogEnd)
{
	CheckLogFile();

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << std::endl;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << std::endl;

		if (!m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = true;
	}

	return CLogOutObject((void*)&m_LogMember);
}

CLogOutObject CLogFile::operator << (const eLOG_FILE_LEVEL& eLogLevel)
{
	CheckLogFile();

	m_LogMember.m_LevelLock.lock();
	eLOG_FILE_LEVEL eTemp = m_LogMember.m_eLogLevel;
	m_LogMember.m_LevelLock.unlock();

	//输出级别低于日志级别，丢弃
	if ((E_LOG_LEVEL_ALL != eTemp) && (short)eLogLevel > (short)eTemp)
	{
		m_LogMember.m_LogLock.lock();
		return CLogOutObject((void*)&m_LogMember, true);
	}

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << CLogFileMember::GetLogLevelStr(eLogLevel);
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << CLogFileMember::GetLogLevelStr(eLogLevel);

		if (m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = false;
	}

	return CLogOutObject((void*)&m_LogMember);
}

void CLogFile::Print(const eLOG_FILE_LEVEL& eLogLevel, const char* szFormat, ...)
{
	if (nullptr == szFormat)
		return;

	CheckLogFile();

	m_LogMember.m_LevelLock.lock();
	eLOG_FILE_LEVEL eTemp = m_LogMember.m_eLogLevel;
	m_LogMember.m_LevelLock.unlock();

	//输出级别低于日志级别，丢弃
	if ((E_LOG_LEVEL_ALL != eTemp) && (short)eLogLevel > (short)eTemp)
	{
		return;
	}

	char Buffer[LOG_FORMAT_MAX_LENGTH]{ 0 };
	int iRet = 0;
	va_list ap;
	va_start(ap, szFormat);

	//格式化
#ifdef _WIN32
	iRet = vsprintf_s(Buffer, LOG_FORMAT_MAX_LENGTH, szFormat, ap);
#else
	iRet = vsnprintf(Buffer, LOG_FORMAT_MAX_LENGTH, szFormat, ap);
#endif

	va_end(ap);

	if (iRet > 0)
	{
#ifdef _DEBUG
		std::cout << GetDateTimeStr() << CLogFileMember::GetLogLevelStr(eLogLevel) << Buffer << std::endl;
#endif

		if (m_LogMember.m_bOpen)
		{
			m_LogMember.m_LogLock.lock();
			m_LogMember.m_Ofstream << GetDateTimeStr() << CLogFileMember::GetLogLevelStr(eLogLevel) << Buffer
				<< std::endl;

			if (!m_LogMember.m_bLogendl)
				m_LogMember.m_bLogendl = true;

			m_LogMember.m_LogLock.unlock();
		}
	}
}

int CLogFile::GetCurrentMillisecond()
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

string CLogFile::GetDateTimeStr(bool bIsDateTimeStr, bool bNeedMilliseconds)
{
	char strDateTime[32] = { '\0' };
	struct tm tmTimeTemp;
	time_t timeTemp = 0;

	//获取当前秒数
	timeTemp = time(NULL);

	//localtime函数不可重入，即不是线程安全的，所以用下面的版本
#ifdef _WIN32
	localtime_s(&tmTimeTemp, &timeTemp);
#else
	localtime_r(&timeTemp, &tmTimeTemp);
#endif

	if (bIsDateTimeStr)
	{
		//上一次输出了结束，才重新输出日期
		if (m_LogMember.m_bLogendl)
		{
			if (bNeedMilliseconds)
			{
				snprintf(strDateTime, 24, "%02d:%02d:%02d.%03d>> ", tmTimeTemp.tm_hour, tmTimeTemp.tm_min,
					tmTimeTemp.tm_sec, GetCurrentMillisecond());
			}
			else
				strftime(strDateTime, 24, "%Y-%m-%d %X>> ", &tmTimeTemp);
		}
	}
	else
	{
		switch (m_LogMember.m_eLogDeco)
		{
		case E_LOG_DECO_MINUTE:
			strftime(strDateTime, 20, "%Y%m%d%H%M", &tmTimeTemp);
			break;
		case E_LOG_DECO_HOUR:
			strftime(strDateTime, 16, "%Y%m%d%H", &tmTimeTemp);
			break;
		case E_LOG_DECO_DAY:
			strftime(strDateTime, 12, "%Y%m%d", &tmTimeTemp);
			break;
		case E_LOG_DECO_MONTH:
			strftime(strDateTime, 8, "%Y%m", &tmTimeTemp);
			break;
		default:
			break;
		}
	}

	return string(strDateTime);
}