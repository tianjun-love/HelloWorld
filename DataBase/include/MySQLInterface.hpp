/********************************************************************
名称:	MySQL数据库访问接口类
功能:	MySQL数据库访问接口
作者:	田俊
时间:	2015-05-22
修改:
*********************************************************************/
#ifndef __MYSQL_INTERFACE_HPP__
#define __MYSQL_INTERFACE_HPP__

#include "DBBaseInterface.hpp"

#ifdef _WIN32
#include <WinSock2.h>
#include "../MySQL/mysql.h"
#else
#include "mysql/mysql.h"
#endif


class CMySQLInterface : public CDBBaseInterface
{
public:
	CMySQLInterface();
	CMySQLInterface(const std::string& szServerIP, unsigned int iServerPort, const std::string& szDBName, 
		const std::string& szUserName, const std::string& szPassWord, bool bAutoCommit, EDB_CHARACTER_SET eCharSet = E_CHARACTER_UTF8, 
		unsigned int iConnTimeOut = 10U);
	CMySQLInterface(const CMySQLInterface& Other) = delete;
	virtual ~CMySQLInterface();

	CMySQLInterface& operator = (const CMySQLInterface& Other) = delete;

	//MYSQL_NEXT_ROW的结果
	struct SNextResult
	{
		unsigned int colCount;              //列数
		my_ulonglong rowCount;              //行数
		std::vector<std::string>  colNames; //列名
		std::vector<CDBRowValue*> data;     //数据

		SNextResult();
		~SNextResult();

		void Clear();
	};

	bool InitEnv() override; //初始化环境
	void FreeEnv() override; //释放资源
	bool Connect() override; //连接数据库
	bool ReConnect() override; //重新连接
	void Disconnect() override; //断开连接
	bool Prepare(const std::string& szSQL, bool bIsStmt = false) override; //发送SQL
	bool BindParm(unsigned int iIndex, EDBDataType eDataType, CDBBindBase* param) override;
	bool BindParm(unsigned int iIndex, char& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, short& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, int& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, int64_t& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, char* value, unsigned long& value_len, unsigned long buffer_len, short& indp) override;
	bool Execute(CDBResultSet* pResultSet, bool bIsStmt = false) override; //执行SQL
	bool ExecuteNoParam(const std::string& szSQL, CDBResultSet* pResultSet) override; //执行SQL，没有绑定参数
	bool ExecuteDirect(const std::string& szSQL) override; //执行SQL，没有返回结果的
	int64_t AffectedRows(bool bIsStmt = false) override; //受影响行数,-1:失败
	bool Commit() override; //提交，0成功
	bool Rollback() override; //回滚
	bool Ping() override; //检查连接是否可到达
	bool SelectDB(const std::string& szDBName); //选择数据库
	std::string GetServerVersion() override; //获取服务器版本信息字符串
	void Test() override; //测试使用

	bool ExecuteProcedure(const std::string& szExecuteSQL, const std::list<std::string>* setParamSQLList,
		const char* szResultSQL, CDBRowValue* pResultRow); //执行存储过程，out参数由结果集返回
	const std::vector<SNextResult*>& GetProcdureNextResult() const; //获取执行ExecuteProcedure可能的隐式结果

private:
	bool BindString(unsigned int iIndex, EDBDataType eStrType, CDBBindString* str) override; //绑定预处理char/varchar参数
	bool BindNumber(unsigned int iIndex, EDBDataType eNumberType, CDBBindNumber* number) override; //绑定预处理数字参数
	bool BindDateTime(unsigned int iIndex, EDBDataType eDateTimeType, CDBBindDateTime* datetime) override; //绑定预处理datetime参数
	bool BindLob(unsigned int iIndex, EDBDataType eLobType, CDBBindLob* lob) override; //绑定预处理lob参数
	bool BindResultInfo(CDBResultSet* pResultSet, bool bIsStmt = false) override; //获取结果信息，列名，长度，类型等
	bool Fetch(CDBRowValue* &pResultRowValue, bool bIsStmt = false) override; //获取下一行，返回行数
	bool GetNextResult(CDBResultSet* pResultSet, bool bIsStmt = false) override; //获取另一个结果集
	bool SetErrorInfo(const char* pAddInfo = nullptr, bool bIsStmt = false) override; //设置错误信息
	void ClearData(bool bClearAll = false) override; //清除中间临时数据
	void Clear() override; //清除所有中间数据，包括临时打开的句柄

	bool CheckPrepareInfo(unsigned int iIndex); //检查预处理情况
	bool CheckStmtParamBind(bool bIsSetErrorInfo = false); //检查预处理SQL参数绑定情况
	void GetProcdureNextResult(); //获取其它的结果集

private:
	static bool               m_bInitLibrary; //是否已经初始化库
	MYSQL                     m_Handle;       //数据库句柄
	MYSQL_RES                 *m_pResult;     //数据库结果集
	std::vector<SNextResult*> m_NextResult;   //数据库NEXT_RESULT结果集

	unsigned long     m_lStmtParamCount; //预处理参数个数
	bool              *m_pStmtBindParm;  //预处理参数绑定情况
	MYSQL_BIND        *m_pStmtParams;    //预处理参数绑定结构体
	MYSQL_BIND        *m_pStmtResult;    //预处理结果绑定结构体
	MYSQL_STMT        *m_pStmtHandle;    //预处理句柄

};

#endif