#include "../include/JsonObject.hpp"
#include <fstream>

JsonObject::JsonObject()
{
}

JsonObject::~JsonObject()
{
}

bool JsonObject::LoadFile(const std::string &FileName, std::string &strError, Json::Value *pRoot)
{
	bool bResult = false;
	bool bNew = false;

	if (nullptr == pRoot)
	{
		bNew = true;
		pRoot = new Json::Value;
	}
	else
	{
		//clean old data
		pRoot->clear();
	}

	if (nullptr != pRoot)
	{
		std::ifstream iFile;

		iFile.open(FileName.c_str(), std::ios_base::in);

		if (iFile.is_open() && iFile.good())
		{
			Json::Reader reader;

			if (reader.parse(iFile, *pRoot, false))
			{
				if (nullptr != &strError)
				{
					bResult = JsonToObject(*pRoot, strError);
				}
				else
				{
					std::string szTemp;
					bResult = JsonToObject(*pRoot, szTemp);
				}
			}
			else
			{
				if (nullptr != &strError)
				{
					strError = "Parse file fail.";
				}
			}

			iFile.close();
		}
		else
		{
			if (nullptr != &strError)
			{
				strError = "Open file fail.";
			}
		}
	}

	if (bNew && nullptr != pRoot)
	{
		delete pRoot;
		pRoot = nullptr;
	}

	return bResult;
}

bool JsonObject::SaveFile(const std::string &FileName, std::string &strError, Json::Value *pRoot)
{
	bool bResult = false;
	bool bNew = false;

	if (nullptr == pRoot)
	{
		bNew = true;
		pRoot = new Json::Value;
	}
	else
	{
		//clean old data
		pRoot->clear();
	}

	if (nullptr != pRoot)
	{
		if (nullptr != &strError)
		{
			bResult = ObjectToJson(*pRoot, strError);
		}
		else
		{
			std::string szTemp;
			bResult = ObjectToJson(*pRoot, szTemp);
		}

		if (bResult)
		{
			std::ofstream oFile;

			oFile.open(FileName.c_str(), std::ios_base::out);
			if (oFile.is_open() && oFile.good())
			{
				Json::StyledWriter writer;
				oFile << writer.write(*pRoot);
				oFile.close();
			}
			else
			{
				if (nullptr != &strError)
				{
					bResult = false;
					strError = "Open file fail.";
				}
			}
		}
	}

	if (bNew && nullptr != pRoot)
	{
		delete pRoot;
		pRoot = nullptr;
	}

	return bResult;
}

bool JsonObject::LoadStr(const std::string &Str, std::string &strError, Json::Value *pRoot)
{
	bool bResult = false;
	bool bNew = false;

	if (nullptr == pRoot)
	{
		bNew = true;
		pRoot = new Json::Value;
	}
	else
	{
		//clean old data
		pRoot->clear();
	}

	if (nullptr != pRoot)
	{
		Json::Reader reader;

		if (reader.parse(Str, *pRoot, false))
		{
			if (nullptr != &strError)
			{
				bResult = JsonToObject(*pRoot, strError);
			}
			else
			{
				std::string szTemp;
				bResult = JsonToObject(*pRoot, szTemp);
			}
		}
		else
		{
			if (nullptr != &strError)
			{
				strError = "Parse str fail.";
			}
		}
	}

	if (bNew && nullptr != pRoot)
	{
		delete pRoot;
		pRoot = nullptr;
	}

	return bResult;
}

bool JsonObject::SaveStr(std::string &Str, std::string &strError, Json::Value *pRoot)
{
	bool bResult = false;
	bool bNew = false;

	if (nullptr == pRoot)
	{
		bNew = true;
		pRoot = new Json::Value;
	}
	else
	{
		//clean old data
		pRoot->clear();
	}

	if (nullptr != pRoot)
	{
		if (nullptr != &strError)
		{
			bResult = ObjectToJson(*pRoot, strError);
		}
		else
		{
			std::string szTemp;
			bResult = ObjectToJson(*pRoot, szTemp);
		}

		if (bResult)
		{
			Json::StyledWriter writer;
			Str = writer.write(*pRoot);

			//去掉尾部的换行
			if (Str[Str.length() - 2] == '\r')
				Str.erase(Str.length() - 2, 2);
			else if (Str[Str.length() - 1] == '\n')
				Str.erase(Str.length() - 1, 1);
		}
	}

	if (bNew && nullptr != pRoot)
	{
		delete pRoot;
		pRoot = nullptr;
	}

	return bResult;
}

std::string JsonObject::ToString() const
{
	return "";
}