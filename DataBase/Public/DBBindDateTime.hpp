/******************************************************
功能：	数据绑定时间类型
作者：	田俊
时间：	2018-12-14
修改：
******************************************************/
#ifndef __DB_BIND_DATE_TIME__
#define __DB_BIND_DATE_TIME__

#include "DBBindBase.hpp"

class CDBBindDateTime : public CDBBindBase
{
public:
	CDBBindDateTime();
	CDBBindDateTime(const CDBBindDateTime& Other) = delete;
	CDBBindDateTime(const char* strDateTime);
	CDBBindDateTime(const std::string& szDateTime);
	~CDBBindDateTime();

	CDBBindDateTime& operator = (const CDBBindDateTime& Other) = delete;
	CDBBindDateTime& operator = (const char* strDateTime);
	CDBBindDateTime& operator = (const std::string& szDateTime);

	bool CheckFormat(EDBDataType eDateTimeType, const char* strDateTime = nullptr);
	const void* GetData() const;
	std::string ToString() const override;
	void Clear(bool bFreeAll = false) override;

private:
	bool Init() override;
	bool BindOCIDateTime(); //绑定OCI的时间戳
	bool CheckIsLeapYear(unsigned int iYear); //检查是否是润年
	bool CheckDate(unsigned int iYear, unsigned int iMonth, unsigned int iDay); //检查日期是否正常
	bool CheckTime(unsigned int iHour, unsigned int iMinute, unsigned int iSecond); //检查时间是否正常

	friend class CMySQLInterface;
	friend class COCIInterface;

private:
	char        m_strBuffer[D_DB_DATETIME_MAX_LEN + 1]; //日期时间字符串
	void        *m_pOCIEnv;                             //oracle OCI环境句柄
	void        *m_pOCIErr;                             //oracle OCI错误句柄
	EDBDataType m_eOCIDateTimeType;                     //oracle 日期时间类型，date或timestamp，第一次绑定时确定
	void        *m_pOCITimestamp;                       //oracle绑定timestamp使用

};

#endif