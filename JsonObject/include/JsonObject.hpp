/***********************************************************/
//function:jsoncpp
//note    :create by tianjun 2014-11-20
/***********************************************************/
#ifndef __JSONOBJECT_HPP__
#define __JSONOBJECT_HPP__

#include <string>
#include "json.h"

class JsonObject
{
public:
	JsonObject();
	virtual ~JsonObject();

	bool LoadFile(const std::string &FileName, std::string &strError = *((std::string*)nullptr), Json::Value *pRoot = nullptr);
	bool SaveFile(const std::string &FileName, std::string &strError = *((std::string*)nullptr), Json::Value *pRoot = nullptr);
	bool LoadStr(const std::string &Str, std::string &strError = *((std::string*)nullptr), Json::Value *pRoot = nullptr);
	bool SaveStr(std::string &Str, std::string &strError = *((std::string*)nullptr), Json::Value *pRoot = nullptr);

	//拼接成json数据格式
	virtual std::string ToString() const;

protected:
	virtual bool JsonToObject(const Json::Value &Root, std::string &strError = *((std::string*)nullptr)) = 0;
	virtual bool ObjectToJson(Json::Value &Root, std::string &strError = *((std::string*)nullptr)) = 0;

};

#endif