/********************************************************************
created:	2014/10/24
file base:	XmlObject
file ext:	cpp
author:		田俊
purpose:	使用tinyxml封装的对象。
*********************************************************************/
#include "../include/XmlObject.hpp"
#include <sstream>

using std::stringstream;

XmlObject::XmlObject()
{
}

XmlObject::~XmlObject()
{
}

bool XmlObject::LoadFile(const std::string &FileName, std::string &strError, TiXmlDocument *XmlDocument)
{
	bool bResult;
	bool bNew;

	//如果没有解析器，申请一个临时。
	if (NULL == XmlDocument)
	{
		bNew = true;
		XmlDocument = new TiXmlDocument();
	}
	else
	{
		bNew = false;

		//如果不是新的解析器，需要清除旧内容。
		XmlDocument->Clear();
	}

	if (NULL != XmlDocument)
	{
		bResult = XmlDocument->LoadFile(FileName);

		if (bResult)
		{
			//解析成功，初始化对象。
			if (NULL != &strError)
			{
				bResult = XmlToObject(*XmlDocument, strError);
			}
			else
			{
				std::string szTempError;
				bResult = XmlToObject(*XmlDocument, szTempError);
			}
		}
		else
		{
			if (NULL != &strError)
			{
				strError = XmlDocument->ErrorDesc();
			}
		}
	}
	else
	{
		bResult = false;
		if (NULL != &strError)
		{
			strError = "new TiXmlDocument fail!";
		}
	}


	if (bNew && NULL != XmlDocument)
	{
		delete XmlDocument;
		XmlDocument = NULL;
	}

	return bResult;
}

bool XmlObject::SaveFile(const std::string &FileName, std::string &strError, TiXmlDocument *XmlDocument) const
{
	bool bResult;
	bool bNew;

	//如果没有解析器，申请一个临时。
	if (NULL == XmlDocument)
	{
		bNew = true;
		XmlDocument = new TiXmlDocument();
	}
	else
	{
		bNew = false;

		//如果不是新的解析器，需要清除旧内容。
		XmlDocument->Clear();
	}

	if (NULL != XmlDocument)
	{
		//解析成功，初始化对象。
		if (NULL != &strError)
		{
			bResult = ObjectToXml(*XmlDocument, strError);
		}
		else
		{
			std::string szTempError;
			bResult = ObjectToXml(*XmlDocument, szTempError);
		}

		if (bResult)
		{
			bResult = XmlDocument->SaveFile(FileName);

			if (!bResult && NULL != &strError)
			{
				strError = XmlDocument->ErrorDesc();
			}
		}
	}
	else
	{
		bResult = false;
		if (NULL != &strError)
		{
			strError = "new TiXmlDocument fail!";
		}
	}


	if (bNew && NULL != XmlDocument)
	{
		delete XmlDocument;
		XmlDocument = NULL;
	}

	return bResult;
}

bool XmlObject::LoadStr(const std::string &StrXml, std::string &strError, TiXmlDocument *XmlDocument)
{
	return LoadStr(StrXml.c_str(), strError, XmlDocument);
}

bool XmlObject::LoadStr(const char* StrXml, std::string &strError, TiXmlDocument *XmlDocument)
{
	bool bResult;
	bool bNew;

	if (NULL == StrXml)
	{
		if (NULL != &strError)
		{
			strError = "input strXml is null !";
		}

		return false;
	}

	//如果没有解析器，申请一个临时。
	if (NULL == XmlDocument)
	{
		bNew = true;
		XmlDocument = new TiXmlDocument();
	}
	else
	{
		bNew = false;

		//如果不是新的解析器，需要清除旧内容。
		XmlDocument->Clear();
	}

	if (NULL != XmlDocument)
	{
		XmlDocument->Parse(StrXml);

		bResult = !XmlDocument->Error();

		if (bResult)
		{
			//解析成功，初始化对象。
			if (NULL != &strError)
			{
				bResult = XmlToObject(*XmlDocument, strError);
			}
			else
			{
				std::string szTempError;
				bResult = XmlToObject(*XmlDocument, szTempError);
			}
		}
		else
		{
			if (NULL != &strError)
			{
				strError = XmlDocument->ErrorDesc();
			}
		}
	}
	else
	{
		bResult = false;
		if (NULL != &strError)
		{
			strError = "new TiXmlDocument fail!";
		}
	}


	if (bNew && NULL != XmlDocument)
	{
		delete XmlDocument;
		XmlDocument = NULL;
	}

	return bResult;
}

