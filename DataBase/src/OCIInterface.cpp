#include "../include/OCIInterface.hpp"
#include "xa.h"

const int COCIInterface::m_iDbmsOutputMax = 2000;

COCIInterface::COCIInterface() : m_pEnv(nullptr), m_pErr(nullptr), m_pSer(nullptr), m_bSrcAutoCommit(false), m_bTransCommit(false)
{
}

COCIInterface::COCIInterface(const std::string& szServerName, const std::string& szUserName, const std::string& szPassWord, 
	const std::string& szCharSet, unsigned int iTimeOut) : m_pEnv(nullptr), m_pErr(nullptr), m_pSer(nullptr), 
m_bSrcAutoCommit(false), m_bTransCommit(false), 
CDBBaseInterface(szServerName, "", 1521, "orcl", szUserName, szPassWord, szCharSet, iTimeOut)
{
}

COCIInterface::~COCIInterface()
{
	FreeEnv();
}

bool COCIInterface::InitEnv() //初始化环境
{
	bool bResult = true;

	//检查连接信息
	if (m_szServerName.empty() || m_szUserName.empty() || m_szPassWord.empty())
	{
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "连接信息为空！");
		return false;
	}

	m_pInitLock->lock();

	//环境句柄，多线程对象模式
	sword iResult = OCIEnvCreate(&m_pEnv, OCI_THREADED | OCI_OBJECT, 0, 0, 0, 0, 0, 0);
	if (OCI_SUCCESS == iResult)
	{
		//错误句柄
		iResult = OCIHandleAlloc((void*)m_pEnv, (void **)&m_pErr, (ub4)OCI_HTYPE_ERROR, (size_t)0, (void **)0);
		if (OCI_SUCCESS != iResult)
		{
			bResult = false;
			m_iErrorCode = iResult;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "创建错误句柄失败！");
		}

		//服务器句柄
		if (bResult)
		{
			iResult = OCIHandleAlloc((void*)m_pEnv, (void **)&m_pSer, OCI_HTYPE_SERVER, (size_t)0, (void **)0);
			if (OCI_SUCCESS != iResult)
			{
				bResult = false;
				m_iErrorCode = iResult;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "创建服务器句柄失败！");
			}
		}
	}
	else
	{
		bResult = false;
		m_iErrorCode = iResult;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "创建环境句柄失败！");
	}

	m_pInitLock->unlock();

	return bResult;
}

void COCIInterface::FreeEnv() //释放资源
{
	Clear();

	Disconnect();

	m_pInitLock->lock();

	FreeUserConnect(m_DefaultConnect);

	if (m_pSer)
	{
		OCIHandleFree(m_pSer, (ub4)OCI_HTYPE_SERVER);
		m_pSer = nullptr;
	}

	if (m_pErr)
	{
		OCIHandleFree(m_pErr, (ub4)OCI_HTYPE_ERROR);
		m_pErr = nullptr;
	}

	if (m_pEnv)
	{
		OCIHandleFree(m_pEnv, (ub4)OCI_HTYPE_ENV);
		m_pEnv = nullptr;
	}

	m_pInitLock->unlock();
}

bool COCIInterface::CreateUserConnect(SOraUserConnect& conn) //创建用户连接信息
{
	//上下文句柄
	if (OCI_SUCCESS == OCIHandleAlloc((void*)m_pEnv, (void**)&m_DefaultConnect.pSvc, OCI_HTYPE_SVCCTX, 0, OCI_DEFAULT))
	{
		//设置属性
		if (OCI_SUCCESS != OCIAttrSet((void*)m_DefaultConnect.pSvc, OCI_HTYPE_SVCCTX, (void*)m_pSer, (ub4)sizeof(m_pSer),
			OCI_ATTR_SERVER, m_pErr))
		{
			SetErrorInfo();
			return false;
		}
	}
	else
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "申请服务上下文句柄失败！");
		return false;
	}

	//创建会话
	if (OCI_SUCCESS == OCIHandleAlloc((void*)m_pEnv, (void**)&m_DefaultConnect.pSess, OCI_HTYPE_SESSION, 0, OCI_DEFAULT))
	{
		//设置用户名
		if (OCI_SUCCESS != OCIAttrSet((void*)m_DefaultConnect.pSess, OCI_HTYPE_SESSION, (void*)m_szUserName.c_str(),
			(ub4)m_szUserName.length(), OCI_ATTR_USERNAME, m_pErr))
		{
			SetErrorInfo();
			return false;
		}

		//设置密码
		if (OCI_SUCCESS != OCIAttrSet((void*)m_DefaultConnect.pSess, OCI_HTYPE_SESSION, (void*)m_szPassWord.c_str(),
			(ub4)m_szPassWord.length(), OCI_ATTR_PASSWORD, m_pErr))
		{
			SetErrorInfo();
			return false;
		}

		//开启会话
		if (OCI_SUCCESS != OCISessionBegin(m_DefaultConnect.pSvc, m_pErr, m_DefaultConnect.pSess,
			OCI_CRED_RDBMS, OCI_DEFAULT))
		{
			SetErrorInfo();
			return false;
		}
	}
	else
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "申请会话句柄失败！");
		return false;
	}

	//将会话添加到上下文句柄中
	if (OCI_SUCCESS != OCIAttrSet((void*)m_DefaultConnect.pSvc, OCI_HTYPE_SVCCTX, (void*)m_DefaultConnect.pSess,
		(ub4)sizeof(OCISession*), OCI_ATTR_SESSION, m_pErr))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

void COCIInterface::FreeUserConnect(SOraUserConnect& conn) //清除用户连接信息
{
	if (m_DefaultConnect.pTrans)
	{
		OCITransDetach(m_DefaultConnect.pSvc, m_pErr, OCI_DEFAULT);
		OCIHandleFree(m_DefaultConnect.pTrans, (ub4)OCI_HTYPE_TRANS);
		m_DefaultConnect.pTrans = nullptr;
	}

	if (m_DefaultConnect.pStmt)
	{
		OCIStmtRelease(m_DefaultConnect.pStmt, m_pErr, NULL, 0, OCI_DEFAULT);
		m_DefaultConnect.pStmt = nullptr;
	}

	if (m_DefaultConnect.pSess)
	{
		OCIHandleFree(m_DefaultConnect.pSess, (ub4)OCI_HTYPE_SESSION);
		m_DefaultConnect.pSess = nullptr;
	}

	if (m_DefaultConnect.pSvc)
	{
		OCIHandleFree(m_DefaultConnect.pSvc, (ub4)OCI_HTYPE_SVCCTX);
		m_DefaultConnect.pSvc = nullptr;
	}
}

bool COCIInterface::Connect(bool bAutoCommit) //连接数据库
{
	if (m_bIsConnect)
	{
		//已经连接则不用处理
		return true;
	}

	m_bIsAutoCommit = bAutoCommit;
	m_bSrcAutoCommit = bAutoCommit;

	if (m_pEnv && m_pErr && m_pSer)
	{
		//单用户连接方式
		/*sword iResult = OCILogon2(m_pEnv, m_pErr, &m_pSvc,
			(const OraText*)m_szUserName.c_str(), (ub4)m_szUserName.length(),
			(const OraText*)m_szPassWord.c_str(), (ub4)m_szPassWord.length(),
			(const OraText*)m_szServerName.c_str(), (ub4)m_szServerName.length(), OCI_DEFAULT);*/

		//连接服务器
		if (OCI_SUCCESS == OCIServerAttach(m_pSer, m_pErr, (const OraText*)m_szServerName.c_str(),
			(ub4)m_szServerName.length(), OCI_DEFAULT))
		{
			//创建用户连接信息
			if (CreateUserConnect(m_DefaultConnect))
			{
				//连接成功
				m_bIsConnect = true;
			}
			else
				return false;
		}
		else
		{
			SetErrorInfo();
			return false;
		}
	}
	else
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未初始化或初始化失败！");
		return false;
	}

	return true;
}

bool COCIInterface::ReConnect() //重新连接
{
	Clear();
	
	if (m_bIsConnect)
	{
		//已经连接则断开
		Disconnect();
		FreeUserConnect(m_DefaultConnect);
	}

	return Connect(m_bIsAutoCommit);
}

