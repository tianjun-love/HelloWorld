#include "../include/DBBaseInterface.hpp"
#include <regex>

#ifdef _WIN32
CDBBaseInterface::Snprintf CDBBaseInterface::Format = _snprintf;
#else
CDBBaseInterface::Snprintf CDBBaseInterface::Format = snprintf;
#endif

std::mutex *CDBBaseInterface::m_pInitLock = new std::mutex();

CDBBaseInterface::CDBBaseInterface() : m_iServerPort(0), m_bConnectState(false), m_bIsAutoCommit(false), m_iConnTimeOut(5), 
m_iErrorCode(0)
{
	memset(m_strSqlState, '\0', MAX_SQL_STATE_LEN);
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);
}

CDBBaseInterface::CDBBaseInterface(const std::string& szServerName, const std::string& szServerIP, unsigned int iServerPort, 
	const std::string& szDBName, const std::string& szUserName, const std::string& szPassWord, bool bAutoCommit, 
	EDB_CHARACTER_SET eCharSet, unsigned int iConnTimeOut) : m_szServerName(szServerName), m_szServerIP(szServerIP), 
m_iServerPort(iServerPort), m_szDBName(szDBName), m_szUserName(szUserName), m_szPassWord(szPassWord), 
m_bIsAutoCommit(bAutoCommit), m_eCharSet(eCharSet), m_iConnTimeOut(iConnTimeOut), m_bConnectState(false), m_iErrorCode(0)
{
	memset(m_strSqlState, '\0', MAX_SQL_STATE_LEN);
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);
}

CDBBaseInterface::~CDBBaseInterface()
{
}

void CDBBaseInterface::SetServerName(const std::string& szServerName) //设置服务名
{
	m_szServerName = szServerName;
}
void CDBBaseInterface::SetServerIP(const std::string& szIP)           //设置数据库IP
{
	m_szServerIP = szIP;
}
void CDBBaseInterface::SetServerPort(unsigned int iPort)              //设置数据库端口
{
	m_iServerPort = iPort;
}
void CDBBaseInterface::SetDBName(const std::string& szDBName)         //设置数据库名
{
	m_szDBName = szDBName;
}
void CDBBaseInterface::SetUserName(const std::string& szUserName)     //设置用户名
{
	m_szUserName = szUserName;
}
void CDBBaseInterface::SetPassWord(const std::string& szPassWord)     //设置密码
{
	m_szPassWord = szPassWord;
}
void CDBBaseInterface::SetCharSet(EDB_CHARACTER_SET eCharSet)         //设置字符集
{
	m_eCharSet = eCharSet;
}
void CDBBaseInterface::SetConnTimeOut(unsigned int iConnTimeOut)  //设置超时
{
	m_iConnTimeOut = iConnTimeOut;
}

const std::string& CDBBaseInterface::GetServerName() const       //获取服务名
{
	return m_szServerName;
}
const std::string& CDBBaseInterface::GetServerIP() const         //获取数据库IP
{
	return m_szServerIP;
}
const unsigned int& CDBBaseInterface::GetServerPort() const      //获取数据库端口
{
	return m_iServerPort;
}
const std::string& CDBBaseInterface::GetDBName() const           //获取数据库名
{
	return m_szDBName;
}
const std::string& CDBBaseInterface::GetUserName() const         //获取用户名
{
	return m_szUserName;
}
const std::string& CDBBaseInterface::GetPassWord() const         //获取密码
{
	return m_szPassWord;
}
EDB_CHARACTER_SET CDBBaseInterface::GetCharSet() const           //获取字符集
{
	return m_eCharSet;
}
const unsigned int& CDBBaseInterface::GetConnTimeOut() const     //获取超时
{
	return m_iConnTimeOut;
}

bool CDBBaseInterface::SetIsAutoCommit(bool bIsAutoCommit)      //设置是否自动提交
{
	m_bIsAutoCommit = bIsAutoCommit;

	return true;
}

bool CDBBaseInterface::GetIsAutoCommit()                        //获取是否自动提交
{
	return m_bIsAutoCommit;
}

bool CDBBaseInterface::GetConnectState()                        //获取是否已经连接
{
	return m_bConnectState;
}

EDB_CHARACTER_SET CDBBaseInterface::ConvertCharacterStringToEnum(const std::string& szCharacter) //字符集串转枚举
{
	EDB_CHARACTER_SET eCharacter = E_CHARACTER_UTF8;

	if ("gbk" == szCharacter)
		eCharacter = E_CHARACTER_GBK;
	else if ("big5" == szCharacter)
		eCharacter = E_CHARACTER_BIG5;
	else if ("gk18030" == szCharacter)
		eCharacter = E_CHARACTER_GK18030;
	else if ("latin1" == szCharacter)
		eCharacter = E_CHARACTER_LATIN1;
	else if ("ascii" == szCharacter)
		eCharacter = E_CHARACTER_ASCII;

	return eCharacter;
}

