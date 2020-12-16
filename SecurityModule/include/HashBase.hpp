/********************************************************************
功能:	自定义HASH算法基类
作者:	田俊
时间:	2020-11-20
修改:
*********************************************************************/
#ifndef __HASH_BASE_HPP__
#define __HASH_BASE_HPP__

#include <string>

class CHashBase
{
public:
	CHashBase();
	virtual ~CHashBase();

	static bool CheckFileExist(const std::string &szFileName);
	static std::string BytesToHexString(const uint8_t *in, size_t length, bool bLowercase = false);
};

#endif
