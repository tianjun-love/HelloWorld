#include "../Public/DBBindLob.hpp"
#include "oci.h"

CDBBindLob::CDBBindLob() : m_pBuffer(nullptr), m_pOCISvc(nullptr), m_pOCIErr(nullptr), m_eOCILobType(DB_DATA_TYPE_NONE), 
m_pOCILob(nullptr)
{
}

CDBBindLob::CDBBindLob(const char* strLob) 
{ 
	Init();
	*this = strLob; 
}

CDBBindLob::CDBBindLob(const std::string& szLob) 
{ 
	Init();
	*this = szLob; 
}

CDBBindLob::~CDBBindLob()
{
	Clear(true);
}

CDBBindLob& CDBBindLob::operator = (const char* strLob)
{
	if (nullptr == strLob)
	{
		Clear();
	}
	else
	{
		m_lDataLength = (unsigned long)strlen(strLob);
		m_nParamIndp = (m_lDataLength > 0 ? 0 : -1);

		if (m_lDataLength < m_lBufferLength)
		{
			memset(m_pBuffer, '\0', m_lBufferLength);
			memcpy(m_pBuffer, strLob, m_lDataLength);
		}
		else
		{
			if (nullptr != m_pBuffer)
			{
				delete[] m_pBuffer;
				m_pBuffer = nullptr;
			}

			m_lBufferLength = m_lDataLength + 1;
			m_pBuffer = new char[m_lBufferLength] { '\0' };
			memcpy(m_pBuffer, strLob, m_lDataLength);
		}

		if (!BindLob())
		{
			//°ó¶¨Ê§°ÜÔòÖÃ¿Õ
			m_nParamIndp = -1;
		}
	}

	return *this;
}

CDBBindLob& CDBBindLob::operator = (const std::string& szLob)
{
	if (szLob.empty())
	{
		Clear();
	}
	else
	{
		m_lDataLength = (unsigned long)szLob.length();
		m_nParamIndp = (m_lDataLength > 0 ? 0 : -1);

		if (m_lDataLength < m_lBufferLength)
		{
			memset(m_pBuffer, '\0', m_lBufferLength);
			memcpy(m_pBuffer, szLob.c_str(), m_lDataLength);
		}
		else
		{
			if (nullptr != m_pBuffer)
			{
				delete[] m_pBuffer;
				m_pBuffer = nullptr;
			}

			m_lBufferLength = m_lDataLength + 1;
			m_pBuffer = new char[m_lBufferLength] { '\0' };
			memcpy(m_pBuffer, szLob.c_str(), m_lDataLength);
		}

		if (!BindLob())
		{
			//°ó¶¨Ê§°ÜÔòÖÃ¿Õ
			m_nParamIndp = -1;
		}
	}

	return *this;
}

bool CDBBindLob::SetBuffer(unsigned long lBufferLength)
{
	if (nullptr != m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = nullptr;
	}

	m_nParamIndp = -1;
	m_lBufferLength = lBufferLength;

	if (m_lBufferLength > 0)
	{
		m_pBuffer = new char[m_lBufferLength] { '\0' };
		if (nullptr == m_pBuffer)
		{
			return false;
		}
	}

	return true;
}

bool CDBBindLob::SetBufferAndValue(unsigned long lBufferLength, void* pValue, unsigned long lValueLength)
{
	if (nullptr != m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = nullptr;
	}

	if (lBufferLength <= lValueLength)
		return false;

	m_nParamIndp = -1;
	m_lDataLength = lValueLength;
	m_lBufferLength = lBufferLength;

	if (m_lBufferLength > 0)
	{
		m_pBuffer = new char[m_lBufferLength] { '\0' };

		if (nullptr != pValue)
		{
			m_nParamIndp = 0;
			memcpy(m_pBuffer, pValue, m_lDataLength);

			if (!BindLob())
			{
				//°ó¶¨Ê§°ÜÔòÖÃ¿Õ
				m_nParamIndp = -1;
				return false;
			}
		}
		else
			return false;
	}

	return true;
}