void COCIInterface::Disconnect() //断开连接
{
	if (m_bIsConnect)
	{
		//单用户方式
		//OCILogoff(m_pSvc, m_pErr);

		//关闭会话
		OCISessionEnd(m_DefaultConnect.pSvc, m_pErr, m_DefaultConnect.pSess, OCI_DEFAULT);

		//断开连接
		OCIServerDetach(m_pSer, m_pErr, OCI_DEFAULT);

		m_bIsConnect = false;
	}
}

bool COCIInterface::Prepare(const std::string& szSQL, bool bIsStmt) //发送SQL
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
	std::string szTempSql(szSQL);
	FormatSQL(szTempSql);

	//清空结果数据
	Clear();

	//if (OCI_SUCCESS != OCIHandleAlloc((void *)m_pEnv, (void **)&m_pStmt, OCI_HTYPE_STMT, (size_t)0, (void **)0))
	//{
	//	SetErrorInfo("创建语句句柄失败");
	//	return false;
	//}

	//发送
	if (OCI_SUCCESS != OCIStmtPrepare2(m_DefaultConnect.pSvc, &m_DefaultConnect.pStmt, m_pErr, (OraText *)szTempSql.c_str(), (ub4)szTempSql.length(),
		(const OraText *)NULL, (ub4)0, (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::BindParm(unsigned int iIndex, EDBDataType eDataType, CDBBindBase* param)
{
	return CDBBaseInterface::BindParm(iIndex, eDataType, param);
}

bool COCIInterface::BindParm(unsigned int iIndex, char& value, short& indp, bool bUnsigned)
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	//绑定
	OCIBind* bind = nullptr;

	if (OCI_SUCCESS != OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, &value,
		(sb4)sizeof(value), SQLT_INT, &indp, NULL, NULL, 0, NULL, OCI_DEFAULT))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::BindParm(unsigned int iIndex, short& value, short& indp, bool bUnsigned)
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	//绑定
	OCIBind* bind = nullptr;

	if (OCI_SUCCESS != OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, &value,
		(sb4)sizeof(value), SQLT_INT, &indp, NULL, NULL, 0, NULL, OCI_DEFAULT))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::BindParm(unsigned int iIndex, int& value, short& indp, bool bUnsigned)
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	//绑定
	OCIBind* bind = nullptr;

	if (OCI_SUCCESS != OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, &value,
		(sb4)sizeof(value), SQLT_INT, &indp, NULL, NULL, 0, NULL, OCI_DEFAULT))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::BindParm(unsigned int iIndex, long long& value, short& indp, bool bUnsigned)
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	//绑定
	OCIBind* bind = nullptr;

	if (OCI_SUCCESS != OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, &value,
		(sb4)sizeof(value), SQLT_INT, &indp, NULL, NULL, 0, NULL, OCI_DEFAULT))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::BindParm(unsigned int iIndex, char* value, unsigned long& value_len, unsigned long buffer_len, short& indp)
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	if (nullptr == value || 0 == buffer_len)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "参数指针或缓存大小不能为空！");
		return false;
	}

	//绑定
	OCIBind* bind = nullptr;

	//以0结尾的字符串
	if (OCI_SUCCESS != OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, value, (sb4)buffer_len,
		SQLT_STR, &indp, NULL, NULL, 0, NULL, OCI_DEFAULT))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::BindString(unsigned int iIndex, EDBDataType eStrType, CDBBindString* str) //绑定预处理char/varchar参数
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	if (nullptr == str)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "参数指针不能为空！");
		return false;
	}
	
	OCIBind* bind = nullptr;

	if (DB_DATA_TYPE_VCHAR == eStrType || DB_DATA_TYPE_CHAR == eStrType)
	{
		//以0结尾的字符串
		if (OCI_SUCCESS != OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, str->m_strBuffer, str->m_lBufferLength,
			SQLT_STR, &str->m_nParamIndp, NULL, NULL, 0, NULL, OCI_DEFAULT))
		{
			SetErrorInfo();
			return false;
		}
	}
	else if (DB_DATA_TYPE_RAW == eStrType)
	{
		if (OCI_SUCCESS != OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, str->m_pRawData, str->m_iRawDataLength,
			SQLT_BIN, &str->m_nParamIndp, NULL, NULL, 0, NULL, OCI_DEFAULT))
		{
			SetErrorInfo();
			return false;
		}
	}
	else
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "此方法只支持char、raw及varchar类型绑定！");
		return false;
	}

	return true;
}

