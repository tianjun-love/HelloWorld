/***********************************************************************
* FUNCTION:     结果集
* AUTHOR:       添加
* DATE：        2015-04-13
* NOTES:        
* MODIFICATION:
**********************************************************************/
#ifndef __DB_RESULT_SET__
#define __DB_RESULT_SET__

#include "DBRowValue.hpp"
#include "DBColumnAttribute.hpp"

class CDBBaseInterface;

class CDBResultSet
{
public:
	CDBResultSet();
	CDBResultSet(unsigned long long iRowCount, unsigned int iColumnCount);
	~CDBResultSet();

	//设置数据库访问接口
	void SetDBInterface(CDBBaseInterface* pDBInterface);

	//获取列数
	unsigned int GetColumnCount() const;

	//获取行数，mysql有效
	unsigned long long GetRowCount() const;

	//设置行数，mysql有效
	void SetRowCount(unsigned long long llRowCount);

	//获取列名
	const char *GetColumnAttrName(unsigned int iPosition) const;

	//创建列属性
	void CreateColumnAttr(unsigned int iColumnCount);

	//添加列属性
	void AddColumnAttr(CDBColumAttribute* lpColumnAttribute, unsigned int iPosition);

	//获取列属性
	CDBColumAttribute* GetColumnAttr(unsigned int iPosition) const;

	//设置行数据
	void SetRowValue(CDBRowValue* lpRowValue);

	//获取行数据
	const CDBRowValue* GetNextRow(bool bIsStmt = false);

	//获取另一个结果集
	bool GetNextResult(bool bIsStmt = false);

	//设置是否是存储过程返回结果集
	void SetIsOutParamResult(bool bIsOutParamResult);

	//获取是否是存储过程out/inout参数返回结果集
	bool SetIsOutParamResult() const;
	
	//关闭结果集
	void Clear();

private:
	CDBColumAttribute    **m_pColumnAttribute;   //列属性
	unsigned int         m_iColumnCount;         //列数
	CDBRowValue          *m_lpRowValue;          //行数据
	unsigned long long   m_llRowCount;           //行数，MySQL有效
	bool                 m_bIsOutParamResult;    //是否是存储过程out参数的返回结果
	CDBBaseInterface*    m_pDBInterface;         //数据库接口

};

#endif // __DB_RESULT_SET__