std::string CDBBaseInterface::ConvertCharacterEnumToString(EDB_CHARACTER_SET eCharacter) //字符集枚举转字符串
{
	std::string szCharacter;

	switch (eCharacter)
	{
	case E_CHARACTER_GBK:
		szCharacter = "gbk";
		break;
	case E_CHARACTER_GK18030:
		szCharacter = "gk18030";
		break;
	case E_CHARACTER_BIG5:
		szCharacter = "big5";
		break;
	case E_CHARACTER_ASCII:
		szCharacter = "ascii";
		break;
	case E_CHARACTER_LATIN1:
		szCharacter = "latin1";
		break;
	default:
		szCharacter = "utf8";
		break;
	}

	return std::move(szCharacter);
}

void CDBBaseInterface::FormatSQL(std::string& szSQL)            //格式化SQL，tab符及制表符换成空格，oracle否则可能出错
{
	for (std::string::size_type Pos = 0; Pos < szSQL.length(); ++Pos)
	{
		if ('\r' == szSQL[Pos] || '\n' == szSQL[Pos] || '\t' == szSQL[Pos] || '\v' == szSQL[Pos] || '\f' == szSQL[Pos])
		{
			szSQL[Pos] = ' ';
		}
	}

	TrimString(szSQL);

	//结尾不是;则加上
	if (!szSQL.empty() && ';' != szSQL[szSQL.length() - 1])
		szSQL += ";";
}

std::string CDBBaseInterface::Replace(const std::string& szSrc, const std::string& szOldStr, const std::string& szNewStr) //替换字符串中的特定字符串
{
	if (szSrc.empty())
	{
		return std::string("");
	}

	std::string szTemp(szSrc);
	std::string::size_type iBegin = 0, iEnd = 0;

	iEnd = szTemp.find(szOldStr, iBegin);
	while (std::string::npos != iEnd)
	{
		szTemp.replace(iEnd, szOldStr.length(), szNewStr);

		iBegin = iEnd + szNewStr.length();
		iEnd = szTemp.find(szOldStr, iBegin);
	}

	return std::move(szTemp);
}

