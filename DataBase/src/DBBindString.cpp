#include "../Public/DBBindString.hpp"

CDBBindString::CDBBindString() 
{ 
	Init(); 
}

CDBBindString::CDBBindString(const CDBBindString& Other)
{
	memset(m_strBuffer, '\0', m_lBufferLength);
	if (0 < m_lDataLength)
	{
		memcpy(m_strBuffer, Other.m_strBuffer, m_lDataLength);
	}

	if (Other.m_iRawBufferLength > 0)
	{
		if (m_iRawBufferLength >= Other.m_iRawBufferLength)
		{
			memset(m_pRawData, 0, m_iRawBufferLength);
		}
		else
		{
			if (nullptr != m_pRawData)
			{
				delete[] m_pRawData;
				m_pRawData = nullptr;
			}

			m_iRawBufferLength = Other.m_iRawBufferLength;
			m_pRawData = new unsigned char[m_iRawBufferLength] { '\0' };
		}

		m_iRawDataLength = Other.m_iRawDataLength;
		memcpy(m_pRawData, Other.m_pRawData, m_iRawDataLength);
	}
	else
	{
		if (nullptr != m_pRawData)
		{
			delete[] m_pRawData;
			m_pRawData = nullptr;
		}

		m_iRawBufferLength = 0;
		m_iRawDataLength = 0;
	}
}

CDBBindString::CDBBindString(const char* strString) 
{ 
	Init(); 
	*this = strString; 
}

CDBBindString::CDBBindString(const std::string& szString) 
{ 
	Init(); 
	*this = szString; 
}

CDBBindString::~CDBBindString() 
{ 
	Clear(true); 
}

CDBBindString& CDBBindString::operator = (const CDBBindString& Other)
{
	if (&Other != this)
	{
		memset(m_strBuffer, '\0', m_lBufferLength);
		if (0 < m_lDataLength)
		{
			memcpy(m_strBuffer, Other.m_strBuffer, m_lDataLength);
		}
	}

	return *this;
}

CDBBindString& CDBBindString::operator = (const char* strString)
{
	if (nullptr == strString)
	{
		Clear();
	}
	else
	{
		m_lDataLength = (unsigned long)strlen(strString);
		m_nParamIndp = (m_lDataLength > 0 ? 0 : -1);
		memset(m_strBuffer, '\0', m_lBufferLength);

		if (m_lDataLength > 0)
		{
			//防止数据过长
			if (m_lDataLength >= m_lBufferLength)
			{
				m_lDataLength = D_DB_STRING_MAX_LEN;
			}

			memcpy(m_strBuffer, strString, m_lDataLength);
		}
	}

	return *this;
}

CDBBindString& CDBBindString::operator = (const std::string& szString)
{
	if (szString.empty())
	{
		Clear();
	}
	else
	{
		m_lDataLength = (unsigned long)szString.length();
		m_nParamIndp = (m_lDataLength > 0 ? 0 : -1);
		memset(m_strBuffer, '\0', m_lBufferLength);

		if (m_lDataLength > 0)
		{
			//防止数据过长
			if (m_lDataLength >= m_lBufferLength)
			{
				m_lDataLength = D_DB_STRING_MAX_LEN;
			}

			memcpy(m_strBuffer, szString.c_str(), m_lDataLength);
		}
	}

	return *this;
}

bool CDBBindString::SetBuffer(unsigned long lBufferLength)
{
	if (nullptr != m_pRawData)
	{
		delete[] m_pRawData;
		m_pRawData = nullptr;
	}

	m_nParamIndp = -1;
	m_iRawBufferLength = lBufferLength;

	if (m_iRawBufferLength > 0)
	{
		m_pRawData = new unsigned char[m_iRawBufferLength] { '\0' };
		if (nullptr == m_pRawData)
		{
			return false;
		}
	}

	return true;
}

bool CDBBindString::SetBufferAndValue(unsigned long lBufferLength, unsigned char* pValue, unsigned long lValueLength)
{
	if (nullptr != m_pRawData)
	{
		delete[] m_pRawData;
		m_pRawData = nullptr;
	}

	if (lBufferLength <= lValueLength)
		return false;

	m_nParamIndp = -1;
	m_iRawDataLength = lValueLength;
	m_iRawBufferLength = lBufferLength;

	if (m_iRawBufferLength > 0)
	{
		m_pRawData = new unsigned char[m_iRawBufferLength] { '\0' };

		if (nullptr != pValue)
		{
			m_nParamIndp = 0;
			memcpy(m_pRawData, pValue, m_iRawDataLength);
		}
		else
			return false;
	}

	return true;
}

bool CDBBindString::SetValue(unsigned char* pValue, unsigned long lValueLength)
{
	if (nullptr == m_pRawData || lValueLength >= m_iRawBufferLength)
	{
		return false;
	}

	if (nullptr != pValue)
	{
		m_iRawDataLength = lValueLength;
		m_nParamIndp = (m_iRawDataLength > 0 ? 0 : -1);
		memcpy(m_pRawData, pValue, m_iRawDataLength);
	}
	else
	{
		Clear();
		return false;
	}

	return true;
}

const char* CDBBindString::GetData() const
{
	if (nullptr != m_pRawData)
		return (const char*)m_pRawData;
	else
		return m_strBuffer;
}

std::string CDBBindString::ToString() const
{
	return std::move(std::string(m_strBuffer));
}

void CDBBindString::Clear(bool bFreeAll)
{
	m_lDataLength = 0;
	m_nParamIndp = -1;
	memset(m_strBuffer, '\0', m_lBufferLength);

	m_iRawDataLength = 0;

	if (bFreeAll)
	{
		m_iRawBufferLength = 0;

		if (nullptr != m_pRawData)
		{
			delete[] m_pRawData;
			m_pRawData = nullptr;
		}
	}
}

bool CDBBindString::Init()
{
	m_lBufferLength = D_DB_STRING_MAX_LEN + 1;
	memset(m_strBuffer, '\0', m_lBufferLength);

	m_iRawBufferLength = 0;
	m_iRawDataLength = 0;
	m_pRawData = nullptr;

	return true;
}