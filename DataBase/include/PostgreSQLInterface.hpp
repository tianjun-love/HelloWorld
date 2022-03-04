/******************************************************
功能：	PostgreSQL接口
作者：	田俊
时间：	2018-12-28
修改：
******************************************************/
#ifndef __POSTGRESQL_INTERFACE_HPP__
#define __POSTGRESQL_INTERFACE_HPP__

#include "libpq-fe.h"
#include "DBBaseInterface.hpp"

class CPostgreSQLInterface : public CDBBaseInterface
{
public:
	CPostgreSQLInterface();
	CPostgreSQLInterface(const std::string& szIP, unsigned int iPort, const std::string& szDBName, const std::string& szUserName, 
		const std::string& szPassWord, bool bAutoCommit, EDB_CHARACTER_SET eCharSet = E_CHARACTER_UTF8, unsigned int iConnTimeOut = 10U);
	CPostgreSQLInterface(const CPostgreSQLInterface& Other) = delete;
	~CPostgreSQLInterface();

	CPostgreSQLInterface& operator = (const CPostgreSQLInterface& Other) = delete;

	bool InitEnv() override; //初始化环境
	void FreeEnv() override; //释放资源
	bool Connect() override; //连接数据库
	bool ReConnect() override; //重新连接
	void Disconnect() override; //断开连接
	bool Prepare(const std::string& szSQL, bool bIsStmt = false) override; //发送SQL
	bool Prepare(const std::string& szStmtName, const std::string& szSQL, int nParams, const Oid *paramTypes); //发送SQL
	bool BindParm(unsigned int iIndex, EDBDataType eDataType, CDBBindBase* param) override;
	bool BindParm(unsigned int iIndex, char& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, short& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, int& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, int64_t& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, char* value, unsigned long& value_len, unsigned long buffer_len, short& indp) override;
	bool Execute(CDBResultSet* pResultSet, bool bIsStmt = false) override; //执行SQL
	bool Execute(CDBResultSet* pResultSet, const std::string& szStmtName, int nParams, const char* const *paramValues,
		const int *paramLengths, const int *paramFormats, int resultFormat); //执行SQL
	bool ExecuteNoParam(const std::string& szSQL, CDBResultSet* pResultSet) override; //执行SQL，没有绑定参数
	bool ExecuteDirect(const std::string& szSQL) override; //执行SQL，没有返回结果的
	int64_t AffectedRows(bool bIsStmt = false) override; //受影响行数,-1:失败
	bool BeginTrans(); //开启事务
	bool EndTrans(); //关闭事务
	bool Commit() override; //提交
	bool Rollback() override; //回滚
	bool Ping() override; //检查连接是否可到达
	std::string GetServerVersion() override; //获取服务器版本信息字符串
	void Test() override; //测试使用

private:
	bool BindString(unsigned int iIndex, EDBDataType eStrType, CDBBindString* str) override; //绑定预处理char/varchar参数
	bool BindNumber(unsigned int iIndex, EDBDataType eNumberType, CDBBindNumber* number) override; //绑定预处理数字参数
	bool BindDateTime(unsigned int iIndex, EDBDataType eDateTimeType, CDBBindDateTime* datetime) override; //绑定预处理datetime参数
	bool BindLob(unsigned int iIndex, EDBDataType eLobType, CDBBindLob* lob) override; //绑定预处理lob参数
	bool BindResultInfo(CDBResultSet* pResultSet, bool bIsStmt = false) override; //获取结果信息，列名，长度，类型等
	bool Fetch(CDBRowValue* &pRowValue, bool bIsStmt = false) override; //获取下一行
	bool GetNextResult(CDBResultSet* pResultSet, bool bIsStmt = false) override; //获取另一个结果集
	bool SetErrorInfo(const char* pAddInfo = nullptr, bool bIsStmt = false) override; //从错误句柄获取错误信息
	void ClearData(bool bClearAll = false) override; //清除中间临时数据
	void Clear() override; //清除所有中间数据，包括临时打开的句柄

private:
	std::string m_szConnectInfo; //连接信息
	PGconn      *m_pConnect;     //连接句柄
	PGresult    *m_pResult;      //结果句柄
	int         m_iRowCount;     //结果总行数
	int         m_iCurrFetchRow; //当前获取的行号

};

#endif