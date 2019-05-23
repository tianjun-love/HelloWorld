#include "../Public/DBBindDateTime.hpp"
#include <regex>
#include "oci.h"

CDBBindDateTime::CDBBindDateTime() : m_pOCIEnv(nullptr), m_pOCIErr(nullptr), m_pOCITimestamp(nullptr)
{ 
	Init(); 
};

CDBBindDateTime::CDBBindDateTime(const char* strDateTime) 
{ 
	Init(); 
	*this = strDateTime;
};

CDBBindDateTime::CDBBindDateTime(const std::string& szDateTime) 
{ 
	Init(); 
	*this = szDateTime; 
};

CDBBindDateTime::~CDBBindDateTime()
{
	Clear(true);
}

CDBBindDateTime& CDBBindDateTime::operator = (const char* strDateTime)
{
	if (nullptr == strDateTime)
	{
		Clear();
	}
	else
	{
		m_lDataLength = (unsigned long)strlen(strDateTime);
		m_nParamIndp = (m_lDataLength > 0 ? 0 : -1);
		memset(m_strBuffer, '\0', m_lBufferLength);

		if (m_lDataLength > 0)
		{
			//防止数据过长
			if (m_lDataLength >= m_lBufferLength)
			{
				m_lDataLength = D_DB_DATETIME_MAX_LEN;
			}

			memcpy(m_strBuffer, strDateTime, m_lDataLength);
			if (!BindOCIDateTime())
			{
				//绑定失败则置空
				m_nParamIndp = -1;
			}
		}
	}

	return *this;
}

CDBBindDateTime& CDBBindDateTime::operator = (const std::string& szDateTime)
{
	if (szDateTime.empty())
	{
		Clear();
	}
	else
	{
		m_lDataLength = (unsigned long)szDateTime.length();
		m_nParamIndp = (m_lDataLength > 0 ? 0 : -1);
		memset(m_strBuffer, '\0', m_lBufferLength);

		if (m_lDataLength > 0)
		{
			//防止数据过长
			if (m_lDataLength >= m_lBufferLength)
			{
				m_lDataLength = D_DB_DATETIME_MAX_LEN;
			}

			memcpy(m_strBuffer, szDateTime.c_str(), m_lDataLength);
			if (!BindOCIDateTime())
			{
				//绑定失败则置空
				m_nParamIndp = -1;
			}
		}
	}

	return *this;
}

bool CDBBindDateTime::BindOCIDateTime() //绑定OCI的时间戳
{
	if (nullptr != m_pOCIEnv && nullptr != m_pOCIErr)
	{
		if (DB_DATA_TYPE_DATETIME == m_eOCIDateTimeType)
		{
			if (CheckFormat(m_eOCIDateTimeType))
			{
				if (OCI_SUCCESS == OCIDateFromText((OCIError*)m_pOCIErr, (const OraText*)m_strBuffer, m_lDataLength,
					(const OraText*)"yyyy-mm-dd hh24:mi:ss", 21, NULL, 0, (OCIDate*)m_strBuffer))
				{
					return true;
				}
			}
		}
		else
		{
			if (CheckFormat(m_eOCIDateTimeType))
			{
				if (OCI_SUCCESS == OCIDateTimeFromText((OCIEnv*)m_pOCIEnv, (OCIError*)m_pOCIErr, (const OraText*)m_strBuffer, m_lDataLength,
					(const OraText*)"yyyy-mm-dd hh24:mi:ss.ff", 24, NULL, 0, (OCIDateTime*)m_pOCITimestamp))
				{
					return true;
				}
			}
		}
	}
	else
		return true;

	return false;
}

