#include "../Public/DBBindNumber.hpp"

CDBBindNumber::CDBBindNumber() : m_bUnsigned(false), m_eDataType(E_NUMBER_DATA_NONE), m_nScale(0) 
{ 
	Init(); 
}

CDBBindNumber::CDBBindNumber(const CDBBindNumber& Other) : m_bUnsigned(Other.m_bUnsigned), m_eDataType(Other.m_eDataType),
m_UData(Other.m_UData), m_nScale(Other.m_nScale)
{
	memset(m_strBuffer, '\0', m_lBufferLength);
	if (0 < m_lDataLength)
	{
		memcpy(m_strBuffer, Other.m_strBuffer, m_lDataLength);
	}
}

CDBBindNumber::CDBBindNumber(const char* strNumber) { Init(); *this = strNumber; }
CDBBindNumber::CDBBindNumber(const std::string& szNumber) { Init(); *this = szNumber; }
CDBBindNumber::CDBBindNumber(const int8_t& ch) { Init(); *this = ch; }
CDBBindNumber::CDBBindNumber(const uint8_t& uch) { Init(); *this = uch; }
CDBBindNumber::CDBBindNumber(const int16_t& st) { Init(); *this = st; }
CDBBindNumber::CDBBindNumber(const uint16_t& ust) { Init(); *this = ust; }
CDBBindNumber::CDBBindNumber(const int32_t& it) { Init(); *this = it; }
CDBBindNumber::CDBBindNumber(const uint32_t& uit) { Init(); *this = uit; }
CDBBindNumber::CDBBindNumber(const long& lg) { Init(); *this = lg; }
CDBBindNumber::CDBBindNumber(const unsigned long& ulg) { Init(); *this = ulg; }
CDBBindNumber::CDBBindNumber(const int64_t& ll) { Init(); *this = ll; }
CDBBindNumber::CDBBindNumber(const uint64_t& ull) { Init(); *this = ull; }
CDBBindNumber::CDBBindNumber(const float& ft) { Init(); *this = ft; }
CDBBindNumber::CDBBindNumber(const double& dt) { Init(); *this = dt; }
CDBBindNumber::~CDBBindNumber() { Clear(true); }

