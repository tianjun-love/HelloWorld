#include "../include/DMDPIInterface.hpp"

CDMDPIInterface::CDMDPIInterface() : m_pEnv(nullptr), m_pCon(nullptr), m_pStmt(nullptr), m_nBindCount(0), 
m_pBindRow(nullptr), m_pBindType(nullptr), m_pBindInd(nullptr)
{
}

CDMDPIInterface::CDMDPIInterface(const std::string& szServerIP, unsigned int iServerPort, const std::string& szUserName, 
	const std::string& szPassWord, bool bAutoCommit, EDB_CHARACTER_SET eCharSet, unsigned int iTimeOut) : 
CDBBaseInterface("", szServerIP, iServerPort, "", szUserName, szPassWord, bAutoCommit, eCharSet, iTimeOut), 
m_pEnv(nullptr), m_pCon(nullptr), m_pStmt(nullptr), m_nBindCount(0), m_pBindRow(nullptr), 
m_pBindType(nullptr), m_pBindInd(nullptr)
{
}

CDMDPIInterface::~CDMDPIInterface()
{
	FreeEnv();
}

bool CDMDPIInterface::InitEnv() //初始化环境
{
	bool bResult = true;
	DPIRETURN rt = 0;

	//检查连接信息
	if (m_szServerIP.empty() || m_szUserName.empty() || m_szPassWord.empty())
	{
		snprintf(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "连接信息为空！");
		return false;
	}

	// 申请环境句柄
	rt = dpi_alloc_env(&m_pEnv);
	if (DSQL_SUCCESS == rt || DSQL_SUCCESS_WITH_INFO == rt)
	{
		//设置字符集
		switch (m_eCharSet)
		{
		case E_CHARACTER_UTF8:
			rt = dpi_set_env_attr(m_pEnv, DSQL_ATTR_LOCAL_CODE, (dpointer)PG_UTF8, 0);
			break;
		case E_CHARACTER_GBK:
			rt = dpi_set_env_attr(m_pEnv, DSQL_ATTR_LOCAL_CODE, (dpointer)PG_GBK, 0);
			break;
		case E_CHARACTER_GK18030:
			rt = dpi_set_env_attr(m_pEnv, DSQL_ATTR_LOCAL_CODE, (dpointer)PG_GB18030, 0);
			break;
		case E_CHARACTER_BIG5:
			rt = dpi_set_env_attr(m_pEnv, DSQL_ATTR_LOCAL_CODE, (dpointer)PG_BIG5, 0);
			break;
		case E_CHARACTER_ASCII:
			rt = dpi_set_env_attr(m_pEnv, DSQL_ATTR_LOCAL_CODE, (dpointer)PG_SQL_ASCII, 0);
			break;
		case E_CHARACTER_LATIN1:
			rt = dpi_set_env_attr(m_pEnv, DSQL_ATTR_LOCAL_CODE, (dpointer)PG_ISO_8859_1, 0);
			break;
		default:
			rt = dpi_set_env_attr(m_pEnv, DSQL_ATTR_LOCAL_CODE, (dpointer)PG_UTF8, 0);
			break;
		}

		if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
		{
			bResult = false;
			SetErrorInfo(DSQL_HANDLE_ENV, m_pEnv);
		}
			
		//设置语言
		if (bResult)
		{
			rt = dpi_set_env_attr(m_pEnv, DSQL_ATTR_LANG_ID, (dpointer)LANGUAGE_CN, 0);

			if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
			{
				bResult = false;
				SetErrorInfo(DSQL_HANDLE_ENV, m_pEnv);
			}
		}

		//申请连接句柄
		if (bResult)
		{
			rt = dpi_alloc_con(m_pEnv, &m_pCon);
			if (DSQL_SUCCESS == rt || DSQL_SUCCESS_WITH_INFO == rt)
			{
				//设置自动提交
				rt = dpi_set_con_attr(m_pCon, DSQL_ATTR_AUTOCOMMIT,
					(m_bIsAutoCommit ? (dpointer)DSQL_AUTOCOMMIT_ON : (dpointer)DSQL_AUTOCOMMIT_OFF), 0);

				if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
				{
					bResult = false;
					SetErrorInfo(DSQL_HANDLE_DBC, m_pCon);
				}

				//设置连接超时
				if (bResult)
				{
					rt = dpi_set_con_attr(m_pCon, DSQL_ATTR_CONNECTION_TIMEOUT, (dpointer)m_iConnTimeOut, 0);

					if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
					{
						bResult = false;
						SetErrorInfo(DSQL_HANDLE_DBC, m_pCon);
					}
				}
			}
			else
			{
				bResult = false;
				SetErrorInfo(DSQL_HANDLE_ENV, m_pEnv);
			}
		}
	}
	else
	{
		bResult = false;
		m_iErrorCode = rt;
		snprintf(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "申请环境句柄失败！");
	}

	return bResult;
}

