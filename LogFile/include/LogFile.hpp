/************************************************
*功能：日志输出模块
*作者：田俊
*时间：2014-11-10
*修改：
2015-09-11 田俊  增加日志等级
*************************************************/

#ifndef __LOG_FILE_HPP__
#define __LOG_FILE_HPP__

#include <iostream>
#include "LogOutObject.hpp"
#include "LogFileMember.hpp"
#include <ctime>
#include <cstring>
#include <list>

class CLogFile 
{
public:
	CLogFile();
	CLogFile(const string& szLogFileName, const string& szLogLevel, const string& szDecollator);
	CLogFile(const CLogFile& Other) = delete;
	~CLogFile();

	CLogFile& operator = (const CLogFile& Other) = delete;

	void SetLogLevel(const string& szLevel); //设置日志等级
	const eLOG_FILE_LEVEL& GetLogLevel() const; //获取日志等级
	void Close(); //关闭日志

	CLogOutObject operator << (const string& Str);
	CLogOutObject operator << (const char* pStr);
	CLogOutObject operator << (const char& cChar);
	CLogOutObject operator << (const signed char& cChar);
	CLogOutObject operator << (const unsigned char& ucChar);
	CLogOutObject operator << (const short& nNumber);
	CLogOutObject operator << (const unsigned short& unNumber);
	CLogOutObject operator << (const int& iNumber);
	CLogOutObject operator << (const unsigned int& uiNumber);
	CLogOutObject operator << (const long& lNumber);
	CLogOutObject operator << (const unsigned long& ulNumber);
	CLogOutObject operator << (const float& fNumber);
	CLogOutObject operator << (const double& dNumber);
	CLogOutObject operator << (const long long& NNumber);
	CLogOutObject operator << (const unsigned long long& uNNumber);
	CLogOutObject operator << (std::ostream& (*_F)(std::ostream&));
	CLogOutObject operator << (const CLogEnd& LogEnd);
	CLogOutObject operator << (const eLOG_FILE_LEVEL& eLogLevel);

#define LOG_FORMAT_MAX_LENGTH (2621445U) //最大一次格式化的日志长度，256K

	void Print(const eLOG_FILE_LEVEL& eLogLevel, const char* szFormat, ...); //按格式打印日志
	void PrintHex(const eLOG_FILE_LEVEL& eLogLevel, const string &szPrompt, unsigned char *buff, size_t len, 
		unsigned char ucPreSpace = 16); //按16进制输出

private:
	void Init(); //初始化
	int GetCurrentMillisecond(); //获取当前毫秒数
	string GetDateTimeStr(bool bIsDateTimeStr = true, bool bNeedMilliseconds = true); //获取日期时间字符串
	eLOGFILE_DECOLLATOR GetDecollator(const string& szDecollator); //获取日志分割标志
	eLOG_FILE_LEVEL GetLogLevel(const string& szLevel); //获取日志等级
	void CheckLogFile(); //检查日志文件是否有变更
	void CreateLogFile(); //创建日志文件

	//忽略大小写比较函数
	typedef int(*F_COMPARE)(const char*, const char*);

private:
	CLogFileMember          m_LogMember;         //日志对象
	bool                    m_bLogInit;          //日志是否初始化
	mutex                   m_Lock;              //日志锁

};

const CLogEnd logendl;  //一行日志输出结束符

#endif
