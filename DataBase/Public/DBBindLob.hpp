/******************************************************
功能：	数据绑定LOB类型
作者：	田俊
时间：	2018-12-14
修改：
******************************************************/
#ifndef __DB_BIND_LOB_HPP__
#define __DB_BIND_LOB_HPP__

#include "DBBindBase.hpp"

class CDBBindLob : public CDBBindBase
{
public:
	CDBBindLob();
	CDBBindLob(const CDBBindLob& Other) = delete;
	CDBBindLob(const char* strLob);
	CDBBindLob(const std::string& szLob);
	~CDBBindLob();

	CDBBindLob& operator = (const CDBBindLob& Other) = delete;
	CDBBindLob& operator = (const char* strLob);
	CDBBindLob& operator = (const std::string& szLob);

	bool SetBuffer(unsigned long lBufferLength);
	bool SetBufferAndValue(unsigned long lBufferLength, void* pValue, unsigned long lValueLength);
	bool SetValue(void* pValue, unsigned long lValueLength);
	const void* GetData() const;
	void Clear(bool bFreeAll = false) override;

private:
	bool BindLob();
	std::string ToString() const override;
	bool Init() override;

	friend class CMySQLInterface;
	friend class COCIInterface;

private:
	char        *m_pBuffer;      //LOB缓存
	void        *m_pOCISvc;      //oracle OCI服务句柄
	void        *m_pOCIErr;      //oracle OCI错误句柄
	EDBDataType m_eOCILobType;   //oracle LOB类型，CLOB或BLOB，第一次绑定时确定
	void        *m_pOCILob;      //oracle OCI LOB句柄

};

#endif