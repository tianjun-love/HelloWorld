/********************************************************************
名称:	数据库接口基类
功能:	数据库接口
作者:	田俊
时间:	2015-04-13
修改:
*********************************************************************/
#ifndef __DB_BASE_INTERFACE_HPP__
#define __DB_BASE_INTERFACE_HPP__

#include "../Public/DBResultSet.hpp"
#include "../Public/DBBindNumber.hpp"
#include "../Public/DBBindDateTime.hpp"
#include "../Public/DBBindLob.hpp"
#include "../Public/DBBindString.hpp"
#include "MemLeakCheck.hpp"

//数据库字符集
enum EDB_CHARACTER_SET
{
	E_CHARACTER_UTF8,    //utf8，mysql里面使用utf8mb4替换
	E_CHARACTER_GBK,     //gbk
	E_CHARACTER_GK18030, //gk18030，mysql使用gb2312
	E_CHARACTER_BIG5,    //big5
	E_CHARACTER_ASCII,   //ascii
	E_CHARACTER_LATIN1   //latin1，即：ISO_8859_1
};

class CDBBaseInterface
{
public:
	CDBBaseInterface();
	CDBBaseInterface(const std::string& szServerName, const std::string& szServerIP, unsigned int iServerPort, 
		const std::string& szDBName, const std::string& szUserName, const std::string& szPassWord, bool bAutoCommit, 
		EDB_CHARACTER_SET eCharSet, unsigned int iConnTimeOut);
	CDBBaseInterface(const CDBBaseInterface& Other) = delete; //数据库连接不允许拷贝
	virtual ~CDBBaseInterface();

	CDBBaseInterface& operator = (const CDBBaseInterface& Other) = delete; //数据库连接不允许赋值拷贝

	void SetServerName(const std::string& szServerName); //设置服务名
	void SetServerIP(const std::string& szIP);           //设置数据库IP
	void SetServerPort(unsigned int iPort);              //设置数据库端口
	void SetDBName(const std::string& szDBName);         //设置数据库名
	void SetUserName(const std::string& szUserName);     //设置用户名
	void SetPassWord(const std::string& szPassWord);     //设置密码
	void SetCharSet(EDB_CHARACTER_SET eCharSet);         //设置字符集
	void SetConnTimeOut(unsigned int iConnTimeOut);      //设置连接超时

	const std::string& GetServerName() const;       //获取服务名
	const std::string& GetServerIP() const;         //获取数据库IP
	const unsigned int& GetServerPort() const;      //获取数据库端口
	const std::string& GetDBName() const;           //获取数据库名
	const std::string& GetUserName() const;         //获取用户名
	const std::string& GetPassWord() const;         //获取密码
	EDB_CHARACTER_SET GetCharSet() const;           //获取字符集
	const unsigned int& GetConnTimeOut() const;     //获取连接超时

	virtual bool SetIsAutoCommit(bool bIsAutoCommit); //设置是否自动提交
	bool GetIsAutoCommit();                           //获取是否自动提交
	bool GetConnectState();                           //获取连接状态
	const char* GetSqlState() const;                  //获取SQL状态
	const int& GetErrorCode() const;                  //获取错误代码
	const char* GetErrorInfo() const;                 //获取错误信息