bool COCIInterface::BindNumber(unsigned int iIndex, EDBDataType eNumberType, CDBBindNumber* number) //绑定预处理数字参数
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	if (nullptr == number)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "参数指针不能为空！");
		return false;
	}

	ub2 dty = 0;

	switch (eNumberType)
	{
	case DB_DATA_TYPE_TINYINT:
	case DB_DATA_TYPE_SMALLINT:
	case DB_DATA_TYPE_MEDIUMINT:
	case DB_DATA_TYPE_INT:
	case DB_DATA_TYPE_BIGINT:
		dty = SQLT_INT;
		break;
	case DB_DATA_TYPE_FLOAT:
		dty = SQLT_BFLOAT;
		break;
	case DB_DATA_TYPE_FLOAT2:
	case DB_DATA_TYPE_DOUBLE:
		dty = SQLT_BDOUBLE;
		break;
	case DB_DATA_TYPE_DECIMAL:
		dty = SQLT_VNU;
		break;
	default:
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%d.", "此方法未支持的绑定类型", (int)eNumberType);
		return false;
	}

	OCIBind* bind = nullptr;
	sword iRet = 0;
	
	if (dty == SQLT_VNU)
	{
		//转成OCINumber
		if (CDBBindNumber::E_NUMBER_DATA_FLOAT == number->m_eDataType)
		{
			iRet = OCINumberFromReal(m_pErr, &(number->m_UData._float), number->GetDataLength(CDBBindNumber::E_NUMBER_DATA_FLOAT), 
				(OCINumber*)number->m_strBuffer);
		}
		else if (CDBBindNumber::E_NUMBER_DATA_DOUBLE == number->m_eDataType)
		{
			iRet = OCINumberFromReal(m_pErr, &(number->m_UData._double), number->GetDataLength(CDBBindNumber::E_NUMBER_DATA_DOUBLE), 
				(OCINumber*)number->m_strBuffer);
		}
		else if (CDBBindNumber::E_NUMBER_DATA_STRING == number->m_eDataType) //字符串数字
		{
			std::string szFormat = "99999999999999999999999990"; //整数暂时支持25个

			//添加小数位
			if (number->m_nScale > 0)
			{
				szFormat.append(".");

				for (unsigned short i = 0; i < number->m_nScale; ++i)
				{
					szFormat.append("9");
				}
			}

			iRet = OCINumberFromText(m_pErr, (const OraText*)number->m_strBuffer, number->m_lDataLength,
				(const OraText*)szFormat.c_str(), szFormat.length(), NULL, 0, (OCINumber*)number->m_strBuffer);
		}
		else
		{
			iRet = OCINumberFromInt(m_pErr, number->GetDataPtr(number->m_eDataType), number->GetDataLength(number->m_eDataType),
				(number->m_bUnsigned ? OCI_NUMBER_UNSIGNED : OCI_NUMBER_SIGNED), (OCINumber*)number->m_strBuffer);
		}

		//绑定
		if (OCI_SUCCESS == iRet)
		{
			iRet = OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, (OCINumber*)number->m_strBuffer, sizeof(OCINumber),
				SQLT_VNU, &number->m_nParamIndp, NULL, NULL, 0, NULL, OCI_DEFAULT);
		}
	}
	else
	{
		iRet = OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, number->GetDataPtr(eNumberType), 
			number->GetDataLength(eNumberType), dty, &number->m_nParamIndp, NULL, NULL, 0, NULL, OCI_DEFAULT);
	}

	if (OCI_SUCCESS != iRet)
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::BindDateTime(unsigned int iIndex, EDBDataType eDateTimeType, CDBBindDateTime* datetime) //绑定预处理datetime参数
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	if (nullptr == datetime)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "参数指针不能为空！");
		return false;
	}

	OCIBind* bind = nullptr;
	sword iRet = OCI_SUCCESS;

	if (DB_DATA_TYPE_DATE == eDateTimeType || DB_DATA_TYPE_DATETIME == eDateTimeType)
	{
		if (datetime->m_lDataLength > 0)
		{
			if (!datetime->CheckFormat(DB_DATA_TYPE_DATETIME))
			{
				m_iErrorCode = -1;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "日期值格式错误！");
				return false;
			}

			iRet = OCIDateFromText(m_pErr, (const OraText*)datetime->m_strBuffer, datetime->m_lDataLength,
				(const OraText*)"yyyy-mm-dd hh24:mi:ss", 21, NULL, 0, (OCIDate*)datetime->m_strBuffer);
		}

		if (OCI_SUCCESS == iRet)
		{
			datetime->m_eOCIDateTimeType = DB_DATA_TYPE_DATETIME;
			datetime->m_pOCIEnv = (void*)m_pEnv;
			datetime->m_pOCIErr = (void*)m_pErr;

			iRet = OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, (OCIDate*)datetime->m_strBuffer, sizeof(OCIDate),
				SQLT_ODT, &datetime->m_nParamIndp, 0, 0, 0, 0, OCI_DEFAULT);
		}
	}
	else if (DB_DATA_TYPE_TIMESTAMP == eDateTimeType)
	{
		if (nullptr == datetime->m_pOCITimestamp)
		{
			if (OCI_SUCCESS != OCIDescriptorAlloc(m_pEnv, &datetime->m_pOCITimestamp, OCI_DTYPE_TIMESTAMP, 0, NULL))
			{
				SetErrorInfo("申请时间戳句柄失败");
				return false;
			}
		}

		if (datetime->m_lDataLength > 0)
		{
			if (!datetime->CheckFormat(DB_DATA_TYPE_TIMESTAMP))
			{
				m_iErrorCode = -1;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "时间戳值格式错误！");
				return false;
			}

			iRet = OCIDateTimeFromText(m_pEnv, m_pErr, (const OraText*)datetime->m_strBuffer, datetime->m_lDataLength,
				(const OraText*)"yyyy-mm-dd hh24:mi:ss.ff", 24, NULL, 0, (OCIDateTime*)datetime->m_pOCITimestamp);
		}

		if (OCI_SUCCESS == iRet)
		{
			datetime->m_eOCIDateTimeType = DB_DATA_TYPE_TIMESTAMP;
			datetime->m_pOCIEnv = (void*)m_pEnv;
			datetime->m_pOCIErr = (void*)m_pErr;

			iRet = OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, &datetime->m_pOCITimestamp, sizeof(OCIDateTime*),
				SQLT_TIMESTAMP, &datetime->m_nParamIndp, 0, 0, 0, 0, OCI_DEFAULT);
		}
	}
	else
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "oracle没有TIME的数据类型！");
		return false;
	}

	if (OCI_SUCCESS != iRet)
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::BindLob(unsigned int iIndex, EDBDataType eLobType, CDBBindLob* lob) //绑定预处理lob参数
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	if (nullptr == lob)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "参数指针不能为空！");
		return false;
	}

	if (nullptr == lob->m_pOCILob)
	{
		if (OCI_SUCCESS != OCIDescriptorAlloc(m_pEnv, &lob->m_pOCILob, OCI_DTYPE_LOB, 0, NULL))
		{
			SetErrorInfo("申请lob句柄失败");
			return false;
		}
	}

	OCIBind* bind = nullptr;
	sword iRet = OCI_SUCCESS;
	lob->m_pOCISvc = m_DefaultConnect.pSvc;
	lob->m_pOCIErr = m_pErr;

	if (DB_DATA_TYPE_BLOB == eLobType)
	{
		if (OCI_SUCCESS != OCILobCreateTemporary(m_DefaultConnect.pSvc, m_pErr, (OCILobLocator*)lob->m_pOCILob, OCI_DEFAULT, SQLCS_IMPLICIT,
			OCI_TEMP_BLOB, FALSE, OCI_DURATION_SESSION))
		{
			SetErrorInfo("创建临时blob句柄失败");
			return false;
		}

		lob->m_eOCILobType = DB_DATA_TYPE_BLOB;

		if (lob->m_lDataLength > 0)
		{
			oraub8 llTemp = lob->m_lDataLength;
			iRet = OCILobWrite2(m_DefaultConnect.pSvc, m_pErr, (OCILobLocator*)lob->m_pOCILob, &llTemp, NULL, 1,
				lob->m_pBuffer, lob->m_lBufferLength, OCI_ONE_PIECE, NULL, NULL, 0, SQLCS_IMPLICIT);
		}

		if (OCI_SUCCESS == iRet)
		{
			iRet = OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, &lob->m_pOCILob, sizeof(OCILobLocator*),
				SQLT_BLOB, &lob->m_nParamIndp, 0, 0, 0, 0, OCI_DEFAULT);
		}
	}
	else if (DB_DATA_TYPE_CLOB == eLobType)
	{
		if (OCI_SUCCESS != OCILobCreateTemporary(m_DefaultConnect.pSvc, m_pErr, (OCILobLocator*)lob->m_pOCILob, OCI_DEFAULT, SQLCS_IMPLICIT,
			OCI_TEMP_CLOB, FALSE, OCI_DURATION_SESSION))
		{
			SetErrorInfo("创建临时clob句柄失败");
			return false;
		}

		lob->m_eOCILobType = DB_DATA_TYPE_CLOB;

		if (lob->m_lDataLength > 0)
		{
			//按字节数写入，不然中文会有问题
			oraub8 llTemp = lob->m_lDataLength;
			iRet = OCILobWrite2(m_DefaultConnect.pSvc, m_pErr, (OCILobLocator*)lob->m_pOCILob, &llTemp, NULL, 1,
				lob->m_pBuffer, lob->m_lBufferLength, OCI_ONE_PIECE, NULL, NULL, 0, SQLCS_IMPLICIT);
		}

		if (OCI_SUCCESS == iRet)
		{
			iRet = OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, &lob->m_pOCILob, sizeof(OCILobLocator*),
				SQLT_CLOB, &lob->m_nParamIndp, 0, 0, 0, 0, OCI_DEFAULT);
		}
	}
	else
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "BFILE类型是只读的，不支持绑定！");
		return false;
	}

	if (OCI_SUCCESS != iRet)
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::BindRefCursor(unsigned int iIndex, CDBBindRefCursor& refCursor) //绑定预处理结果参数
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	if (nullptr == refCursor.m_pRefCursor)
	{
		if (OCI_SUCCESS != OCIHandleAlloc((void *)m_pEnv, (void **)&refCursor.m_pRefCursor, OCI_HTYPE_STMT, (size_t)0, (void **)0))
		{
			SetErrorInfo("申请语句句柄失败");
			return false;
		}

		refCursor.m_pOCIErr = m_pErr;
	}

	OCIBind* bind = nullptr;

	if (OCI_SUCCESS != OCIBindByPos(m_DefaultConnect.pStmt, &bind, m_pErr, iIndex + 1, &refCursor.m_pRefCursor, sizeof(OCIStmt*),
		SQLT_RSET, &refCursor.m_nParamIndp, NULL, NULL, 0, NULL, OCI_DEFAULT))
	{
		SetErrorInfo();
		return false;
	}
	
	return true;
}

bool COCIInterface::Execute(CDBResultSet* pResultSet, bool bIsStmt) //执行SQL
{
	if (!CheckPrepare())
	{
		//语句未准备
		return false;
	}

	ub4 iIters = 0; //对非查询语句，表明语句执行次数，必需大于等于1；查询语句，表明一次性从服务器获取的结果集行数，不确定设置为0
	ub4 iSqlType = 0;
	ub4 iMode = OCI_DEFAULT;

	//获取语句类型
	if (OCI_SUCCESS != OCIAttrGet(m_DefaultConnect.pStmt, OCI_HTYPE_STMT, &iSqlType, (ub4)0, OCI_ATTR_STMT_TYPE, m_pErr))
	{
		SetErrorInfo();
		return false;
	}
	else
	{
		if (OCI_STMT_SELECT != iSqlType)
		{
			iIters = 1;

			if (m_bIsAutoCommit)
				iMode = OCI_COMMIT_ON_SUCCESS;
		}
	}

	//执行
	if (OCI_SUCCESS != OCIStmtExecute(m_DefaultConnect.pSvc, m_DefaultConnect.pStmt, m_pErr, iIters, (ub4)0, (CONST OCISnapshot*)0, 
		(OCISnapshot*)0, iMode))
	{
		SetErrorInfo();
		return false;
	}
	else
	{
		//有结果集，绑定
		if (pResultSet)
		{
			return BindResultInfo(pResultSet);
		}
	}

	return true;
}

