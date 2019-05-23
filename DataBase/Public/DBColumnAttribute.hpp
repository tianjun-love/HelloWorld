/***********************************************************************
* FUNCTION:     列属性
* AUTHOR:       田俊
* DATE：        2015-04-13
* NOTES:        
* MODIFICATION:
**********************************************************************/
#ifndef __DB_COLUMN_ATTRIBUTE_HPP__
#define __DB_COLUMN_ATTRIBUTE_HPP__

#include "DBPublic.hpp"

class CDBColumAttribute
{
public:
	CDBColumAttribute()
	{
		Clear();
	}

	~CDBColumAttribute()
	{
	}

	void SetColumnName(const char *pszColumnName)
	{
		if (NULL != pszColumnName)
		{
			strncpy(m_pszColumnName, pszColumnName, D_DB_COLNUM_NAME_LENGTH + 1);
		}
	}

	void Clear()
	{
		memset(m_pszColumnName, '\0', D_DB_COLNUM_NAME_LENGTH + 1);
		m_iFieldNameLen = 0;
		m_eDataType = DB_DATA_TYPE_NONE;
		m_iFieldDataLen = 0;
		m_iPrecision = 0;
		m_nScale = 0;
		m_bNullAble = true;
	}

public:
	char            m_pszColumnName[D_DB_COLNUM_NAME_LENGTH + 1]; //字段名
	unsigned int    m_iFieldNameLen;  //字段名长度
	EDBDataType		m_eDataType;      //数据类型
	unsigned long	m_iFieldDataLen;  //字段数据最大长度
	unsigned int	m_iPrecision;     //Oracle:包括小数点的总位数，Mysql:查询出的数据的最大长度
	unsigned short	m_nScale;         //小数后位数
	bool            m_bNullAble;      //是否请允许为空

};

#endif