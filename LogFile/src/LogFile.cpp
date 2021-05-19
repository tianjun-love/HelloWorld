#include "../include/LogFile.hpp"
#include "../include/LogFileBackup.hpp"
#include <stdarg.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#endif

CLogFile::CLogFile() : m_bLogInit(false), m_iBackupRetain(-1), m_pBackupThread(nullptr)
{
}

CLogFile::CLogFile(const string& szLogFileName, const string& szLogLevel, const string& szDecollator,
	int iBackupRetain) : m_iBackupRetain(iBackupRetain), m_pBackupThread(nullptr)
{
	m_LogMember.m_szFileName = szLogFileName;
	m_LogMember.m_eLogDeco = GetDecollator(szDecollator);
	m_LogMember.m_eLogLevel = GetLogLevel(szLogLevel);
}

CLogFile::~CLogFile()
{
	Close();
}

bool CLogFile::Open(string &szError)
{
	if (!m_bLogInit)
	{
		m_bLogInit = true;
		m_LogMember.m_szDate = GetDateTimeStr(false);

		return CreateLogFile(szError);
	}

	return true;
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

		if (nullptr != m_pBackupThread)
		{
			m_pBackupThread->join();
			delete m_pBackupThread;
			m_pBackupThread = nullptr;
		}
	}
}

void CLogFile::ActivateBackupOnece()
{
	if (m_bLogInit)
	{
		//启动备份线程
		if (nullptr != m_pBackupThread)
		{
			m_pBackupThread->join();
			delete m_pBackupThread;
			m_pBackupThread = nullptr;
		}

		m_pBackupThread = new std::thread(&CLogFile::LogFileBackup, this);
	}
}

eLOGFILE_DECOLLATOR CLogFile::GetDecollator(const string& szDecollator)
{
	if (0 == CLogFileMember::StrCaseCmp("hour", szDecollator.c_str()))
	{
		return E_LOG_DECO_HOUR;
	}
	else if (0 == CLogFileMember::StrCaseCmp("day", szDecollator.c_str()))
	{
		return E_LOG_DECO_DAY;
	}
	else if (0 == CLogFileMember::StrCaseCmp("month", szDecollator.c_str()))
	{
		return E_LOG_DECO_MONTH;
	}
	else
	{
		return E_LOG_DECO_DAY;
	}
}

eLOG_FILE_LEVEL CLogFile::GetLogLevel(const string& szLevel)
{
	if (0 == CLogFileMember::StrCaseCmp("all", szLevel.c_str()))
	{
		return E_LOG_LEVEL_ALL;
	}
	else if (0 == CLogFileMember::StrCaseCmp("serious", szLevel.c_str()))
	{
		return E_LOG_LEVEL_SERIOUS;
	}
	else if (0 == CLogFileMember::StrCaseCmp("warn", szLevel.c_str()))
	{
		return E_LOG_LEVEL_WARN;
	}
	else if (0 == CLogFileMember::StrCaseCmp("prompt", szLevel.c_str()))
	{
		return E_LOG_LEVEL_PROMPT;
	}
	else if (0 == CLogFileMember::StrCaseCmp("debug", szLevel.c_str()))
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

void CLogFile::CheckLogFile()
{
	if (m_bLogInit)
	{
		string szFileDate, szCurrDate, szError;

		m_Lock.lock();

		szFileDate = m_LogMember.m_szDate;
		szCurrDate = GetDateTimeStr(false);

		if (szFileDate != szCurrDate || !(m_LogMember.m_Ofstream.is_open() && m_LogMember.m_Ofstream.good()))
		{
			m_LogMember.m_szDate = szCurrDate;

			if (!CreateLogFile(szError))
			{
#ifdef _DEBUG
				std::cerr << GetDateTimeStr() << szError << std::endl;
#endif
			}

			//检查备份
			ActivateBackupOnece();
		}

		m_Lock.unlock();
	}
}

bool CLogFile::CreateLogFile(string &szError)
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
	}
	else
	{
		szError = "Open log file: " + szFileName + " failed.";
		return false;
	}

	return true;
}