bool COCIInterface::ExecuteNoParam(const std::string& szSQL, CDBResultSet* pResultSet) //执行SQL，没有绑定参数
{
	if (Prepare(szSQL))
	{
		return Execute(pResultSet);
	}

	return false;
}

bool COCIInterface::ExecuteDirect(const std::string& szSQL) //执行SQL，没有返回结果的
{
	if (Prepare(szSQL))
	{
		ub4 iSqlType = 0;

		//获取语句类型
		if (OCI_SUCCESS != OCIAttrGet(m_DefaultConnect.pStmt, OCI_HTYPE_STMT, &iSqlType, (ub4)0, OCI_ATTR_STMT_TYPE, m_pErr))
		{
			SetErrorInfo();
			return false;
		}
		else
		{
			if (OCI_STMT_SELECT == iSqlType || OCI_STMT_UNKNOWN == iSqlType
				|| OCI_STMT_BEGIN == iSqlType || OCI_STMT_DECLARE == iSqlType
				|| OCI_STMT_CALL == iSqlType)
			{
				m_iErrorCode = -1;
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "不支持的语句类型！");
				return false;
			}
		}

		//执行
		if (OCI_SUCCESS != OCIStmtExecute(m_DefaultConnect.pSvc, m_DefaultConnect.pStmt, m_pErr, (ub4)1, (ub4)0, (CONST OCISnapshot*)0, (OCISnapshot*)0,
			OCI_COMMIT_ON_SUCCESS))
		{
			SetErrorInfo();
			return false;
		}
		else
			return true;
	}

	return false;
}

bool COCIInterface::BeginTrans() //开启事务
{
	EndTrans();

	if (OCI_SUCCESS != OCIHandleAlloc((void*)m_pEnv, (void**)&m_DefaultConnect.pTrans, OCI_HTYPE_TRANS, 0, OCI_DEFAULT))
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "申请事务句柄失败！");
		return false;
	}

	//将事务添加到上下文句柄中
	if (OCI_SUCCESS != OCIAttrSet((void*)m_DefaultConnect.pSvc, OCI_HTYPE_SVCCTX, (void*)m_DefaultConnect.pTrans,
		(ub4)sizeof(OCITrans*), OCI_ATTR_TRANS, m_pErr))
	{
		SetErrorInfo();
		return false;
	}

	/*以全局事务id = [1000, 123, 1]开始一个事务*/
	XID gxid;
	gxid.formatID = 1000; /*格式id = 1000 */
	gxid.gtrid_length = 3; /*gtrid = 123 */
	gxid.data[0] = 1;
	gxid.data[1] = 2;
	gxid.data[2] = 3;
	gxid.bqual_length = 1; /*bqual = 1 */
	gxid.data[3] = 1;

	if (OCI_SUCCESS != OCIAttrSet((void *)m_DefaultConnect.pTrans, OCI_HTYPE_TRANS, (void*)&gxid,
		sizeof(XID), OCI_ATTR_XID, m_pErr))
	{
		SetErrorInfo();
		return false;
	}

	if (OCI_SUCCESS != OCITransStart(m_DefaultConnect.pSvc, m_pErr, (uword)5, OCI_TRANS_NEW))
	{
		SetErrorInfo();
		return false;
	}

	//设置手动提交
	m_bIsAutoCommit = false;
	m_bTransCommit = false;

	return true;
}

bool COCIInterface::EndTrans() //关闭事务
{
	if (m_DefaultConnect.pTrans)
	{
		//恢复原来的提交模式
		m_bIsAutoCommit = m_bSrcAutoCommit;

		//如果没有提交，则提交
		if (!m_bTransCommit)
		{
			OCITransCommit(m_DefaultConnect.pSvc, m_pErr, OCI_DEFAULT);
			m_bTransCommit = true;
		}

		if (OCI_SUCCESS != OCIHandleFree(m_DefaultConnect.pTrans, (ub4)OCI_HTYPE_TRANS))
		{
			SetErrorInfo();
			m_DefaultConnect.pTrans = nullptr;
			return false;
		}
	}

	return true;
}