bool CDBBaseInterface::MatchFormat(const std::string& szMatchStr, const char* strFormat) //使用正则表达式检查字符串格式
{
	if (!szMatchStr.empty() && nullptr != strFormat)
	{
		try //如果输入的正则表达式格式不正确，会发生异常
		{
			const std::regex patten(strFormat);
			return std::regex_match(szMatchStr, patten);
		}
		catch (...)
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool CDBBaseInterface::TrimString(std::string& szStr, short type)
{
	bool bResult = true;

	if (!szStr.empty())
	{
		try
		{
			//去除左边
			if (type <= 0)
				szStr.erase(szStr.begin(), std::find_if(szStr.begin(), szStr.end(), std::not1(std::ptr_fun(::isspace))));

			//去除右边
			if (type >= 0)
				szStr.erase(std::find_if(szStr.rbegin(), szStr.rend(), std::not1(std::ptr_fun(::isspace))).base(), szStr.end());
		}
		catch (...)
		{
			bResult = false;
		}
	}

	return bResult;
}

bool CDBBaseInterface::ConvertBinaryToHex(const unsigned char* binary, uint64_t binary_len, std::string& szOutHex) //二进制数据转十六进制字符串
{
	szOutHex.clear();

	if (nullptr == binary && 0 == binary_len)
		return true;
	else if (nullptr == binary || 0 == binary_len)
		return false;

	uint64_t n = 0, iStrLen = binary_len * 2 + 1;
	char *pStrTemp = new char[iStrLen];
	memset(pStrTemp, '\0', iStrLen);

	for (uint64_t i = 0; i < binary_len; ++i)
	{
		n += sprintf(pStrTemp + n, "%02X", binary[i]);
	}

	szOutHex = pStrTemp;
	delete[] pStrTemp;

	return true;
}

bool CDBBaseInterface::ConvertHexToBinary(const std::string& szInHex, unsigned char* binary, uint64_t& binary_len) //十六进制字符串转二进制数据
{
	if (nullptr == binary || 0 == binary_len)
		return false;

	if (szInHex.length() / 2 > binary_len)
		return false;

	memset(binary, 0, binary_len);
	binary_len = 0;

	if (szInHex.empty())
		return true;

	for (std::string::size_type i = 0; i < szInHex.length();)
	{
		unsigned char cTemp = 0;
		
		if (ConvertHexToChar(szInHex[i], szInHex[i + 1], cTemp))
		{
			binary[binary_len++] = cTemp;
			i += 2;
		}
		else
			return false;

		//防止字符数是单数
		if (i + 1 == szInHex.length())
		{
			ConvertHexToChar(szInHex[i], '0', cTemp);
			binary[binary_len++] = cTemp;
			break;
		}
	}

	return true;
}

bool CDBBaseInterface::ConvertHexToChar(const char& chB, const char& chE, unsigned char& chOut) //将两位十六进制字符转成char
{
	chOut = 0;

	if (chB >= '0' && chB <= '9')
		chOut = chB - '0';
	else if (chB >= 'a' && chB <= 'f')
		chOut = chB - 'a' + 10;
	else if (chB >= 'A' && chB <= 'F')
		chOut = chB - 'A' + 10;
	else
		return false;

	if (chE >= '0' && chE <= '9')
		chOut = (chOut << 4) + chE - '0';
	else if (chE >= 'a' && chE <= 'f')
		chOut = (chOut << 4) + chE - 'a' + 10;
	else if (chE >= 'A' && chE <= 'F')
		chOut = (chOut << 4) + chE - 'A' + 10;
	else
		return false;

	return true;
}

const char* CDBBaseInterface::GetSqlState() const  //获取SQL状态
{
	return m_strSqlState;
}

const int& CDBBaseInterface::GetErrorCode() const  //获取错误代码
{
	return m_iErrorCode;
}

const char* CDBBaseInterface::GetErrorInfo() const //获取错误信息
{
	return m_strErrorBuf;
}

bool CDBBaseInterface::BindParm(unsigned int iIndex, EDBDataType eDataType, CDBBindBase* param) //绑定预处理参数
{
	switch (eDataType)
	{
	case DB_DATA_TYPE_NONE:
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未指定绑定参数类型！");
		return false;
	case DB_DATA_TYPE_CHAR:
	case DB_DATA_TYPE_VCHAR:
	case DB_DATA_TYPE_RAW:
	{
		if (nullptr == param)
		{
			return BindString(iIndex, eDataType, nullptr);
		}
		else
		{
			CDBBindString* pTemp = dynamic_cast<CDBBindString*>(param);
			if (nullptr != pTemp)
				return BindString(iIndex, eDataType, pTemp);
			else
			{
				m_iErrorCode = -1;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "绑定参数类型错误，应为CDBBindString！");
				return false;
			}
		}
	}
	case DB_DATA_TYPE_TINYINT:
	case DB_DATA_TYPE_SMALLINT:
	case DB_DATA_TYPE_MEDIUMINT:
	case DB_DATA_TYPE_INT:
	case DB_DATA_TYPE_BIGINT:
	case DB_DATA_TYPE_FLOAT:
	case DB_DATA_TYPE_FLOAT2:
	case DB_DATA_TYPE_DOUBLE:
	case DB_DATA_TYPE_YEAR:
	case DB_DATA_TYPE_DECIMAL:
	{
		if (nullptr == param)
		{
			return BindNumber(iIndex, eDataType, nullptr);
		}
		else
		{
			CDBBindNumber* pTemp = dynamic_cast<CDBBindNumber*>(param);
			if (nullptr != pTemp)
				return BindNumber(iIndex, eDataType, pTemp);
			else
			{
				m_iErrorCode = -1;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "绑定参数类型错误，应为CDBBindNumber！");
				return false;
			}
		}
	}
	case DB_DATA_TYPE_TIME:
	case DB_DATA_TYPE_DATE:
	case DB_DATA_TYPE_DATETIME:
	case DB_DATA_TYPE_TIMESTAMP:
	{
		if (nullptr == param)
		{
			return BindDateTime(iIndex, eDataType, nullptr);
		}
		else
		{
			CDBBindDateTime* pTemp = dynamic_cast<CDBBindDateTime*>(param);
			if (nullptr != pTemp)
				return BindDateTime(iIndex, eDataType, pTemp);
			else
			{
				m_iErrorCode = -1;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "绑定参数类型错误，应为CDBBindDateTime！");
				return false;
			}
		}
	}
	case DB_DATA_TYPE_CLOB:
	case DB_DATA_TYPE_BLOB:
	case DB_DATA_TYPE_BFILE:
	{
		if (nullptr == param)
		{
			return BindLob(iIndex, eDataType, nullptr);
		}
		else
		{
			CDBBindLob* pTemp = dynamic_cast<CDBBindLob*>(param);
			if (nullptr != pTemp)
				return BindLob(iIndex, eDataType, pTemp);
			else
			{
				m_iErrorCode = -1;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "绑定参数类型错误，应为CDBBindLob！");
				return false;
			}
		}
	}
	default:
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%d！", "未知的绑定参数类型", (int)eDataType);
		return false;
	}
}