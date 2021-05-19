#include "../include/LogFileBackup.hpp"
#include "../include/LogFile.hpp"
#include "QtumCompress/include/QtumCompress.hpp"
#include <time.h>

CLogFileBackup::CLogFileBackup(void *pLogFile) : m_pLogFile(pLogFile)
{
}

CLogFileBackup::~CLogFileBackup()
{
}

/**************************************************
描述：	日志备份处理
参数：	szLogFileNamePrefix 日志文件名前缀，如：../log/QuantumNMSServer
*		eDeco 日志文件分割类型
*		iCurrentLogRetainDays 当前日志保留天数，0：只保留当天，<0：不备份
返回：	无
**************************************************/
void CLogFileBackup::BackupHandle(const string &szLogFileNamePrefix, eLOGFILE_DECOLLATOR eDeco, int iCurrentLogRetainDays)
{
	if (0 > iCurrentLogRetainDays)
		return;

	CLogFile *pLogFile = (CLogFile*)m_pLogFile;
	string szLogPath, szLogBackupPath, szFileNamePrefix, szLogfileSrcName, szLogfileTargzName, szError;
	std::list<CQtumCompress::SFileInfo> filesList;
	CQtumCompress comp;
	std::list<std::string> compressFileList;

	//分析文件路径
	string::size_type iPos = szLogFileNamePrefix.find_last_of("/");
#ifdef _WIN32
	if (string::npos == iPos)
		iPos = szLogFileNamePrefix.find_last_of("\\");
#endif

	if (string::npos == iPos)
	{
		szLogPath = ".";
		szLogBackupPath = ".";
		szFileNamePrefix = szLogFileNamePrefix;
	}
	else
	{
		szLogPath = szLogFileNamePrefix.substr(0, iPos);
		szFileNamePrefix = szLogFileNamePrefix.substr(iPos + 1);

		//不严谨，windows上可能有问题，项目内能用即可
		if ("/" == szLogPath 
			|| (3 == szLogPath.length() && ':' == szLogPath[1] && ('/' == szLogPath[2] || '\\' == szLogPath[2])))
		{
			szLogBackupPath = szLogPath;
		}
		else
		{
			string::size_type iPos = szLogPath.find_last_of("/");
#ifdef _WIN32
			if (string::npos == iPos)
				iPos = szLogPath.find_last_of("\\");
#endif

			if (string::npos == iPos)
				szLogBackupPath = "log_backup";
			else
				szLogBackupPath = szLogPath.substr(0, iPos + 1) + "log_backup";
		}
	}

	//如果不存在则创建备份文件目录
	if (!comp.MkDir(szLogBackupPath, 0755, false, szError))
	{
		if (NULL != pLogFile)
		{
			pLogFile->Print(E_LOG_LEVEL_WARN, "Logfile backup service create backup dir: %s failed: %s", szLogBackupPath.c_str(),
				szError.c_str());
		}
	}

	//处理
	if (NULL != pLogFile)
	{
		pLogFile->Print(E_LOG_LEVEL_PROMPT, "Logfile backup service check and handle log files start...");
	}

	//获取日志目录的文件信息
	if (comp.GetDirInfo(szLogPath, filesList, szError, false))
	{
		for (const auto &file : filesList)
		{
			//只处理文件
			if (CQtumCompress::e_File == file.eType)
			{
				//对比时间，文件名如：QuantumNMSServer_20210513.log
				if (CompareLogfileDate(file.szName.substr(szFileNamePrefix.length() + 1), eDeco, iCurrentLogRetainDays))
				{
					szLogfileSrcName = file.szBaseDir + "/" + file.szName;
					szLogfileTargzName = szLogBackupPath + "/" + file.szName.substr(0, file.szName.find_last_of(".")) + ".tar.gz";
					compressFileList.push_back(szLogfileSrcName);

					//压缩到备份目录
					if (comp.CompressGZIP(compressFileList, szLogfileTargzName, CQtumCompress::e_Level_Default, szError))
					{
						//成功后删除源文件
						if (!comp.RmFile(szLogfileSrcName, false, szError))
						{
							if (NULL != pLogFile)
							{
								pLogFile->Print(E_LOG_LEVEL_WARN, "Logfile backup service compress file: %s ==> %s success, delete source file failed: %s", 
									szLogfileSrcName.c_str(), szLogfileTargzName.c_str(), szError.c_str());
							}
						}

						if (NULL != pLogFile)
						{
							pLogFile->Print(E_LOG_LEVEL_PROMPT, "Logfile backup service compress file: %s ==> %s success.", 
								szLogfileSrcName.c_str(), szLogfileTargzName.c_str());
						}
					}
					else
					{
						if (NULL != pLogFile)
						{
							pLogFile->Print(E_LOG_LEVEL_WARN, "Logfile backup service compress file: %s ==> %s failed: %s", 
								szLogfileSrcName.c_str(), szLogfileTargzName.c_str(), szError.c_str());
						}
					}

					compressFileList.clear();
				}
			}
		}
	}
	else
	{
		if (NULL != pLogFile)
		{
			pLogFile->Print(E_LOG_LEVEL_WARN, "Logfile backup service get log dir: %s files failed: %s", szLogPath.c_str(),
				szError.c_str());
		}
	}

	filesList.clear();

	if (NULL != pLogFile)
	{
		pLogFile->Print(E_LOG_LEVEL_PROMPT, "Logfile backup service check and handle log files end.");
	}

	return;
}

