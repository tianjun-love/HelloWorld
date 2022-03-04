/***********************************************************************
* FUNCTION:     数据库行数据
* AUTHOR:       田俊
* DATE：        2015-04-13
* NOTES:        
* MODIFICATION:
**********************************************************************/
#ifndef __DB_ROW_VALUE_HPP__
#define __DB_ROW_VALUE_HPP__

#include "DBColumnValue.hpp"

class CDBRowValue
{
public:
	CDBRowValue()
	{
		m_iColumnCount    = 0;
		m_pDBColumnValues = NULL;
	}

	CDBRowValue(const CDBRowValue &Other)
	{
		m_iColumnCount    = Other.m_iColumnCount;
		m_pDBColumnValues = NULL;

		if (m_iColumnCount > 0)
		{
			CreateColumn(m_iColumnCount);
			for (unsigned int i = 0; i < m_iColumnCount; i++)
			{
				CDBColumValue* lpTemp = *(Other.m_pDBColumnValues + i);
				CDBColumValue* lpDBColumnValue = NULL;

				if (lpTemp != NULL)
				{
					lpDBColumnValue = new CDBColumValue(*lpTemp);
				}
				else
				{
					lpDBColumnValue = new CDBColumValue();
				}

				AddColumnValue(lpDBColumnValue, i);
			}
		}
	}

	CDBRowValue &operator = (const CDBRowValue &Other)
	{
		m_iColumnCount    = Other.m_iColumnCount;
		m_pDBColumnValues = NULL;

		if (m_iColumnCount > 0)
		{
			CreateColumn(m_iColumnCount);
			for (unsigned int i = 0; i < m_iColumnCount; i++)
			{
				CDBColumValue* lpTemp = *(Other.m_pDBColumnValues + i);
				CDBColumValue* lpDBColumnValue = NULL;

				if (lpTemp != NULL)
				{
					lpDBColumnValue = new CDBColumValue(*lpTemp);
				}
				else
				{
					lpDBColumnValue = new CDBColumValue();
				}
				
				AddColumnValue(lpDBColumnValue, i);
			}
		}

		return *this;
	}

	CDBRowValue(unsigned int iColumnCount)
	{
		CreateColumn(iColumnCount);
	}
	
	~CDBRowValue()
	{
		Clear();
	}

	void ClearData()
	{
		if (m_pDBColumnValues != NULL)
		{
			for (unsigned int i = 0; i < m_iColumnCount; i++)
			{
				CDBColumValue* lpColumnValue = *(m_pDBColumnValues + i);
				lpColumnValue->ClearData();
			}
		}
	}

	void Clear()
	{
		if (m_pDBColumnValues != NULL)
		{
			for (unsigned int i = 0; i < m_iColumnCount; i++)
			{
				CDBColumValue* lpColumnValue = *(m_pDBColumnValues + i);
				if (NULL != lpColumnValue)
				{
					lpColumnValue->Clear();
					delete lpColumnValue;
					lpColumnValue = NULL;
				}
			}
			
			m_iColumnCount = 0;
			delete[] m_pDBColumnValues;
			m_pDBColumnValues = NULL;
		}
	}

	unsigned int GetColnumCount() const
	{
		return m_iColumnCount;
	}

	void CreateColumn(unsigned int iColumnCount)
	{
		if (iColumnCount > 0)
		{
			m_pDBColumnValues = new CDBColumValue*[iColumnCount];
			m_iColumnCount = iColumnCount;

			for (unsigned int i = 0; i < m_iColumnCount; i++)
			{
				*(m_pDBColumnValues + i) = NULL;
			}
		}
	}

	void AddColumnValue(CDBColumValue* lpDBColumnValue, unsigned int iPosition)
	{
		if (iPosition >= 0 && iPosition < m_iColumnCount)
		{
			*(m_pDBColumnValues + iPosition) = lpDBColumnValue;
		}
	}

	CDBColumValue* GetColumnValue(unsigned int iPosition) const
	{
		if (iPosition >=0 && iPosition < m_iColumnCount)
		{
			return *(m_pDBColumnValues + iPosition);
		}
		else
		{
			return NULL;
		}
	}

