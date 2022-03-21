/*******************************************
功能：日期时间处理类
作者：田俊
时间：2019-05-22
修改：
*******************************************/
#ifndef __DATE_TIME_HPP__
#define __DATE_TIME_HPP__

#include <string>
#include <ostream>
#include <ctime>

class CDateTime
{
private:
	friend class CDate;
	friend class CTime;

	enum ETIMEPART
	{
		e_NONE = 0,          //none
		e_YEAR = 1,          //year
		e_MON  = 2,          //month
		e_DAY  = 3,          //day
		e_HOUR = 4,          //hour
		e_MIN  = 5,          //min
		e_SEC  = 6,          //sec
		e_WEEK = 7           //week
	};
public:
	CDateTime();
	CDateTime(unsigned int iYear, unsigned int iMon, unsigned int iDay, unsigned int iHour, unsigned int iMin, 
		unsigned int iSec);
	CDateTime(time_t lSeconds);
	CDateTime(const std::string& szDateTime);
	CDateTime(const CDateTime& Other);
	virtual ~CDateTime();

	CDateTime& operator =(const CDateTime& Other);
	CDateTime& operator =(time_t lSeconds);
	CDateTime& operator =(const std::string& szDateTime);

	static clock_t GetRunMs();                                             //get program run time
	static time_t GetSeconds();                                            //get from 1970-01-01 00:00:00 to now seconds
	static time_t GetMilliSeconds();                                    //get from 1970-01-01 00:00:00 to now milli
	static time_t GetMicroSeconds();                                    //get from 1970-01-01 00:00:00 to now micro
	static bool CheckIsLeapYear(unsigned int iYear);                       //check leap year
	void InitCurrDateTime(time_t lSecond = 0);                             //init by current time
	time_t GetDateTimeSeconds() const;                                     //return m_lSeconds
	void SetDateTimeSeconds(time_t lSecond);                               //set m_lSeconds
	clock_t GetDateTimePart(const std::string& szPart, clock_t iAdd = 0) const; //get datetime part
	std::string GetDateTimeStr(bool bNeedMilliseconds = false) const;           //get date str
	std::string GetDateStr(const std::string& szDecollator = "-") const;        //get day str
	std::string GetTimeStr(const std::string& szDecollator = ":", bool bNeedMilliseconds = false) const; //get time str
	std::string GetDateMonStr(const std::string& szDecollator = "") const;
	std::string GetDateHourStr(const std::string& szDecollator = "") const;
	std::string GetDateMinStr(const std::string& szDecollator = "") const;

	void AddYear(int iYear);
	void AddMonth(int iMonth);
	void AddDay(int iDay);
	void AddHour(int iHour);
	void AddMinute(time_t iMin);
	void AddSecond(time_t iSecond);

	int GetYear() const;
	int GetMonth() const;
	int GetDay() const;
	int GetHour() const;
	int GetMinute() const;
	int GetSecond() const;
	int GetWDay() const;

	void SetYear(int iValue);
	void SetMonth(int iValue);
	void SetDay(int iValue);
	void SetHour(int iValue);
	void SetMinute(int iValue);
	void SetSecond(int iValue);

	bool operator ==(const CDateTime& Other) const;
	bool operator !=(const CDateTime& Other) const;
	bool operator <(const CDateTime& Other) const;
	bool operator >(const CDateTime& Other) const;
	bool operator <=(const CDateTime& Other) const;
	bool operator >=(const CDateTime& Other) const;
	time_t operator -(const CDateTime& Other) const;
	operator std::string() const;

protected:
	static ETIMEPART GetEnum(const std::string& szPart);
	static void LocalTime(const time_t& tSeconds, tm& tmTemp);  //init tm struct
	static void CopyFromDateTimeStr(time_t& tSeconds, const std::string& szDateTime);  //init by string
	static int GetCurrentMillisecond(); //get current millisecond

private:
	time_t            m_lSeconds;

};

std::ostream& operator <<(std::ostream& out, const CDateTime& Other);

#endif
