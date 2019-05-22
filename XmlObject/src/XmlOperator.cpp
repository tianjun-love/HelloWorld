#include "../include/XmlOperator.hpp"
using namespace GDASK;

//构造初始化文件路径、名称与文档对象
CXmlOperator::CXmlOperator():m_pXmlDoc(new TiXmlDocument)
{

}

//析构文档对象
CXmlOperator::~CXmlOperator()
{
	if (NULL != m_pXmlDoc)
	{
		delete m_pXmlDoc;
		m_pXmlDoc = NULL;
	}
}

//获取文件路径
const char* CXmlOperator::getXmlPath() const
{
	return m_szFilePath.c_str();
}

//获取文件名
const char* CXmlOperator::getXmlName() const
{
	return m_szFileName.c_str();
}

//解析加载文件
bool CXmlOperator::loadFile(const string &szFileName, const string &szFilePath, string &szError)
{
	bool bSucc;

	m_pXmlDoc->Clear();		//m_pXmlDoc不可能为NULL

	m_szFilePath = szFilePath;
	if ('/' != m_szFilePath.at(m_szFilePath.length()-1))
	{
		m_szFilePath += '/';
	}
	m_szFileName = szFileName;

	bSucc = m_pXmlDoc->LoadFile(m_szFilePath+m_szFileName);
	if (!bSucc)
	{
		m_pXmlDoc->Clear();
		szError = m_pXmlDoc->ErrorDesc();
	}

	return bSucc;
}

//保存对象到文件
bool CXmlOperator::saveFile(const string &szFileName, const string &szFilePath, string &szError) const
{
	//文档样式是否有错
	if (m_pXmlDoc->Error())
	{
		szError = m_pXmlDoc->ErrorDesc();
		szError += ",fail to save file!";
		return false;
	}

	bool bSucc;
	string szTemp(szFilePath);
	if ('/' != szTemp.at(szTemp.length()-1))
	{
		szTemp += '/';
	}
	szTemp += szFileName;

	bSucc = m_pXmlDoc->SaveFile(szTemp);
	if (!bSucc)
	{
		szError = "fail to save file!";
	}

	return bSucc;
}

//字符串加载成文档对象
bool CXmlOperator::loadStr(const string &szStrXml, string &szError)
{
	bool bSucc = true;

	m_pXmlDoc->Clear();
	m_pXmlDoc->Parse(szStrXml.c_str());
	if (m_pXmlDoc->Error())
	{
		bSucc = false;
		szError = m_pXmlDoc->ErrorDesc();
		m_pXmlDoc->Clear();
	}

	return bSucc;
}

//文档对象保存到字符串
bool CXmlOperator::saveStr(string &szStrXml, string &szError) const
{
	bool bSucc = false;

	TiXmlPrinter printer;
	if (m_pXmlDoc->Accept(&printer))
	{
		bSucc = true;
		szStrXml = printer.CStr();
	}
	else
	{
		szError = m_pXmlDoc->ErrorDesc();
	}

	return bSucc;
}

// 获取节点名称
string CXmlOperator::getElemName(TiXmlElement* pCurElem) const
{
	return (NULL == pCurElem)?"":pCurElem->ValueStr();
}

//获取根节点
TiXmlElement* CXmlOperator::getRootElem() const
{
	return m_pXmlDoc->RootElement();
}

//获取指定节点元素上一个（指名）同级节点元素
TiXmlElement* CXmlOperator::getPreElem(TiXmlElement* pCurElem, const char* pcName /* = NULL */) const
{
	if (NULL != pCurElem)
	{
		TiXmlNode* pcNode = pCurElem;
		if (NULL != pcName)
		{
			do
			{
				//直到向前找到是元素的节点，或找不到
				pcNode = pcNode->PreviousSibling(pcName);
			} while (NULL != pcNode && TiXmlNode::TINYXML_ELEMENT != pcNode->Type());
		}
		else
		{
			do
			{
				pcNode = pcNode->PreviousSibling();
			} while (NULL != pcNode && TiXmlNode::TINYXML_ELEMENT != pcNode->Type());
		}

		if (NULL != pcNode)
		{
			return pcNode->ToElement();
		}
	}

	return NULL;
}