void CDMDPIInterface::FreeEnv() //释放资源
{
	Clear();
	Disconnect();

	if (m_pCon)
	{
		dpi_free_con(m_pCon);
		m_pCon = nullptr;
	}

	if (m_pEnv)
	{
		dpi_free_env(m_pEnv);
		m_pEnv = nullptr;
	}
}

bool CDMDPIInterface::Connect() //连接数据库
{
	bool bResult = true;

	if (!m_bConnectState)
	{
		if (m_pEnv && m_pCon)
		{
			std::string szServer = m_szServerIP + ":" + std::to_string(m_iServerPort);
			DPIRETURN rt = 0;
			
			rt = dpi_login(m_pCon, (sdbyte*)szServer.c_str(), (sdbyte*)m_szUserName.c_str(), (sdbyte*)m_szPassWord.c_str());

			if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
			{
				bResult = false;
				SetErrorInfo(DSQL_HANDLE_DBC, m_pCon);
			}
		}
		else
		{
			bResult = false;
			m_iErrorCode = -1;
			snprintf(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未初始化或初始化失败环境及连接句柄！");
		}
	}

	return bResult;
}

bool CDMDPIInterface::ReConnect() //重新连接
{
	Clear();
	Disconnect();

	return Connect();
}

void CDMDPIInterface::Disconnect() //断开连接
{
	if (m_bConnectState)
	{
		if (m_pCon)
			dpi_logout(m_pCon);

		m_bConnectState = false;
	}
}

bool CDMDPIInterface::Prepare(const std::string& szSQL, bool bIsStmt)            //发送SQL
{
	if (szSQL.empty())
	{
		m_iErrorCode = -1;
		snprintf(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "SQL为空！");
		return false;
	}

	//清空结果数据
	ClearData(true);

	DPIRETURN rt = 0;
	std::string szTemp(szSQL);
	FormatSQL(szTemp);

	//申请语句句柄
	rt = dpi_alloc_stmt(m_pCon, &m_pStmt);
	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_DBC, m_pCon);
		return false;
	}

	//准备语句
	rt = dpi_prepare(m_pStmt, (sdbyte*)szTemp.c_str());
	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}
	else
	{
		//获取要绑定的参数个数
		rt = dpi_number_params(m_pStmt, &m_nBindCount);
		if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
		{
			SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
			return false;
		}
		else
		{
			if (m_nBindCount > 0)
			{
				//创建绑定参数
				m_pBindRow = new void*[m_nBindCount] { nullptr };

				//创建绑定类型
				m_pBindType = new EDBDataType[m_nBindCount] { DB_DATA_TYPE_NONE };

				//创建绑定参数长度
				m_pBindInd = new slength[m_nBindCount] { -1 };
			}
		}
	}

	return true;
}

bool CDMDPIInterface::BindParm(unsigned int iIndex, char& value, short& indp, bool bUnsigned)
{
	if (nullptr == m_pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "Prepare方法未调用！");
		return false;
	}

	//检查索引
	if (!CheckBindIndex(iIndex))
	{
		return false;
	}

	if (nullptr == m_pBindRow[iIndex - 1])
	{
		m_pBindRow[iIndex - 1] = new char{ value };
		m_pBindType[iIndex - 1] = DB_DATA_TYPE_CHAR;
		m_pBindInd[iIndex - 1] = 1;
	}
	else
		*((char*)(m_pBindRow[iIndex - 1])) = value;

	//绑定
	DPIRETURN rt = dpi_bind_param(m_pStmt, (udint2)iIndex, DSQL_PARAM_INPUT, DSQL_C_CHAR, DSQL_TINYINT, 1, 0, 
		&value, 1, &m_pBindInd[iIndex - 1]);

	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}

	return true;
}

