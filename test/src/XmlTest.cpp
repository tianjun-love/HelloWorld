#include "../include/XmlTest.hpp"

CXmlTest::CXmlTest()
{
	m_iAge = 0;
	m_szGender = "ƒ–";
}

CXmlTest::~CXmlTest()
{
}

bool CXmlTest::XmlToObject(const TiXmlDocument &XmlDocument, string &strError)
{
	bool bResult = true;
	TiXmlHandle Handle((TiXmlNode*)&XmlDocument);
	const TiXmlElement *pElement, *pSonElement, *pElementNode;
	string szTemp;

	pElement = Handle.FirstChild("Person").ToElement();
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
		pElement = Handle.FirstChild("Other").ToElement();
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

bool CXmlTest::ObjectToXml(TiXmlDocument &XmlDocument, string &strError) const
{
	bool bResult = true;
	vector<string>::const_iterator Iter;
	TiXmlElement *pElement, *pSonElement, *pTemp;
	TiXmlText *pText;

	pElement = new TiXmlElement("Person");
	if (NULL != pElement)
	{
		pElement->SetAttribute("Type", "XmlTest");
		pElement->SetAttribute("Name", m_szName);
		pElement->SetAttribute("Age", m_iAge);
		pElement->SetAttribute("Gender", m_szGender);
		pElement->SetAttribute("TestStr", m_szTestStr);

		pSonElement = new TiXmlElement("Friend");
		if (NULL != pSonElement)
		{
			for (Iter = m_FriendVec.begin(); Iter != m_FriendVec.end(); ++Iter)
			{
				pTemp = new TiXmlElement("friend");
				if (NULL != pTemp)
				{
					pText = new TiXmlText(*Iter);
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
		pElement = new TiXmlElement("Other");
		if (NULL != pElement)
		{
			pElement->SetAttribute("other", m_szOther);

			pSonElement = new TiXmlElement("Like");
			if (NULL != pSonElement)
			{
				pTemp = new TiXmlElement("like1");
				if (NULL != pTemp)
				{
					pText = new TiXmlText(m_szLike1);
					pTemp->LinkEndChild(pText);
					pText = NULL;
					pSonElement->LinkEndChild(pTemp);
					pTemp = NULL;
				}

				pTemp = new TiXmlElement("like2");
				if (NULL != pTemp)
				{
					pText = new TiXmlText(m_szLike2);
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

string CXmlTest::ObjectType() const
{
	return string("XmlTest");
}

void CXmlTest::DealXml(TiXmlDocument& RecvXml, const TiXmlDocument& SendXml)
{
	//Ω” ’
	string szRecv = "Xml¥Æ";
	RecvXml.Clear();
	RecvXml.Parse(szRecv.c_str());

	//...

	//∑¢ÀÕ
	TiXmlPrinter Printer;
	SendXml.Accept(&Printer);
	string szSend = Printer.CStr();

	//...
}