bool COCIInterface::Commit() //提交
{
	m_bTransCommit = true;

	if (OCI_SUCCESS != OCITransCommit(m_DefaultConnect.pSvc, m_pErr, OCI_DEFAULT))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::Rollback() //回滚
{
	if (OCI_SUCCESS != OCITransRollback(m_DefaultConnect.pSvc, m_pErr, OCI_DEFAULT))
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::Ping() //检查连接是否可到达
{
	return (OCI_SUCCESS == OCIPing(m_DefaultConnect.pSvc, m_pErr, OCI_DEFAULT));
}

std::string COCIInterface::GetServerVersion() //获取服务器版本信息字符串
{
	OraText buf[64] = { '\0' };

	if (OCI_SUCCESS == OCIServerVersion(m_DefaultConnect.pSvc, m_pErr, buf, 64, OCI_HTYPE_SVCCTX))
		return std::move(std::string((char*)buf));
	else
	{
		SetErrorInfo();
		return std::move(std::string(""));
	}
}

void COCIInterface::FreeDefineInfo(SOraDefineInfo& def) //释放返回结果绑定信息
{
	if (m_DefaultDefine.pDefineRow)
	{
		for (unsigned long i = 0; i < m_DefaultDefine.lDefineCount; ++i)
		{
			//不为空才处理
			if (!m_DefaultDefine.pDefineRow[i])
				continue;

			switch (m_DefaultDefine.pDefineType[i])
			{
			case DB_DATA_TYPE_TIME:
				delete (OCITime*)m_DefaultDefine.pDefineRow[i];
				break;
			case DB_DATA_TYPE_DATE:
			case DB_DATA_TYPE_DATETIME:
				delete (OCIDate*)m_DefaultDefine.pDefineRow[i];
				break;
			case DB_DATA_TYPE_TIMESTAMP:
				OCIDescriptorFree(m_DefaultDefine.pDefineRow[i], (ub4)OCI_DTYPE_TIMESTAMP);
				break;
			case DB_DATA_TYPE_BLOB:
			case DB_DATA_TYPE_CLOB:
				OCIDescriptorFree(m_DefaultDefine.pDefineRow[i], (ub4)OCI_DTYPE_LOB);
				break;
			case DB_DATA_TYPE_BFILE:
				OCIDescriptorFree(m_DefaultDefine.pDefineRow[i], (ub4)OCI_DTYPE_FILE);
				break;
			default:
				delete[](char*)m_DefaultDefine.pDefineRow[i];
				break;
			}

			m_DefaultDefine.pDefineRow[i] = nullptr;
		}

		delete[] m_DefaultDefine.pDefineRow;
		m_DefaultDefine.pDefineRow = nullptr;
	}

	if (m_DefaultDefine.pDefineType)
	{
		delete[] m_DefaultDefine.pDefineType;
		m_DefaultDefine.pDefineType = nullptr;
	}

	if (m_DefaultDefine.pDefineDp)
	{
		delete[] m_DefaultDefine.pDefineDp;
		m_DefaultDefine.pDefineDp = nullptr;
	}

	m_DefaultDefine.lDefineCount = 0;
}

bool COCIInterface::BindResultInfo(CDBResultSet* pResultSet, bool bIsStmt) //绑定结果集
{
	if (!pResultSet)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "结果集地址为空！");
		return false;
	}
	else
		pResultSet->SetDBInterface(this);

	//获取结果集field数量
	ub4 iColNum = 0, iRowNum = 0;
	OCIStmt* tempStmt = (bIsStmt ? m_DefaultConnect.pImplicitStmt : m_DefaultConnect.pStmt);
	if (OCI_SUCCESS == OCIAttrGet((void*)tempStmt, (ub4)OCI_HTYPE_STMT, &iColNum, (ub4 *)0, (ub4)OCI_ATTR_PARAM_COUNT, m_pErr))
	{
		//有结果才处理
		if (0 == iColNum)
			return true;

		//取所有列描述
		OCIParam* pColumnsParam = nullptr;//列描述，要释放
		text*     pColumnName = nullptr;  //字段名称，不需释放
		ub4       ColumnNameLength;       //名称长度
		ub2       DataSize;               //数据长度
		ub2       DataType;               //Oracle 内部数据类型
		ub2       Precision;              //包括小数点的总位数
		sb1       Scale;                  //小数点个数
		ub1       IsNULL;                 //是否可以为null

		CDBRowValue* pRowValue = new CDBRowValue(iColNum);
		pResultSet->CreateColumnAttr(iColNum);
		pResultSet->SetRowValue(pRowValue);

		m_DefaultDefine.lDefineCount = iColNum;
		m_DefaultDefine.pDefineRow = new void*[iColNum]{ nullptr };
		m_DefaultDefine.pDefineDp = new sb2[iColNum]{ 0 };
		m_DefaultDefine.pDefineType = new EDBDataType[iColNum];

		//属性下标从1开始
		for (ub4 i = 1; i <= iColNum; ++i)
		{
			if (OCI_SUCCESS == OCIParamGet(tempStmt, OCI_HTYPE_STMT, m_pErr, (void **)&pColumnsParam, i))
			{
				CDBColumAttribute* pColumnAttribute = new CDBColumAttribute();

				//获取属性
				OCIAttrGet(pColumnsParam, OCI_DTYPE_PARAM, &pColumnName, (ub4 *)&ColumnNameLength, OCI_ATTR_NAME, m_pErr);
				OCIAttrGet(pColumnsParam, OCI_DTYPE_PARAM, (void*)&DataSize, (ub4 *)0, OCI_ATTR_DATA_SIZE, m_pErr);
				OCIAttrGet(pColumnsParam, OCI_DTYPE_PARAM, (void*)&Precision, (ub4 *)0, OCI_ATTR_PRECISION, m_pErr);
				OCIAttrGet(pColumnsParam, OCI_DTYPE_PARAM, (void*)&Scale, (ub4 *)0, OCI_ATTR_SCALE, m_pErr);
				OCIAttrGet(pColumnsParam, OCI_DTYPE_PARAM, (void*)&IsNULL, (ub4 *)0, OCI_ATTR_IS_NULL, m_pErr);
				OCIAttrGet(pColumnsParam, OCI_DTYPE_PARAM, (void*)&DataType, (ub4 *)0, OCI_ATTR_DATA_TYPE, m_pErr);

				pColumnAttribute->SetColumnName((char*)pColumnName);
				pColumnAttribute->m_iFieldNameLen = ColumnNameLength;
				pColumnAttribute->m_iFieldDataLen = DataSize;
				pColumnAttribute->m_iPrecision = Precision;
				pColumnAttribute->m_nScale = Scale;
				pColumnAttribute->m_bNullAble = ((0 != IsNULL) ? true : false);

				switch (DataType)
				{
				case SQLT_CHR:
				case SQLT_STR:
				case SQLT_LNG:
				case SQLT_VCS:
				case SQLT_AVC:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_VCHAR;
					if (SQLT_LNG == DataType) //最大2G
						pColumnAttribute->m_iFieldDataLen = D_DB_STRING_MAX_LEN;
					break;
				case SQLT_NUM:
				case SQLT_INT:
				case SQLT_FLT:
				case SQLT_VNU:
				case SQLT_PDN:
					if (0 == pColumnAttribute->m_nScale) //方便使用
					{
						if (pColumnAttribute->m_iPrecision < 3)
							pColumnAttribute->m_eDataType = DB_DATA_TYPE_TINYINT;
						else if (pColumnAttribute->m_iPrecision < 5)
							pColumnAttribute->m_eDataType = DB_DATA_TYPE_SMALLINT;
						else if (pColumnAttribute->m_iPrecision < 8)
							pColumnAttribute->m_eDataType = DB_DATA_TYPE_MEDIUMINT;
						else if (pColumnAttribute->m_iPrecision < 10)
							pColumnAttribute->m_eDataType = DB_DATA_TYPE_INT;
						else
						pColumnAttribute->m_eDataType = DB_DATA_TYPE_BIGINT;
					}
					else
						pColumnAttribute->m_eDataType = DB_DATA_TYPE_DECIMAL;
					pColumnAttribute->m_iFieldDataLen = D_DB_NUMBER_MAX_LEN;
					break;
				case SQLT_DATE:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_DATE;
					pColumnAttribute->m_iFieldDataLen = sizeof(OCIDate);
					break;
				case SQLT_TIME:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_TIME;
					pColumnAttribute->m_iFieldDataLen = sizeof(OCITime);
					break;
				case SQLT_DAT:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_DATETIME;
					pColumnAttribute->m_iFieldDataLen = sizeof(OCIDate);
					break;
				case SQLT_AFC:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_CHAR;
					break;
				case SQLT_IBFLOAT:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_FLOAT;
					break;
				case SQLT_IBDOUBLE:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_DOUBLE;
					break;
				case SQLT_LVC:
				case SQLT_CLOB: //最大4G
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_CLOB;
					pColumnAttribute->m_iFieldDataLen = sizeof(OCILobLocator*);
					break;
				case SQLT_BIN: //二进制数据
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_RAW; //转成16进制
					break;
				case SQLT_LBI: //长二进制数据，最大2G
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_RAW; //转成16进制
					pColumnAttribute->m_iFieldDataLen = D_DB_STRING_MAX_LEN;
					break;
				case SQLT_LVB:
				case SQLT_BLOB: //最大4G
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_BLOB;
					pColumnAttribute->m_iFieldDataLen = sizeof(OCILobLocator*);
					break;
				case SQLT_BFILE:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_BFILE;
					pColumnAttribute->m_iFieldDataLen = sizeof(OCIBFileLocator*);
					break;
				case SQLT_INTERVAL_YM:
				case SQLT_INTERVAL_DS:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_VCHAR; //使用varchar接收
					break;
				case SQLT_TIMESTAMP:
				case SQLT_TIMESTAMP_TZ:
				case SQLT_TIMESTAMP_LTZ:
					pColumnAttribute->m_eDataType = DB_DATA_TYPE_TIMESTAMP;
					pColumnAttribute->m_iFieldDataLen = sizeof(OCIDateTime*);
					break;
				default:
					break;
				}

				//释放空间
				sword nTMP = OCIDescriptorFree(pColumnsParam, OCI_DTYPE_PARAM);
				pColumnsParam = nullptr;

				pResultSet->AddColumnAttr(pColumnAttribute, i - 1);
				CDBColumValue* pValue = new CDBColumValue();
				pRowValue->AddColumnValue(pValue, i - 1);

				//申请返回数据空间
				void* pTemp = nullptr;
				
				switch (pColumnAttribute->m_eDataType)
				{
				case DB_DATA_TYPE_TIME:
					pTemp = new OCITime;
					break;
				case DB_DATA_TYPE_DATE:
				case DB_DATA_TYPE_DATETIME:
					pTemp = new OCIDate;
					break;
				case DB_DATA_TYPE_TIMESTAMP:
				{
					if (OCI_SUCCESS != OCIDescriptorAlloc(m_pEnv, &pTemp, OCI_DTYPE_TIMESTAMP, 0, NULL))
					{
						SetErrorInfo();
						return false;
					}
				}
					break;
				case DB_DATA_TYPE_BLOB:
				case DB_DATA_TYPE_CLOB:
				{
					if (OCI_SUCCESS != OCIDescriptorAlloc(m_pEnv, &pTemp, OCI_DTYPE_LOB, 0, NULL))
					{
						SetErrorInfo();
						return false;
					}
				}
					break;
				case DB_DATA_TYPE_BFILE:
				{
					if (OCI_SUCCESS != OCIDescriptorAlloc(m_pEnv, &pTemp, OCI_DTYPE_FILE, 0, NULL))
					{
						SetErrorInfo();
						return false;
					}
				}
					break;
				default:
					pTemp = new char[pColumnAttribute->m_iFieldDataLen + 1]{ '\0' };
					break;
				}

				if (nullptr == pTemp)
				{
					SetErrorInfo("申请绑定结果内存失败");
					return false;
				}

				m_DefaultDefine.pDefineRow[i - 1] = pTemp;
				m_DefaultDefine.pDefineType[i - 1] = pColumnAttribute->m_eDataType;
				OCIDefine *define = nullptr;
				sword iResult = OCI_SUCCESS;

				//绑定
				switch (pColumnAttribute->m_eDataType)
				{
				case DB_DATA_TYPE_TIME:
					iResult = OCIDefineByPos(tempStmt, &define, m_pErr, i, (void*)pTemp, pColumnAttribute->m_iFieldDataLen,
						SQLT_TIME, &m_DefaultDefine.pDefineDp[i - 1], (ub2*)0, (ub2*)0, OCI_DEFAULT);
					break;
				case DB_DATA_TYPE_DATE:
				case DB_DATA_TYPE_DATETIME:
					iResult = OCIDefineByPos(tempStmt, &define, m_pErr, i, (void*)pTemp, pColumnAttribute->m_iFieldDataLen,
						SQLT_ODT, &m_DefaultDefine.pDefineDp[i - 1], (ub2*)0, (ub2*)0, OCI_DEFAULT);
					break;
				case DB_DATA_TYPE_TIMESTAMP:
					iResult = OCIDefineByPos(tempStmt, &define, m_pErr, i, &m_DefaultDefine.pDefineRow[i - 1], pColumnAttribute->m_iFieldDataLen,
						SQLT_TIMESTAMP, &m_DefaultDefine.pDefineDp[i - 1], (ub2*)0, (ub2*)0, OCI_DEFAULT);
					break;
				case DB_DATA_TYPE_BLOB:
					iResult = OCIDefineByPos(tempStmt, &define, m_pErr, i, &m_DefaultDefine.pDefineRow[i - 1], pColumnAttribute->m_iFieldDataLen,
						SQLT_BLOB, &m_DefaultDefine.pDefineDp[i - 1], (ub2*)0, (ub2*)0, OCI_DEFAULT);
					break;
				case DB_DATA_TYPE_CLOB:
					iResult = OCIDefineByPos(tempStmt, &define, m_pErr, i, &m_DefaultDefine.pDefineRow[i - 1], pColumnAttribute->m_iFieldDataLen,
						SQLT_CLOB, &m_DefaultDefine.pDefineDp[i - 1], (ub2*)0, (ub2*)0, OCI_DEFAULT);
					break;
				case DB_DATA_TYPE_BFILE:
					iResult = OCIDefineByPos(tempStmt, &define, m_pErr, i, &m_DefaultDefine.pDefineRow[i - 1], pColumnAttribute->m_iFieldDataLen,
						SQLT_BFILE, &m_DefaultDefine.pDefineDp[i - 1], (ub2*)0, (ub2*)0, OCI_DEFAULT);
					break;
				default:
					iResult = OCIDefineByPos(tempStmt, &define, m_pErr, i, (void*)pTemp, pColumnAttribute->m_iFieldDataLen + 1,
						SQLT_STR, &m_DefaultDefine.pDefineDp[i - 1], (ub2*)0, (ub2*)0, OCI_DEFAULT);
					break;
				}

				if (OCI_SUCCESS != iResult)
				{
					SetErrorInfo();
					return false;
				}
			}
			else
			{
				SetErrorInfo();
				return false;
			}
		}
	}
	else
	{
		SetErrorInfo();
		return false;
	}

	return true;
}

