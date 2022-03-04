#include "../include/XmlTest.hpp"

CXmlTest::CXmlTest()
{
	m_iAge = 0;
	m_szGender = "ÄÐ";
}

CXmlTest::~CXmlTest()
{
}

bool CXmlTest::XmlToObject(tinyxml2::XMLHandle &XmlHandle, string &strError)
{
	bool bResult = true;
	const tinyxml2::XMLElement *pElement, *pSonElement, *pElementNode;
	string szTemp;

	pElement = XmlHandle.FirstChildElement("Person").ToElement();
	if (NULL != pElement)
	{
		string szTemp;
		ToString(szTemp, pElement->Attribute("Type"));
		if ("XmlTest" == szTemp)
		{
			ToString(m_szName, pElement->Attribute("Name"));
			ToInt(m_iAge, pElement->Attribute("Age"));
			ToString(m_szGender, pElement->Attribute("Gender"));
			ToString(m_szTestStr, pElement->Attribute("TestStr"));

			pSonElement = pElement->FirstChildElement("Friend");
			if (NULL != pSonElement)
			{
				pElementNode = pSonElement->FirstChildElement();
				while (NULL != pElementNode)
				{
					szTemp = pElementNode->GetText();
					m_FriendVec.push_back(szTemp);
					pElementNode = pElementNode->NextSiblingElement();
				}
			}
		}
	}
	else
	{
		strError = "Can't find Person node !";
		bResult = false;
	}

	if (bResult)
	{
		pElement = XmlHandle.FirstChildElement("Other").ToElement();
		if (NULL != pElement)
		{
			ToString(m_szOther, pElement->Attribute("other"));

			pSonElement = pElement->FirstChildElement("Like");
			if (NULL != pSonElement)
			{
				pElementNode = pSonElement->FirstChildElement();
				while (NULL != pElementNode)
				{
					szTemp = pElementNode->Value();
					if ("like1" == szTemp)
					{
						m_szLike1 = pElementNode->GetText();
					}
					else if ("like2" == szTemp)
					{
						m_szLike2 = pElementNode->GetText();
					}

					pElementNode = pElementNode->NextSiblingElement();
				}
			}
		}
	}

	return bResult;
}

bool CXmlTest::ObjectToXml(tinyxml2::XMLDocument &XmlDocument, string &strError) const
{
	bool bResult = true;
	vector<string>::const_iterator Iter;
	tinyxml2::XMLElement *pElement, *pSonElement, *pTemp;
	tinyxml2::XMLText *pText;

	pElement = XmlDocument.NewElement("Person");
	if (NULL != pElement)
	{
		pElement->SetAttribute("Type", "XmlTest");
		pElement->SetAttribute("Name", m_szName.c_str());
		pElement->SetAttribute("Age", m_iAge);
		pElement->SetAttribute("Gender", m_szGender.c_str());
		pElement->SetAttribute("TestStr", m_szTestStr.c_str());

		pSonElement = XmlDocument.NewElement("Friend");
		if (NULL != pSonElement)
		{
			for (Iter = m_FriendVec.begin(); Iter != m_FriendVec.end(); ++Iter)
			{
				pTemp = XmlDocument.NewElement("friend");
				if (NULL != pTemp)
				{
					pText = XmlDocument.NewText(Iter->c_str());
					pTemp->LinkEndChild(pText);
					pText = NULL;
					pSonElement->LinkEndChild(pTemp);
					pTemp = NULL;
				}
			}

			pElement->LinkEndChild(pSonElement);
			pSonElement = NULL;
		}
		else
		{
			strError = "new Friend XmlElement fail .";
			bResult = false;
		}

		XmlDocument.LinkEndChild(pElement);
		pElement = NULL;
	}
	else
	{
		strError = "new Person XmlElement fail .";
		bResult = false;
	}

	if (bResult)
	{
		pElement = XmlDocument.NewElement("Other");
		if (NULL != pElement)
		{
			pElement->SetAttribute("other", m_szOther.c_str());

			pSonElement = XmlDocument.NewElement("Like");
			if (NULL != pSonElement)
			{
				pTemp = XmlDocument.NewElement("like1");
				if (NULL != pTemp)
				{
					pText = XmlDocument.NewText(m_szLike1.c_str());
					pTemp->LinkEndChild(pText);
					pText = NULL;
					pSonElement->LinkEndChild(pTemp);
					pTemp = NULL;
				}

				pTemp = XmlDocument.NewElement("like2");
				if (NULL != pTemp)
				{
					pText = XmlDocument.NewText(m_szLike2.c_str());
					pTemp->LinkEndChild(pText);
					pText = NULL;
					pSonElement->LinkEndChild(pTemp);
					pTemp = NULL;
				}

				pElement->LinkEndChild(pSonElement);
			}

			XmlDocument.LinkEndChild(pElement);
		}
		else
		{
			strError = "new Other XmlElement fail .";
			bResult = false;
		}
	}

	return bResult;
}

bool CXmlTest::CheckData(string &strError) const
{
	return true;
}

string CXmlTest::ObjectType() const
{
	return string("XmlTest");
}