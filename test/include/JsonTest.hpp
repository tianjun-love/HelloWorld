#ifndef __JSONTEST_HPP__
#define __JSONTEST_HPP__

#include <vector>
#include "JsonObject/include/JsonObject.hpp"

class CJsonTest : public JsonObject
{
public:
	struct SLike
	{
		std::string szName;
		int         iAge = 0;
	};

	class CObject
	{
	public:
		CObject() {};
		~CObject() {};

	public:
		long long llValue = 0;
		std::string szValue;
	};

public:
	CJsonTest();
	~CJsonTest();

	bool JsonToObject(const Json::Value &Root, std::string &strError = *((std::string*)nullptr));
	bool ObjectToJson(Json::Value &Root, std::string &strError = *((std::string*)nullptr));

public:
	std::string        m_szName;
	int                m_iAge = 0;
	std::vector<SLike> m_Like;
	std::vector<int>   m_Num;
	CObject            m_Object;
};

#endif