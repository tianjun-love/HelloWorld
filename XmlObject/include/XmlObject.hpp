/********************************************************************
created:	2014/10/24
file base:	XmlObject
file ext:	hpp
author:		tianjun
purpose:	use tinyxml
Modify:
*********************************************************************/

#ifndef __XML_OBJECT_HPP__
#define __XML_OBJECT_HPP__

//use STL
#ifndef TIXML_USE_STL
#define TIXML_USE_STL
#endif //TIXML_USE_STL

#include "tinyxml.h"
#include <string>

class TiXmlDocument;

class XmlObject
{
public:
	XmlObject();
	virtual ~XmlObject();

	/********************************************************************
	name    :	LoadFile
	function:	load from file
	parm    : 	FileName        file name
			    strError        error desc
				XmlDocument		xml parser
	return  :   success return true else return false
	*********************************************************************/
	bool LoadFile(const std::string &FileName, std::string &strError = *((std::string*)NULL), TiXmlDocument *XmlDocument = NULL);

	/********************************************************************
	name    :	SaveFile
	function:	save to file
	parm    : 	FileName        file name
				strError        error desc
				XmlDocument		xml parser
	return  :   success return true else return false
	*********************************************************************/
	bool SaveFile(const std::string &FileName, std::string &strError = *((std::string*)NULL), TiXmlDocument *XmlDocument = NULL) const;

	/********************************************************************
	name    :	LoadStr
	function:	load from string
	parm    : 	StrXml          string name
				strError        error desc
				XmlDocument		xml parser
	return  :   success return true else return false
	*********************************************************************/
	bool LoadStr(const std::string &StrXml, std::string &strError = *((std::string*)NULL), TiXmlDocument *XmlDocument = NULL);
	bool LoadStr(const char* StrXml, std::string &strError = *((std::string*)NULL), TiXmlDocument *XmlDocument = NULL);

	/********************************************************************
	name    :	SaveStr
	function:	save to string
	parm    : 	StrXml          string name
				strError        error desc
				XmlDocument		xml parser
	return  :   success return true else return false
	*********************************************************************/
	bool SaveStr(std::string &StrXml, std::string &strError = *((std::string*)NULL), TiXmlDocument *XmlDocument = NULL) const;

	/********************************************************************
	name    :	SaveStr
	function:	save to string
	parm    : 	StrXml          string name, need delete
				DataLength      data length
				strError        error desc
				XmlDocument		xml parser
	return  :   success return true else return false
	*********************************************************************/
	bool SaveStr(char* StrXml, int& DataLength, std::string &strError = *((std::string*)NULL), TiXmlDocument *XmlDocument = NULL) const;

	/********************************************************************
	name    :	XmlToObject
	function:   xml to object
	parm    : 	XmlDocument		xml parser
				strError		error desc
	return  :	success return true else return false
	*********************************************************************/
	virtual bool XmlToObject(const TiXmlDocument &XmlDocument, std::string &strError) = 0;

	/********************************************************************
	name    :	ObjectToXml
	function:	object to xml
	parm    : 	XmlDocument		xml parser
			    strError		error desc
	return  :	success return true else return false
	*********************************************************************/
	virtual bool ObjectToXml(TiXmlDocument &XmlDocument, std::string &strError) const = 0;

	/********************************************************************
	name    :   CheckData
	function:	check data
	parm    : 	strError error info
	return  :	all data right to return true
	*********************************************************************/
	virtual bool CheckData(std::string &strError) const = 0;

    /********************************************************************
	name    :   GetObjectType
	function:	get object type
	parm    : 	none
	return  :	object type string sr
	*********************************************************************/
	virtual std::string GetObjectType() const;

	bool ToString(std::string &Out, const char *pszStr);
	bool ToInt(int &Out, const char *pszStr);
	bool ToUint(unsigned int &Out, const char *pszStr);
	bool ToShort(short &Out, const char *pszStr);
	bool ToUshort(unsigned short &Out, const char *pszStr);
	bool ToDouble(double &Out, const char *pszStr);
	bool ToBool(bool& Out, const char *pszStr);
	bool ToLong(long &Out, const char *pszStr);
	bool ToLongLong(long long &Out, const char *pszStr);
};

#endif //__XML_OBJECT_HPP__
