/*******************************************
功能：日期处理类
作者：田俊
时间：2019-05-22
修改：
*******************************************/
#ifndef __DATE_HPP__
#define __DATE_HPP__

#include "DateTime.hpp"

class CDate : public CDateTime
{
public:
	CDate();
	CDate(unsigned int iYear, unsigned int iMon, unsigned int iDay);
	CDate(const std::string &szDate);
	CDate(time_t lSeconds);
	CDate(const CDateTime &DateTime);
	CDate(const CDate &Other);
	virtual ~CDate();

	CDate &operator=(const CDate &Other);
	CDate &operator=(const std::string &szDate);
	CDate &operator=(time_t lSeconds);
	CDate &operator=(const CDateTime &DateTime);
	operator std::string() const;

	void InitCurrDate(time_t lSecond = time_t(0));

private:
	void CopyFromDate(const std::string &szDate);
	void CopyFromDateTime(const CDateTime &Other);
	void truncateTime(time_t &tDateTime);

};

std::ostream& operator<<(std::ostream &out, const CDate& Date);

#endif