	const char *GetValue(unsigned int iPosition) const
	{
		const char *pszValue = NULL;
		CDBColumValue* lpDBColumnValue = NULL;

		if (iPosition >=0 && iPosition < m_iColumnCount)
		{
			lpDBColumnValue = *(m_pDBColumnValues + iPosition);
		}

		if (lpDBColumnValue != NULL && !lpDBColumnValue->m_bIsNull)
		{
			pszValue = lpDBColumnValue->GetValue();
		}

		return pszValue;
	}

	void SetValue(unsigned int iPosition, const char* pszValue)
	{
		CDBColumValue* lpDBColumnValue = NULL;

		if (iPosition >=0 && iPosition < m_iColumnCount)
		{
			lpDBColumnValue = *(m_pDBColumnValues + iPosition);
		}
		
		if (lpDBColumnValue != NULL)
		{
			lpDBColumnValue->SetValue(pszValue);
		}
	}

	const char *operator[](unsigned int iPosition) const
	{
		return GetValue(iPosition);
	}

	std::string ToString(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		std::string szResult;

		if (NULL != pData)
		{
			szResult = pData;
		}

		return std::move(szResult);
	}

	char ToChar(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		char cResult = D_DB_NULL_CHAR_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%c", &cResult, 1);
#else
			sscanf(pData, "%c", &cResult);
#endif // WIN32
		}

		return cResult;
	}

	unsigned char ToUChar(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		unsigned char cResult = D_DB_NULL_UCHAR_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%c", &cResult, 1);
#else
			sscanf(pData, "%c", &cResult);
#endif // WIN32
		}

		return cResult;
	}

	short ToShort(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		short iResult = D_DB_NULL_INTGER_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%hd", &iResult);
#else
			sscanf(pData, "%hd", &iResult);
#endif // WIN32
		}

		return iResult;
	}

	unsigned short ToUShort(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		unsigned short iResult = D_DB_NULL_USHORT_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%hu", &iResult);
#else
			sscanf(pData, "%hu", &iResult);
#endif // WIN32
		}

		return iResult;
	}

	int ToInt(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		int iResult = D_DB_NULL_INTGER_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%d", &iResult);
#else
			sscanf(pData, "%d", &iResult);
#endif // WIN32
		}

		return iResult;
	}

	unsigned int ToUInt(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		unsigned int iResult = D_DB_NULL_UINT_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%u", &iResult);
#else
			sscanf(pData, "%u", &iResult);
#endif // WIN32
		}

		return iResult;
	}

	long ToLong(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		long lResult = D_DB_NULL_INTGER_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%ld", &lResult);
#else
			sscanf(pData, "%ld", &lResult);
#endif // WIN32
		}

		return lResult;
	}

	unsigned long ToULong(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		unsigned long lResult = D_DB_NULL_ULONG_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%lu", &lResult);
#else
			sscanf(pData, "%lu", &lResult);
#endif // WIN32
		}

		return lResult;
	}

	long long ToLongLong(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		long long llResult = D_DB_NULL_INTGER_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%lld", &llResult);
#else
			sscanf(pData, "%lld", &llResult);
#endif // WIN32
		}

		return llResult;
	}

	unsigned long long ToULongLong(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		unsigned long long llResult = D_DB_NULL_ULLONG_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%llu", &llResult);
#else
			sscanf(pData, "%llu", &llResult);
#endif // WIN32
		}

		return llResult;
	}

	float ToFloat(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		float fResult = D_DB_NULL_DECIMAL_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%f", &fResult);
#else
			sscanf(pData, "%f", &fResult);
#endif // WIN32
		}

		return fResult;
	}

	double ToDouble(unsigned int iPosition) const
	{
		const char* pData = GetValue(iPosition);
		double dResult = D_DB_NULL_DECIMAL_VALUE;

		if (NULL != pData)
		{
#ifdef _WIN32
			sscanf_s(pData, "%lf", &dResult);
#else
			sscanf(pData, "%lf", &dResult);
#endif // WIN32
		}

		return dResult;
	}

	bool IsNull(unsigned int iPosition) const
	{
		bool bIsNull = true;
		CDBColumValue* lpDBColumnValue = NULL;

		if (iPosition >=0 && iPosition < m_iColumnCount)
		{
			lpDBColumnValue = *(m_pDBColumnValues + iPosition);
		}

		if (lpDBColumnValue != NULL)
		{
			bIsNull = lpDBColumnValue->m_bIsNull;
		}
		
		return bIsNull;
	}

private:
	CDBColumValue	 **m_pDBColumnValues; //行数据
	unsigned int     m_iColumnCount;      //列数

};

#endif