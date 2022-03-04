/******************************************************
���ܣ�	�󶨲�������
���ߣ�	�￡
ʱ�䣺	2018-12-18
�޸ģ�
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
	unsigned long GetDataLength() const { return m_lDataLength; };
	virtual std::string ToString() const = 0;
	virtual void Clear(bool bFreeAll = false) = 0;

private:
	virtual bool Init() = 0;

protected:
	unsigned long m_lDataLength;   //���ݳ��ȣ������ַ��������������
	unsigned long m_lBufferLength; //���ݻ���buf����
	short         m_nParamIndp;    //��ָʾ����oracleʹ�ã�-1����ֵ��0����ֵ��out�Ͳ�����Ϊ-1

};

#endif