/*************************************************
功能:	达梦数据库DPI接口对象
作者:	田俊
时间:	2021-07-27
修改:
*************************************************/
#ifndef __DB_DM_DPI_INTERFACE_HPP__
#define __DB_DM_DPI_INTERFACE_HPP__

#include "DBBaseInterface.hpp"

#include "../DM_DPI/DPI.h"
#include "../DM_DPI/DPIext.h"
#include "../DM_DPI/DPItypes.h"

class CDMDPIInterface : public CDBBaseInterface
{
public:
	CDMDPIInterface();
	CDMDPIInterface(const std::string& szServerIP, unsigned int iServerPort, const std::string& szUserName, 
		const std::string& szPassWord, bool bAutoCommit, EDB_CHARACTER_SET eCharSet = E_CHARACTER_UTF8, unsigned int iTimeOut = 10U);
	CDMDPIInterface(const CDMDPIInterface &Other) = delete;
	virtual ~CDMDPIInterface();

	CDMDPIInterface& operator = (const CDMDPIInterface &Other) = delete;

	bool InitEnv() override;                                                          //初始化环境
	void FreeEnv() override;                                                          //释放资源
	bool Connect() override;                                                          //连接数据库
	bool ReConnect() override;                                                        //重新连接
	void Disconnect() override;                                                       //断开连接
	bool Prepare(const std::string& szSQL, bool bIsStmt = false) override;            //发送SQL
	bool BindParm(unsigned int iIndex, char& value, short& indp, bool bUnsigned = false) override; //绑定参数，注意索引从1开始
	bool BindParm(unsigned int iIndex, short& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, int& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, int64_t& value, short& indp, bool bUnsigned = false) override;
	bool BindParm(unsigned int iIndex, char* value, unsigned long& value_len, unsigned long buffer_len, short &indp) override;
	bool Execute(CDBResultSet* pResultSet, bool bIsStmt = false) override;            //执行SQL
	bool ExecuteNoParam(const std::string& szSQL, CDBResultSet* pResultSet) override; //执行SQL，不用绑定参数
	int64_t AffectedRows(bool bIsStmt = false) override;                            //影响行数
	bool Commit() override;                                                           //提交，0成功
	bool Rollback() override;                                                         //回滚
	bool Ping() override;                                                             //检查连接是否可到达
	bool SetIsAutoCommit(bool bIsAutoCommit) override;                                //设置是否自动提交

private:
	bool BindString(unsigned int iIndex, EDBDataType eStrType, CDBBindString* str) override; //绑定预处理char/varchar参数
	bool BindNumber(unsigned int iIndex, EDBDataType eNumberType, CDBBindNumber* number) override; //绑定预处理数字参数
	bool BindDateTime(unsigned int iIndex, EDBDataType eDateTimeType, CDBBindDateTime* datetime) override; //绑定预处理datetime参数
	bool BindLob(unsigned int iIndex, EDBDataType eLobType, CDBBindLob* lob) override; //绑定预处理lob参数
	bool BindResultInfo(CDBResultSet *pResultSet, bool bIsStmt = false) override;      //获取结果信息，列名，长度，类型等
	bool Fetch(CDBRowValue* &pRowValue, bool bIsStmt) override;                        //获取下一行，返回行数
	bool GetNextResult(CDBResultSet* pResultSet, bool bIsStmt = false) override;       //获取另一个结果集
	bool SetErrorInfo(const char* pAddInfo = nullptr, bool bIsStmt = false) override;  //设置错误信息，达梦调用方式不一样
	bool SetErrorInfo(sdint2 hndl_type, dhandle hndl);                                 //获取错误信息
	void ClearData(bool bClearAll = false) override;                                   //清除绑定结果数据
	void Clear() override;                                                             //清除所有中间数据，包括临时打开的句柄

	bool CheckBindIndex(unsigned int iIndex);   //检查绑定参数索引

private:
	dhenv            m_pEnv;         //环境句柄
	dhcon            m_pCon;         //连接句柄
	dhstmt           m_pStmt;        //语句句柄

	sdint2           m_nBindCount;   //绑定字段数
	void**           m_pBindRow;     //绑定行数据
	EDBDataType      *m_pBindType;   //绑定字段类型
	slength          *m_pBindInd;    //绑定的字段值长度

};

#endif