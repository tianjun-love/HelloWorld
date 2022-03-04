#include "../include/PostgreSQLInterface.hpp"

CPostgreSQLInterface::CPostgreSQLInterface() : m_pConnect(nullptr), m_pResult(nullptr), m_iRowCount(0), m_iCurrFetchRow(0)
{
}

CPostgreSQLInterface::CPostgreSQLInterface(const std::string& szIP, unsigned int iPort, const std::string& szDBName,
	const std::string& szUserName, const std::string& szPassWord, bool bAutoCommit, EDB_CHARACTER_SET eCharSet, 
	unsigned int iConnTimeOut) : CDBBaseInterface("", szIP, iPort, szDBName, szUserName, szPassWord, bAutoCommit, eCharSet, iConnTimeOut),
m_pConnect(nullptr), m_pResult(nullptr), m_iRowCount(0), m_iCurrFetchRow(0)

{
}

CPostgreSQLInterface::~CPostgreSQLInterface()
{
	FreeEnv();
}

bool CPostgreSQLInterface::InitEnv() //初始化环境
{
	//检查连接信息
	if (m_szServerIP.empty() || m_szDBName.empty() || m_szUserName.empty() || m_szPassWord.empty())
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "连接信息为空！");
		return false;
	}

	m_szConnectInfo = "hostaddr=" + m_szServerIP
		+ " port=" + std::to_string(m_iServerPort)
		+ " dbname=" + m_szDBName
		+ " user=" + m_szUserName
		+ " password=" + m_szPassWord
		+ " connect_timeout=" + std::to_string(m_iConnTimeOut)
		+ " client_encoding=" + ConvertCharacterEnumToString(m_eCharSet);

	return true;
}

void CPostgreSQLInterface::FreeEnv() //释放资源
{
	Clear();
	m_szConnectInfo.clear();
	Disconnect();
}

bool CPostgreSQLInterface::Connect() //连接数据库
{
	if (m_szConnectInfo.empty())
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未初始化环境！");
		return false;
	}

	if (m_bConnectState)
	{
		//已经连接，不处理
		return true;
	}

	m_pInitLock->lock();
	m_pConnect = PQconnectdb(m_szConnectInfo.c_str());
	m_pInitLock->unlock();

	if (!m_pConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "创建连接句柄失败！");
		return false;
	}

	//检查连接状态
	if (CONNECTION_OK != PQstatus(m_pConnect))
	{
		SetErrorInfo();
		return false;
	}

	m_bConnectState = true;

	return true;
}

bool CPostgreSQLInterface::ReConnect() //重新连接
{
	Disconnect();
	return Connect();
}

void CPostgreSQLInterface::Disconnect() //断开连接
{
	if (m_bConnectState)
	{
		if (m_pConnect)
		{
			PQfinish(m_pConnect);
			m_pConnect = nullptr;
		}

		m_bConnectState = false;
	}
}

bool CPostgreSQLInterface::Prepare(const std::string& szSQL, bool bIsStmt) //发送SQL
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::Prepare(const std::string& szStmtName, const std::string& szSQL, int nParams, const Oid *paramTypes) //发送SQL
{
	if (!m_bConnectState)
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

	//发送
	m_pResult = PQprepare(m_pConnect, szStmtName.c_str(), szTemp.c_str(), nParams, paramTypes);

	if (!m_pResult)
	{
		m_iErrorCode = -1;
		SetErrorInfo();
		return false;
	}

	//检查
	if (PGRES_COMMAND_OK != PQresultStatus(m_pResult))
	{
		m_iErrorCode = -1;
		SetErrorInfo(nullptr, true);
		return false;
	}

	return true;
}

