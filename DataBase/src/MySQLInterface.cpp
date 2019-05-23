#include "../include/MySQLInterface.hpp"

bool CMySQLInterface::m_bInitLibrary = false;

CMySQLInterface::SNextResult::SNextResult() : rowCount(0), colCount(0) 
{
}

CMySQLInterface::SNextResult::~SNextResult() 
{ 
	Clear(); 
}

void CMySQLInterface::SNextResult::Clear()
{
	for (CDBRowValue* &lp : data)
	{
		if (nullptr != lp)
		{
			lp->Clear();
			delete lp;
			lp = nullptr;
		}
	}

	colNames.clear();
	data.clear();
	rowCount = 0;
	colCount = 0;
}

CMySQLInterface::CMySQLInterface() : m_pResult(nullptr), m_pStmtHandle(nullptr), m_pStmtParams(nullptr), m_pStmtResult(nullptr)
{
}

CMySQLInterface::CMySQLInterface(const std::string& szServerIP, unsigned int iServerPort, const std::string& szDBName,
	const std::string& szUserName, const std::string& szPassWord, const std::string& szCharSet, unsigned int iTimeOut) : 
m_pResult(nullptr), m_lStmtParamCount(0), m_pStmtBindParm(nullptr), m_pStmtHandle(nullptr), m_pStmtParams(nullptr), 
m_pStmtResult(nullptr), CDBBaseInterface("", szServerIP, iServerPort, szDBName, szUserName, szPassWord, szCharSet, iTimeOut)
{
}

CMySQLInterface::~CMySQLInterface()
{
	FreeEnv();
}

bool CMySQLInterface::InitEnv() //初始化环境
{
	//检查连接信息
	if (m_szServerIP.empty() || m_szDBName.empty() || m_szUserName.empty() || m_szPassWord.empty())
	{
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "连接信息错误！");
		return false;
	}

	//保证线程安全
	m_pInitLock->lock();

	if (!m_bInitLibrary)
	{
		if (0 != mysql_library_init(0, NULL, NULL))
		{
			m_iErrorCode = -1;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "初始化MySQL库失败！");
			m_pInitLock->unlock();
			return false;
		}
		else
			m_bInitLibrary = true;
	}

	if (!mysql_init(&m_Handle))
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "初始化MySQL句柄失败！");
		m_pInitLock->unlock();
		return false;
	}

	m_pInitLock->unlock();

	//连接超时
	mysql_options(&m_Handle, MYSQL_OPT_CONNECT_TIMEOUT, &m_iTimeOut);

	//读超时
	int iTimeOut = 30;
	//mysql_options(&m_Handle, MYSQL_OPT_READ_TIMEOUT, &iTimeOut);

	//写超时
	//mysql_options(&m_Handle, MYSQL_OPT_WRITE_TIMEOUT, &iTimeOut);

	//自动重连
	char cFlag = 1;
	mysql_options(&m_Handle, MYSQL_OPT_RECONNECT, &cFlag);

	//使用远程连接
	cFlag = 1;
	//mysql_options(&m_Handle, MYSQL_OPT_USE_REMOTE_CONNECTION, &cFlag);

	//截断不提示，1为不提示
	cFlag = 0;
	mysql_options(&m_Handle, MYSQL_REPORT_DATA_TRUNCATION, &cFlag);

	return true;
}

void CMySQLInterface::FreeEnv() //释放资源
{
	Clear();
	Disconnect();

	m_pInitLock->lock();

	if (m_bInitLibrary)
	{
		mysql_library_end();
		m_bInitLibrary = false;
	}

	m_pInitLock->unlock();
}

bool CMySQLInterface::Connect(bool bAutoCommit) //连接数据库
{
	if (!m_bIsConnect)
	{
		m_bIsAutoCommit = bAutoCommit;

		if (!mysql_real_connect(&m_Handle, m_szServerIP.c_str(), m_szUserName.c_str(), m_szPassWord.c_str(), m_szDBName.c_str(),
			m_iServerPort, nullptr, CLIENT_BASIC_FLAGS))
		{
			SetErrorInfo();
			return false;
		}

		m_bIsConnect = true;

		//设置自动提交
		if (!m_bIsAutoCommit) //MySQL默认是自动提交
		{
			mysql_autocommit(&m_Handle, 0);
		}

		//字符集
		mysql_set_character_set(&m_Handle, m_szCharSet.c_str());
	}

	return true;
}

bool CMySQLInterface::ReConnect() //重新连接
{
	Disconnect();
	return Connect(m_bIsAutoCommit);
}

void CMySQLInterface::Disconnect() //断开连接
{
	if (m_bIsConnect)
	{
		Clear();
		mysql_close(&m_Handle);
		mysql_thread_end();
		m_bIsConnect = false;
	}
}