bool CDBBindLob::SetValue(void* pValue, unsigned long lValueLength)
{
	if (nullptr == m_pBuffer || lValueLength >= m_lBufferLength)
	{
		return false;
	}

	if (nullptr != pValue)
	{
		m_lDataLength = lValueLength;
		m_nParamIndp = (m_lDataLength > 0 ? 0 : -1);
		memcpy(m_pBuffer, pValue, m_lDataLength);

		if (!BindLob())
		{
			//°ó¶¨Ê§°ÜÔòÖÃ¿Õ
			m_nParamIndp = -1;
			return false;
		}
	}
	else
	{
		Clear();
		return false;
	}

	return true;
}

const void* CDBBindLob::GetData() const
{
	return m_pOCILob;
}

void CDBBindLob::Clear(bool bFreeAll)
{
	m_nParamIndp = -1;
	m_lDataLength = 0;

	if (nullptr != m_pBuffer)
	{
		if (bFreeAll)
		{
			delete[] m_pBuffer;
			m_pBuffer = nullptr;
			m_lBufferLength = 0;

			if (nullptr != m_pOCILob)
			{
				OCILobFreeTemporary((OCISvcCtx*)m_pOCISvc, (OCIError*)m_pOCIErr, (OCILobLocator*)m_pOCILob);
				OCIDescriptorFree(m_pOCILob, OCI_DTYPE_LOB);
				m_pOCILob = nullptr;
			}

			m_pOCISvc = nullptr;
			m_pOCIErr = nullptr;
			m_eOCILobType = DB_DATA_TYPE_NONE;
		}
		else
		{
			memset(m_pBuffer, '\0', m_lBufferLength);
		}
	}
}

bool CDBBindLob::BindLob()
{
	if (nullptr != m_pOCISvc && nullptr != m_pOCIErr && nullptr != m_pOCILob)
	{
		oraub8 llTempLength = 0;
		sword iRet = OCI_SUCCESS;

		if (OCI_SUCCESS == OCILobGetLength2((OCISvcCtx*)m_pOCISvc, (OCIError*)m_pOCIErr, (OCILobLocator*)m_pOCILob,
			&llTempLength))
		{
			if (llTempLength > 0)
			{
				//Çå¿ÕÊý¾Ý
				iRet = OCILobErase2((OCISvcCtx*)m_pOCISvc, (OCIError*)m_pOCIErr, (OCILobLocator*)m_pOCILob, &llTempLength, 1);
			}

			if (OCI_SUCCESS == iRet)
			{
				oraub8 llTemp = m_lDataLength;

				if (DB_DATA_TYPE_BLOB == m_eOCILobType)
				{
					iRet = OCILobWrite2((OCISvcCtx*)m_pOCISvc, (OCIError*)m_pOCIErr, (OCILobLocator*)m_pOCILob, &llTemp, NULL, 1,
						m_pBuffer, m_lBufferLength, OCI_ONE_PIECE, NULL, NULL, 0, SQLCS_IMPLICIT);
				}
				else
				{
					iRet = OCILobWrite2((OCISvcCtx*)m_pOCISvc, (OCIError*)m_pOCIErr, (OCILobLocator*)m_pOCILob, NULL, &llTemp, 1,
						m_pBuffer, m_lBufferLength, OCI_ONE_PIECE, NULL, NULL, 0, SQLCS_IMPLICIT);
				}

				if (OCI_SUCCESS == iRet)
				{
					return true;
				}
			}
		}
	}
	else
		return true;

	return false;
}

std::string CDBBindLob::ToString() const
{
	return std::move(std::string("LOB"));
}

bool CDBBindLob::Init()
{
	m_pOCISvc = nullptr;
	m_pOCIErr = nullptr;
	m_eOCILobType = DB_DATA_TYPE_NONE;
	m_pOCILob = nullptr;

	return true;
}