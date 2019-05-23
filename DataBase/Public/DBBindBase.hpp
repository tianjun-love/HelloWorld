/******************************************************
功能：	绑定参数基类
作者：	田俊
时间：	2018-12-18
修改：
******************************************************/
#ifndef __DB_BIND_BASE__
#define __DB_BIND_BASE__

#include "DBPublic.hpp"

class CDBBindBase
{
public:
	CDBBindBase() : m_lDataLength(0), m_lBufferLength(0), m_nParamIndp(-1) {};
	CDBBindBase(const CDBBindBase& Other) : m_lDataLength(Other.m_lDataLength), m_lBufferLength(Other.m_lBufferLength),
		m_nParamIndp(Other.m_nParamIndp) {};
	~CDBBindBase() {};

	CDBBindBase& operator = (const CDBBindBase& Other)
	{
		if (&Other != this)
		{
			m_lDataLength = Other.m_lDataLength;
			m_lBufferLength = Other.m_lBufferLength;
			m_nParamIndp = Other.m_nParamIndp;
		}

		return *this;
	}

	void SetParamIndp(short indp) { m_nParamIndp = indp; };
	short GetParamIndp() const { return m_nParamIndp; };
	virtual std::string ToString() const = 0;
	virtual void Clear(bool bFreeAll = false) = 0;

private:
	virtual bool Init() = 0;

protected:
	unsigned long m_lDataLength;   //数据长度，用于字符串或二进制数据
	unsigned long m_lBufferLength; //数据缓存buf长度
	short         m_nParamIndp;    //绑定指示器，oracle使用，-1：空值，0：有值，out型参数必为-1

};

#endif