	//数据库操作相关函数，不同的数据库实现方法不同
	virtual bool InitEnv() = 0; //初始化环境
	virtual void FreeEnv() = 0; //释放资源
	virtual bool Connect() = 0; //连接数据库
	virtual bool ReConnect() = 0; //重新连接
	virtual void Disconnect() = 0; //断开连接
	virtual bool Prepare(const std::string& szSQL, bool bIsStmt = false) = 0; //发送SQL
	virtual bool BindParm(unsigned int iIndex, EDBDataType eDataType, CDBBindBase* param); //绑定预处理参数
	virtual bool BindParm(unsigned int iIndex, char& value, short& indp, bool bUnsigned = false) = 0;
	virtual bool BindParm(unsigned int iIndex, short& value, short& indp, bool bUnsigned = false) = 0;
	virtual bool BindParm(unsigned int iIndex, int& value, short& indp, bool bUnsigned = false) = 0;
	virtual bool BindParm(unsigned int iIndex, int64_t& value, short& indp, bool bUnsigned = false) = 0;
	virtual bool BindParm(unsigned int iIndex, char* value, unsigned long& value_len, unsigned long buffer_len, short &indp) = 0;
	virtual bool Execute(CDBResultSet* pResultSet, bool bIsStmt = false) = 0; //执行SQL
	virtual bool ExecuteNoParam(const std::string& szSQL, CDBResultSet* pResultSet) = 0; //执行SQL，没有绑定参数
	virtual bool ExecuteDirect(const std::string& szSQL) = 0; //执行SQL，没有返回结果的
	virtual int64_t AffectedRows(bool bIsStmt = false) = 0; //受影响行数，注意oracle在select语句不要使用
	virtual bool Commit() = 0; //提交
	virtual bool Rollback() = 0; //回滚
	virtual bool Ping() = 0; //检查连接是否可到达
	virtual std::string GetServerVersion() = 0; //获取服务器版本信息字符串
	virtual void Test() = 0; //测试使用

private:
	virtual bool BindString(unsigned int iIndex, EDBDataType eStrType, CDBBindString* str) = 0; //绑定预处理char/varchar参数
	virtual bool BindNumber(unsigned int iIndex, EDBDataType eNumberType, CDBBindNumber* number) = 0; //绑定预处理数字参数
	virtual bool BindDateTime(unsigned int iIndex, EDBDataType eDateTimeType, CDBBindDateTime* datetime) = 0; //绑定预处理datetime参数
	virtual bool BindLob(unsigned int iIndex, EDBDataType eLobType, CDBBindLob* lob) = 0; //绑定预处理lob参数
	virtual bool BindResultInfo(CDBResultSet* pResultSet, bool bIsStmt = false) = 0; //获取结果信息，列名，长度，类型等
	virtual bool Fetch(CDBRowValue* &pRowValue, bool bIsStmt = false) = 0; //获取下一行
	virtual bool GetNextResult(CDBResultSet* pResultSet, bool bIsStmt = false) = 0; //获取另一个结果集
	virtual bool SetErrorInfo(const char* pAddInfo = nullptr, bool bIsStmt = false) = 0; //设置错误信息
	virtual void ClearData(bool bClearAll = false) = 0; //清除中间临时数据
	virtual void Clear() = 0; //清除所有中间数据，包括临时打开的句柄

	//结果集可以获取到行数据
	friend CDBResultSet;

public:
	static EDB_CHARACTER_SET ConvertCharacterStringToEnum(const std::string& szCharacter); //字符集串转枚举
	static std::string ConvertCharacterEnumToString(EDB_CHARACTER_SET eCharacter); //字符集枚举转字符串
	static void FormatSQL(std::string& szSQL); //格式化SQL，回车符及换行符换成空格，否则可能出错
	static std::string Replace(const std::string& szSrc, const std::string& szOldStr, const std::string& szNewStr); //替换字符串中的特定字符串
	static bool MatchFormat(const std::string& szMatchStr, 
		const char* strFormat = "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})"); //使用正则表达式检查字符串格式
	static bool TrimString(std::string& szString, short type = 0); //去除字符串左右的空格，<0: 左边，=0:两边，>0:右边
	static bool ConvertBinaryToHex(const unsigned char* binary, uint64_t binary_len, std::string& szOutHex); //二进制数据转十六进制字符串
	static bool ConvertHexToBinary(const std::string& szInHex, unsigned char* binary, uint64_t& binary_len); //十六进制字符串转二进制数据
	static bool ConvertHexToChar(const char& chB, const char& chE, unsigned char& chOut); //将两位十六进制字符转成char

	typedef int(*Snprintf)(char* buf, size_t count, const char* format, ...); //格式化数据函数指针

protected:
	#define MAX_ERROR_INFO_LEN 256   //错误缓存最大字节数
	#define MAX_SQL_STATE_LEN 8      //sql状态最大长度

	std::string       m_szServerName;   //数据库服务名
	std::string       m_szServerIP;     //数据库IP
	unsigned int      m_iServerPort;    //数据库端口
	std::string       m_szDBName;       //数据库名
	std::string       m_szUserName;     //用户名
	std::string       m_szPassWord;     //密码
	EDB_CHARACTER_SET m_eCharSet;       //字符集
	unsigned int      m_iConnTimeOut;   //连接超时，秒


	bool              m_bConnectState;   //连接状态
	bool              m_bIsAutoCommit;   //是否自动提交
	static Snprintf   Format;            //格式化函数
	static std::mutex *m_pInitLock;      //初始化库锁

	char            m_strSqlState[MAX_SQL_STATE_LEN];  //SQL状态
	int             m_iErrorCode;                      //错误代码
	char            m_strErrorBuf[MAX_ERROR_INFO_LEN]; //错误信息

};

#endif