//获取指定节点元素下一(指名)同级节点元素
TiXmlElement* CXmlOperator::getNextElem(TiXmlElement* pPreElem, const char* pcName /* = NULL */) const
{
	if (NULL != pPreElem)
	{
		return (NULL == pcName)?pPreElem->NextSiblingElement():pPreElem->NextSiblingElement(pcName);
	}
	return NULL;
}

//获取指定节点元素第一个（指名）子节点元素
TiXmlElement* CXmlOperator::getFirstChildElem(TiXmlElement* pParElem, const char* pcName /* = NULL */) const
{
	if (NULL != pParElem)
	{
		return (NULL == pcName)?pParElem->FirstChildElement():pParElem->FirstChildElement(pcName);
	}
	return NULL;
}

//获取指定节点元素最后一个（指名）子节点元素
TiXmlElement* CXmlOperator::getLastChildElem(TiXmlElement* pParElem, const char* pcName /* = NULL */) const
{
	if (NULL == pParElem)
	{
		return NULL;
	}

	//获取到第一个子节点
	TiXmlElement* pSearchElem = pParElem->FirstChildElement();
	if (NULL != pSearchElem)
	{
		if (NULL != pcName)
		{
			//循环向后获取
			while(pSearchElem->NextSiblingElement(pcName))
			{
				pSearchElem = pSearchElem->NextSiblingElement(pcName);
			}
		}
		else
		{
			while(pSearchElem->NextSiblingElement())
			{
				pSearchElem = pSearchElem->NextSiblingElement();
			}
		}

		return pSearchElem;
	}
	return NULL;
}

//获取指定节点上一个（指名）同级节点
TiXmlNode* CXmlOperator::getPreNode(TiXmlNode* pCurNode, const char* pcName /* = NULL */) const
{
	if (NULL != pCurNode)
	{
		return (NULL == pcName)?pCurNode->PreviousSibling():pCurNode->PreviousSibling(pcName);
	}
	return NULL;
}

//获取指定节点下一个（指名）同级节点
TiXmlNode* CXmlOperator::getNextNode(TiXmlNode* pCurNode, const char* pcName /* = NULL */) const
{
	if (NULL != pCurNode)
	{
		return (NULL == pcName)?pCurNode->NextSibling():pCurNode->NextSibling(pcName);
	}

	return NULL;
}

//获取指定节点的第一个（指名）子节点
TiXmlNode* CXmlOperator::getFirstChildNode(TiXmlNode* pParNode, const char* pcName /* = NULL */) const
{
	if (NULL != pParNode)
	{
		return (NULL == pcName)?pParNode->FirstChild():pParNode->FirstChild(pcName);
	}
	return NULL;
}

//获取指定节点的最后一个（指名）子节点
TiXmlNode* CXmlOperator::getLastChildNode(TiXmlNode* pParNode, const char* pcName /* = NULL */) const
{
	if (NULL != pParNode)
	{
		return (NULL == pcName)?pParNode->LastChild():pParNode->LastChild(pcName);
	}
	return NULL;
}

//获取指定节点的父节点
TiXmlNode* CXmlOperator::getParent(TiXmlNode* pCurNode) const
{
	if (NULL != pCurNode)
	{
		return pCurNode->Parent();
	}
	return NULL;
}

//打印输出XML文档内容
void CXmlOperator::printDoc() const
{
	if (NULL != m_pXmlDoc)
	{
		m_pXmlDoc->Print();
	}
}

//获取指定节点元素包含的文本值
const char* CXmlOperator::getElemText(TiXmlElement* pElem) const
{
	const char* cpResult = NULL;
	if (NULL != pElem)
	{
		cpResult = pElem->GetText();
		return (NULL==cpResult)?"":cpResult;
	}
	return "";
}

