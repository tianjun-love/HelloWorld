#ifndef __XMLTEST_HPP__
#define __XMLTEST_HPP__

#include "XmlObject/include/XmlObject.hpp"
#include <vector>

using std::vector;
using std::string;

class CXmlTest : public XmlObject
{
public:
	CXmlTest();
	virtual ~CXmlTest();

	bool XmlToObject(const TiXmlDocument &XmlDocument, string &strError);
	bool ObjectToXml(TiXmlDocument &XmlDocument, string &strError) const;
	string ObjectType() const;
	bool CheckData(string &strError) const { return true; };

	void DealXml(TiXmlDocument& RecvXml, const TiXmlDocument& SendXml);

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