bool COCIInterface::Fetch(CDBRowValue* &pRowValue, bool bIsStmt) //获取下一行
{
	bool bResult = true;

	//没有结果集，返回
	if (!pRowValue)
		return true;

	OCIStmt* tempStmt = (bIsStmt ? m_DefaultConnect.pImplicitStmt : m_DefaultConnect.pStmt);
	sword iResult = OCIStmtFetch2(tempStmt, m_pErr, (ub4)1, OCI_FETCH_NEXT,(sb4)1, OCI_DEFAULT);

	if (OCI_SUCCESS == iResult)
	{
		for (unsigned int i = 0; i < m_DefaultDefine.lDefineCount; ++i)
		{
			if (-1 == m_DefaultDefine.pDefineDp[i]) //-1是值为空
			{
				pRowValue->SetValue(i, nullptr);
				continue;
			}

			const void* pTemp = m_DefaultDefine.pDefineRow[i];

			switch (m_DefaultDefine.pDefineType[i])
			{
			case DB_DATA_TYPE_DATE:
			{
				char buf[12] = { '\0' };
				const OCIDate* pDate = (OCIDate*)pTemp;

				Format(buf, 12, "%4d-%02d-%02d", pDate->OCIDateYYYY, pDate->OCIDateMM, pDate->OCIDateDD);

				pRowValue->SetValue(i, buf);
			}
				break;
			case DB_DATA_TYPE_TIME:
			{
				char buf[10] = { '\0' };
				const OCITime* pDate = (OCITime*)pTemp;

				Format(buf, 10, "%02d:%02d:%02d", pDate->OCITimeHH, pDate->OCITimeMI, pDate->OCITimeSS);

				pRowValue->SetValue(i, buf);
			}
				break;
			case DB_DATA_TYPE_DATETIME:
			{
				char buf[22] = { '\0' };
				const OCIDate* pDate = (OCIDate*)pTemp;

				Format(buf, 22, "%4d-%02d-%02d %02d:%02d:%02d", pDate->OCIDateYYYY, pDate->OCIDateMM, pDate->OCIDateDD,
					pDate->OCIDateTime.OCITimeHH, pDate->OCIDateTime.OCITimeMI, pDate->OCIDateTime.OCITimeSS);

				pRowValue->SetValue(i, buf);
			}
				break;
			case DB_DATA_TYPE_TIMESTAMP:
			{
				const OraText* fmt = (OraText*)"yyyy-mm-dd hh24:mi:ss.ff";
				char buf[32] = { '\0' };
				ub4 buf_len = 32;

				if (OCI_SUCCESS == OCIDateTimeToText(m_pEnv, m_pErr, (OCIDateTime*)pTemp, fmt, 24, 3, NULL, 0, &buf_len, (OraText*)buf))
				{
					pRowValue->SetValue(i, buf);
				}
				else
				{
					SetErrorInfo();
					return false;
				}
			}
				break;
			case DB_DATA_TYPE_BLOB:
			case DB_DATA_TYPE_CLOB:
			case DB_DATA_TYPE_BFILE:
			{
				//OCIBFileLocator就是OCILobLocator
				OCILobLocator* pLob = (OCILobLocator*)pTemp;
				oraub8 iLobLen = 0, iBufLen = 0, iReadBytes = 0, iReadChars = 0;
				bool bFlag = true;

				//blob,bfile以字节为单位，clob以字符为单位
				if (OCI_SUCCESS == OCILobGetLength2(m_DefaultConnect.pSvc, m_pErr, pLob, &iLobLen))
				{
					if (iLobLen > 0)
					{
						if (DB_DATA_TYPE_CLOB == m_DefaultDefine.pDefineType[i])
							iReadChars = iLobLen;
						else
							iReadBytes = iLobLen;

						//buf长度为是8的倍数且多8
						if (iLobLen >= D_DB_LOB_RETURN_MAX_LEN)
							iBufLen = D_DB_LOB_RETURN_MAX_LEN + 8;
						else
						{
							if ((iLobLen % 8) == 0)
								iBufLen = iLobLen + 8;
							else
								iBufLen = (iLobLen / 8 + 2) * 8;
						}

						ub1 *pBufLob = new ub1[iBufLen]{ 0 };

						if (OCI_SUCCESS == OCILobRead2(m_DefaultConnect.pSvc, m_pErr, pLob, &iReadBytes, &iReadChars, 1, pBufLob, iBufLen,
							OCI_ONE_PIECE, NULL, NULL, 0, SQLCS_IMPLICIT))
						{
							if (DB_DATA_TYPE_CLOB == m_DefaultDefine.pDefineType[i])
								pRowValue->SetValue(i, (char*)pBufLob);
							else
							{
								//转十六进制字符串
								std::string szTemp;
								ConvertBinaryToHex(pBufLob, iReadBytes, szTemp);
								pRowValue->SetValue(i, szTemp.c_str());
							}	
						}
						else
							bResult = false;

						delete[] pBufLob;
					}
					else
						pRowValue->SetValue(i, nullptr);
				}
				else
					bResult = false;

				if (!bResult)
				{
					SetErrorInfo();
					return false;
				}
			}
				break;
			default:
				pRowValue->SetValue(i, (char*)pTemp);
				break;
			}
		}
	}
	else
	{
		bResult = false;

		if (OCI_NO_DATA != iResult)
			SetErrorInfo();
		else
		{
			m_iErrorCode = D_DB_NO_DATA;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有结果行！");
		}
	}

	return bResult;
}