void CLogFile::LogFileBackup()
{
	CLogFileBackup backup(this);

	//备份处理
	backup.BackupHandle(m_LogMember.m_szFileName, m_LogMember.m_eLogDeco, m_iBackupRetain);
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

void CLogFile::PrintHex(const eLOG_FILE_LEVEL& eLogLevel, const string &szPrompt, unsigned char *buff, size_t len, unsigned char ucPreSpace)
{
	if (nullptr == buff || 0 == len)
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

	unsigned char ch = 0, temp[16] = { 0 };
	size_t iPos = 0, iSurplusLen = 0;
	unsigned int uiAddress = 0;
	char *pPreSpace = new char[ucPreSpace + 1]{ 0 };
	std::string szPrint, szDump;
	char printbuf[128] = { 0 };
	const std::string szHead = "Address   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  Dump",
		szFullFormat = "%s%08x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  %s\n";

	memset(pPreSpace, ' ', ucPreSpace);

	//头
	snprintf(printbuf, sizeof(printbuf), "%s%s\n", pPreSpace, szHead.c_str());
	szPrint = printbuf;
	memset(printbuf, 0, sizeof(printbuf));

	//数据
	do
	{
		iSurplusLen = len - iPos;

		if (iSurplusLen >= 16)
		{
			memcpy(temp, buff + iPos, 16);

			for (unsigned int i = 0; i < 16; ++i)
			{
				ch = temp[i];

				if (ch >= 32 && ch <= 126)
					szDump += (char)ch;
				else
					szDump += ".";
			}

			snprintf(printbuf, sizeof(printbuf), szFullFormat.c_str(), pPreSpace, uiAddress, temp[0], temp[1], temp[2], temp[3],
				temp[4], temp[5], temp[6], temp[7], temp[8], temp[9], temp[10], temp[11], temp[12], temp[13], temp[14], temp[15],
				szDump.c_str());
			szPrint.append(printbuf);
			memset(printbuf, 0, sizeof(printbuf));

			iPos += 16;
		}
		else
		{
			if (iSurplusLen > 0)
			{
				memcpy(temp, buff + iPos, iSurplusLen);

				for (unsigned int i = 0; i < iSurplusLen; ++i)
				{
					ch = temp[i];

					if (ch >= 32 && ch <= 126)
						szDump += (char)ch;
					else
						szDump += ".";
				}

				//计算格式
				unsigned int uiWide = 48;

				snprintf(printbuf, sizeof(printbuf), "%s%08x", pPreSpace, uiAddress);
				szPrint.append(printbuf);
				memset(printbuf, 0, sizeof(printbuf));

				for (unsigned int i = 0; i < iSurplusLen; ++i)
				{
					uiWide -= 3;

					snprintf(printbuf, sizeof(printbuf), " %02x", temp[i]);
					szPrint.append(printbuf);
					memset(printbuf, 0, sizeof(printbuf));
				}

				snprintf(printbuf, sizeof(printbuf), "  %*s", uiWide, " ");
				szPrint.append(printbuf + szDump + "\n");
			}

			iPos += iSurplusLen;
		}

		++uiAddress;
		memset(temp, 0, sizeof(temp));
		szDump.clear();
	} while (iPos < len);

	delete[] pPreSpace;

#ifdef _DEBUG
	std::cout << GetDateTimeStr() << CLogFileMember::GetLogLevelStr(eLogLevel) << szPrompt << "\n" << szPrint << std::endl;
#endif

	if (m_LogMember.m_bOpen)
	{
		m_LogMember.m_LogLock.lock();
		m_LogMember.m_Ofstream << GetDateTimeStr() << CLogFileMember::GetLogLevelStr(eLogLevel) << szPrompt << "\n" << szPrint
			<< std::endl;

		if (!m_LogMember.m_bLogendl)
			m_LogMember.m_bLogendl = true;

		m_LogMember.m_LogLock.unlock();
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