bool CDMDPIInterface::BindParm(unsigned int iIndex, short& value, short& indp, bool bUnsigned)
{
	if (nullptr == m_pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "Prepare方法未调用！");
		return false;
	}

	//检查索引
	if (!CheckBindIndex(iIndex))
	{
		return false;
	}

	if (nullptr == m_pBindRow[iIndex - 1])
	{
		m_pBindRow[iIndex - 1] = new short{ value };
		m_pBindType[iIndex - 1] = DB_DATA_TYPE_SMALLINT;
		m_pBindInd[iIndex - 1] = 2;
	}
	else
		((short*)(m_pBindRow[iIndex - 1]))[0] = value;

	//绑定
	DPIRETURN rt = dpi_bind_param(m_pStmt, (udint2)iIndex, DSQL_PARAM_INPUT, (bUnsigned ? DSQL_C_USHORT : DSQL_C_SSHORT), 
		DSQL_SMALLINT, 2, 0, &value, 2, &m_pBindInd[iIndex - 1]);

	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}

	return true;
}

bool CDMDPIInterface::BindParm(unsigned int iIndex, int& value, short& indp, bool bUnsigned)
{
	if (nullptr == m_pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "Prepare方法未调用！");
		return false;
	}

	//检查索引
	if (!CheckBindIndex(iIndex))
	{
		return false;
	}

	if (nullptr == m_pBindRow[iIndex - 1])
	{
		m_pBindRow[iIndex - 1] = new int{ value };
		m_pBindType[iIndex - 1] = DB_DATA_TYPE_INT;
		m_pBindInd[iIndex - 1] = 4;
	}
	else
		((int*)(m_pBindRow[iIndex - 1]))[0] = value;

	//绑定
	DPIRETURN rt = dpi_bind_param(m_pStmt, (udint2)iIndex, DSQL_PARAM_INPUT, (bUnsigned ? DSQL_C_ULONG : DSQL_C_SLONG), 
		DSQL_INT, 4, 0, &value, 4, &m_pBindInd[iIndex - 1]);

	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}

	return true;
}

bool CDMDPIInterface::BindParm(unsigned int iIndex, int64_t& value, short& indp, bool bUnsigned)
{
	if (nullptr == m_pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "Prepare方法未调用！");
		return false;
	}

	//检查索引
	if (!CheckBindIndex(iIndex))
	{
		return false;
	}

	if (nullptr == m_pBindRow[iIndex - 1])
	{
		m_pBindRow[iIndex - 1] = new int64_t{ value };
		m_pBindType[iIndex - 1] = DB_DATA_TYPE_BIGINT;
		m_pBindInd[iIndex - 1] = 8;
	}
	else
		((int64_t*)(m_pBindRow[iIndex - 1]))[0] = value;

	//绑定
	DPIRETURN rt = dpi_bind_param(m_pStmt, (udint2)iIndex, DSQL_PARAM_INPUT, (bUnsigned ? DSQL_C_UBIGINT : DSQL_C_SBIGINT), 
		DSQL_INT, 8, 0, &value, 8, &m_pBindInd[iIndex - 1]);

	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}

	return true;
}