//获取指定节点元素的（指名）属性的属性值
string CXmlOperator::getElemAttriValue(const TiXmlElement* pElem,
	const char* pcName) const
{
	const char* cpResult = NULL;
	if ((NULL != pElem) && (NULL != pcName))
	{
		cpResult = pElem->Attribute(pcName);
		return (NULL==cpResult)?"":cpResult;
	}
	printf("param error!\n");
	return "";
}

//获取指定节点元素的第一个属性对象
TiXmlAttribute* CXmlOperator::getFirstAttribute(TiXmlElement* pElem) const
{
	if (NULL != pElem)
	{
		return pElem->FirstAttribute();
	}

	return NULL;
}

//获取指定节点元素的最后一个属性对象
TiXmlAttribute* CXmlOperator::getLastAttribute(TiXmlElement* pElem) const
{
	if (NULL != pElem)
	{
		return pElem->LastAttribute();
	}
	return NULL;
}

//添加根节点
TiXmlElement* CXmlOperator::addRootElement(const char* pcRootName)
{
	if (NULL != pcRootName)
	{
		//创建根节点
		TiXmlElement* pRootElement = new TiXmlElement(pcRootName);
		if (NULL != pRootElement)
		{
			m_pXmlDoc->LinkEndChild(pRootElement);
			return m_pXmlDoc->RootElement();
		}
	}
	return NULL;
}

//在指定的节点元素最后添加子节点元素(节点文本缺省)
TiXmlElement* CXmlOperator::addChildElement(TiXmlElement* pParElem,
	const char* pcChilName, const char* pcChilText /* = NULL */)
{
	//节点元素指针和新增节点名称都不为空
	if ((NULL != pParElem) && (NULL != pcChilName))
	{
		TiXmlElement* pChildElem = new TiXmlElement(pcChilName);
		if (NULL != pChildElem)
		{
			pParElem->LinkEndChild(pChildElem);
			//节点文本不为空
			if (NULL != pcChilText)
			{
				addElemText(pChildElem, pcChilText);
			}
			return pChildElem;
		}
	}
	return NULL;
}

//增加XML文档声明
bool CXmlOperator::addDeclaration(const char* pcVersion /* = "1.0" */,
	const char* pcEnCoding /* = "gb2312" */, const char* pcStandalone /* = "Yes" */)
{
	if ((NULL != pcVersion) && (NULL != pcEnCoding) && (NULL != pcStandalone))
	{
		TiXmlDeclaration* pDeclaration = new TiXmlDeclaration(pcVersion, pcEnCoding, pcStandalone);
		if (NULL != pDeclaration)
		{
			m_pXmlDoc->LinkEndChild(pDeclaration);
			return true;
		}
	}
	return false;
}

//增加XML文档注释
bool CXmlOperator::addComment(TiXmlNode* pElem, const char* pcComment)
{
	if ((NULL != pElem) && (NULL != pcComment))
	{
		TiXmlComment* pComment = new TiXmlComment(pcComment);
		pElem->LinkEndChild(pComment);
		return true;
	}
	return false;
}

//为指定节点元素添加文本
bool CXmlOperator::addElemText(TiXmlElement* pElem, const char* pcText)
{
	if ((NULL != pElem) && (NULL != pcText))
	{
		TiXmlText* pText = new TiXmlText(pcText);
		pElem->LinkEndChild(pText);
		return true;
	}
	return false;
}

//为指定节点元素添加 属性 和 属性值
bool CXmlOperator::addElemAttribute(TiXmlElement* pElem,
	const char* pcAttriName, const char* pcAttriValue)
{
	if ((NULL != pElem) && (NULL != pcAttriName) && (NULL != pcAttriValue))
	{
		pElem->SetAttribute(pcAttriName, pcAttriValue);
		return true;
	}
	return false;
}

//在指定的子节点位置前插入节点
TiXmlNode* CXmlOperator::insertBeforeChild(TiXmlNode* pFather,
	TiXmlNode* pBefThis, const TiXmlNode& txAddThis)
{
	if ((NULL != pFather) && (NULL != pBefThis) && (NULL != pFather))
	{
		return pFather->InsertBeforeChild(pBefThis, txAddThis);;
	}
	return NULL;
}