bool XmlObject::SaveStr(std::string &StrXml, std::string &strError, TiXmlDocument *XmlDocument) const
{
	bool bResult;
	bool bNew;

	//如果没有解析器，申请一个临时。
	if (NULL == XmlDocument)
	{
		bNew = true;
		XmlDocument = new TiXmlDocument();
	}
	else
	{
		bNew = false;

		//如果不是新的解析器，需要清除旧内容。
		XmlDocument->Clear();
	}

	if (NULL != XmlDocument)
	{
		//解析成功，初始化对象。
		if (NULL != &strError)
		{
			bResult = ObjectToXml(*XmlDocument, strError);
		}
		else
		{
			std::string szTempError;
			bResult = ObjectToXml(*XmlDocument, szTempError);
		}

		if (bResult)
		{
			TiXmlPrinter printer;
			bResult = XmlDocument->Accept(&printer);

			if (bResult)
			{
				StrXml = printer.Str();

				//去掉尾部的换行
				if (StrXml[StrXml.length() - 2] == '\r')
					StrXml.erase(StrXml.length() - 2, 2);
				else if (StrXml[StrXml.length() - 1] == '\n')
					StrXml.erase(StrXml.length() - 1, 1);
			}
			else
			{
				if (NULL != &strError)
				{
					strError = XmlDocument->ErrorDesc();
				}
			}
		}
	}
	else
	{
		bResult = false;
		if (NULL != &strError)
		{
			strError = "new TiXmlDocument fail!";
		}
	}

	if (bNew && NULL != XmlDocument)
	{
		delete XmlDocument;
		XmlDocument = NULL;
	}

	return bResult;
}

bool XmlObject::SaveStr(char* StrXml, int& DataLength, std::string &strError, TiXmlDocument *XmlDocument) const
{
	if (NULL != StrXml)
	{
		delete[] StrXml;
		StrXml = NULL;
	}

	DataLength = 0;

	std::string szResult;
	if (SaveStr(szResult, strError, XmlDocument))
	{
		DataLength = (int)szResult.length();
		StrXml = new char[DataLength + 1]{ '\0' };
		
		if (NULL != StrXml)
			memcpy(StrXml, szResult.c_str(), DataLength);
		else
		{
			if (NULL != &strError)
			{
				strError = "new data buffer fail!";
			}

			return false;
		}
	}
	else
		return false;

	return true;
}

bool XmlObject::ToString(std::string &Out, const char *pszStr)
{
	bool bResult;

	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		Out = pszStr;

		bResult = true;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

bool XmlObject::ToInt(int &Out, const char *pszStr)
{
	bool bResult;

	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		Out = atoi(pszStr);
		bResult = true;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

bool XmlObject::ToUint(unsigned int &Out, const char *pszStr)
{
	bool bResult;

	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		Out = (unsigned int)atoi(pszStr);
		bResult = true;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

bool XmlObject::ToShort(short &Out, const char *pszStr)
{
	bool bResult;

	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		Out = (short)atoi(pszStr);
		bResult = true;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

bool XmlObject::ToUshort(unsigned short &Out, const char *pszStr)
{
	bool bResult;

	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		Out = (unsigned short)atoi(pszStr);
		bResult = true;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

bool XmlObject::ToDouble(double &Out, const char *pszStr)
{
	bool bResult;

	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		Out = atof(pszStr);
		bResult = true;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

bool XmlObject::ToBool(bool& Out, const char *pszStr)
{
	bool bResult;

	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		std::string szTemp = pszStr;

		if ("true" == szTemp)
		{
			Out = true;
		}
		else
		{
			Out = false;
		}

		bResult = true;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

bool XmlObject::ToLong(long &Out, const char*pszStr)
{
	bool bResult;

	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		Out = atol(pszStr);
		bResult = true;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

bool XmlObject::ToLongLong(long long &Out, const char*pszStr)
{
	bool bResult;

	if (NULL != pszStr && strlen(pszStr) >= 1)
	{
		Out = atoll(pszStr);
		bResult = true;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

std::string XmlObject::GetObjectType() const
{
	return std::move(std::string(""));
}