bool CDMDPIInterface::BindParm(unsigned int iIndex, char* value, unsigned long& value_len, unsigned long buffer_len, short &indp)
{
	if (nullptr == m_pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "Prepare方法未调用！");
		return false;
	}

	//检查索引
	if (!CheckBindIndex(iIndex))
	{
		return false;
	}

	if (nullptr == m_pBindRow[iIndex - 1])
	{
		m_pBindRow[iIndex - 1] = new char[value_len + 1] { '\0' };
		m_pBindType[iIndex - 1] = DB_DATA_TYPE_VCHAR;
		m_pBindInd[iIndex - 1] = value_len;

		memcpy(m_pBindRow[iIndex - 1], value, value_len);
	}
	else
	{
		m_pBindInd[iIndex - 1] = value_len;

		if ((slength)value_len <= m_pBindInd[iIndex - 1])
			memcpy(m_pBindRow[iIndex - 1], value, value_len);
		else
		{
			delete[] m_pBindRow[iIndex - 1];

			m_pBindRow[iIndex - 1] = new char[value_len + 1] { '\0' };
			memcpy(m_pBindRow[iIndex - 1], value, value_len);
		}
	}

	//绑定
	DPIRETURN rt = dpi_bind_param(m_pStmt, (udint2)iIndex, DSQL_PARAM_INPUT, DSQL_C_NCHAR, DSQL_VARCHAR, (ulength)value_len + 1, 0, 
		&value, (slength)value_len + 1, &m_pBindInd[iIndex - 1]);

	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}

	return true;
}

bool CDMDPIInterface::BindString(unsigned int iIndex, EDBDataType eStrType, CDBBindString* str) //绑定预处理char/varchar参数
{
	//暂未实现
	//dpi_bind_param();

	return false;
}

bool CDMDPIInterface::BindNumber(unsigned int iIndex, EDBDataType eNumberType, CDBBindNumber* number) //绑定预处理数字参数
{
	//暂未实现
	//dpi_bind_param();

	return false;
}

bool CDMDPIInterface::BindDateTime(unsigned int iIndex, EDBDataType eDateTimeType, CDBBindDateTime* datetime) //绑定预处理datetime参数
{
	if (nullptr == m_pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "Prepare方法未调用！");
		return false;
	}

	//检查索引
	if (!CheckBindIndex(iIndex))
	{
		return false;
	}

	return false;
}

bool CDMDPIInterface::BindLob(unsigned int iIndex, EDBDataType eLobType, CDBBindLob* lob) //绑定预处理lob参数
{
	//暂未实现
	//dpi_bind_param();

	return false;
}

bool CDMDPIInterface::Execute(CDBResultSet* pResultSet, bool bIsStmt)            //执行SQL
{
	if (!m_bConnectState)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	if (nullptr == m_pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "准备语句未发送！");
		return false;
	}

	//直接执行
	DPIRETURN rt = dpi_exec(m_pStmt);
	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}
	else
	{
		if (pResultSet)
		{
			sdint8 affected_rows = 0;
			rt = dpi_row_count(m_pStmt, &affected_rows);

			if (affected_rows > 0)
			{
				pResultSet->SetRowCount(affected_rows);
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

	return true;
}

bool CDMDPIInterface::ExecuteNoParam(const std::string& szSQL, CDBResultSet* pResultSet) //执行SQL
{
	if (szSQL.empty())
	{
		m_iErrorCode = -1;
		snprintf(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "SQL为空！");
		return false;
	}

	//清空结果数据
	ClearData(true);

	DPIRETURN rt = 0;
	std::string szTemp(szSQL);
	FormatSQL(szTemp);

	//申请语句句柄
	rt = dpi_alloc_stmt(m_pCon, &m_pStmt);
	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_DBC, m_pCon);
		return false;
	}

	//直接执行
	rt = dpi_exec_direct(m_pStmt, (sdbyte*)szTemp.c_str());
	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}
	else
	{
		if (pResultSet)
		{
			sdint8 affected_rows = 0;
			DPIRETURN rt = dpi_row_count(m_pStmt, &affected_rows);

			if (affected_rows > 0)
			{
				pResultSet->SetRowCount(affected_rows);
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
	
	return true;
}

int64_t CDMDPIInterface::AffectedRows(bool bIsStmt) //影响行数
{
	DPIRETURN rt = 0;
	sdint8 affected_rows = 0;

	rt = dpi_row_count(m_pStmt, &affected_rows);
	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return -1;
	}

	return affected_rows;
}

bool CDMDPIInterface::Commit() //提交，0成功
{
	DPIRETURN rt = dpi_commit(m_pCon);

	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_DBC, m_pCon);
		return false;
	}

	return true;
}