//在指定的子节点位置后插入节点
TiXmlNode* CXmlOperator::insertAfterChild(TiXmlNode* pFather,
	TiXmlNode* pAftThis, const TiXmlNode& txAddThis)
{
	if ((NULL != pFather) && (NULL != pAftThis))
	{
		return pFather->InsertAfterChild(pAftThis, txAddThis);
	}
	return NULL;
}

//替换指定的子节点
TiXmlNode* CXmlOperator::replaceChild(TiXmlNode* pFather,
	TiXmlNode* pReplaceThis, const TiXmlNode& txWithThis)
{
	if ((NULL != pFather) && (NULL != pReplaceThis))
	{
		return pFather->ReplaceChild(pReplaceThis, txWithThis);
	}
	return NULL;
}

//删除指定的子节点
bool CXmlOperator::removeChild(TiXmlNode* pFathNode, TiXmlNode* pRemoveThis)
{
	if ((NULL != pFathNode) && (NULL != pRemoveThis))
	{
		return pFathNode->RemoveChild(pRemoveThis);
	}
	return false;
}

//删除指定节点元素的指定属性
bool CXmlOperator::removeAttribute(TiXmlElement* pElem, const char* pcName)
{
	if ((NULL != pElem) && (NULL != pcName))
	{
		pElem->RemoveAttribute(pcName);
		return true;
	}
	return false;
}

//删除指定节点元素的所有子节点
bool CXmlOperator::clear(TiXmlElement* pElem)
{
	if (NULL != pElem)
	{
		pElem->Clear();
		return true;
	}
	return false;
}

void CXmlOperator::ClearDocument()
{
	m_pXmlDoc->Clear();
}

//修改指定节点元素的文本
bool CXmlOperator::modifyElemText(TiXmlElement* pElem, const char* pcNewValue)
{
	if ((NULL != pElem) && (NULL != pcNewValue))
	{
		TiXmlNode* pOldNode = NULL;
		TiXmlNode* pResult = NULL;

		//获取下一文本节点
		pOldNode = pElem->FirstChild();
		if (NULL != pOldNode)
		{
			TiXmlText newText(pcNewValue);
			//替换成新的文本节点
			pResult = pElem->ReplaceChild(pOldNode, newText);
			if (NULL != pResult)
			{
				return true;
			}
		}
	}
	return false;
}

//修改指定节点元素的（指名）属性的属性值
bool CXmlOperator::modifyElemAttri(TiXmlElement* pElem,
	const char* pcAttriName, const char* pcAttriValue)
{
	if ((NULL != pElem) && (NULL != pcAttriName) && (NULL != pcAttriValue ))
	{
		TiXmlAttribute* pOldAttribute = NULL;
		//获取第一个属性指针
		pOldAttribute = pElem->FirstAttribute();
		while(NULL != pOldAttribute)
		{
			if (0 == strcmp(pOldAttribute->Name(), pcAttriName))
			{
				pOldAttribute->SetValue(pcAttriValue);
				return true;
			}
			pOldAttribute = pOldAttribute->Next();
		}
	}
	return false;
}

//直接返回节点包含的下一级子节点的个数
int CXmlOperator::countChildNode(TiXmlNode* pNode,
	string szNodeName/* ="" */) const
{
	int iCount = 0;
	if (NULL != pNode)
	{
		TiXmlNode* pCurrentNode = NULL;

		if (strcmp("",szNodeName.c_str()))
		{
			//获取第一个节点指针
			pCurrentNode = pNode->FirstChild(szNodeName);
			while (NULL != pCurrentNode)
			{
				++iCount;
				pCurrentNode = pCurrentNode->NextSibling(szNodeName);
			}
		}
		else
		{
			//获取第一个节点指针
			pCurrentNode = pNode->FirstChild();
			while (NULL != pCurrentNode)
			{
				++iCount;
				pCurrentNode = pCurrentNode->NextSibling();
			}
		}
	}

	return iCount;
}

