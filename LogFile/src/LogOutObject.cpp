#include "../include/LogOutObject.hpp"

//方便使用
#define m_Log (*((CLogFileMember*)m_pLogMember))

CLogOutObject::CLogOutObject(void* pLogMember, bool bLost) : m_pLogMember(pLogMember), m_bLost(bLost), m_bIsLastOutputEnd(false)
{}

CLogOutObject::~CLogOutObject()
{
	if (m_pLogMember)
	{
		if (m_bIsLastOutputEnd)
			m_Log.m_bLogendl = true;
		m_Log.m_LogLock.unlock();
		m_pLogMember = nullptr;
	}
}

CLogOutObject& CLogOutObject::operator << (const string& Str)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << Str;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << Str;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const char*  pStr)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << pStr;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << pStr;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const char& cChar)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << cChar;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << cChar;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const signed char& cChar)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << cChar;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << cChar;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const unsigned char& ucChar)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << ucChar;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << ucChar;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const short& nNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << nNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << nNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const unsigned short& unNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << unNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << unNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const int& iNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << iNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << iNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const unsigned int& uiNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << uiNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << uiNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const long& lNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << lNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << lNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const unsigned long& ulNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << ulNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << ulNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const float& fNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << fNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << fNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const double& dNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << dNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << dNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const long long& NNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << NNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << NNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const unsigned long long& uNNumber)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << uNNumber;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << uNNumber;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (std::ostream& (*_F)(std::ostream&))
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << std::endl;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << std::endl;
	}

	return *this;
}

CLogOutObject& CLogOutObject::operator << (const CLogEnd& LogEnd)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << std::endl;
#endif
		if (!m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = true;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << std::endl;
	}
	
	return *this;
}

CLogOutObject& CLogOutObject::operator << (const eLOG_FILE_LEVEL& eLogLevel)
{
	if (!m_bLost)
	{
#ifdef _DEBUG
		std::cout << eLogLevel;
#endif
		if (m_bIsLastOutputEnd)
			m_bIsLastOutputEnd = false;

		if (m_Log.m_bOpen)
			m_Log.m_Ofstream << eLogLevel;
	}

	return *this;
}
