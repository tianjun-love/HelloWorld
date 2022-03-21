#include "../include/JsonObject.hpp"
#include <fstream>

CNlohmannjsonObject::CNlohmannjsonObject()
{
}

CNlohmannjsonObject::~CNlohmannjsonObject()
{
}

/************************************************
*���ܣ�	��json�ļ�ת���ɶ���
*������	szFileName json�ļ�����
*		szError ������Ϣ
*���أ�	�ɹ�����true
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
*���ܣ�	��json����ת�����ļ�
*������	szFileName json�ļ�����
*		szError ������Ϣ
*���أ�	�ɹ�����true
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
*���ܣ�	��json�ַ���ת���ɶ���
*������	szJson json�ַ���
*		szError ������Ϣ
*���أ�	�ɹ�����true
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
*���ܣ�	��json����ת�����ַ���
*������	szJson ת������ַ���
*		szError ������Ϣ
*���أ�	�ɹ�����true
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