//直接返回节点包含的下一级子节点元素的个数
int CXmlOperator::countChildElem(TiXmlElement* pElem,
	string szElemName /* = "" */) const
{
	int iCount = 0;
	if (NULL != pElem)
	{
		TiXmlElement* pCurrentElem = NULL;

		if (strcmp("",szElemName.c_str()))
		{
			//获取第一个节点元素指针
			pCurrentElem = pElem->FirstChildElement(szElemName);
			while (NULL != pCurrentElem)
			{
				++iCount;
				pCurrentElem = pCurrentElem->NextSiblingElement(szElemName);
			}
		}
		else
		{
			pCurrentElem = pElem->FirstChildElement();
			while (NULL != pCurrentElem)
			{
				++iCount;
				pCurrentElem = pCurrentElem->NextSiblingElement();
			}
		}
	}

	return iCount;
}

//直接根据节点指针与子节点的序号得到子节点的指针
TiXmlNode* CXmlOperator::getChildByCount(TiXmlNode* pNode, const int iOrder) const
{
	int iMaxCount = 0;
	int iCount = 1;

	if (NULL != pNode)
	{
		//计算子节点个数
		iMaxCount = countChildNode(pNode);
		pNode = pNode->FirstChild();
		if ((iOrder > 0) && (iMaxCount >= iOrder))
		{
			for (; iCount<iOrder; ++iCount)
			{
				pNode = pNode->NextSibling();
			}
			return pNode;
		}
	}

	return NULL;
}

//直接根据节点指针和子节点的名称得到子节点的指针
TiXmlNode* CXmlOperator::getChildByName(TiXmlNode* pNode, const char* pcName) const
{
	if (NULL != pNode)
	{
		if (NULL != pcName)
		{
			return pNode->FirstChild(pcName);
		}
	}
	return NULL;
}

//根据节点指针一次取出所有的属性及属性值
bool CXmlOperator::getAllAttribute(TiXmlElement* pElem,
	map<string, string> &mapAttribute) const
{
	if (NULL != pElem)
	{
		TiXmlAttribute* pAttribute = pElem->FirstAttribute();
		if (NULL != pAttribute)
		{
			string szName = "";
			string szValue = "";

			//从第一个属性开始循环获取
			while(pAttribute)
			{
				szName = pAttribute->NameTStr();
				szValue = pAttribute->ValueStr();
				mapAttribute.insert(pair<string, string>(szName, szValue));

				pAttribute = pAttribute->Next();
			}
			return true;
		}
	}
	//节点为NULL 或 不存在属性则返回false
	return false;
}

//以各子节点作为属性保存信息的情况，根据节点指针一次取出所有属性及属性值
bool CXmlOperator::getAllAttributeByChild(TiXmlElement* pElem,
	map<string, string> &mapAttribute) const
{
	if (NULL != pElem)
	{
		TiXmlElement* pChileElem = pElem->FirstChildElement();
		if (NULL != pChileElem)
		{
			string szName = "";
			char* pcValue = NULL;
			const char* pcTemp = NULL;

			//从第一个子节点元素开始循环获取
			while (pChileElem)
			{
				szName = pChileElem->ValueStr();
			    pcTemp = pChileElem->GetText();

				if (pcTemp == NULL)
				{
					mapAttribute.insert(pair<string, string>(szName, ""));
					pChileElem = pChileElem->NextSiblingElement();
					continue;
				}
				pcValue = new char[strlen(pcTemp) + 1];
				memcpy(pcValue, pcTemp, strlen(pcTemp) + 1);
				//strcpy(pcValue, pcTemp);

				mapAttribute.insert(pair<string, string>(szName, pcValue));
				pChileElem = pChileElem->NextSiblingElement();
				delete pcValue;//释放内存
			}
			pcValue = NULL;
			return true;
		}
	}

	return false;
}

//获取文档对象
TiXmlDocument* CXmlOperator::getDoc() const
{
	return m_pXmlDoc;
}
