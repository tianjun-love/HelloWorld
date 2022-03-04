#include "../include/JsonObject.hpp"
#include <fstream>

CJsonObject::CJsonObject()
{
}

CJsonObject::~CJsonObject()
{
}

bool CJsonObject::LoadFromFile(const std::string &szFileName, std::string &strError)
{
	bool bResult = false;
	std::ifstream iFile;

	if (szFileName.empty())
	{
		strError = "File name is empty !";
		return bResult;
	}

	iFile.open(szFileName.c_str(), std::ios_base::in);
	if (iFile.is_open() && iFile.good())
	{
		Json::Value Root;
		Json::Reader reader;

		if (reader.parse(iFile, Root, false))
		{
			if (Json::objectValue == Root.type())
			{
				if (JsonToObject(Root, strError))
				{
					//检查数据正确性
					bResult = CheckData(strError);
				}
			}
			else
				strError = "File comment format wrong !";
		}
		else
			strError = "Parse file fail:" + reader.getFormattedErrorMessages();

		iFile.close();
	}
	else
		strError = "Open file fail.";

	return bResult;
}

bool CJsonObject::SaveToFile(const std::string &szFileName, std::string &strError)
{
	bool bResult = false;
	Json::Value Root;

	if (szFileName.empty())
	{
		strError = "File name is empty !";
		return bResult;
	}

	if (ObjectToJson(Root, strError))
	{
		std::ofstream oFile;

		oFile.open(szFileName.c_str(), std::ios_base::out);
		if (oFile.is_open() && oFile.good())
		{
			Json::StyledWriter writer;

			oFile << writer.write(Root);
			oFile.close();
			bResult = true;
		}
		else
			strError = "Open file fail.";
	}

	return bResult;
}

bool CJsonObject::LoadFromString(const std::string &szJson, std::string &strError)
{
	bool bResult = false;
	Json::Value Root;
	Json::Reader reader;

	if (szJson.empty())
	{
		strError = "Json data is empty !";
		return bResult;
	}

	if (reader.parse(szJson, Root, false))
	{
		if (Json::objectValue == Root.type())
		{
			if (JsonToObject(Root, strError))
			{
				//检查数据正确性
				bResult = CheckData(strError);
			}
		}
		else
			strError = "Json data format wrong !";
	}
	else
		strError = "Parse file fail:" + reader.getFormattedErrorMessages();

	return bResult;
}

bool CJsonObject::SaveToString(std::string &szJson, std::string &strError)
{
	bool bResult = false;
	Json::Value Root;

	if (ObjectToJson(Root, strError))
	{
		Json::StyledWriter writer;

		szJson = writer.write(Root);

		//去掉尾部的换行
		if (szJson[szJson.length() - 2] == '\r')
			szJson.erase(szJson.length() - 2, 2);
		else if (szJson[szJson.length() - 1] == '\n')
			szJson.erase(szJson.length() - 1, 1);

		bResult = true;
	}

	return bResult;
}

std::string CJsonObject::ToString() const
{
	return "";
}