bool CDMDPIInterface::Rollback() //回滚
{
	DPIRETURN rt = dpi_rollback(m_pCon);

	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		SetErrorInfo(DSQL_HANDLE_DBC, m_pCon);
		return false;
	}

	return true;
}

bool CDMDPIInterface::Ping() //检查连接是否可到达
{
	m_iErrorCode = -1;
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);
	snprintf(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");

	return false;
}

bool CDMDPIInterface::SetIsAutoCommit(bool bIsAutoCommit) //设置是否自动提交
{
	//设置自动提交
	DPIRETURN rt = dpi_set_con_attr(m_pCon, DSQL_ATTR_AUTOCOMMIT,
		(bIsAutoCommit ? (dpointer)DSQL_AUTOCOMMIT_ON : (dpointer)DSQL_AUTOCOMMIT_OFF), 0);

	if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
	{
		CDBBaseInterface::SetIsAutoCommit(!bIsAutoCommit);
		SetErrorInfo(DSQL_HANDLE_DBC, m_pCon);

		return false;
	}
	else
		m_bIsAutoCommit = bIsAutoCommit;

	return true;
}

bool CDMDPIInterface::SetErrorInfo(const char* pAddInfo, bool bIsStmt)
{
	//不支持这个方法
	return false;
}

bool CDMDPIInterface::SetErrorInfo(sdint2 hndl_type, dhandle hndl) //获取错误信息
{
	bool bRet = true;
	DPIRETURN rt = 0;
	sdbyte msg[SDBYTE_MAX] = { '\0' };
	sdint2 rec_num = 1, msg_len = 0;
	int copy_index = 0;

	m_iErrorCode = 0;
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);

	//获取错误信息集合
	do
	{
		rt = dpi_get_diag_rec(hndl_type, hndl, rec_num, &m_iErrorCode, msg, (sdint2)SDBYTE_MAX, &msg_len);

		if (DSQL_SUCCESS == rt || DSQL_SUCCESS_WITH_INFO == rt)
		{
			rec_num++;

			if (copy_index == 0)
			{
				memcpy(m_strErrorBuf, msg, msg_len);
				copy_index = msg_len;
			}
			else
			{
				if ((copy_index + msg_len + 2) < MAX_ERROR_INFO_LEN)
				{
					m_strErrorBuf[copy_index] = ',';
					copy_index++;
					memcpy(m_strErrorBuf + copy_index, msg, msg_len);
					copy_index += msg_len;
				}
				else
					break;
			}
		}
		else if (DSQL_ERROR == rt)
		{
			bRet = false;
			m_iErrorCode = -1;
			snprintf(m_strErrorBuf, MAX_ERROR_INFO_LEN, "诊断信息索引号：%hd 错误或错误信息buff长度小于等于0！", rec_num);
		}
		else if (DSQL_INVALID_HANDLE == rt)
		{
			bRet = false;
			m_iErrorCode = -1;
			snprintf(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "获取错误信息失败：无效的句柄！");
		}

	} while (DSQL_SUCCESS == rt);

	return bRet;
}

