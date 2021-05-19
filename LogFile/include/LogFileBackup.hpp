/*************************************************
功能:	日志文件备份功能类
作者:	田俊
时间:	2021-05-18
修改:
*************************************************/
#ifndef __LOG_FILE_BACKUP_HPP__
#define __LOG_FILE_BACKUP_HPP__

#include "LogFileMember.hpp"

class CLogFileBackup
{
public:
	CLogFileBackup(void *pLogFile);
	~CLogFileBackup();

	void BackupHandle(const string &szLogFileNamePrefix, eLOGFILE_DECOLLATOR eDeco, int iCurrentLogRetainDays);

private:
	static bool CompareLogfileDate(const string &szFileDate, eLOGFILE_DECOLLATOR eDeco, int iCurrentLogRetainDays);

private:
	void     *m_pLogFile;        //日志对象

};

#endif