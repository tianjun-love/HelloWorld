/***********************************************************************
* FUNCTION:     列值
* AUTHOR:       田俊
* DATE：        2015-04-13
* NOTES:        
* MODIFICATION:
**********************************************************************/
#ifndef __DB_COLUMN_VALUE_HPP__
#define __DB_COLUMN_VALUE_HPP__

#include "DBPublic.hpp"

class CDBColumValue
{
public:
	CDBColumValue()
	{
		m_bIsNull = true;
		m_lDataLen = 0;
		m_pData = NULL;
		m_lBuffLen = 0;
	}

	CDBColumValue(unsigned long lBuffLen)
	{
		m_bIsNull = true;
		m_lDataLen = 0;
		m_lBuffLen = lBuffLen;
		m_pData = new char[m_lBuffLen];
		memset(m_pData, '\0', m_lBuffLen);
	}

	CDBColumValue(CDBColumValue &Other)
	{
		m_bIsNull = Other.m_bIsNull;
		m_lDataLen = Other.m_lDataLen;
		m_pData = NULL;
		m_lBuffLen = Other.m_lBuffLen;

		if (!Other.m_bIsNull && Other.m_pData)
		{
			m_pData = new char[m_lBuffLen];
			memset(m_pData, '\0', m_lBuffLen);
			strncpy(m_pData, Other.m_pData, m_lDataLen);
		}
	}
	
	~CDBColumValue()
	{
		Clear();
	}

	bool operator==(const CDBColumValue  &Other) const
	{
		if ((m_bIsNull && Other.m_bIsNull) || (m_lDataLen == 0 && Other.m_lDataLen == 0))
		{
			return true;
		}
		else if (m_lDataLen > 0 && Other.m_lDataLen > 0)
		{
			if (strcmp(m_pData, Other.m_pData) == 0)
			{
				return true;
			}
		}

		return false;
	}

	void SetValue(const char *pszValue)
	{
		if (NULL == pszValue)
		{
			ClearData();
			return;
		}
			
		m_lDataLen = (unsigned long)strlen(pszValue);
		if (0 == m_lDataLen)
		{
			ClearData();
			return;
		}

		if (m_lDataLen < m_lBuffLen)
		{
			ClearData(false);
		}
		else
		{
			m_lBuffLen = m_lDataLen + 1;
			m_pData = new char[m_lBuffLen];
			memset(m_pData, '\0', m_lBuffLen);
		}

		m_bIsNull = false;
		strncpy(m_pData, pszValue, m_lDataLen);
	}

	const char* GetValue() const
	{
		if (m_bIsNull)
			return nullptr;
		else
			return m_pData;
	}

	operator const char*() const
	{
		if (m_bIsNull)
			return nullptr;
		else
			return m_pData;
	}

	void ClearData(bool bSetZeroDataLen = true)
	{
		if (NULL != m_pData)
		{
			m_bIsNull = true;
			memset(m_pData, '\0', m_lBuffLen);

			if (bSetZeroDataLen)
			{
				m_lDataLen = 0;
			}
		}
	}

	void Clear()
	{
		if (NULL != m_pData)
		{
			delete[] m_pData;
			m_pData = NULL;
			m_lDataLen = 0;
			m_bIsNull = true;
			m_lBuffLen = 0;
		}
	}

public:
	bool	         m_bIsNull;		//是否为空
	unsigned long    m_lDataLen;    //数据长度
	char             *m_pData;      //数据
	unsigned long    m_lBuffLen;    //容器长度

};

#endif // __DB_COLUMN_VALUE__
