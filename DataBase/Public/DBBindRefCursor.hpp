/******************************************************
功能：	oracle绑定SYS_REFCURSOR类
作者：	田俊
时间：	2018-12-18
修改：
******************************************************/
#ifndef __DB_BIND_REF_CURSOR_HPP__
#define __DB_BIND_REF_CURSOR_HPP__

#include "DBBindBase.hpp"

class CDBBindRefCursor : public CDBBindBase
{
public:
	CDBBindRefCursor();
	CDBBindRefCursor(const CDBBindRefCursor& Other) = delete;
	~CDBBindRefCursor();
	CDBBindRefCursor& operator = (const CDBBindRefCursor& Other) = delete;

	std::string ToString() const override;
	void Clear(bool bFreeAll = false) override;

private:
	bool Init() override;

	friend class COCIInterface;

private:
	void *m_pOCIErr;     //OCI错误句柄，绑定时确定
	void *m_pRefCursor;  //结果句柄OCIStmt*，绑定时确定

};

#endif