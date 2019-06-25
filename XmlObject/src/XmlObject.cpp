/********************************************************************
created:	2014/10/24
file base:	XmlObject
file ext:	cpp
author:		�￡
purpose:	ʹ��tinyxml��װ�Ķ���
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

	//���û�н�����������һ����ʱ��
	if (NULL == XmlDocument)
	{
		bNew = true;
		XmlDocument = new TiXmlDocument();
	}
	else
	{
		bNew = false;

		//��������µĽ���������Ҫ��������ݡ�
		XmlDocument->Clear();
	}

	if (NULL != XmlDocument)
	{
		bResult = XmlDocument->LoadFile(FileName);

		if (bResult)
		{
			//�����ɹ�����ʼ������
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

	//���û�н�����������һ����ʱ��
	if (NULL == XmlDocument)
	{
		bNew = true;
		XmlDocument = new TiXmlDocument();
	}
	else
	{
		bNew = false;

		//��������µĽ���������Ҫ��������ݡ�
		XmlDocument->Clear();
	}

	if (NULL != XmlDocument)
	{
		//�����ɹ�����ʼ������
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

	//���û�н�����������һ����ʱ��
	if (NULL == XmlDocument)
	{
		bNew = true;
		XmlDocument = new TiXmlDocument();
	}
	else
	{
		bNew = false;

		//��������µĽ���������Ҫ��������ݡ�
		XmlDocument->Clear();
	}

	if (NULL != XmlDocument)
	{
		XmlDocument->Parse(StrXml);

		bResult = !XmlDocument->Error();

		if (bResult)
		{
			//�����ɹ�����ʼ������
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

	//���û�н�����������һ����ʱ��
	if (NULL == XmlDocument)
	{
		bNew = true;
		XmlDocument = new TiXmlDocument();
	}
	else
	{
		bNew = false;

		//��������µĽ���������Ҫ��������ݡ�
		XmlDocument->Clear();
	}

	if (NULL != XmlDocument)
	{
		//�����ɹ�����ʼ������
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

				//ȥ��β���Ļ���
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
