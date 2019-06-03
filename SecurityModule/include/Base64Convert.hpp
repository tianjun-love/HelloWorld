/********************************************************************
名称:	Base64转换类
功能:	Base64转换
作者:	田俊
时间:	2015-03-21
修改:
*********************************************************************/
#ifndef __BASE64_CONVERT_HPP__
#define __BASE64_CONVERT_HPP__

#include <string>

class CBase64Convert
{
public:
	CBase64Convert();
	~CBase64Convert();

	static std::string Encode64(const char* str, size_t strLength, bool bLineFeed = false);    //转换成base64
	static std::string Encode64(const std::string& szStr, bool bLineFeed = false);    //转换成base64
	static std::string Decode64(const std::string& szBase64); //解码base64
	static char* Decode64(const std::string& szBase64, size_t& strLength); //解码base64

private:
	static inline char table64(const char c); //根据字符范围获取字符
	static inline void trimString(std::string& str, short type = 0); //删除字符串两端的空格、换行及制表符

private:
	static const std::string alphabet64;
	static const char        pad;
	static const char        np;
	static const char        table64vals[128];

};

#endif