bool CDMDPIInterface::BindResultInfo(CDBResultSet *pResultSet, bool bIsStmt) //绑定结果集
{
	if (NULL == pResultSet)
	{
		m_iErrorCode = -1;
		snprintf(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果集地址为空！");
		return false;
	}
	else
		pResultSet->SetDBInterface(this);

	//获取结果集field数量
	sdint2 col_cnt = 0;
	DPIRETURN rt = dpi_number_columns(m_pStmt, &col_cnt);
	if (DSQL_SUCCESS == rt || DSQL_SUCCESS_WITH_INFO == rt)
	{
		//没有数据直接返回
		if (0 == col_cnt)
			return true;

		//取所有列描述
		sdbyte  ColumnName[SDBYTE_MAX];  //字段名称
		sdint2  ColumnNameLength;        //名称长度
		sdint2  DataType;                //内部数据类型
		ulength DataSize;                //数据最大长度
		sdint2  Scale;                   //小数点个数
		sdint2  IsNULL;                  //是否可以为null

		CDBRowValue *pRowValue = new CDBRowValue(col_cnt);
		pResultSet->CreateColumnAttr(col_cnt);
		pResultSet->SetRowValue(pRowValue);

		m_nBindCount = col_cnt;
		m_pBindRow = new void*[col_cnt] { nullptr };
		m_pBindType = new EDBDataType[col_cnt];
		m_pBindInd = new slength[col_cnt] { 0 };

		for (sdint2 iPos = 0; iPos < col_cnt; ++iPos)
		{
			//DataType表示字段的数据库类型，即：DSQL_XX，列号从1开始
			rt = dpi_desc_column(m_pStmt, iPos + 1, ColumnName, (sdint2)SDBYTE_MAX, &ColumnNameLength, &DataType, &DataSize,
				&Scale, &IsNULL);

			if (DSQL_SUCCESS == rt || DSQL_SUCCESS_WITH_INFO == rt)
			{
				CDBColumAttribute *pColumnAttribute = new CDBColumAttribute();

				pColumnAttribute->SetColumnName((const char*)ColumnName, ColumnNameLength);
				pColumnAttribute->m_iFieldDataLen = (unsigned long)DataSize;
				pColumnAttribute->m_iPrecision = (unsigned int)DataSize;
				pColumnAttribute->m_nScale = Scale;
				pColumnAttribute->m_bNullAble = ((DSQL_NULLABLE == IsNULL) ? true : false);

				switch (DataType)
				{
				case DSQL_CHAR:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_CHAR;
					break;
				case DSQL_VARCHAR:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_VCHAR;
					break;
				case DSQL_TINYINT:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_TINYINT;
					break;
				case DSQL_SMALLINT:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_SMALLINT;
					break;
				case DSQL_INT:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_INT;
					break;
				case DSQL_BIGINT:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_BIGINT;
					break;
				case DSQL_DEC:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_DECIMAL;
					break;
				case DSQL_FLOAT:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_FLOAT;
					break;
				case DSQL_DOUBLE:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_DOUBLE;
					break;
				case DSQL_DATE:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_DATE;
					break;
				case DSQL_TIME:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_TIME;
					break;
				case DSQL_TIMESTAMP:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_TIMESTAMP;
					break;
				case DSQL_BINARY:
				case DSQL_BIT:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_RAW;
					break;
				case DSQL_BLOB:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_BLOB;
					pColumnAttribute->m_iFieldDataLen = 1024 * 1024; //暂时支持1M
					break;
				case DSQL_CLOB:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_CLOB;
					pColumnAttribute->m_iFieldDataLen = 1024 * 1024; //暂时支持1M
					break;
				case DSQL_BFILE:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_BFILE;
					break;
				default:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_VCHAR;
					break;
				}

				pResultSet->AddColumnAttr(pColumnAttribute, iPos);
				CDBColumValue *pValue = new CDBColumValue();
				pRowValue->AddColumnValue(pValue, iPos);

				//绑定返回数据
				void* pTemp = nullptr;

				if (pColumnAttribute->m_eDataType == DB_DATA_TYPE_DATE)
					pTemp = (void*)new dpi_date_t();
				else if (pColumnAttribute->m_eDataType == DB_DATA_TYPE_TIME)
					pTemp = (void*)new dpi_time_t();
				else if (pColumnAttribute->m_eDataType == DB_DATA_TYPE_DATETIME
					|| pColumnAttribute->m_eDataType == DB_DATA_TYPE_TIMESTAMP)
					pTemp = (void*)new dpi_timestamp_t();
				else if (pColumnAttribute->m_eDataType == DB_DATA_TYPE_BLOB)
					pTemp = (dhloblctr)nullptr;
				else
					pTemp = new char[pColumnAttribute->m_iFieldDataLen + 1]{ '\0' };

				m_pBindRow[iPos] = pTemp;
				m_pBindType[iPos] = pColumnAttribute->m_eDataType;

				if (pColumnAttribute->m_eDataType == DB_DATA_TYPE_DATE)
					rt = dpi_bind_col(m_pStmt, iPos + 1, DSQL_C_DATE, (dpointer)pTemp, (slength)sizeof(dpi_date_t), &m_pBindInd[iPos]);
				else if (pColumnAttribute->m_eDataType == DB_DATA_TYPE_TIME)
					rt = dpi_bind_col(m_pStmt, iPos + 1, DSQL_C_TIME, (dpointer)pTemp, (slength)sizeof(dpi_time_t), &m_pBindInd[iPos]);
				else if (pColumnAttribute->m_eDataType == DB_DATA_TYPE_DATETIME
					|| pColumnAttribute->m_eDataType == DB_DATA_TYPE_TIMESTAMP)
					rt = dpi_bind_col(m_pStmt, iPos + 1, DSQL_C_TIMESTAMP, (dpointer)pTemp, (slength)sizeof(dpi_timestamp_t), &m_pBindInd[iPos]);
				else if (pColumnAttribute->m_eDataType == DB_DATA_TYPE_BLOB)
					rt = dpi_bind_col(m_pStmt, iPos + 1, DSQL_C_LOB_HANDLE, (dpointer)&pTemp, (slength)sizeof(dhloblctr), &m_pBindInd[iPos]);
				else
				{
					rt = dpi_bind_col(m_pStmt, iPos + 1, DSQL_C_NCHAR, (dpointer)pTemp, (slength)(pColumnAttribute->m_iFieldDataLen + 1),
						&m_pBindInd[iPos]);
				}

				if (DSQL_SUCCESS != rt && DSQL_SUCCESS_WITH_INFO != rt)
				{
					SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
					return false;
				}
			}
			else
			{
				SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
				return false;
			}
		}
	}
	else
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}

	return true;
}