bool CMySQLInterface::Prepare(const std::string& szSQL, bool bIsStmt) //发送SQL
{
	if (!m_bIsConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	if (szSQL.empty())
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "SQL为空！");
		return false;
	}

	//格式化SQL
	std::string szTemp(szSQL);
	FormatSQL(szTemp);

	//清空结果数据
	Clear();

	//处理
	if (bIsStmt)
	{
		//初始化句柄
		m_pStmtHandle = mysql_stmt_init(&m_Handle);
		if (m_pStmtHandle)
		{
			//发送准备语句
			int iResult = mysql_stmt_prepare(m_pStmtHandle, szSQL.c_str(), (unsigned long)szSQL.length());
			if (0 != iResult)
			{
				SetErrorInfo(nullptr, true);
				mysql_stmt_close(m_pStmtHandle);
				m_pStmtHandle = nullptr;
				return false;
			}
			else
			{
				//获取参数个数并申请内存
				m_lStmtParamCount = mysql_stmt_param_count(m_pStmtHandle);
				if (m_lStmtParamCount >= 1)
				{
					m_pStmtParams = new MYSQL_BIND[m_lStmtParamCount];
					m_pStmtBindParm = new bool[m_lStmtParamCount] {false};
					if (!m_pStmtParams || !m_pStmtBindParm)
					{
						m_iErrorCode = -1;
						Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "申请绑定参数结构体失败！");
						mysql_stmt_close(m_pStmtHandle);
						m_pStmtHandle = nullptr;
						return false;
					}
					else
						memset(m_pStmtParams, 0, sizeof(MYSQL_BIND) * m_lStmtParamCount);
				}
			}
		}
		else
		{
			SetErrorInfo("初始化预处理句柄失败");
			return false;
		}
	}
	else
	{
		//发送执行语句
		if (0 != mysql_send_query(&m_Handle, szTemp.c_str(), (unsigned long)szTemp.length()))
		{
			SetErrorInfo();
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::BindParm(unsigned int iIndex, EDBDataType eDataType, CDBBindBase* param)
{
	return CDBBaseInterface::BindParm(iIndex, eDataType, param);
}

bool CMySQLInterface::BindParm(unsigned int iIndex, char& value, short& indp, bool bUnsigned)
{
	if (!CheckPrepareInfo(iIndex))
	{
		//预处理未完成或索引错误
		return false;
	}

	if (-1 == indp)
	{
		m_pStmtParams[iIndex].buffer_type = MYSQL_TYPE_NULL;
		m_pStmtParams[iIndex].buffer = nullptr;
		m_pStmtParams[iIndex].buffer_length = 0;
	}
	else
	{
		m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_TINY;
		m_pStmtParams[iIndex].buffer_length = (unsigned long)sizeof(value);
		m_pStmtParams[iIndex].buffer = &value;
		m_pStmtParams[iIndex].is_unsigned = bUnsigned;
	}

	m_pStmtBindParm[iIndex] = true;

	//全部参数设置完成，绑定
	if (CheckStmtParamBind())
	{
		if (0 != mysql_stmt_bind_param(m_pStmtHandle, m_pStmtParams))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::BindParm(unsigned int iIndex, short& value, short& indp, bool bUnsigned)
{
	if (!CheckPrepareInfo(iIndex))
	{
		//预处理未完成或索引错误
		return false;
	}

	if (-1 == indp)
	{
		m_pStmtParams[iIndex].buffer_type = MYSQL_TYPE_NULL;
		m_pStmtParams[iIndex].buffer = nullptr;
		m_pStmtParams[iIndex].buffer_length = 0;
	}
	else
	{
		m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_SHORT;
		m_pStmtParams[iIndex].buffer_length = (unsigned long)sizeof(value);
		m_pStmtParams[iIndex].buffer = &value;
		m_pStmtParams[iIndex].is_unsigned = bUnsigned;
	}

	m_pStmtBindParm[iIndex] = true;

	//全部参数设置完成，绑定
	if (CheckStmtParamBind())
	{
		if (0 != mysql_stmt_bind_param(m_pStmtHandle, m_pStmtParams))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::BindParm(unsigned int iIndex, int& value, short& indp, bool bUnsigned)
{
	if (!CheckPrepareInfo(iIndex))
	{
		//预处理未完成或索引错误
		return false;
	}

	if (-1 == indp)
	{
		m_pStmtParams[iIndex].buffer_type = MYSQL_TYPE_NULL;
		m_pStmtParams[iIndex].buffer = nullptr;
		m_pStmtParams[iIndex].buffer_length = 0;
	}
	else
	{
		m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_LONG;
		m_pStmtParams[iIndex].buffer_length = (unsigned long)sizeof(value);
		m_pStmtParams[iIndex].buffer = &value;
		m_pStmtParams[iIndex].is_unsigned = bUnsigned;
	}

	m_pStmtBindParm[iIndex] = true;

	//全部参数设置完成，绑定
	if (CheckStmtParamBind())
	{
		if (0 != mysql_stmt_bind_param(m_pStmtHandle, m_pStmtParams))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::BindParm(unsigned int iIndex, long long& value, short& indp, bool bUnsigned)
{
	if (!CheckPrepareInfo(iIndex))
	{
		//预处理未完成或索引错误
		return false;
	}

	if (-1 == indp)
	{
		m_pStmtParams[iIndex].buffer_type = MYSQL_TYPE_NULL;
		m_pStmtParams[iIndex].buffer = nullptr;
		m_pStmtParams[iIndex].buffer_length = 0;
	}
	else
	{
		m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_LONGLONG;
		m_pStmtParams[iIndex].buffer_length = (unsigned long)sizeof(value);
		m_pStmtParams[iIndex].buffer = &value;
		m_pStmtParams[iIndex].is_unsigned = bUnsigned;
	}

	m_pStmtBindParm[iIndex] = true;

	//全部参数设置完成，绑定
	if (CheckStmtParamBind())
	{
		if (0 != mysql_stmt_bind_param(m_pStmtHandle, m_pStmtParams))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::BindParm(unsigned int iIndex, char* value, unsigned long& value_len, unsigned long buffer_len, short& indp)
{
	if (!CheckPrepareInfo(iIndex))
	{
		//预处理未完成或索引错误
		return false;
	}

	if (nullptr == value || 0 == value_len || 0 == buffer_len || -1 == indp)
	{
		m_pStmtParams[iIndex].buffer_type = MYSQL_TYPE_NULL;
		m_pStmtParams[iIndex].buffer = nullptr;
		m_pStmtParams[iIndex].buffer_length = 0;
	}
	else
	{
		m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_VAR_STRING;
		m_pStmtParams[iIndex].buffer_length = buffer_len;
		m_pStmtParams[iIndex].length = &value_len;
		m_pStmtParams[iIndex].buffer = value;
	}

	m_pStmtBindParm[iIndex] = true;

	//全部参数设置完成，绑定
	if (CheckStmtParamBind())
	{
		if (0 != mysql_stmt_bind_param(m_pStmtHandle, m_pStmtParams))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::BindString(unsigned int iIndex, EDBDataType eStrType, CDBBindString* str) //绑定预处理char/varchar参数
{
	if (!CheckPrepareInfo(iIndex))
	{
		//预处理未完成或索引错误
		return false;
	}

	//绑定参数
	if (nullptr == str)
	{
		m_pStmtParams[iIndex].buffer_type = MYSQL_TYPE_NULL;
		m_pStmtParams[iIndex].buffer = nullptr;
		m_pStmtParams[iIndex].buffer_length = 0;
	}
	else
	{
		if (DB_DATA_TYPE_CHAR == eStrType)
			m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_STRING;
		else if (DB_DATA_TYPE_VCHAR == eStrType)
			m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_VAR_STRING;
		else
		{
			m_iErrorCode = -1;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "此方法只支持char及varchar类型绑定！");
			return false;
		}

		m_pStmtParams[iIndex].buffer_length = str->m_lBufferLength;
		m_pStmtParams[iIndex].length = &str->m_lDataLength;
		m_pStmtParams[iIndex].buffer = str->m_strBuffer;
	}

	m_pStmtBindParm[iIndex] = true;

	//全部参数设置完成，绑定
	if (CheckStmtParamBind())
	{
		if (0 != mysql_stmt_bind_param(m_pStmtHandle, m_pStmtParams))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::BindNumber(unsigned int iIndex, EDBDataType eNumberType, CDBBindNumber* number) //绑定预处理数字参数
{
	if (!CheckPrepareInfo(iIndex))
	{
		//预处理未完成或索引错误
		return false;
	}

	//绑定参数
	if (nullptr == number)
	{
		m_pStmtParams[iIndex].buffer_type = MYSQL_TYPE_NULL;
		m_pStmtParams[iIndex].buffer = nullptr;
		m_pStmtParams[iIndex].buffer_length = 0;
	}
	else
	{
		switch (eNumberType)
		{
		case DB_DATA_TYPE_TINYINT:
		case DB_DATA_TYPE_YEAR:
			if (DB_DATA_TYPE_TINYINT == eNumberType)
				m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_TINY;
			else
				m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_YEAR;

			m_pStmtParams[iIndex].buffer_length = number->GetDataLength(eNumberType);
			m_pStmtParams[iIndex].buffer = number->GetDataPtr(eNumberType);
			m_pStmtParams[iIndex].is_unsigned = number->m_bUnsigned;

			break;
		case DB_DATA_TYPE_SMALLINT:
			m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_SHORT;
			m_pStmtParams[iIndex].buffer_length = number->GetDataLength(DB_DATA_TYPE_SMALLINT);
			m_pStmtParams[iIndex].buffer = number->GetDataPtr(eNumberType);
			m_pStmtParams[iIndex].is_unsigned = number->m_bUnsigned;

			break;
		case DB_DATA_TYPE_MEDIUMINT:
		case DB_DATA_TYPE_INT:
			if (DB_DATA_TYPE_MEDIUMINT == eNumberType)
				m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_INT24;
			else
				m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_LONG;
				
			m_pStmtParams[iIndex].buffer_length = number->GetDataLength(DB_DATA_TYPE_INT);
			m_pStmtParams[iIndex].buffer = number->GetDataPtr(eNumberType);
			m_pStmtParams[iIndex].is_unsigned = number->m_bUnsigned;

			break;
		case DB_DATA_TYPE_BIGINT:
			m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_LONGLONG;
			m_pStmtParams[iIndex].buffer_length = number->GetDataLength(DB_DATA_TYPE_BIGINT);
			m_pStmtParams[iIndex].buffer = number->GetDataPtr(eNumberType);
			m_pStmtParams[iIndex].is_unsigned = number->m_bUnsigned;

			break;
		case DB_DATA_TYPE_FLOAT:
			m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_FLOAT;
			m_pStmtParams[iIndex].buffer_length = number->GetDataLength(DB_DATA_TYPE_FLOAT);
			m_pStmtParams[iIndex].buffer = number->GetDataPtr(eNumberType);
			m_pStmtParams[iIndex].is_unsigned = number->m_bUnsigned;

			break;
		case DB_DATA_TYPE_DOUBLE:
			m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_DOUBLE;
			m_pStmtParams[iIndex].buffer_length = number->GetDataLength(DB_DATA_TYPE_DOUBLE);
			m_pStmtParams[iIndex].buffer = number->GetDataPtr(eNumberType);
			m_pStmtParams[iIndex].is_unsigned = number->m_bUnsigned;

			break;
		case DB_DATA_TYPE_DECIMAL:
			m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_NEWDECIMAL;
			m_pStmtParams[iIndex].buffer_length = number->m_lBufferLength;
			m_pStmtParams[iIndex].length = &number->m_lDataLength;
			m_pStmtParams[iIndex].buffer = number->m_strBuffer;

			break;
		default:
			m_iErrorCode = -1;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%d.", "此方法未支持的绑定类型", (int)eNumberType);
			return false;
		}
	}

	m_pStmtBindParm[iIndex] = true;

	//全部参数设置完成，绑定
	if (CheckStmtParamBind())
	{
		if (0 != mysql_stmt_bind_param(m_pStmtHandle, m_pStmtParams))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::BindDateTime(unsigned int iIndex, EDBDataType eDateTimeType, CDBBindDateTime* datetime) //绑定预处理Sdatetime参数
{
	if (!CheckPrepareInfo(iIndex))
	{
		//预处理未完成或索引错误
		return false;
	}

	//绑定参数
	if (nullptr == datetime)
	{
		m_pStmtParams[iIndex].buffer_type = MYSQL_TYPE_NULL;
		m_pStmtParams[iIndex].buffer = nullptr;
		m_pStmtParams[iIndex].buffer_length = 0;
	}
	else
	{
		if (DB_DATA_TYPE_DATETIME == eDateTimeType
			|| DB_DATA_TYPE_DATE == eDateTimeType
			|| DB_DATA_TYPE_TIME == eDateTimeType
			|| DB_DATA_TYPE_TIMESTAMP == eDateTimeType)
		{
			m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_VAR_STRING;
			m_pStmtParams[iIndex].buffer_length = datetime->m_lBufferLength;
			m_pStmtParams[iIndex].length = &datetime->m_lDataLength;
			m_pStmtParams[iIndex].buffer = datetime->m_strBuffer;
		}
		else
		{
			m_iErrorCode = -1;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "此方法只支持日期时间相关类型绑定！");
			return false;
		}
	}

	m_pStmtBindParm[iIndex] = true;

	//全部参数设置完成，绑定
	if (CheckStmtParamBind())
	{
		if (0 != mysql_stmt_bind_param(m_pStmtHandle, m_pStmtParams))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::BindLob(unsigned int iIndex, EDBDataType eLobType, CDBBindLob* lob) //绑定预处理lob参数
{
	if (!CheckPrepareInfo(iIndex))
	{
		//预处理未完成或索引错误
		return false;
	}

	//绑定参数
	if (nullptr == lob)
	{
		m_pStmtParams[iIndex].buffer_type = MYSQL_TYPE_NULL;
		m_pStmtParams[iIndex].buffer = nullptr;
		m_pStmtParams[iIndex].buffer_length = 0;
	}
	else
	{
		if (DB_DATA_TYPE_BLOB == eLobType 
			|| DB_DATA_TYPE_CLOB == eLobType
			|| DB_DATA_TYPE_BFILE == eLobType)
		{
			if (lob->m_lDataLength <= 255)
				m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_TINY_BLOB;
			else if (lob->m_lDataLength <= 65536) //64K
				m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_BLOB;
			else if (lob->m_lDataLength <= 16777216) //16M
				m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_MEDIUM_BLOB;
			else //4G
				m_pStmtParams[iIndex].buffer_type = FIELD_TYPE_LONG_BLOB;

			m_pStmtParams[iIndex].buffer_length = lob->m_lBufferLength;
			m_pStmtParams[iIndex].length = &lob->m_lDataLength;
			m_pStmtParams[iIndex].buffer = lob->m_pBuffer;
		}
		else
		{
			m_iErrorCode = -1;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "此方法只支持LOB相关类型绑定！");
			return false;
		}
	}

	m_pStmtBindParm[iIndex] = true;

	//全部参数设置完成，绑定
	if (CheckStmtParamBind())
	{
		if (0 != mysql_stmt_bind_param(m_pStmtHandle, m_pStmtParams))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::Execute(CDBResultSet* pResultSet, bool bIsStmt) //执行SQL
{
	if (!m_bIsConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	//处理
	if (bIsStmt)
	{
		if (nullptr == m_pStmtHandle)
		{
			m_iErrorCode = -1;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "准备语句未发送！");
			return false;
		}

		//检查参数绑定情况
		if (!CheckStmtParamBind(true))
		{
			//里面已经设置错误信息
			return false;
		}

		//执行
		if (0 != mysql_stmt_execute(m_pStmtHandle))
		{
			SetErrorInfo(nullptr, true);
			return false;
		}

		//保存结果到本地
		int iRet = mysql_stmt_store_result(m_pStmtHandle);
		if (0 != iRet)
		{
			SetErrorInfo("保存结果到本地失败", true);
			return false;
		}

		//获取列信息
		m_pResult = mysql_stmt_result_metadata(m_pStmtHandle);
		if (!m_pResult)
		{
			m_iErrorCode = D_DB_NO_DATA;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");
		}
		else
		{
			//获取结果
			if (pResultSet)
			{
				//有结果才处理
				unsigned long long llRowCount = mysql_stmt_num_rows(m_pStmtHandle);
				if (llRowCount > 0)
				{
					pResultSet->SetIsOutParamResult((m_Handle.server_status & SERVER_PS_OUT_PARAMS) > 0);
					pResultSet->SetRowCount(llRowCount);

					unsigned int iColumnCount = mysql_num_fields(m_pResult);
					m_pStmtResult = new MYSQL_BIND[iColumnCount];
					if (!m_pStmtResult)
					{
						m_iErrorCode = -1;
						Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "申请绑定结果结构体失败！");
						return false;
					}
					else
						memset(m_pStmtResult, 0, sizeof(MYSQL_BIND) * iColumnCount);

					//绑定结果参数
					if (!BindResultInfo(pResultSet, true))
					{
						//里面已经设置错误信息
						return false;
					}

					if (0 != mysql_stmt_bind_result(m_pStmtHandle, m_pStmtResult))
					{
						SetErrorInfo("绑定结果集失败", true);
						return false;
					}
				}
				else
				{
					m_iErrorCode = D_DB_NO_DATA;
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");
					return false;
				}
			}
		}
	}
	else
	{
		//执行并获取结果
		if (0 != mysql_read_query_result(&m_Handle))
		{
			SetErrorInfo();
			return false;
		}
		else
		{
			//获取结果信息
			m_pResult = mysql_store_result(&m_Handle);
			if (!m_pResult)
			{
				m_iErrorCode = D_DB_NO_DATA;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");
			}
			else
			{
				if (pResultSet)
				{
					//获取结果行数
					unsigned long long llRouCount = mysql_num_rows(m_pResult);

					//设置结果行数
					if (llRouCount > 0)
					{
						pResultSet->SetRowCount(llRouCount);
						return BindResultInfo(pResultSet);
					}
					else
					{
						m_iErrorCode = D_DB_NO_DATA;
						Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");
						return false;
					}
				}
			}
		}
	}
	
	return true;
}

bool CMySQLInterface::ExecuteNoParam(const std::string& szSQL, CDBResultSet* pResultSet) //执行SQL，没有绑定参数
{
	if (!m_bIsConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	if (szSQL.empty())
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "SQL为空！");
		return false;
	}

	//格式化SQL
	std::string szTemp(szSQL);
	FormatSQL(szTemp);

	//清空结果数据
	Clear();

	//执行并获取结果
	if (0 != mysql_real_query(&m_Handle, szTemp.c_str(), (unsigned long)szTemp.length()))
	{
		SetErrorInfo();
		return false;
	}
	else
	{
		m_pResult = mysql_store_result(&m_Handle);
		if (!m_pResult)
		{
			m_iErrorCode = D_DB_NO_DATA;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");
		}
		else
		{
			//获取结果信息
			if (pResultSet)
			{
				//获取结果行数
				unsigned long long llRouCount = mysql_num_rows(m_pResult);

				//设置结果行数
				if (llRouCount > 0)
				{
					pResultSet->SetRowCount(llRouCount);
					return BindResultInfo(pResultSet);
				}
				else
				{
					m_iErrorCode = D_DB_NO_DATA;
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");
					return false;
				}
			}
		}
	}

	return true;
}

bool CMySQLInterface::ExecuteDirect(const std::string& szSQL) //执行SQL，没有返回结果的
{
	if (!m_bIsConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	if (szSQL.empty())
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "SQL为空！");
		return false;
	}

	//格式化SQL
	std::string szTemp(szSQL);
	FormatSQL(szTemp);

	//检查语句类型
	std::string::size_type iPos = szTemp.find("select ");
	if (std::string::npos == iPos)
	{
		iPos = szTemp.find("SELECT ");

		if (std::string::npos == iPos)
		{
			iPos = szTemp.find("call ");

			if (std::string::npos == iPos)
			{
				iPos = szTemp.find("CALL ");
			}
		}
	}

	if (std::string::npos != iPos && 0 == iPos)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的语句类型！");
		return false;
	}

	//清空结果数据
	Clear();

	//执行并获取结果
	if (0 != mysql_real_query(&m_Handle, szTemp.c_str(), (unsigned long)szTemp.length()))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

long long CMySQLInterface::AffectedRows(bool bIsStmt) //受影响行数
{
	if (!m_bIsConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return -1;
	}

	if (bIsStmt)
	{
		if (nullptr == m_pStmtHandle)
		{
			m_iErrorCode = -1;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "准备语句未发送！");
			return -1;
		}

		return (long long)mysql_stmt_num_rows(m_pStmtHandle);
	}
	else
	{
		return mysql_affected_rows(&m_Handle);
	}
}

/******************************************************************
功能：	执行存储过程
参数：	szExecuteSQL 存储过程SQL，如："call SP_TEST(100,@1,@2)"
*		setParamSQLList inout参数，如："set @1 = 'xx'"
*		szResultSQL 获取结果SQL，如："select @1, @2"
*		pResultRowValue 返回结果行，不要结果置NULL
返回：	成功返回ture
******************************************************************/
bool CMySQLInterface::ExecuteProcedure(const std::string& szExecuteSQL, const std::list<std::string>* setParamSQLList,
	const char* szResultSQL, CDBRowValue* pResultRowValue) //执行存储过程，out参数由结果集返回
{
	if (!m_bIsConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	if (szExecuteSQL.empty())
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "执行存储过程SQL为空！");
		return false;
	}

	//清空结果数据
	Clear();
	int iResult = 0;

	//设置变量
	if (setParamSQLList)
	{
		for (const auto& parmSQL : *setParamSQLList)
		{
			iResult = mysql_real_query(&m_Handle, parmSQL.c_str(), (unsigned long)parmSQL.length());
			if (0 != iResult)
			{
				SetErrorInfo("设置参数失败");
				return false;
			}
		}
	}

	//执行存储过程
	iResult = mysql_real_query(&m_Handle, szExecuteSQL.c_str(), (unsigned long)szExecuteSQL.length());
	if (0 != iResult)
	{
		SetErrorInfo("执行存储过程失败");
		return false;
	}

	//可能有其它输出结果
	GetProcdureNextResult();

	//获取结果
	if (szResultSQL)
	{
		//查询结果
		iResult = mysql_real_query(&m_Handle, szResultSQL, (unsigned long)strlen(szResultSQL));
		if (0 != iResult)
		{
			SetErrorInfo("查询返回结果失败");
			return false;
		}

		//获取结果集
		m_pResult = mysql_store_result(&m_Handle);
		if (!m_pResult)
		{
			m_iErrorCode = D_DB_NO_DATA;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");

			if (pResultRowValue)
			{
				return false;
			}
		}
		else
		{
			//获取值
			if (pResultRowValue)
			{
				//获取结果行数
				unsigned long long llRouCount = mysql_num_rows(m_pResult);

				if (llRouCount > 0)
				{
					//获取行
					MYSQL_ROW pResultRow = mysql_fetch_row(m_pResult);
					if (!pResultRow) //结果行为空，判断是否有错误发生
					{
						SetErrorInfo("获取结果行失败");
						return false;
					}


					unsigned int iFieldCount = mysql_num_fields(m_pResult);
					pResultRowValue->Clear();
					pResultRowValue->CreateColumn(iFieldCount);

					for (unsigned int i = 0; i < iFieldCount; ++i)
					{
						CDBColumValue* pValue = new CDBColumValue();
						pResultRowValue->AddColumnValue(pValue, i);
						pResultRowValue->SetValue(i, pResultRow[i]);
					}
				}
				else
				{
					m_iErrorCode = D_DB_NO_DATA;
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");
					return false;
				}
			}
		}
	}

	return true;
}

const std::vector<CMySQLInterface::SNextResult*>& CMySQLInterface::GetProcdureNextResult() const //获取执行ExecuteProcedure可能的隐式结果
{
	return m_NextResult;
}

bool CMySQLInterface::Commit() //提交，0成功
{
	if (0 != mysql_commit(&m_Handle))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool CMySQLInterface::Rollback() //回滚
{
	if (0 != mysql_rollback(&m_Handle))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

unsigned long long CMySQLInterface::AffectedRows() //影响行数
{
	if (m_pStmtHandle)
		return mysql_stmt_affected_rows(m_pStmtHandle);
	else
		return mysql_affected_rows(&m_Handle);
}

bool CMySQLInterface::Ping() //检查连接是否可到达
{
	return (0 == mysql_ping(&m_Handle));
}

std::string CMySQLInterface::GetServerVersion() //获取服务器版本信息字符串
{
	const char* info = mysql_get_server_info(&m_Handle);

	if (info)
		return std::move(std::string(info));
	else
	{
		SetErrorInfo();
		return std::move(std::string(""));
	}
}

bool CMySQLInterface::BindResultInfo(CDBResultSet* pResultSet, bool bIsStmt) //获取结果信息，列名，长度，类型等
{
	if (!pResultSet || !m_pResult)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果集参数不能为空！");
		return false;
	}

	unsigned int iFieldCount = mysql_num_fields(m_pResult);
	pResultSet->SetDBInterface(this);

	pResultSet->CreateColumnAttr(iFieldCount);
	CDBRowValue* pRowValue = new CDBRowValue(iFieldCount);
	pResultSet->SetRowValue(pRowValue);

	for (unsigned int i = 0; i < iFieldCount; ++i)
	{
		CDBColumAttribute* pColumnAttribute = new CDBColumAttribute();

		pColumnAttribute->SetColumnName(m_pResult->fields[i].name);
		pColumnAttribute->m_iFieldNameLen = m_pResult->fields[i].name_length;
		pColumnAttribute->m_bNullAble = (0 == IS_NOT_NULL(m_pResult->fields[i].flags));
		pColumnAttribute->m_iFieldDataLen = m_pResult->fields[i].length; //字段长度，(char,varchar)跟字符集有关，如GBK，则为真实长度的2倍
		pColumnAttribute->m_iPrecision = m_pResult->fields[i].max_length; //该字段所查询结果中，最长值
		pColumnAttribute->m_nScale = m_pResult->fields[i].decimals; //精度

		//数据类型
		switch (m_pResult->fields[i].type)
		{
		case FIELD_TYPE_DECIMAL:
		case FIELD_TYPE_NEWDECIMAL:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_DECIMAL;
			break;
		case FIELD_TYPE_STRING:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_CHAR;
			break;
		case FIELD_TYPE_VAR_STRING:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_VCHAR;
			break;
		case FIELD_TYPE_TINY:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_TINYINT;
			break;
		case FIELD_TYPE_SHORT:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_SMALLINT;
			break;
		case FIELD_TYPE_INT24:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_MEDIUMINT;
			break;
		case FIELD_TYPE_LONG:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_INT;
			break;
		case FIELD_TYPE_LONGLONG:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_BIGINT;
			break;
		case FIELD_TYPE_YEAR:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_YEAR;
			break;
		case FIELD_TYPE_DATE:
		case FIELD_TYPE_NEWDATE:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_DATE;
			break;
		case FIELD_TYPE_TIME:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_TIME;
			break;
		case FIELD_TYPE_DATETIME:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_DATETIME;
			break;
		case FIELD_TYPE_TIMESTAMP:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_TIMESTAMP;
			break;
		case FIELD_TYPE_FLOAT:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_FLOAT;
			break;
		case FIELD_TYPE_DOUBLE:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_DOUBLE;
			break;
		case FIELD_TYPE_BLOB: //blob和text，要判断
			{
				if (BINARY_FLAG & m_pResult->fields[i].flags)
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_BLOB;
				else
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_CLOB;
			}
			break;
		default:
			pColumnAttribute->m_eDataType = DB_DATA_TYPE_VCHAR; //默认当字符串处理
			break;
		}

		CDBColumValue* pValue = nullptr;

		if (bIsStmt)
		{
			if (pColumnAttribute->m_iFieldDataLen > D_DB_LOB_RETURN_MAX_LEN)
				pColumnAttribute->m_iFieldDataLen = D_DB_LOB_RETURN_MAX_LEN;

			pValue = new CDBColumValue(pColumnAttribute->m_iFieldDataLen);
		}
		else
		{
			//设置为查询数据中的最大长度
			pValue = new CDBColumValue(pColumnAttribute->m_iPrecision + 1);
		}

		if (pValue)
		{
			if (bIsStmt)
			{
				if (m_pStmtResult)
				{
					m_pStmtResult[i].buffer_type = MYSQL_TYPE_VAR_STRING; //全部使用str接收
					m_pStmtResult[i].buffer_length = pColumnAttribute->m_iFieldDataLen;
					m_pStmtResult[i].buffer = pValue->m_pData;
					m_pStmtResult[i].length = &pValue->m_lDataLen;
					m_pStmtResult[i].is_null = &pValue->m_bIsNull;
				}
				else
				{
					m_iErrorCode = -1;
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "绑定结果集为空！");
					pValue->Clear();
					delete pValue;
					pResultSet->Clear();
					return false;
				}
			}

			pRowValue->AddColumnValue(pValue, i);
			pResultSet->AddColumnAttr(pColumnAttribute, i);
		}
		else
		{
			m_iErrorCode = -1;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "申请列值对象失败！");
			pResultSet->Clear();
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::Fetch(CDBRowValue* &pRowValue, bool bIsStmt) //获取下一行
{
	bool bResult = true;

	if (!m_pResult) //有结果集才获取
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有存储结果集！");
		return false;
	}

	if (bIsStmt)
	{
		int iRet = mysql_stmt_fetch(m_pStmtHandle);
		if (0 != iRet)
		{
			if (MYSQL_NO_DATA == iRet)
			{
				m_iErrorCode = D_DB_NO_DATA;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有结果行！");
			}
			else
				SetErrorInfo(nullptr, true);

			bResult = false;
		}
	}
	else
	{
		MYSQL_ROW pResultRow = mysql_fetch_row(m_pResult);

		if (!pResultRow) //结果行为空，判断是否有错误发生
		{
			SetErrorInfo();
			if (0 == m_iErrorCode)
			{
				//没有数据了
				m_iErrorCode = D_DB_NO_DATA;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有结果行！");
			}

			bResult = false;
		}
		else
		{
			//设置数据
			for (unsigned int i = 0; i < pRowValue->GetColnumCount(); ++i)
			{
				pRowValue->SetValue(i, pResultRow[i]);
			}
		}
	}
	
	return bResult;
}

bool CMySQLInterface::GetNextResult(CDBResultSet* pResultSet, bool bIsStmt) //获取另一个结果集
{
	if (!pResultSet)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果集参数为空！");
		return false;
	}

	//释放绑定结果
	ClearData();
	pResultSet->Clear();

	if (bIsStmt)
	{
		int iRet = mysql_stmt_next_result(m_pStmtHandle);
		if (0 == iRet) //-1是没有结果了
		{
			//保存结果到本地
			iRet = mysql_stmt_store_result(m_pStmtHandle);
			if (0 != iRet)
			{
				SetErrorInfo("保存结果到本地失败", true);
				return false;
			}

			pResultSet->SetIsOutParamResult((m_Handle.server_status & SERVER_PS_OUT_PARAMS) > 0);

			//获取列信息
			m_pResult = mysql_stmt_result_metadata(m_pStmtHandle);
			if (!m_pResult)
			{
				//经测试，没有结果iRet也是0，调用者要通过errCode判断
				m_iErrorCode = D_DB_NO_DATA;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有结果行！");
				return false;
			}

			//有结果才处理
			unsigned long long llRowCount = mysql_stmt_num_rows(m_pStmtHandle);
			if (llRowCount > 0)
			{
				pResultSet->SetRowCount(llRowCount);

				unsigned int iColumnCount = mysql_num_fields(m_pResult);
				m_pStmtResult = new MYSQL_BIND[iColumnCount];
				if (!m_pStmtResult)
				{
					m_iErrorCode = -1;
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "申请绑定结果结构体失败！");
					return false;
				}
				else
					memset(m_pStmtResult, 0, sizeof(MYSQL_BIND) * iColumnCount);

				//绑定结果参数
				if (!BindResultInfo(pResultSet, true))
				{
					//里面已经设置错误信息
					return false;
				}

				if (0 != mysql_stmt_bind_result(m_pStmtHandle, m_pStmtResult))
				{
					SetErrorInfo("绑定结果集失败", true);
					return false;
				}
			}
		}
		else //调用者要通过errCode判断是否出错
		{
			if (-1 == iRet)
			{
				m_iErrorCode = D_DB_NO_DATA;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有结果行！");
			}
			else
				SetErrorInfo(nullptr, true);

			return false;
		}
	}
	else
	{
		//调用者要通过errCode判断是否出错
		if (0 == mysql_next_result(&m_Handle))
		{
			m_pResult = mysql_store_result(&m_Handle);
			if (nullptr == m_pResult)
			{
				m_iErrorCode = D_DB_NO_DATA;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有结果行！");
				return false;
			}
			else
			{
				//获取结果行数
				unsigned long long llRouCount = mysql_num_rows(m_pResult);

				//设置结果行数
				if (llRouCount > 0)
				{
					pResultSet->SetRowCount(llRouCount);
					return BindResultInfo(pResultSet);
				}
			}
		}
		else
		{
			m_iErrorCode = D_DB_NO_DATA;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有结果行！");
			return false;
		}
	}

	return true;
}

bool CMySQLInterface::CheckStmtParamBind(bool bIsSetErrorInfo) //检查预处理SQL参数绑定情况
{
	for (unsigned long i = 0; i < m_lStmtParamCount; ++i)
	{
		if (false == m_pStmtBindParm[i])
		{
			if (bIsSetErrorInfo)
			{
				m_iErrorCode = -1;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%d未绑定！", "预处理参数下标", i);
			}

			return false;
		}
	}

	return true;
}

bool CMySQLInterface::CheckPrepareInfo(unsigned int iIndex) //检查预处理情况
{
	if (nullptr == m_pStmtParams)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "Prepare方法未调用！");
		return false;
	}

	if (iIndex >= m_lStmtParamCount)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "参数索引错误！");
		return false;
	}

	return true;
}

void CMySQLInterface::GetProcdureNextResult() //获取其它的结果集
{
	MYSQL_RES* pRes = mysql_store_result(&m_Handle);
	while (nullptr != pRes)
	{
		my_ulonglong iRowCount = mysql_num_rows(pRes);
		if (iRowCount > 0)
		{
			SNextResult *pNextRes = new SNextResult();
			pNextRes->rowCount = iRowCount;
			pNextRes->colCount = mysql_num_fields(pRes);

			//获取字段名
			for (unsigned int i = 0; i < pNextRes->colCount; ++i)
			{
				if (nullptr != pRes->fields[i].name)
					pNextRes->colNames.push_back(pRes->fields[i].name);
				else
					pNextRes->colNames.push_back(std::to_string(i) + "_null");
			}

			//获取数据
			MYSQL_ROW pResultRow = mysql_fetch_row(pRes);
			while (nullptr != pResultRow)
			{
				CDBRowValue* pRow = new CDBRowValue(pNextRes->colCount);

				for (unsigned int i = 0; i < pNextRes->colCount; ++i)
				{
					CDBColumValue* pValue = new CDBColumValue();
					pRow->AddColumnValue(pValue, i);
					pRow->SetValue(i, pResultRow[i]);
				}

				pNextRes->data.push_back(pRow);
				pResultRow = mysql_fetch_row(pRes);
			}

			m_NextResult.push_back(pNextRes);
		}

		mysql_free_result(pRes);
		pRes = nullptr;

		//获取下一个结果集
		if (0 == mysql_next_result(&m_Handle))
		{
			pRes = mysql_store_result(&m_Handle);
		}
	}
}

bool CMySQLInterface::SetSqlState(bool bIsStmt) //设置SQL执行状态
{
	memset(m_strSqlState, '\0', MAX_SQL_STATE_LEN);

	if (bIsStmt)
	{
		if (m_pStmtHandle)
			Format(m_strSqlState, MAX_SQL_STATE_LEN, "%s", mysql_stmt_sqlstate(m_pStmtHandle));
		else
		{
			memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "预处理句柄为空！");
			return false;
		}
	}
	else
		Format(m_strSqlState, MAX_SQL_STATE_LEN, "%s", mysql_sqlstate(&m_Handle));

	return true;
}

bool CMySQLInterface::SetErrorInfo(const char* pAddInfo, bool bIsStmt) //设置错误信息
{
	m_iErrorCode = 0;
	memset(m_strSqlState, '\0', MAX_SQL_STATE_LEN);
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);

	if (bIsStmt)
	{
		if (m_pStmtHandle)
		{
			m_iErrorCode = mysql_stmt_errno(m_pStmtHandle);
			Format(m_strSqlState, MAX_SQL_STATE_LEN, "%s", mysql_stmt_sqlstate(m_pStmtHandle));

			if (nullptr == pAddInfo)
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", mysql_stmt_error(m_pStmtHandle));
			else
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%s", pAddInfo, mysql_stmt_error(m_pStmtHandle));
		}
		else
		{
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "预处理句柄为空！");
			return false;
		}
	}
	else
	{
		m_iErrorCode = mysql_errno(&m_Handle);
		Format(m_strSqlState, MAX_SQL_STATE_LEN, "%s", mysql_sqlstate(&m_Handle));

		if (nullptr == pAddInfo)
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", mysql_error(&m_Handle));
		else
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%s", pAddInfo, mysql_error(&m_Handle));
	}

	return true;
}

void CMySQLInterface::ClearData(bool bClearAll) //清除结果数据
{
	m_lStmtParamCount = 0;
	if (m_pStmtBindParm)
	{
		delete[] m_pStmtBindParm;
		m_pStmtBindParm = nullptr;
	}

	if (m_pResult)
	{
		mysql_free_result(m_pResult);
		m_pResult = nullptr;
	}

	if (bClearAll)
	{
		for (SNextResult* nr : m_NextResult)
		{
			if (nullptr != nr)
			{
				nr->Clear();
				delete nr;
				nr = nullptr;
			}
		}

		m_NextResult.clear();
	}

	if (m_pStmtHandle)
	{
		mysql_stmt_free_result(m_pStmtHandle);
	}

	if (m_pStmtParams)
	{
		delete[] m_pStmtParams;
		m_pStmtParams = nullptr;
	}

	if (m_pStmtResult)
	{
		delete[] m_pStmtResult;
		m_pStmtResult = nullptr;
	}

	m_iErrorCode = 0;
	memset(m_strSqlState, '\0', MAX_SQL_STATE_LEN);
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);


	if (bClearAll)
	{
		//调用存储过程可能产生多个结果集
		while (0 == mysql_next_result(&m_Handle))
		{
			m_pResult = mysql_store_result(&m_Handle);

			if (m_pResult)
			{
				mysql_free_result(m_pResult);
				m_pResult = nullptr;
			}
			else
				break;
		}

		if (m_pStmtHandle)
		{
			while (0 == mysql_stmt_next_result(m_pStmtHandle))
			{
				if (0 == mysql_stmt_store_result(m_pStmtHandle))
				{
					mysql_stmt_free_result(m_pStmtHandle);
				}
			}
		}
	}
}

void CMySQLInterface::Clear() //清除所有中间数据，包括临时打开的句柄
{
	//清理数据
	ClearData(true);

	//关闭句柄
	if (m_pStmtHandle)
	{
		mysql_stmt_close(m_pStmtHandle);
		m_pStmtHandle = nullptr;
	}
}

void CMySQLInterface::Test() //测试使用
{

}