/**************************************************
描述：	比较日志文件时间
参数：	szFileDate 日志文件时间，如：20210518
*		eDeco 日志文件分割类型
*		iCurrentLogRetainDays 当前日志保留天数，0：只保留当天，<0：不备份
返回：	要压缩备份返回true
**************************************************/
bool CLogFileBackup::CompareLogfileDate(const string &szFileDate, eLOGFILE_DECOLLATOR eDeco, int iCurrentLogRetainDays)
{
	if (0 > iCurrentLogRetainDays)
		return false;

	//检查格式
	if (szFileDate.empty())
		return false;
	else
	{
		int iMinDateLen = 6; //按月

		if (E_LOG_DECO_DAY == eDeco) //按天
			iMinDateLen = 8;
		else if (E_LOG_DECO_HOUR == eDeco) //按小时
			iMinDateLen = 10;

		if ((int)szFileDate.length() < iMinDateLen)
			return false;

		for (int i = 0; i < iMinDateLen; ++i)
		{
			if (szFileDate[i] < '0' || szFileDate[i] > '9')
				return false;
		}
	}

	const time_t retainDaysToSeconds = iCurrentLogRetainDays * 86400; //天数转换为秒数
	time_t currentSeconds = time(NULL), fileSeconds = 0;
	struct tm fileDateTime;

	//localtime函数不可重入，即不是线程安全的，所以用下面的版本
#ifdef _WIN32
	localtime_s(&fileDateTime, &currentSeconds);
#else
	localtime_r(&currentSeconds, &fileDateTime);
#endif
	
	//保证除开要比较的值，其它属性一致
	fileDateTime.tm_year = atoi(szFileDate.substr(0, 4).c_str()) - 1900;
	fileDateTime.tm_mon = atoi(szFileDate.substr(4, 2).c_str()) - 1;

	if (E_LOG_DECO_DAY == eDeco || E_LOG_DECO_HOUR == eDeco)
		fileDateTime.tm_mday = atoi(szFileDate.substr(6, 2).c_str());

	if (E_LOG_DECO_HOUR == eDeco)
		fileDateTime.tm_hour = atoi(szFileDate.substr(8, 2).c_str());

	fileSeconds = mktime(&fileDateTime);

	//比较
	if ((currentSeconds - fileSeconds) > retainDaysToSeconds)
		return true;

	return false;
}