/*******************************************
功能：时间处理类
作者：田俊
时间：2019-05-22
修改：
*******************************************/
#ifndef __TIME_HPP__
#define __TIME_HPP__

#include "DateTime.hpp"

class CTime : public CDateTime
{
public:
	CTime();
	CTime(unsigned int iHour, unsigned int iMin, unsigned int iSec);
	CTime(const std::string& szTime);
	CTime(time_t lSeconds);
	CTime(const CDateTime& DateTime);
	CTime(const CTime& Other);
	virtual ~CTime();

	CTime &operator=(const CTime &Other);
	CTime &operator=(const std::string &szTime);
	CTime &operator=(time_t lSeconds);
	CTime &operator=(const CDateTime &DateTime);
	operator std::string() const;

	void InitCurrTime(time_t lSecond = 0);

private:
	void CopyFromTime(const std::string &szTime);
	void CopyFromDateTime(const CDateTime &DateTime);
	void truncateDate(time_t &tDateTime);

};

std::ostream& operator<<(std::ostream &out, const CTime& Time);

#endif