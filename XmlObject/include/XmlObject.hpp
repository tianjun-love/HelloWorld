/***********************************************************
*功能:	xml功能类
*作者：	田俊
*时间：	2022-03-04
*修改：	
***********************************************************/
#ifndef __XML_OBJECT_HPP__
#define __XML_OBJECT_HPP__

#include <string>
#include "tinyXML2/tinyxml2.h"

class CXmlObject
{
public:
	CXmlObject();
	virtual ~CXmlObject();

	bool LoadFromFile(const std::string &szFileName, std::string &strError);
	bool SaveToFile(const std::string &szFileName, std::string &strError, bool bAddDeclaration = false) const;
	bool LoadFromString(const std::string &szXml, std::string &strError);
	bool LoadFromString(const char* pszXml, size_t ullLength, std::string &strError);
	bool SaveToString(std::string &szXml, std::string &strError, bool bAddDeclaration = false) const;
	bool SaveToString(char** pszXml, size_t *ullLength, std::string &strError, bool bAddDeclaration = false) const;

	virtual std::string GetObjectType() const;

protected:
	virtual bool XmlToObject(tinyxml2::XMLHandle &XmlHandle, std::string &strError) = 0;
	virtual bool ObjectToXml(tinyxml2::XMLDocument &XmlDocument, std::string &strError) const = 0;
	virtual bool CheckData(std::string &strError) const = 0;

	static bool ToString(std::string &szOut, const char *pszStr);
	static bool ToBool(bool& bOut, const char *pszStr);
	static bool ToNumeral(short &nOut, const char *pszStr);
	static bool ToNumeral(unsigned short &unOut, const char *pszStr);
	static bool ToNumeral(int &iOut, const char *pszStr);
	static bool ToNumeral(unsigned int &uiOut, const char *pszStr);
	static bool ToNumeral(float &fOut, const char *pszStr);
	static bool ToNumeral(double &dOut, const char *pszStr);
	static bool ToNumeral(long &lOut, const char *pszStr);
	static bool ToNumeral(unsigned long &ulOut, const char *pszStr);
	static bool ToNumeral(long long &llOut, const char *pszStr);
	static bool ToNumeral(unsigned long long &ullOut, const char *pszStr);

};

#endif //__XML_OBJECT_HPP__