bool CDMDPIInterface::Fetch(CDBRowValue* &pRowValue, bool bIsStmt) //获取下一行
{
	bool bResult = true;
	ulength row_num = 0;

	//没有结果集，返回
	if (!pRowValue)
		return true;

	DPIRETURN rt = dpi_fetch(m_pStmt, &row_num);
	if (DSQL_SUCCESS == rt || DSQL_SUCCESS_WITH_INFO == rt)
	{
		char buff[24] = { '\0' };

		for (sdint2 i = 0; i < m_nBindCount; ++i)
		{
			if (m_pBindInd[i] <= 0)
			{
				//空值处理
				pRowValue->SetValue(i, nullptr);
			}
			else
			{
				switch (m_pBindType[i])
				{
				case DB_DATA_TYPE_DATE:
				{
					const dpi_date_t* pDate = (dpi_date_t*)m_pBindRow[i];

					snprintf(buff, sizeof(buff), "%4d-%02d-%02d", pDate->year, pDate->month, pDate->day);
					pRowValue->SetValue(i, buff);
				}
				break;
				case DB_DATA_TYPE_TIME:
				{
					const dpi_time_t* pDate = (dpi_time_t*)m_pBindRow[i];

					snprintf(buff, sizeof(buff), "%02d:%02d:%02d", pDate->hour, pDate->minute, pDate->second);
					pRowValue->SetValue(i, buff);
				}
				break;
				case DB_DATA_TYPE_DATETIME:
				case DB_DATA_TYPE_TIMESTAMP:
				{
					const dpi_timestamp_t* pDate = (dpi_timestamp_t*)m_pBindRow[i];

					snprintf(buff, sizeof(buff), "%4d-%02d-%02d %02d:%02d:%02d", pDate->year, pDate->month, pDate->day,
						pDate->hour, pDate->minute, pDate->second);
					pRowValue->SetValue(i, buff);
				}
				break;
				case DB_DATA_TYPE_BLOB:
				{
					dhloblctr hloblctr = (dhloblctr)m_pBindRow[i];
					const slength tmpbuf_len = 1024;
					sdbyte tmpbuf[tmpbuf_len];
					slength val_len = 0;
					slength len = 0, rlen = 0;
					ulength pos = 1; //从1开始

					//获取数据总长度
					rt = dpi_lob_get_length(hloblctr, &len);

					while (len > 0)
					{
						rlen = (len > tmpbuf_len) ? tmpbuf_len : len;

						rt = dpi_lob_read(hloblctr, pos, DSQL_C_BINARY, rlen, tmpbuf, tmpbuf_len, &val_len);

						//处理，暂时不支持

						len -= val_len;
						pos += val_len;
					}

					pRowValue->SetValue(i, nullptr);
				}
				break;
				default:
					pRowValue->SetValue(i, (char*)m_pBindRow[i]);
					break;
				}
			}

			memset(buff, 0, sizeof(buff));
		}
	}
	else
	{
		if (DSQL_NO_DATA != rt)
		{
			bResult = false;
			SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		}

		pRowValue->Clear();
		delete pRowValue;
		pRowValue = nullptr;
		ClearData(true);
	}

	return bResult;
}

