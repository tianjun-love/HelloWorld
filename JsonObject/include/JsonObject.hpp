/***********************************************************/
//function:jsoncpp
//note    :create by tianjun 2014-11-20
/***********************************************************/
#ifndef __JSONOBJECT_HPP__
#define __JSONOBJECT_HPP__

#include <string>
#include "json.h"

using std::string;

class JsonObject
{
public:
	JsonObject();
	virtual ~JsonObject();

	bool LoadFile(const string &FileName, string &strError = *((string*)nullptr), Json::Value *pRoot = nullptr);
	bool SaveFile(const string &FileName, string &strError = *((string*)nullptr), Json::Value *pRoot = nullptr);
	bool LoadStr(const string &Str, string &strError = *((string*)nullptr), Json::Value *pRoot = nullptr);
	bool SaveStr(string &Str, string &strError = *((string*)nullptr), Json::Value *pRoot = nullptr);

	//拼接成json数据格式
	virtual string ToString() const;

protected:
	virtual bool JsonToObject(const Json::Value &Root, string &strError = *((string*)nullptr)) = 0;
	virtual bool ObjectToJson(Json::Value &Root, string &strError = *((string*)nullptr)) = 0;

};

#endif