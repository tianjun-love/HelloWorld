#ifndef __XMLTEST_HPP__
#define __XMLTEST_HPP__

#include "XmlObject/include/XmlObject.hpp"
#include <vector>

using std::vector;
using std::string;

class CXmlTest : public CXmlObject
{
public:
	CXmlTest();
	virtual ~CXmlTest();

	string ObjectType() const;

protected:
	bool XmlToObject(tinyxml2::XMLHandle &XmlHandle, string &strError) override;
	bool ObjectToXml(tinyxml2::XMLDocument &XmlDocument, string &strError) const override;
	bool CheckData(string &strError) const override;

public:
	string                m_szName;      //����
	int                   m_iAge;        //����
	string                m_szGender;    //�Ա�
	string                m_szTestStr;   //���Դ�
	vector<string>        m_FriendVec;   //����
	string                m_szLike1;     //����1
	string                m_szLike2;     //����2
	string                m_szOther;     //����

};

#endif