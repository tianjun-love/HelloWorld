#include "../Public/DBBindRefCursor.hpp"
#include "oci.h"

CDBBindRefCursor::CDBBindRefCursor() : m_pOCIErr(nullptr), m_pRefCursor(nullptr)
{
	m_lBufferLength = sizeof(OCIStmt*);
}

CDBBindRefCursor::~CDBBindRefCursor()
{
	Clear(true);
}

std::string CDBBindRefCursor::ToString() const
{
	return std::move(std::string("SYS_REFCURSOR"));
}

void CDBBindRefCursor::Clear(bool bFreeAll)
{
	m_nParamIndp = OCI_IND_NULL;
	m_lDataLength = 0;

	if (bFreeAll && nullptr != m_pRefCursor)
	{
		OCIHandleFree(m_pRefCursor, (ub4)OCI_HTYPE_STMT);
		m_pOCIErr = nullptr;
		m_pRefCursor = nullptr;
	}
}

bool CDBBindRefCursor::Init()
{
	return true;
}