bool CPostgreSQLInterface::BindParm(unsigned int iIndex, EDBDataType eDataType, CDBBindBase* param)
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::BindParm(unsigned int iIndex, char& value, short& indp, bool bUnsigned)
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::BindParm(unsigned int iIndex, short& value, short& indp, bool bUnsigned)
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::BindParm(unsigned int iIndex, int& value, short& indp, bool bUnsigned)
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::BindParm(unsigned int iIndex, long long& value, short& indp, bool bUnsigned)
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::BindParm(unsigned int iIndex, char* value, unsigned long& value_len, unsigned long buffer_len, short& indp)
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::Execute(CDBResultSet* pResultSet, bool bIsStmt) //执行SQL
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::Execute(CDBResultSet* pResultSet, const std::string& szStmtName, int nParams, const char* const *paramValues,
	const int *paramLengths, const int *paramFormats, int resultFormat) //执行SQL
{
	if (!m_bConnectState)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	//清空结果数据
	Clear();
	
	//执行
	m_pResult = PQexecPrepared(m_pConnect, szStmtName.c_str(), nParams, paramValues, paramLengths, paramFormats, resultFormat);

	if (!m_pResult)
	{
		m_iErrorCode = -1;
		SetErrorInfo();
		return false;
	}

	//检查执行状态
	ExecStatusType stat = PQresultStatus(m_pResult);
	if (PGRES_COMMAND_OK != stat && PGRES_TUPLES_OK != stat)
	{
		m_iErrorCode = -1;
		SetErrorInfo(nullptr, true);
		return false;
	}

	//获取结果信息
	if (pResultSet)
	{
		//获取行数
		m_iRowCount = PQntuples(m_pResult);

		//设置结果行数
		if (m_iRowCount > 0)
		{
			pResultSet->SetRowCount(m_iRowCount);
			return BindResultInfo(pResultSet);
		}
		else
		{
			m_iErrorCode = D_DB_NO_DATA;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");
			return false;
		}
	}

	return true;
}

bool CPostgreSQLInterface::ExecuteNoParam(const std::string& szSQL, CDBResultSet* pResultSet) //执行SQL，没有绑定参数
{
	if (!m_bConnectState)
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

	//执行
	m_pResult = PQexec(m_pConnect, szTemp.c_str());

	if (!m_pResult)
	{
		m_iErrorCode = -1;
		SetErrorInfo();
		return false;
	}

	//检查执行状态
	ExecStatusType stat = PQresultStatus(m_pResult);
	if (PGRES_COMMAND_OK != stat && PGRES_TUPLES_OK != stat)
	{
		m_iErrorCode = -1;
		SetErrorInfo(nullptr, true);
		return false;
	}

	//获取结果信息
	if (pResultSet)
	{
		//获取行数
		m_iRowCount = PQntuples(m_pResult);
		
		//设置结果行数
		if (m_iRowCount > 0)
		{
			pResultSet->SetRowCount(m_iRowCount);
			return BindResultInfo(pResultSet);
		}
		else
		{
			m_iErrorCode = D_DB_NO_DATA;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果行为空！");
			return false;
		}
	}

	return 0;
}

bool CPostgreSQLInterface::ExecuteDirect(const std::string& szSQL) //执行SQL，没有返回结果的
{
	if (!m_bConnectState)
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
	}

	if (std::string::npos != iPos && 0 == iPos)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的语句类型！");
		return false;
	}

	//清空结果数据
	Clear();

	//执行
	m_pResult = PQexec(m_pConnect, szTemp.c_str());

	if (!m_pResult)
	{
		m_iErrorCode = -1;
		SetErrorInfo();
		return false;
	}

	//检查执行状态
	if (PGRES_COMMAND_OK != PQresultStatus(m_pResult))
	{
		m_iErrorCode = -1;
		SetErrorInfo(nullptr, true);
		return false;
	}

	return true;
}

long long CPostgreSQLInterface::AffectedRows(bool bIsStmt) //受影响行数
{
	if (!m_bConnectState)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return -1;
	}

	if (!m_pResult)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未获取结果集！");
		return -1;
	}

	const char* pRet = PQcmdTuples(m_pResult);

	if (!pRet)
		return -1;
	else
		return std::atoll(pRet);
}

