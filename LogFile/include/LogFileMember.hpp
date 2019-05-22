#ifndef __LOG_FILE_MEMBER_HPP__
#define __LOG_FILE_MEMBER_HPP__

#include <string>
#include <fstream>
#include <mutex>

using std::string;
using std::ofstream;
using std::ios;
using std::mutex;

//日志等级枚举
enum eLOG_FILE_LEVEL
{
	E_LOG_LEVEL_ALL       = 0, //所有
	E_LOG_LEVEL_SERIOUS   = 1, //严重
	E_LOG_LEVEL_WARN      = 2, //警告
	E_LOG_LEVEL_PROMPT    = 3, //提示
	E_LOG_LEVEL_DEBUG     = 4  //调试
};

//日志分割方式
enum eLOGFILE_DECOLLATOR
{
	E_LOG_DECO_MINUTE = 0,     //按分钟分割
	E_LOG_DECO_HOUR   = 1,     //按小时分割
	E_LOG_DECO_DAY    = 2,     //按天分割
	E_LOG_DECO_MONTH  = 3      //按月分割
};

class CLogFileMember
{
public:
	friend class CLogFile;
	friend class CLogOutObject;

public:
	CLogFileMember();
	CLogFileMember(const string& szFileName, const string& szDate, eLOG_FILE_LEVEL eLogLevel = E_LOG_LEVEL_ALL,
		eLOGFILE_DECOLLATOR eLogDeco = E_LOG_DECO_DAY);
	CLogFileMember(const CLogFileMember& Other) = delete;
	~CLogFileMember();

	CLogFileMember& operator = (const CLogFileMember& Other) = delete;

	static string GetLogLevelStr(eLOG_FILE_LEVEL eLevel); //获取日志等级字符串

private:
	ofstream            m_Ofstream;   //日志输出文件流
	mutex               m_LogLock;    //日志输出锁
	string              m_szFileName; //日志文件名，如：log/AlarmSystemManager
	string              m_szDate;     //日志时间，如：20150915
	eLOG_FILE_LEVEL     m_eLogLevel;  //日志等级
	mutex               m_LevelLock;  //修改日志等级锁
	eLOGFILE_DECOLLATOR m_eLogDeco;   //日志分割标志
	bool                m_bOpen;      //日志文件是否打开
	bool                m_bLogendl;   //是否已经输入logendl
};

#endif