CDBBindNumber& CDBBindNumber::operator = (const CDBBindNumber& Other)
{
	if (&Other != this)
	{
		m_bUnsigned = Other.m_bUnsigned;
		m_eDataType = Other.m_eDataType;
		m_UData = Other.m_UData;
		m_nScale = Other.m_nScale;

		memset(m_strBuffer, '\0', m_lBufferLength);
		if (0 < m_lDataLength)
		{
			memcpy(m_strBuffer, Other.m_strBuffer, m_lDataLength);
		}
	}

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const char* strNumber)
{
	if (nullptr == strNumber)
	{
		Clear();
	}
	else
	{
		memset(&m_UData, 0, sizeof(m_UData));
		m_bUnsigned = false;
		m_lDataLength = (unsigned long)strlen(strNumber);
		m_nParamIndp = (m_lDataLength > 0 ? 0 : -1);
		memset(m_strBuffer, '\0', m_lBufferLength);

		if (m_lDataLength > 0)
		{
			//防止数据过长
			if (m_lDataLength >= m_lBufferLength)
			{
				m_lDataLength = D_DB_NUMBER_MAX_LEN;
			}

			m_nScale = ReckonScale(strNumber, m_lDataLength);
			memcpy(m_strBuffer, strNumber, m_lDataLength);
		}
	}

	m_eDataType = E_NUMBER_DATA_STRING;

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const std::string& szNumber)
{
	if (szNumber.empty())
	{
		Clear();
	}
	else
	{
		memset(&m_UData, 0, sizeof(m_UData));
		m_bUnsigned = false;
		m_lDataLength = (unsigned long)szNumber.length();
		m_nParamIndp = (m_lDataLength > 0 ? 0 : -1);
		memset(m_strBuffer, '\0', m_lBufferLength);

		if (m_lDataLength > 0)
		{
			//防止数据过长
			if (m_lDataLength >= m_lBufferLength)
			{
				m_lDataLength = D_DB_NUMBER_MAX_LEN;
			}

			m_nScale = ReckonScale(szNumber.c_str(), m_lDataLength);
			memcpy(m_strBuffer, szNumber.c_str(), m_lDataLength);
		}
	}

	m_eDataType = E_NUMBER_DATA_STRING;

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const int8_t& ch)
{
	m_bUnsigned = false;
	m_eDataType = E_NUMBER_DATA_INT8;
	m_UData._char = ch;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%hd", (short)ch);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const uint8_t& uch)
{
	m_bUnsigned = true;
	m_eDataType = E_NUMBER_DATA_INT8;
	m_UData._uchar = uch;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%hu", (unsigned short)uch);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const int16_t& st)
{
	m_bUnsigned = false;
	m_eDataType = E_NUMBER_DATA_INT16;
	m_UData._short = st;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%hd", st);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const uint16_t& ust)
{
	m_bUnsigned = true;
	m_eDataType = E_NUMBER_DATA_INT16;
	m_UData._ushort = ust;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%hu", ust);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const int32_t& it)
{
	m_bUnsigned = false;
	m_eDataType = E_NUMBER_DATA_INT32;
	m_UData._int = it;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%d", it);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const uint32_t& uit)
{
	m_bUnsigned = true;
	m_eDataType = E_NUMBER_DATA_INT32;
	m_UData._uint = uit;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%u", uit);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const long& lg)
{
	m_bUnsigned = false;
	m_eDataType = E_NUMBER_DATA_INT32;
	m_UData._int = lg;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%ld", lg);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const unsigned long& ulg)
{
	m_bUnsigned = true;
	m_eDataType = E_NUMBER_DATA_INT32;
	m_UData._uint = ulg;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%lu", ulg);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const int64_t& ll)
{
	m_bUnsigned = false;
	m_eDataType = E_NUMBER_DATA_INT64;
	m_UData._longlong = ll;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%lld", ll);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const uint64_t& ull)
{
	m_bUnsigned = true;
	m_eDataType = E_NUMBER_DATA_INT64;
	m_UData._ulonglong = ull;
	m_nParamIndp = 0;

	m_nScale = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, "%llu", ull);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const float& ft)
{
	m_bUnsigned = false;
	m_eDataType = E_NUMBER_DATA_FLOAT;
	m_UData._float = ft;
	m_nParamIndp = 0;

	if (0 == m_nScale)
		m_nScale = 3; //默认精度3

	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, ("%." + std::to_string(m_nScale) + "f").c_str(), ft);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

CDBBindNumber& CDBBindNumber::operator = (const double& de)
{
	m_bUnsigned = false;
	m_eDataType = E_NUMBER_DATA_DOUBLE;
	m_UData._double = de;
	m_nParamIndp = 0;

	if (0 == m_nScale)
		m_nScale = 5; //默认精度5

	memset(m_strBuffer, '\0', m_lBufferLength);
	snprintf(m_strBuffer, D_DB_NUMBER_MAX_LEN, ("%." + std::to_string(m_nScale) + "lf").c_str(), de);
	m_lDataLength = (unsigned long)strlen(m_strBuffer);

	return *this;
}

void* CDBBindNumber::GetDataPtr(EDBDataType eType)
{
	ConvertStrToDigit(eType);

	switch (eType)
	{
	case DB_DATA_TYPE_TINYINT:
	case DB_DATA_TYPE_YEAR:
		if (m_bUnsigned)
			return &m_UData._uchar;
		else
			return &m_UData._char;
	case DB_DATA_TYPE_SMALLINT:
		if (m_bUnsigned)
			return &m_UData._ushort;
		else
			return &m_UData._short;
	case DB_DATA_TYPE_MEDIUMINT:
	case DB_DATA_TYPE_INT:
		if (m_bUnsigned)
			return &m_UData._uint;
		else
			return &m_UData._int;
	case DB_DATA_TYPE_BIGINT:
		if (m_bUnsigned)
			return &m_UData._ulonglong;
		else
			return &m_UData._longlong;
	case DB_DATA_TYPE_FLOAT:
		return &m_UData._float;
	case DB_DATA_TYPE_FLOAT2:
	case DB_DATA_TYPE_DOUBLE:
		return &m_UData._double;
	case DB_DATA_TYPE_DECIMAL:
		return (void*)m_strBuffer;
	default:
		return nullptr;
	}
}

void* CDBBindNumber::GetDataPtr(ENumberDataType eType)
{
	switch (eType)
	{
	case E_NUMBER_DATA_INT8:
		if (m_bUnsigned)
			return &m_UData._uchar;
		else
			return &m_UData._char;
	case E_NUMBER_DATA_INT16:
		if (m_bUnsigned)
			return &m_UData._ushort;
		else
			return &m_UData._short;
	case E_NUMBER_DATA_INT32:
		if (m_bUnsigned)
			return &m_UData._uint;
		else
			return &m_UData._int;
	case E_NUMBER_DATA_INT64:
		if (m_bUnsigned)
			return &m_UData._ulonglong;
		else
			return &m_UData._longlong;
	case E_NUMBER_DATA_FLOAT:
		return &m_UData._float;
	case E_NUMBER_DATA_DOUBLE:
		return &m_UData._double;
	case E_NUMBER_DATA_STRING:
		return (void*)m_strBuffer;
	default:
		return nullptr;
	}
}

unsigned long CDBBindNumber::GetDataLength(EDBDataType eType)
{
	ConvertStrToDigit(eType);

	switch (eType)
	{
	case DB_DATA_TYPE_TINYINT:
	case DB_DATA_TYPE_YEAR:
		return 1;
	case DB_DATA_TYPE_SMALLINT:
		return 2;
	case DB_DATA_TYPE_MEDIUMINT:
	case DB_DATA_TYPE_INT:
		return 4;
	case DB_DATA_TYPE_BIGINT:
		return 8;
	case DB_DATA_TYPE_FLOAT:
		return 8;
	case DB_DATA_TYPE_FLOAT2:
	case DB_DATA_TYPE_DOUBLE:
		return 8;
	case DB_DATA_TYPE_DECIMAL:
		return m_lDataLength;
	default:
		return 0;
	}
}

unsigned long CDBBindNumber::GetDataLength(ENumberDataType eType)
{
	switch (eType)
	{
	case E_NUMBER_DATA_INT8:
		return 1;
	case E_NUMBER_DATA_INT16:
		return 2;
	case E_NUMBER_DATA_INT32:
		return 4;
	case E_NUMBER_DATA_INT64:
		return 8;
	case E_NUMBER_DATA_FLOAT:
		return 4;
	case E_NUMBER_DATA_DOUBLE:
		return 8;
	case E_NUMBER_DATA_STRING:
		return m_lDataLength;
	default:
		return 0;
	}
}

const CDBBindNumber::UDigitParm& CDBBindNumber::GetData() const
{
	return m_UData;
}

void CDBBindNumber::SetFloatScale(unsigned short scale) //设置浮点型
{
	if (scale > 6)
		m_nScale = 6; //最大支持精度6
	else
		m_nScale = scale;
}

std::string CDBBindNumber::ToString() const
{
	if (m_lDataLength > 0)
		return std::move(std::string(m_strBuffer));
	else
		return std::move(std::string(""));
}

void CDBBindNumber::Clear(bool bFreeAll)
{
	m_nParamIndp = -1;
	m_bUnsigned = false;
	m_eDataType = E_NUMBER_DATA_NONE;
	memset(&m_UData, 0, sizeof(m_UData));
	m_nScale = 0;
	m_lDataLength = 0;
	memset(m_strBuffer, '\0', m_lBufferLength);
}

bool CDBBindNumber::Init()
{
	m_nScale = 0;
	m_eDataType = E_NUMBER_DATA_NONE;
	memset(&m_UData, 0, sizeof(m_UData));
	m_lBufferLength = D_DB_NUMBER_MAX_LEN + 1;
	memset(m_strBuffer, 0, m_lBufferLength);

	return true;
}

unsigned short CDBBindNumber::ReckonScale(const char* strNumber, unsigned long length)
{
	if (nullptr == strNumber || 0 == length)
		return 0;
	else
	{
		unsigned short scale = 0;

		for (unsigned long i = 0; i < length; ++i)
		{
			if ('.' == strNumber[i])
			{
				scale = (unsigned short)(length - i - 1);
				break;
			}
		}

		return scale;
	}
}

void CDBBindNumber::ConvertStrToDigit(EDBDataType eType)
{
	//有设置字符串值才转
	if (7 == m_eDataType && m_lDataLength > 0)
	{
		switch (eType)
		{
		case DB_DATA_TYPE_TINYINT:
		case DB_DATA_TYPE_YEAR:
			m_UData._char = (char)std::strtol(m_strBuffer, nullptr, 10);
			break;
		case DB_DATA_TYPE_SMALLINT:
			m_UData._short = (short)std::strtol(m_strBuffer, nullptr, 10);
			break;
		case DB_DATA_TYPE_MEDIUMINT:
		case DB_DATA_TYPE_INT:
			m_UData._int = (int)std::strtol(m_strBuffer, nullptr, 10);
			break;
		case DB_DATA_TYPE_BIGINT:
			m_UData._longlong = std::strtoll(m_strBuffer, nullptr, 10);
			break;
		case DB_DATA_TYPE_FLOAT:
			m_UData._float = std::strtof(m_strBuffer, nullptr);
			break;
		case DB_DATA_TYPE_FLOAT2:
		case DB_DATA_TYPE_DOUBLE:
			m_UData._double = std::strtod(m_strBuffer, nullptr);
			break;
		default:
			break;
		}
	}
}