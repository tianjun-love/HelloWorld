/******************************************************
功能：	数据绑定时间类型
作者：	田俊
时间：	2018-12-14
修改：
******************************************************/
#ifndef __DB_BIND_STRING_HPP__
#define __DB_BIND_STRING_HPP__

#include "DBBindBase.hpp"

class CDBBindString : public CDBBindBase
{
public:
	CDBBindString();
	CDBBindString(const CDBBindString& Other);
	CDBBindString(const char* strString);
	CDBBindString(const std::string& szString);
	~CDBBindString();

	CDBBindString& operator = (const CDBBindString& Other);
	CDBBindString& operator = (const char* strString);
	CDBBindString& operator = (const std::string& szString);

	bool SetBuffer(unsigned long lBufferLength);
	bool SetBufferAndValue(unsigned long lBufferLength, unsigned char* pValue, unsigned long lValueLength);
	bool SetValue(unsigned char* pValue, unsigned long lValueLength);
	const char* GetData() const;
	std::string ToString() const override;
	void Clear(bool bFreeAll = false) override;

private:
	bool Init() override;

	friend class CMySQLInterface;
	friend class COCIInterface;

private:
	char m_strBuffer[D_DB_STRING_MAX_LEN + 1]; //字符串

	unsigned int   m_iRawBufferLength; //RAW数据缓存长度，oracl使用
	unsigned int   m_iRawDataLength;   //RAW数据长度，oracl使用
	unsigned char* m_pRawData;         //RAW数据，oracl使用

};

#endif