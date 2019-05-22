#ifndef __LOG_OUT_OBJECT_HPP__
#define __LOG_OUT_OBJECT_HPP__

#include <iostream>
#include "LogFileMember.hpp"
#include "LogEnd.hpp"

class CLogOutObject
{
public:
	CLogOutObject(void* pLogMember, bool bLost = false);
	~CLogOutObject();

	CLogOutObject& operator << (const string& Str);
	CLogOutObject& operator << (const char*  pStr);
	CLogOutObject& operator << (const char& cChar);
	CLogOutObject& operator << (const signed char& cChar);
	CLogOutObject& operator << (const unsigned char& ucChar);
	CLogOutObject& operator << (const short& nNumber);
	CLogOutObject& operator << (const unsigned short& unNumber);
	CLogOutObject& operator << (const int& iNumber);
	CLogOutObject& operator << (const unsigned int& uiNumber);
	CLogOutObject& operator << (const long& lNumber);
	CLogOutObject& operator << (const unsigned long& ulNumber);
	CLogOutObject& operator << (const float& fNumber);
	CLogOutObject& operator << (const double& dNumber);
	CLogOutObject& operator << (const long long& NNumber);
	CLogOutObject& operator << (const unsigned long long& uNNumber);
	CLogOutObject& operator << (std::ostream& (*_F)(std::ostream&));
	CLogOutObject& operator << (const CLogEnd& LogEnd);
	CLogOutObject& operator << (const eLOG_FILE_LEVEL& eLogLevel);

private:
	void*      m_pLogMember;       //日志对象
	bool       m_bLost;            //是否输出
	bool       m_bIsLastOutputEnd; //是否最后输出结束

};

#endif