bool CPostgreSQLInterface::BeginTrans() //开启事务
{
	bool bRet = true;

	if (!m_bConnectState)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	//清空结果数据
	Clear();

	//执行
	m_pResult = PQexec(m_pConnect, "begin");

	if (!m_pResult)
	{
		m_iErrorCode = -1;
		SetErrorInfo();
		bRet = false;
	}

	//检查执行状态
	if (bRet)
	{
		if (PGRES_COMMAND_OK != PQresultStatus(m_pResult))
		{
			m_iErrorCode = -1;
			SetErrorInfo(nullptr, true);
			bRet = false;
		}
	}

	PQclear(m_pResult);
	m_pResult = nullptr;

	return bRet;
}

bool CPostgreSQLInterface::EndTrans() //关闭事务
{
	bool bRet = true;

	if (!m_bConnectState)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	//清空结果数据
	Clear();

	//执行
	m_pResult = PQexec(m_pConnect, "end");

	if (!m_pResult)
	{
		m_iErrorCode = -1;
		SetErrorInfo();
		bRet = false;
	}

	//检查执行状态
	if (bRet)
	{
		if (PGRES_COMMAND_OK != PQresultStatus(m_pResult))
		{
			m_iErrorCode = -1;
			SetErrorInfo(nullptr, true);
			bRet = false;
		}
	}

	PQclear(m_pResult);
	m_pResult = nullptr;

	return bRet;
}

bool CPostgreSQLInterface::Commit() //提交
{
	bool bRet = true;

	if (!m_bConnectState)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	//清空结果数据
	Clear();

	//执行
	m_pResult = PQexec(m_pConnect, "commit");

	if (!m_pResult)
	{
		m_iErrorCode = -1;
		SetErrorInfo();
		bRet = false;
	}

	//检查执行状态
	if (bRet)
	{
		if (PGRES_COMMAND_OK != PQresultStatus(m_pResult))
		{
			m_iErrorCode = -1;
			SetErrorInfo(nullptr, true);
			bRet = false;
		}
	}

	PQclear(m_pResult);
	m_pResult = nullptr;

	return bRet;
}

bool CPostgreSQLInterface::Rollback() //回滚
{
	bool bRet = true;

	if (!m_bConnectState)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	//清空结果数据
	Clear();

	//执行
	m_pResult = PQexec(m_pConnect, "rollback");

	if (!m_pResult)
	{
		m_iErrorCode = -1;
		SetErrorInfo();
		bRet = false;
	}

	//检查执行状态
	if (bRet)
	{
		if (PGRES_COMMAND_OK != PQresultStatus(m_pResult))
		{
			m_iErrorCode = -1;
			SetErrorInfo(nullptr, true);
			bRet = false;
		}
	}

	PQclear(m_pResult);
	m_pResult = nullptr;

	return bRet;
}

bool CPostgreSQLInterface::Ping() //检查连接是否可到达
{
	if (m_szConnectInfo.empty())
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未初始化环境！");
		return false;
	}

	return (PQPING_OK == PQping(m_szConnectInfo.c_str()));
}

std::string CPostgreSQLInterface::GetServerVersion() //获取服务器版本信息字符串
{
	std::string szRet;

	if (m_pConnect)
	{
		szRet = "Server version:" + std::to_string(PQserverVersion(m_pConnect))
			+ " protocol:" + std::to_string(PQprotocolVersion(m_pConnect));
	}
	else
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到服务器！");
	}

	return std::move(szRet);
}