bool CDBBindDateTime::CheckFormat(EDBDataType eDateTimeType, const char* strDateTime)
{
	//为空就检查自己
	if (nullptr == strDateTime)
	{
		strDateTime = m_strBuffer;
	}

	if (DB_DATA_TYPE_TIME == eDateTimeType)
	{
		const std::regex patten("(\\d{2}):(\\d{2}):(\\d{2})");

		if (std::regex_match(strDateTime, patten))
		{
			unsigned int iHour = 0, iMinute = 0, iSecond = 0;

			sscanf(strDateTime, "%u:%u:%u", &iHour, &iMinute, &iSecond);
			return CheckTime(iHour, iMinute, iSecond);
		}
		else
			return false;
	}
	else if (DB_DATA_TYPE_DATE == eDateTimeType)
	{
		const std::regex patten("(\\d{4})-(\\d{2})-(\\d{2})");
		
		if (std::regex_match(strDateTime, patten))
		{
			unsigned int iYear = 0, iMonth = 0, iDay = 0;

			sscanf(strDateTime, "%u-%u-%u", &iYear, &iMonth, &iDay);
			return CheckDate(iYear, iMonth, iDay);
		}
		else
			return false;
	}
	else if (DB_DATA_TYPE_DATETIME == eDateTimeType)
	{
		const std::regex patten("(\\d{4})-(\\d{2})-(\\d{2}) (\\d{2}):(\\d{2}):(\\d{2})");
		
		if (std::regex_match(strDateTime, patten))
		{
			unsigned int iYear = 0, iMonth = 0, iDay = 0, iHour = 0, iMinute = 0, iSecond = 0;

			sscanf(strDateTime, "%u-%u-%u %u:%u:%u", &iYear, &iMonth, &iDay, &iHour, &iMinute, &iSecond);
			return (CheckDate(iYear, iMonth, iDay) && CheckTime(iHour, iMinute, iSecond));
		}
		else
			return false;
	}
	else if (DB_DATA_TYPE_TIMESTAMP == eDateTimeType) //oracle使用
	{
		const std::regex patten("(\\d{4})-(\\d{2})-(\\d{2}) (\\d{2}):(\\d{2}):(\\d{2})\\.(\\d{1,3})");
		
		if (std::regex_match(strDateTime, patten))
		{
			unsigned int iYear = 0, iMonth = 0, iDay = 0, iHour = 0, iMinute = 0, iSecond = 0, iMillSec = 0;

			sscanf(strDateTime, "%u-%u-%u %u:%u:%u", &iYear, &iMonth, &iDay, &iHour, &iMinute, &iSecond);
			return (CheckDate(iYear, iMonth, iDay) && CheckTime(iHour, iMinute, iSecond));
		}
		else
			return false;
	}
	else
	{
		return false;
	}
}

const void* CDBBindDateTime::GetData() const
{
	if (nullptr != m_pOCITimestamp)
		return m_pOCITimestamp;
	else
		return (const void*)m_strBuffer;
}

std::string CDBBindDateTime::ToString() const
{
	if (m_lDataLength > 0)
		return std::move(std::string(m_strBuffer));
	else
		return std::move(std::string(""));
}

void CDBBindDateTime::Clear(bool bFreeAll)
{
	m_lDataLength = 0;
	m_nParamIndp = -1;
	memset(m_strBuffer, '\0', m_lBufferLength);

	if (bFreeAll && nullptr != m_pOCITimestamp)
	{
		OCIDescriptorFree(m_pOCITimestamp, (ub4)OCI_DTYPE_TIMESTAMP); //绑定时生成，这里释放
		m_eOCIDateTimeType = DB_DATA_TYPE_NONE;
		m_pOCITimestamp = nullptr;
		m_pOCIEnv = nullptr;
		m_pOCIErr = nullptr;
	}
}

bool CDBBindDateTime::Init()
{
	m_pOCIEnv = nullptr;
	m_pOCIErr = nullptr;
	m_pOCITimestamp = nullptr;
	m_lBufferLength = D_DB_DATETIME_MAX_LEN + 1;
	memset(m_strBuffer, '\0', m_lBufferLength);

	return true;
}

bool CDBBindDateTime::CheckIsLeapYear(unsigned int iYear)
{
	if (0 == iYear)
	{
		return false; //年份必须大于0
	}

	if (0 == (iYear % 400))
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

bool CDBBindDateTime::CheckDate(unsigned int iYear, unsigned int iMonth, unsigned int iDay) //检查日期是否正常
{
	if (iYear > 0)
	{
		if (1 <= iMonth && iMonth <= 12 && 1 <= iDay && iDay <= 31)
		{
			switch (iMonth)
			{
			case 2:
				if (CheckIsLeapYear(iYear)) //润年有29天
				{
					if (iDay >= 30)
						return false;
				}
				else
				{
					if (iDay >= 29) //平年28天
						return false;
				}

				break;
			case 4:
			case 6:
			case 9:
			case 11:
				if (31 == iDay)
					return false;
				break;
			default:
				break;
			}

			return true;
		}
		else
			return false;
	}
	else
		return false;
}

bool CDBBindDateTime::CheckTime(unsigned int iHour, unsigned int iMinute, unsigned int iSecond) //检查时间是否正常
{
	if (iHour <= 23 && iMinute <= 59 && iMinute <= 59)
		return true;
	else
		return false;
}