#include "../include/XmlObject.hpp"

CXmlObject::CXmlObject()
{
}

CXmlObject::~CXmlObject()
{
}

bool CXmlObject::LoadFromFile(const std::string &szFileName, std::string &strError)
{
	bool bResult = false;
	tinyxml2::XMLDocument Doc;

	if (szFileName.empty())
	{
		strError = "File name is empty !";
		return bResult;
	}

	if (tinyxml2::XML_SUCCESS == Doc.LoadFile(szFileName.c_str()))
	{
		tinyxml2::XMLHandle Han(Doc);

		//加载成功，子类处理
		if (XmlToObject(Han, strError))
		{
			//检查数据正确性
			bResult = CheckData(strError);
		}
	}
	else
		strError = std::string("Load file failed:") + Doc.ErrorStr();

	return bResult;
}

bool CXmlObject::SaveToFile(const std::string &szFileName, std::string &strError, bool bAddDeclaration) const
{
	bool bResult = false;
	tinyxml2::XMLDocument Doc;

	if (szFileName.empty())
	{
		strError = "File name is empty !";
		return bResult;
	}

	if (ObjectToXml(Doc, strError))
	{
		//加头
		if (bAddDeclaration)
		{
			tinyxml2::XMLDeclaration *pDec = Doc.NewDeclaration(NULL);
			Doc.InsertFirstChild(pDec);
		}

		if (tinyxml2::XML_SUCCESS == Doc.SaveFile(szFileName.c_str()))
			bResult = true;
		else
			strError = std::string("Save file failed:") + Doc.ErrorStr();
	}

	return bResult;
}

bool CXmlObject::LoadFromString(const std::string &szXml, std::string &strError)
{
	return LoadFromString(szXml.c_str(), szXml.length(), strError);
}

bool CXmlObject::LoadFromString(const char* pszXml, size_t ullLength, std::string &strError)
{
	bool bResult = false;
	tinyxml2::XMLDocument Doc;

	if (NULL == pszXml || 0 == strlen(pszXml) || 0 == ullLength)
	{
		strError = "XML data string is empty !";
		return bResult;
	}

	if (tinyxml2::XML_SUCCESS == Doc.Parse(pszXml, ullLength))
	{
		tinyxml2::XMLHandle Han(Doc);

		//加载成功，子类处理
		if (XmlToObject(Han, strError))
		{
			//检查数据正确性
			bResult = CheckData(strError);
		}
	}
	else
		strError = std::string("Parse XML string failed:") + Doc.ErrorStr();

	return bResult;
}

bool CXmlObject::SaveToString(std::string &szXml, std::string &strError, bool bAddDeclaration) const
{
	bool bResult = false;
	tinyxml2::XMLDocument Doc;

	if (ObjectToXml(Doc, strError))
	{
		tinyxml2::XMLPrinter Pri;

		//加头
		if (bAddDeclaration)
		{
			tinyxml2::XMLDeclaration *pDec = Doc.NewDeclaration(NULL);
			Doc.InsertFirstChild(pDec);
		}

		//转字符串
		Doc.Print(&Pri);
		szXml = Pri.CStr();

		if (szXml.length() >= 2)
		{
			//去掉尾部的换行
			if (szXml[szXml.length() - 2] == '\r')
				szXml.erase(szXml.length() - 2, 2);
			else if (szXml[szXml.length() - 1] == '\n')
				szXml.erase(szXml.length() - 1, 1);
		}

		bResult = true;
	}

	return bResult;
}

bool CXmlObject::SaveToString(char** pszXml, size_t *ullLength, std::string &strError, bool bAddDeclaration) const
{
	bool bResult = false;
	std::string szResult;

	if (NULL != *pszXml)
	{
		delete[] *pszXml;
		*pszXml = NULL;
	}

	*ullLength = 0;

	//处理
	if (SaveToString(szResult, strError, bAddDeclaration))
	{
		*ullLength = szResult.length();
		*pszXml = new char[*ullLength + 1]{ '\0' };

		if (NULL != *pszXml)
		{
			memcpy(*pszXml, szResult.c_str(), *ullLength);
			bResult = true;
		}
		else
		{
			*ullLength = 0;
			strError = "New data cache return null !";
		}
	}

	return bResult;
}

std::string CXmlObject::GetObjectType() const
{
	return std::move(std::string(""));
}

bool CXmlObject::ToString(std::string &szOut, const char *pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		szOut = pszStr;
	else
		return false;

	return true;
}

bool CXmlObject::ToBool(bool& bOut, const char *pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		if (0 == strcmp("true", pszStr))
			bOut = true;
		else
			bOut = false;
	}
	else
		return false;

	return true;
}

bool CXmlObject::ToShort(int16_t &nOut, const char *pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		nOut = (int16_t)atoi(pszStr);
	else
		return false;

	return true;
}

bool CXmlObject::ToUshort(uint16_t &unOut, const char *pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		unOut = (uint16_t)atoi(pszStr);
	else
		return false;

	return true;
}

bool CXmlObject::ToInt(int32_t &iOut, const char *pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		iOut = (int32_t)atoi(pszStr);
	}
	else
		return false;

	return true;
}

bool CXmlObject::ToUint(uint32_t &uiOut, const char *pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		uiOut = (uint32_t)std::strtoul(pszStr, NULL, 10);
	else
		return false;
	
	return true;
}

bool CXmlObject::ToFloat(float &fOut, const char *pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		fOut = std::strtof(pszStr, NULL);
	else
		return false;

	return true;
}

bool CXmlObject::ToDouble(double &dOut, const char *pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		dOut = std::strtod(pszStr, NULL);
	else
		return false;

	return true;
}

bool CXmlObject::ToInt64(int64_t &llOut, const char*pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		llOut = std::strtoll(pszStr, NULL, 10);
	else
		return false;

	return true;
}

bool CXmlObject::ToUint64(uint64_t &ullOut, const char*pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		ullOut = std::strtoull(pszStr, NULL, 10);
	else
		return false;

	return true;
}

#ifndef _WIN32
bool CXmlObject::ToLongLong(long long &llOut, const char*pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		llOut = std::strtoll(pszStr, NULL, 10);
	else
		return false;

	return true;
}

bool CXmlObject::ToUlongLong(unsigned long long &ullOut, const char*pszStr)
{
	if (NULL != pszStr && strlen(pszStr) >= 1)
		ullOut = std::strtoull(pszStr, NULL, 10);
	else
		return false;

	return true;
}
#endif