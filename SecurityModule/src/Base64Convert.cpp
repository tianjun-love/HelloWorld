#include "../include/Base64Convert.hpp"
#include <algorithm>
#include <functional>

const std::string CBase64Convert::alphabet64  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char CBase64Convert::pad                = '=';
const char CBase64Convert::np                 = (char)std::string::npos;
const char CBase64Convert::table64vals[128]   =
{
	62, np, np, np, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, np, np, np, np, np,
	np, np,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23, 24, 25, np, np, np, np, np, np, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

CBase64Convert::CBase64Convert()
{
}

CBase64Convert::~CBase64Convert()
{
}

/********************************************************************
名称:	table64
功能:	根据字符范围获取字符
参数: 	c 要获取的字符
返回:    获取的字符
*********************************************************************/
inline char CBase64Convert::table64(const char c)
{
	return ((c < 43 || c > 122) ? np : table64vals[c - 43]);
}

/********************************************************************
名称:	trimString
功能:	删除字符串两端的空格、换行及制表符
参数: 	str 要处理的字符串
*		type 处理类型，0：两边，-1：左边，1：右边
返回:    无
*********************************************************************/
inline void CBase64Convert::trimString(std::string& str, short type)
{
	if (!str.empty())
	{
		//去除开头的
		if (type <= 0)
			str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun(::isspace))));

		//去除结尾的
		if (type >= 0)
			str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun(::isspace))).base(), str.end());

		//去除中间换行
		str.erase(std::remove_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), '\n')), str.end());
	}
}

/********************************************************************
名称:	Encode64
功能:	转换成base64
参数: 	str 要转换的char字符串
*		strLength char字符串长度
*		bLineFeed 是否每76字节换行
返回:    返回base64字符串
*********************************************************************/
std::string CBase64Convert::Encode64(const char* str, size_t strLength, bool bLineFeed)
{
	std::string encoded;
	char c;
	size_t uiLineFeedLength = 0;

	//编码规则为（3*8=4*6）：每3字节共24位 => 6位一分共4段，6位对应一编码表里面一个字符，高位补0
	const size_t uiEncodeLength = (strLength / 3 + 1) * 4;

	if (bLineFeed)
		encoded.reserve(uiEncodeLength + uiEncodeLength / 76 + 1);
	else
		encoded.reserve(uiEncodeLength);

	for (size_t i = 0; i < strLength; ++i)
	{
		c = static_cast<char>((str[i] >> 2) & 0x3f);
		encoded += alphabet64[c];

		c = static_cast<char>((str[i] << 4) & 0x3f);
		if (++i < strLength)
			c = static_cast<char>(c | static_cast<char>((str[i] >> 4) & 0x0f));
		encoded += alphabet64[c];

		if (i < strLength)
		{
			c = static_cast<char>((str[i] << 2) & 0x3c);
			if (++i < strLength)
				c = static_cast<char>(c | static_cast<char>((str[i] >> 6) & 0x03));
			encoded += alphabet64[c];
		}
		else
		{
			++i;
			encoded += pad;
		}

		if (i < strLength)
		{
			c = static_cast<char>(str[i] & 0x3f);
			encoded += alphabet64[c];
		}
		else
		{
			encoded += pad;
		}

		//每76字节换行，RFC822标准
		if (bLineFeed && encoded.length() == (76 * (uiLineFeedLength + 1) + uiLineFeedLength))
		{
			encoded += '\n';
			++uiLineFeedLength;
		}
	}

	//最后加换行
	if (bLineFeed && !encoded.empty())
	{
		encoded += '\n';
	}

	return std::move(encoded);
}

/********************************************************************
名称:	Encode64
功能:	转换成base64
参数: 	szStr 要转换的字符串
*		bLineFeed 是否每76字节换行
返回:    返回base64字符串
*********************************************************************/
std::string CBase64Convert::Encode64(const std::string& szStr, bool bLineFeed)
{
	return std::move(Encode64(szStr.c_str(), szStr.length(), bLineFeed));
}

/********************************************************************
名称:	Decode64
功能:	解码base64
参数: 	szBase64 要转换的字符串
返回:    返回正常字符串
*********************************************************************/
std::string CBase64Convert::Decode64(const std::string& szBase64)
{
	std::string decoded;
	size_t strLength = 0;
	char *pResult = nullptr;

	pResult = Decode64(szBase64, strLength);
	if (nullptr != pResult)
	{
		decoded = pResult;
		delete[] pResult;
	}

	return std::move(decoded);
}

/********************************************************************
名称:	Decode64
功能:	解码base64
参数: 	szBase64 要转换的字符串
*		strLength 转换后的长度
返回:    返回char字符串
*********************************************************************/
char* CBase64Convert::Decode64(const std::string& szBase64, size_t& strLength)
{
	char c, d;
	std::string szTempBase64(szBase64);
	trimString(szTempBase64, 1); //去除掉结尾可能存在的空格
	const size_t length = szTempBase64.length();
	char *pResult = nullptr;
	strLength = 0;

	if (length >= 1)
	{
		pResult = new char[((length / 4 + 1) * 3)]{ '\0' };

		for (size_t i = 0; i < length; ++i)
		{
			c = table64(szTempBase64[i]);
			++i;
			d = table64(szTempBase64[i]);
			c = (char)((c << 2) | ((d >> 4) & 0x3));
			pResult[strLength++] = c;

			if (++i < length)
			{
				c = szTempBase64[i];
				if (pad == c)
				{
					break;
				}

				c = table64(szTempBase64[i]);
				d = (char)(((d << 4) & 0xf0) | ((c >> 2) & 0xf));
				pResult[strLength++] = d;
			}

			if (++i < length)
			{
				d = szTempBase64[i];
				if (pad == d)
				{
					break;
				}

				d = table64(szTempBase64[i]);
				c = (char)(((c << 6) & 0xc0) | d);
				pResult[strLength++] = c;
			}
		}
	}

	return pResult;
}