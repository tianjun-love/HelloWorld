#include "../include/JsonObject.hpp"
#include <fstream>

CNlohmannjsonObject::CNlohmannjsonObject()
{
}

CNlohmannjsonObject::~CNlohmannjsonObject()
{
}

/************************************************
*功能：	将json文件转换成对象
*参数：	szFileName json文件名称
*		szError 错误信息
*返回：	成功返回true
************************************************/
bool CNlohmannjsonObject::LoadFromFile(const std::string& szFileName, std::string& szError)
{
	if (szFileName.empty())
	{
		szError = "File name is empty !";
		return false;
	}

	bool bResult = false;
	std::ifstream inFile;

	inFile.open(szFileName, std::ios_base::in | std::ios_base::binary);
	if (inFile.is_open() && inFile.good())
	{
		nlohmann::json root;

		try
		{
			inFile >> root;

			if (root.is_object())
				bResult = JsonToObject(root, szError);
			else
				szError = "File content format wrong !";
		}
		catch (const std::exception& ex)
		{
			szError = std::string("Parse file document error:") + ex.what();
		}
		catch (...)
		{
			szError = "Parse file document unknow error !";
		}

		inFile.close();
	}
	else
		szError = "Open file failed !";

	return bResult;
}

/************************************************
*功能：	将json对象转换成文件
*参数：	szFileName json文件名称
*		szError 错误信息
*返回：	成功返回true
************************************************/
bool CNlohmannjsonObject::SaveToFile(const std::string& szFileName, std::string& szError)
{
	if (szFileName.empty())
	{
		szError = "File name is empty !";
		return false;
	}

	bool bResult = false;
	nlohmann::json root;

	if (ObjectToJson(root, szError))
	{
		std::ofstream outFile;

		outFile.open(szFileName, std::ios_base::out);
		if (outFile.is_open() && outFile.good())
		{
			try
			{
				outFile << root.dump(4);
				bResult = true;
			}
			catch (const std::exception& ex)
			{
				szError = std::string("Save file document error:") + ex.what();
			}
			catch (...)
			{
				szError = "Save  file document unknow error !";
			}

			outFile.close();
		}
		else
			szError = "Open file failed !";
	}

	return bResult;
}

/************************************************
*功能：	将json字符串转换成对象
*参数：	szJson json字符串
*		szError 错误信息
*返回：	成功返回true
************************************************/
bool CNlohmannjsonObject::LoadFromString(const std::string& szJson, std::string& szError)
{
	if (szJson.empty())
	{
		szError = "Json data is empty !";
		return false;
	}

	bool bResult = false;
	
	try
	{
		nlohmann::json root = nlohmann::json::parse(szJson);

		if (root.is_object())
			bResult = JsonToObject(root, szError);
		else
			szError = "String content format wrong !";
	}
	catch (const std::exception& ex)
	{
		szError = std::string("Parse string document error:") + ex.what();
	}
	catch (...)
	{
		szError = "Parse string document unknow error !";
	}

	return bResult;
}

/************************************************
*功能：	将json对象转换成字符串
*参数：	szJson 转换后的字符串
*		szError 错误信息
*返回：	成功返回true
************************************************/
bool CNlohmannjsonObject::SaveToString(std::string& szJson, std::string& szError)
{
	bool bResult = false;
	nlohmann::json root;

	if (ObjectToJson(root, szError))
	{
		try
		{
			szJson = root.dump(4);
			bResult = true;
		}
		catch (const std::exception& ex)
		{
			szError = std::string("Save string document error:") + ex.what();
		}
		catch (...)
		{
			szError = "Save string document unknow error !";
		}
	}

	return bResult;
}

std::string CNlohmannjsonObject::ToString() const
{
	return "{}";
}