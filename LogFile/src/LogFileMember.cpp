#include "../include/LogFileMember.hpp"

CLogFileMember::CLogFileMember() : m_szFileName("log"), m_szDate("20150101"),
m_eLogLevel(E_LOG_LEVEL_ALL), m_eLogDeco(E_LOG_DECO_DAY), m_bOpen(false), m_bLogendl(true)
{
}

CLogFileMember::CLogFileMember(const string& szFileName, const string& szDate, eLOG_FILE_LEVEL eLogLevel, eLOGFILE_DECOLLATOR eLogDeco) :
m_szFileName(szFileName), m_szDate(szDate), m_eLogLevel(eLogLevel), m_eLogDeco(eLogDeco),
m_bOpen(false), m_bLogendl(true)
{
}

CLogFileMember::~CLogFileMember()
{
}

string CLogFileMember::GetLogLevelStr(eLOG_FILE_LEVEL eLevel)
{
	string szTemp("[ALL]:");

	switch (eLevel)
	{
	case E_LOG_LEVEL_SERIOUS:
		szTemp = "[SERIOUS]:";
		break;
	case E_LOG_LEVEL_WARN:
		szTemp = "[WARN]:";
		break;
	case E_LOG_LEVEL_PROMPT:
		szTemp = "[PROMPT]:";
		break;
	case E_LOG_LEVEL_DEBUG:
		szTemp = "[DEBUG]:";
		break;
	default:
		break;
	}

	return std::move(szTemp);
}
