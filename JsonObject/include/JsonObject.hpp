/***********************************************************
*功能:	json功能类
*作者：	田俊
*时间：	2022-03-04
*修改：	
***********************************************************/
#ifndef __JSON_OBJECT_HPP__
#define __JSON_OBJECT_HPP__

#include "jsoncpp/json.h"

class CJsonObject
{
public:
	CJsonObject();
	virtual ~CJsonObject();

	bool LoadFromFile(const std::string &szFileName, std::string &strError);
	bool SaveToFile(const std::string &szFileName, std::string &strError);
	bool LoadFromString(const std::string &szJson, std::string &strError);
	bool SaveToString(std::string &szJson, std::string &strError);

	virtual std::string ToString() const;

protected:
	virtual bool JsonToObject(const Json::Value &Root, std::string &strError) = 0;
	virtual bool ObjectToJson(Json::Value &Root, std::string &strError) const = 0;
	virtual bool CheckData(std::string &strError) const = 0;

};

#endif