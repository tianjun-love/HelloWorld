#include "../Public/DBResultSet.hpp"
#include "../include/DBBaseInterface.hpp"

CDBResultSet::CDBResultSet()
{
	m_lpRowValue = NULL;
	m_pColumnAttribute = NULL;
	m_iColumnCount = 0;
	m_llRowCount = 0;
	m_bIsOutParamResult = false;
	m_pDBInterface = NULL;
}

CDBResultSet::CDBResultSet(unsigned long long iRowCount, unsigned int iColumnCount)
{
	m_lpRowValue = NULL;
	m_pColumnAttribute = NULL;
	m_llRowCount = iRowCount;
	m_bIsOutParamResult = false;
	m_pDBInterface = NULL;

	CreateColumnAttr(iColumnCount);
}

CDBResultSet::~CDBResultSet()
{
	Clear();
}

//设置数据库访问接口
void CDBResultSet::SetDBInterface(CDBBaseInterface* pDBInterface)
{
	m_pDBInterface = pDBInterface;
}

//获取列数
unsigned int CDBResultSet::GetColumnCount() const
{
	return m_iColumnCount;
}

//获取行数
unsigned long long CDBResultSet::GetRowCount() const
{
	return m_llRowCount;
}

//设置行数
void CDBResultSet::SetRowCount(unsigned long long llRowCount)
{
	m_llRowCount = llRowCount;
}

//获取列名
const char *CDBResultSet::GetColumnAttrName(unsigned int iPosition) const
{
	const char *pszColumnAttrName = NULL;
	CDBColumAttribute* lpColumnAttrValue = NULL;

	if (iPosition >= 0 && iPosition < m_iColumnCount)
	{
		lpColumnAttrValue = *(m_pColumnAttribute + iPosition);
	}

	if (lpColumnAttrValue)
	{
		pszColumnAttrName = lpColumnAttrValue->m_pszColumnName;
	}

	return pszColumnAttrName;
}

//创建列属性
void CDBResultSet::CreateColumnAttr(unsigned int iColumnCount)
{
	m_pColumnAttribute = new CDBColumAttribute*[iColumnCount];
	m_iColumnCount = iColumnCount;

	for (unsigned int i = 0; i < m_iColumnCount; i++)
	{
		*(m_pColumnAttribute + i) = NULL;
	}
}

//添加列属性
void CDBResultSet::AddColumnAttr(CDBColumAttribute* lpColumnAttribute, unsigned int iPosition)
{
	if (iPosition >= 0 && iPosition < m_iColumnCount)
	{
		CDBColumAttribute* lpTemp = *(m_pColumnAttribute + iPosition);
		if (lpTemp != NULL)
		{
			delete lpTemp;
			lpTemp = NULL;
		}

		*(m_pColumnAttribute + iPosition) = lpColumnAttribute;
	}
}

//获取列属性
CDBColumAttribute* CDBResultSet::GetColumnAttr(unsigned int iPosition) const
{
	CDBColumAttribute* lpColumnAttr = NULL;

	if (iPosition >= 0 && iPosition < m_iColumnCount)
	{
		lpColumnAttr = *(m_pColumnAttribute + iPosition);;
	}

	return lpColumnAttr;
}

//设置行数据
void CDBResultSet::SetRowValue(CDBRowValue* lpRowValue)
{
	if (m_lpRowValue != NULL)
	{
		delete m_lpRowValue;
		m_lpRowValue = NULL;
	}

	m_lpRowValue = lpRowValue;
}

//获取行数据
const CDBRowValue* CDBResultSet::GetNextRow(bool bIsStmt)
{
	if (m_pDBInterface && m_pDBInterface->Fetch(m_lpRowValue, bIsStmt))
		return m_lpRowValue;
	else
		return nullptr;
}

//获取另一个结果集
bool CDBResultSet::GetNextResult(bool bIsStmt)
{
	if (m_pDBInterface)
		return m_pDBInterface->GetNextResult(this, bIsStmt);
	else
		return false;
}

void CDBResultSet::SetIsOutParamResult(bool bIsOutParamResult)
{
	m_bIsOutParamResult = bIsOutParamResult;
}

//获取是否是存储过程返回结果集
bool CDBResultSet::SetIsOutParamResult() const
{
	return m_bIsOutParamResult;
}

//关闭结果集
void CDBResultSet::Clear()
{
	//删除数据
	if (m_lpRowValue != NULL)
	{
		m_lpRowValue->Clear();
		delete m_lpRowValue;
		m_lpRowValue = NULL;
	}

	//删除列属性
	for (unsigned int i = 0; i < m_iColumnCount; i++)
	{
		CDBColumAttribute* lpColumnAttrName = *(m_pColumnAttribute + i);
		if (lpColumnAttrName != NULL)
		{
			delete lpColumnAttrName;
			lpColumnAttrName = NULL;
		}
	}

	m_iColumnCount = 0;
	m_llRowCount = 0;
	delete[] m_pColumnAttribute;
	m_pColumnAttribute = NULL;
	m_bIsOutParamResult = false;
}
