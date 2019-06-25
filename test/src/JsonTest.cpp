#include "../include/JsonTest.hpp"

CJsonTest::CJsonTest()
{
}

CJsonTest::~CJsonTest()
{
}

bool CJsonTest::JsonToObject(const Json::Value &Root, std::string &strError)
{
	bool bResult = true;

	m_szName = Root["NAME"].asString();
	m_iAge   = Root["AGE"].asInt();

	if (!Root["LIKE"].isNull())
	{
		SLike like;

		for (unsigned int i = 0; i < Root["LIKE"].size(); ++i)
		{
			like.szName = Root["LIKE"][i]["NAME"].asString();
			like.iAge = Root["LIKE"][i]["AGE"].asInt();

			m_Like.push_back(like);
		}
	}

	if (!Root["NUM"].isNull())
	{
		for (unsigned i = 0; i < Root["NUM"].size(); ++i)
		{
			m_Num.push_back(Root["NUM"][i].asInt());
		}
	}

	if (!Root["OBJECT"].isNull())
	{
		m_Object.llValue = Root["OBJECT"]["LL"].asInt64();
		m_Object.szValue = Root["OBJECT"]["SZ"].asString();
	}

	return bResult;
}

bool CJsonTest::ObjectToJson(Json::Value &Root, std::string &strError)
{
	bool bResult = true;
	Json::Value value;

	Root["NAME"] = m_szName;
	Root["AGE"] = m_iAge;

	if (!m_Like.empty())
	{
		for (unsigned int i = 0; i < m_Like.size(); ++i)
		{
			SLike &like = m_Like[i];

			value["NAME"] = like.szName;
			value["AGE"]  = like.iAge;
			Root["LIKE"].append(value);
			value.clear();
		}
	}

	if (!m_Num.empty())
	{
		for (unsigned int i = 0; i < m_Num.size(); ++i)
		{
			Root["NUM"].append(m_Num[i]);
		}
	}

	Json::Value oValue(Json::objectValue);
	oValue["LL"] = m_Object.llValue;
	oValue["SZ"] = m_Object.szValue;
	Root["OBJECT"] = oValue;
	oValue.clear();

	return bResult;
}