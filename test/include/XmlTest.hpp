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
	string                m_szName;      //名称
	int                   m_iAge;        //年龄
	string                m_szGender;    //性别
	string                m_szTestStr;   //测试串
	vector<string>        m_FriendVec;   //朋友
	string                m_szLike1;     //爱好1
	string                m_szLike2;     //爱好2
	string                m_szOther;     //其它

};

#endif