bool COCIInterface::GetNextResult(CDBResultSet* pResultSet, bool bIsStmt) //获取另一个结果集
{
	if (!m_bIsConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	if (nullptr == m_DefaultConnect.pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "准备语句未发送！");
		return false;
	}

	//释放绑定结果
	ClearData();
	pResultSet->Clear();

	//查看是否有隐式结果，存储过程产生
	bool bResult = true;
	void* result = nullptr;
	ub4 rsetcnt = 0, rtype = 0;

	if (OCI_SUCCESS == OCIAttrGet((void*)m_DefaultConnect.pStmt, OCI_HTYPE_STMT, &rsetcnt, 0, OCI_ATTR_IMPLICIT_RESULT_COUNT, m_pErr))
	{
		if (rsetcnt > 0)
		{
			//需要存储过程内DBMS_SQL.RETURN_RESULT(sys_refcursor变量)返回游标
			sword iRet = OCIStmtGetNextResult(m_DefaultConnect.pStmt, m_pErr, &result, &rtype, OCI_DEFAULT);
			if (OCI_SUCCESS == iRet)
			{
				if (OCI_RESULT_TYPE_SELECT == rtype) //仅支持这一种类型
				{
					//不用释放，释放顶级语句句柄时，将自动关闭并释放所有隐式结果集
					m_DefaultConnect.pImplicitStmt = (OCIStmt*)result;
					if (m_DefaultConnect.pImplicitStmt)
						bResult = BindResultInfo(pResultSet, true);
					else
					{
						m_iErrorCode = -1;
						Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "返回隐式结果集为空！");
						bResult = false;
					}
				}
				else
				{
					m_iErrorCode = -1;
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%d", "未知的结果类型：", rtype);
					bResult = false;
				}
			}
			else
			{
				bResult = false;

				if (OCI_NO_DATA != iRet)
					SetErrorInfo();
				else
				{
					//没有数据返回OCI_NO_DATA
					bResult = false;
					m_iErrorCode = D_DB_NO_DATA;
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有结果行！");
				}
			}
		}
		else
		{
			bResult = false;
			m_iErrorCode = D_DB_NO_DATA;
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有结果行！");
		}
	}
	else
	{
		bResult = false;
		SetErrorInfo();
	}

	return bResult;
}

void COCIInterface::ClearData(bool bClearAll) //清除结果信息
{
	FreeDefineInfo(m_DefaultDefine);
	m_DefaultConnect.pImplicitStmt = nullptr;

	m_iErrorCode = 0;
	memset(m_strSqlState, '\0', MAX_SQL_STATE_LEN);
	memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);
}

void COCIInterface::Clear() //清除所有中间数据，包括临时打开的句柄
{
	ClearData(true);

	if (m_DefaultConnect.pStmt)
	{
		OCIStmtRelease(m_DefaultConnect.pStmt, m_pErr, NULL, 0, OCI_DEFAULT);
		m_DefaultConnect.pStmt = nullptr;
	}
}

bool COCIInterface::CheckPrepare() //检查语句是否已经发送
{
	if (!m_bIsConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	if (nullptr == m_DefaultConnect.pStmt)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "准备语句未发送！");
		return false;
	}

	return true;
}

bool COCIInterface::SetSqlState(bool bIsStmt) //设置SQL执行状态
{
	return SetErrorInfo();
}

bool COCIInterface::SetErrorInfo(const char* pAddInfo, bool bIsStmt) //从错误句柄获取错误信息
{
	if (m_pErr)
	{
		OraText sqlStat = '0'; //oracle 8.x及更高版本不支持
		memset(m_strSqlState, '\0', MAX_SQL_STATE_LEN);
		memset(m_strErrorBuf, '\0', MAX_ERROR_INFO_LEN);

		sword iResult = OCIErrorGet((void*)m_pErr, (ub4)1, &sqlStat, &m_iErrorCode, (text*)m_strErrorBuf,
			(ub4)MAX_ERROR_INFO_LEN - 1, (ub4)OCI_HTYPE_ERROR);

		if (OCI_SUCCESS == iResult)
		{
			Format(m_strSqlState, MAX_SQL_STATE_LEN, "%c", sqlStat);
			if (pAddInfo)
				Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%s", pAddInfo, m_strErrorBuf);
			return true;
		}
		else
		{
			m_iErrorCode = iResult;

			if (OCI_NO_DATA == m_iErrorCode)
			{
				if (pAddInfo)
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%s", pAddInfo, "没有错误信息！");
				else
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "没有错误信息！");
			}
			else
			{
				if (pAddInfo)
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%s", pAddInfo, "获取错误信息失败！");
				else
					Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "获取错误信息失败！");
			}

			return false;
		}
	}
	else
	{
		m_iErrorCode = -1;
		if (pAddInfo)
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s:%s", pAddInfo, "错误句柄为空！");
		else
			Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "错误句柄为空！");
		return false;
	}
}

bool COCIInterface::GetPararmResult(const CDBBindRefCursor& refCursor, CDBResultSet& resultSet) //获取绑定SYS_REFCURSOR参数的结果
{
	if (!m_bIsConnect)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "未连接到数据库服务！");
		return false;
	}

	if (!refCursor.m_pRefCursor)
	{
		m_iErrorCode = -1;
		Format(m_strErrorBuf, MAX_ERROR_INFO_LEN, "%s", "参数未绑定！");
		return false;
	}

	//清空数据
	Clear();
	m_DefaultConnect.pStmt = (OCIStmt*)refCursor.m_pRefCursor;

	return BindResultInfo(&resultSet);
}

void COCIInterface::GetOCIHandleForBatch(OCISvcCtx* &pSvc, OCIStmt* &pStmt, OCIError* &pErr) //获取OCI句柄，用于批量读取或插入，调用Prepare后自己实现
{
	pSvc = m_DefaultConnect.pSvc;
	pStmt = m_DefaultConnect.pStmt;
	pErr = m_pErr;

	//批量读取例子
	/*
	OCIDefine* df[3] = { nullptr, nullptr, nullptr };
	std::string sqlstmt = "select I_ID,S_NAME,I_AGE from T_TEST";

	struct STest
	{
		long long llId        = 0;
		char      strName[64] = { '\0' };
		long long llAge       = 0;
	};

	STest dataTest[10]; //一次读取10行

	//发送语句
	if (!Prepare(sqlstmt))
	{
		return;
	}

	//绑定参数
	sword status = OCIDefineByPos(m_DefaultConnect.pStmt, &df[0], m_pErr, 1, &dataTest[0].llId, 8, SQLT_INT, NULL, 0, 0, OCI_DEFAULT);
		
	if (OCI_SUCCESS == status)
		status = OCIDefineByPos(m_DefaultConnect.pStmt, &df[1], m_pErr, 2, &dataTest[0].strName, 64, SQLT_STR, NULL, 0, 0, OCI_DEFAULT);

	if (OCI_SUCCESS == status)
		status = OCIDefineByPos(m_DefaultConnect.pStmt, &df[2], m_pErr, 3, &dataTest[0].llAge, 8, SQLT_INT, NULL, 0, 0, OCI_DEFAULT);

	//绑定数组
	if (OCI_SUCCESS == status)
		status = OCIDefineArrayOfStruct(df[0], m_pErr, sizeof(STest), 0, 0, 0);

	if (OCI_SUCCESS == status)
		status = OCIDefineArrayOfStruct(df[1], m_pErr, sizeof(STest), 0, 0, 0);

	if (OCI_SUCCESS == status)
		status = OCIDefineArrayOfStruct(df[2], m_pErr, sizeof(STest), 0, 0, 0);

	//执行
	if (OCI_SUCCESS == status)
		status = OCIStmtExecute(m_DefaultConnect.pSvc, m_DefaultConnect.pStmt, m_pErr, 0, 0, NULL, NULL, OCI_DEFAULT);

	if (OCI_SUCCESS == status)
	{
		do
		{
			//一次读取多行
			status = OCIStmtFetch2(m_DefaultConnect.pStmt, m_pErr, 10, OCI_FETCH_NEXT, 1, OCI_DEFAULT);

			//如果读取的条数小于10或者没有数据了，会返回OCI_NO_DATA
			if (OCI_SUCCESS == status || OCI_NO_DATA == status)
			{
				for (int i = 0; i < 10; ++i)
				{
					if (dataTest[i].llId > 0 && strlen(dataTest[i].strName) > 0)
					{
						//处理
						//...
					}
					else
						break; //没有数据了
				}

				//清空，准备下一次读取
				memset(&dataTest, 0, sizeof(dataTest));
			}
			else
				break;

		} while (OCI_NO_DATA != status);
	}

	if (OCI_SUCCESS != status && OCI_NO_DATA != status)
	{
		SetErrorInfo();
	}
	*/
}