bool CDMDPIInterface::GetNextResult(CDBResultSet* pResultSet, bool bIsStmt)       //获取另一个结果集
{
	if (!m_bConnectState)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	if (nullptr == m_pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "准备语句未发送或未执行！");
		return false;
	}

	//获取剩余的结果集
	DPIRETURN rt = dpi_more_results(m_pStmt);

	if (DSQL_SUCCESS == rt || DSQL_SUCCESS_WITH_INFO == rt)
	{
		if (pResultSet)
		{
			sdint8 affected_rows = 0;
			rt = dpi_row_count(m_pStmt, &affected_rows);

			if (affected_rows > 0)
			{
				ClearData(false);
				pResultSet->SetRowCount(affected_rows);
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
	else if (DSQL_NO_DATA == rt)
	{
		m_iErrorCode = D_DB_NO_DATA;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有更多的结果集了！");
		return false;
	}
	else
	{
		SetErrorInfo(DSQL_HANDLE_STMT, m_pStmt);
		return false;
	}

	return true;
}

void CDMDPIInterface::ClearData(bool bClearAll) //清除结果信息
{
	if (m_pBindRow)
	{
		for (sdint2 i = 0; i < m_nBindCount; ++i)
		{
			//不为空才处理
			if (!m_pBindRow[i])
				continue;

			switch (m_pBindType[i])
			{
			case DB_DATA_TYPE_DATE:
				delete (dpi_date_t*)m_pBindRow[i];
				m_pBindRow[i] = nullptr;
				break;
			case DB_DATA_TYPE_TIME:
				delete (dpi_time_t*)m_pBindRow[i];
				m_pBindRow[i] = nullptr;
				break;
			case DB_DATA_TYPE_DATETIME:
			case DB_DATA_TYPE_TIMESTAMP:
				delete (dpi_timestamp_t*)m_pBindRow[i];
				m_pBindRow[i] = nullptr;
				break;
			case DB_DATA_TYPE_BLOB:
				dpi_free_lob_locator((dhloblctr)m_pBindRow[i]);
				m_pBindRow[i] = nullptr;
			default:
				delete[] (char*)m_pBindRow[i];
				m_pBindRow[i] = nullptr;
				break;
			}
		}

		delete[] m_pBindRow;
		m_pBindRow = nullptr;
	}

	if (m_pBindType)
	{
		delete[] m_pBindType;
		m_pBindType = nullptr;
	}

	if (m_pBindInd)
	{
		delete[] m_pBindInd;
		m_pBindInd = nullptr;
	}

	m_nBindCount = 0;

	if (m_pStmt)
	{
		dpi_close_cursor(m_pStmt);

		if (bClearAll)
		{
			dpi_free_stmt(m_pStmt);
			m_pStmt = nullptr;
		}
	}

	m_iErrorCode = 0;
	memset(m_strSqlState, '\0', MAX_SQL_STATE_LEN);
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);
}

void CDMDPIInterface::Clear() //清除所有中间数据，包括临时打开的句柄
{
	ClearData(true);
}

bool CDMDPIInterface::CheckBindIndex(unsigned int iIndex) //检查参数绑定索引
{
	if (0 == iIndex || (sdint2)iIndex > m_nBindCount)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "参数索引:%u 错误！", iIndex);
		return false;
	}

	return true;
}