bool CPostgreSQLInterface::BindString(unsigned int iIndex, EDBDataType eStrType, CDBBindString* str) //绑定预处理char/varchar参数
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::BindNumber(unsigned int iIndex, EDBDataType eNumberType, CDBBindNumber* number) //绑定预处理数字参数
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::BindDateTime(unsigned int iIndex, EDBDataType eDateTimeType, CDBBindDateTime* datetime) //绑定预处理datetime参数
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::BindLob(unsigned int iIndex, EDBDataType eLobType, CDBBindLob* lob) //绑定预处理lob参数
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::BindResultInfo(CDBResultSet* pResultSet, bool bIsStmt) //获取结果信息，列名，长度，类型等
{
	if (!pResultSet || !m_pResult)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果集参数不能为空！");
		return false;
	}

	int iFieldCount = PQnfields(m_pResult);
	pResultSet->SetDBInterface(this);

	pResultSet->CreateColumnAttr(iFieldCount);
	CDBRowValue* pRowValue = new CDBRowValue(iFieldCount);
	pResultSet->SetRowValue(pRowValue);

	for (int i = 0; i < iFieldCount; ++i)
	{
		CDBColumAttribute* pColumnAttribute = new CDBColumAttribute();
		char *pColumnName = PQfname(m_pResult, i);

		pColumnAttribute->SetColumnName(pColumnName, (unsigned int)strlen(pColumnName));
		int iSize = PQfsize(m_pResult, i); //变长会返回-1
		//Oid iType = PQftype(m_pResult, i); //类型，在pg_type_d.h中，都以字符串存储

		CDBColumValue* pValue = nullptr;

		if (iSize > 0)
			pValue = new CDBColumValue(iSize * 8); //默认大小
		else
			pValue = new CDBColumValue(256); //默认大小

		pRowValue->AddColumnValue(pValue, i);
		pResultSet->AddColumnAttr(pColumnAttribute, i);
	}

	return true;
}

bool CPostgreSQLInterface::Fetch(CDBRowValue* &pRowValue, bool bIsStmt) //获取下一行
{
	bool bResult = true;

	if (!m_pResult) //有结果集才获取
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有存储结果集！");
		return false;
	}

	if ((m_iCurrFetchRow + 1) >= m_iRowCount)
	{
		m_iErrorCode = D_DB_NO_DATA;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有更多的行了！");
		return false;
	}

	int rowNum = m_iCurrFetchRow++;

	for (unsigned int i = 0; i < pRowValue->GetColnumCount(); ++i)
	{
		const char* pT = PQgetvalue(m_pResult, rowNum, i);
		pRowValue->SetValue(i, pT);
	}

	return bResult;
}

bool CPostgreSQLInterface::GetNextResult(CDBResultSet* pResultSet, bool bIsStmt) //获取另一个结果集
{
	m_iErrorCode = -1;
	Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的操作！");
	return false;
}

bool CPostgreSQLInterface::SetErrorInfo(const char* pAddInfo, bool bIsStmt) //从错误句柄获取错误信息
{
	m_iErrorCode = -1;
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);

	if (m_pConnect && !bIsStmt)
	{
		if (pAddInfo)
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%s", pAddInfo, PQerrorMessage(m_pConnect));
		else
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", PQerrorMessage(m_pConnect));
	}
	else if (m_pResult && bIsStmt)
	{
		if (pAddInfo)
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%s", pAddInfo, PQresultErrorMessage(m_pResult));
		else
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", PQresultErrorMessage(m_pResult));
	}
	else
	{
		if (pAddInfo)
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%s", pAddInfo, "连接或结果句柄为空！");
		else
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "连接或结果句柄为空！");

		return false;
	}

	return true;
}

void CPostgreSQLInterface::ClearData(bool bClearAll) //清除中间临时数据
{
	if (m_pResult)
	{
		PQclear(m_pResult);
		m_pResult = nullptr;
	}

	m_iRowCount = 0;
	m_iCurrFetchRow = 0;

	m_iErrorCode = 0;
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);
}

void CPostgreSQLInterface::Clear() //清除所有中间数据，包括临时打开的句柄
{
	ClearData(true);
}

void CPostgreSQLInterface::Test() //测试使用
{

}