bool COCIInterface::SetDbmsOutputEnable(bool bEnable) //设置DBMS_OUT输出是否生效
{
	bool bResult = true;
	std::string szSetSql;
	OCIStmt* pStmt = nullptr;

	if (bEnable)
		szSetSql = "begin DBMS_OUTPUT.enable(" + std::to_string(m_iDbmsOutputMax) + "); end;";
	else
		szSetSql = "begin DBMS_OUTPUT.disable; end;";

	if (OCI_SUCCESS == OCIHandleAlloc((void *)m_pEnv, (void **)&pStmt, OCI_HTYPE_STMT, (size_t)0, (void **)0))
	{
		if (OCI_SUCCESS == OCIStmtPrepare(pStmt, m_pErr, (const OraText*)szSetSql.c_str(), (ub4)szSetSql.length(), 
			OCI_NTV_SYNTAX, OCI_DEFAULT))
		{
			if (OCI_SUCCESS != OCIStmtExecute(m_DefaultConnect.pSvc, pStmt, m_pErr, 1, 0, NULL, NULL, OCI_DEFAULT))
			{
				bResult = false;
				SetErrorInfo();
			}
		}
		else
		{
			bResult = false;
			SetErrorInfo();
		}

		OCIHandleFree(pStmt, (ub4)OCI_HTYPE_STMT);
	}
	else
	{
		bResult = false;
		SetErrorInfo();
	}

	return bResult;
}

bool COCIInterface::GetDbmsOutput(std::string& szOutput, bool bGetOneRow) //获取DBMS_OUT.put_line的输出
{
	bool bResult = true;
	OCIStmt* pStmt = nullptr;
	std::string szSql = "begin DBMS_OUTPUT.get_line(:value, :status); end;";
	OCIBind* dbd[2] = { nullptr };
	sb2 dp[2] = { 0 };
	char outBuf[m_iDbmsOutputMax + 1] = { '\0' };
	ub4 outStatus = 0; //0：有值，1：没有值

	if (OCI_SUCCESS == OCIHandleAlloc((void *)m_pEnv, (void **)&pStmt, OCI_HTYPE_STMT, (size_t)0, (void **)0))
	{
		if (OCI_SUCCESS == OCIStmtPrepare(pStmt, m_pErr, (const OraText*)szSql.c_str(), (ub4)szSql.length(), OCI_NTV_SYNTAX, OCI_DEFAULT))
		{
			sword iRet = OCIBindByPos(pStmt, &dbd[0], m_pErr, 1, outBuf, m_iDbmsOutputMax, SQLT_STR, &dp[0], NULL, NULL, 0,
				NULL, OCI_DEFAULT);

			if (OCI_SUCCESS == iRet)
				iRet = OCIBindByPos(pStmt, &dbd[1], m_pErr, 2, &outStatus, 4, SQLT_INT, &dp[1], NULL, NULL, 0, NULL, OCI_DEFAULT);

			if (OCI_SUCCESS == iRet)
			{
				szOutput.clear();

				do
				{
					iRet = OCIStmtExecute(m_DefaultConnect.pSvc, pStmt, m_pErr, 1, 0, NULL, NULL, OCI_DEFAULT);

					if (OCI_SUCCESS == iRet)
					{
						if (0 == outStatus)
						{
							if (szOutput.empty())
								szOutput = outBuf;
							else
								szOutput += std::string("\n") + outBuf;

							memset(outBuf, 0, m_iDbmsOutputMax + 1);
							outStatus = 0;
						}
						else
						{
							break; //没有数据了
						}
					}
					else
					{
						bResult = false;
						SetErrorInfo();
					}
				} while (!bGetOneRow && OCI_SUCCESS == iRet);
			}
			else
			{
				bResult = false;
				SetErrorInfo();
			}
		}
		else
		{
			bResult = false;
			SetErrorInfo();
		}

		OCIHandleFree(pStmt, (ub4)OCI_HTYPE_STMT);
	}
	else
	{
		bResult = false;
		SetErrorInfo();
	}

	return bResult;
}

void COCIInterface::Test() //测试使用
{
	sb2 dp[4] = { -1, -1, -1, -1 };
	OCIDefine* df[4] = { nullptr, nullptr, nullptr, nullptr };
	//OraText* sqlstmt = (OraText*)"begin SP_TEST(:1,:2,:3,:4); end;";
	OraText* sqlstmt = (OraText*)"select I_ID,S_VARCHAR,I_AGE from T_TEST";

	struct STest
	{
		long long llId = 0;
		char strName[64] = { '\0' };
		long long llAge = 0;
	};

	STest dataTest[20];

	sword status = OCIStmtPrepare2(m_DefaultConnect.pSvc, &m_DefaultConnect.pStmt, m_pErr, sqlstmt, strlen((char*)sqlstmt), 
		NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT);
	if (OCI_SUCCESS != status)
		SetErrorInfo();

	status = OCIDefineByPos(m_DefaultConnect.pStmt, &df[0], m_pErr, 1, &dataTest[0].llId, 8, SQLT_INT, NULL, 0, 0, OCI_DEFAULT);
	if (OCI_SUCCESS != status)
		SetErrorInfo();

	status = OCIDefineByPos(m_DefaultConnect.pStmt, &df[1], m_pErr, 2, &dataTest[0].strName, 64, SQLT_STR, NULL, 0, 0, OCI_DEFAULT);
	if (OCI_SUCCESS != status)
		SetErrorInfo();

	status = OCIDefineByPos(m_DefaultConnect.pStmt, &df[2], m_pErr, 3, &dataTest[0].llAge, 8, SQLT_INT, NULL, 0, 0, OCI_DEFAULT);
	if (OCI_SUCCESS != status)
		SetErrorInfo();

	status = OCIDefineArrayOfStruct(df[0], m_pErr, sizeof(STest), 0, 0, 0);
	status = OCIDefineArrayOfStruct(df[1], m_pErr, sizeof(STest), 0, 0, 0);
	status = OCIDefineArrayOfStruct(df[2], m_pErr, sizeof(STest), 0, 0, 0);

	status = OCIStmtExecute(m_DefaultConnect.pSvc, m_DefaultConnect.pStmt, m_pErr, 0, 0, NULL, NULL, OCI_DEFAULT);
	if (OCI_SUCCESS != status)
		SetErrorInfo();

	do
	{
		//一次读取多行
		status = OCIStmtFetch2(m_DefaultConnect.pStmt, m_pErr, 4, OCI_FETCH_NEXT, 1, OCI_DEFAULT);
		if (OCI_SUCCESS != status)
			SetErrorInfo();



	} while (OCI_NO_DATA != status);

	status = OCIStmtRelease(m_DefaultConnect.pStmt, m_pErr, NULL, 0, OCI_DEFAULT);
	m_DefaultConnect.pStmt = nullptr;
}