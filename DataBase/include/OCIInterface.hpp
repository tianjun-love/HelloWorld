/********************************************************************
名称:	OCI接口类
功能:	OCI接口
作者:	田俊
时间:	2015-04-13
修改:
*********************************************************************/
#ifndef __OCI_INTERFACE_HPP__
#define __OCI_INTERFACE_HPP__

#include "oci.h"
#include "DBBaseInterface.hpp"
#include "../Public/DBBindRefCursor.hpp"

class COCIInterface : public CDBBaseInterface
{
public:
	//单个用户连接信息
	struct SOraUserConnect
	{
		OCISvcCtx  *pSvc = nullptr;   //服务上下文句柄，包含服务器、会话及事物句柄
		OCISession *pSess = nullptr;  //会话句柄
		OCIStmt    *pStmt = nullptr;  //语句句柄
		OCIStmt    *pImplicitStmt = nullptr;//隐式结果句柄，不用释放，释放顶级语句句柄时，将自动关闭并释放所有隐式结果集
		OCITrans   *pTrans = nullptr;       //全局事务句柄，需要才开启，在服务上下文句柄中会有简单的本地隐式事务
	};

	//返回结果绑定信息
	struct SOraDefineInfo
	{
		unsigned long  lDefineCount = 0;         //绑定结果字段数
		void           **pDefineRow = nullptr;   //绑定结果行
		sb2            *pDefineDp = nullptr;     //绑定指示器，-2是被截断，-1是空值，0完整读出
		EDBDataType    *pDefineType = nullptr;   //绑定结果字段类型
	};

public:
	COCIInterface();
	COCIInterface(const std::string& szServerName, const std::string& szUserName, const std::string& szPassWord, bool bAutoCommit, 
		EDB_CHARACTER_SET eCharSet = E_CHARACTER_UTF8, unsigned int iConnTimeOut = 10U);
	COCIInterface(const COCIInterface& Other) = delete;
	virtual ~COCIInterface();

	COCIInterface& operator = (const COCIInterface& Other) = delete;

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
	bool BindRefCursor(unsigned int iIndex, CDBBindRefCursor& refCursor); //绑定预处理结果参数
	bool Execute(CDBResultSet* pResultSet, bool bIsStmt = false) override; //执行SQL
	bool ExecuteNoParam(const std::string& szSQL, CDBResultSet* pResultSet) override; //执行SQL，没有绑定参数
	bool ExecuteDirect(const std::string& szSQL) override; //执行SQL，没有返回结果的
	int64_t AffectedRows(bool bIsStmt = false) override; //受影响行数,-1:失败
	bool BeginTrans(); //开启事务，暂时不建议使用
	bool EndTrans(); //关闭事务，暂时不建议使用
	bool Commit() override; //提交
	bool Rollback() override; //回滚
	bool Ping() override; //检查连接是否可到达
	std::string GetServerVersion() override; //获取服务器版本信息字符串
	void Test() override; //测试使用

	void GetOCIHandleForBatch(OCISvcCtx* &pSvc, OCIStmt* &pStmt, OCIError* &pErr); //获取OCI句柄，用于批量读取或插入，调用Prepare后自己实现
	bool GetPararmResult(const CDBBindRefCursor& refCursor, CDBResultSet& resultSet); //获取绑定SYS_REFCURSOR参数的结果
	bool SetDbmsOutputEnable(bool bEnable = true); //设置DBMS_OUT输出是否生效
	bool GetDbmsOutput(std::string& szOutput, bool bGetOneRow = true); //获取DBMS_OUT.put_line的输出

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
	bool CheckPrepare(); //检查语句是否已经发送
	bool CreateUserConnect(SOraUserConnect& conn); //创建用户连接信息
	void FreeUserConnect(SOraUserConnect& conn); //清除用户连接信息
	void FreeDefineInfo(SOraDefineInfo& def); //释放返回结果绑定信息

	friend class CDBBindRefCursor;

private:
	OCIEnv           *m_pEnv;          //环境句柄
	OCIError         *m_pErr;          //错误句柄
	OCIServer        *m_pSer;          //服务器句柄
	SOraUserConnect  m_DefaultConnect; //默认连接
	SOraDefineInfo   m_DefaultDefine;  //默认绑定结果信息
	static const int m_iDbmsOutputMax; //默认20000bytes，最小2000，最大1000000
	bool             m_bSrcAutoCommit; //原始自动提交标志
	bool             m_bTransCommit;   //事务是否已经提交

};

#endif