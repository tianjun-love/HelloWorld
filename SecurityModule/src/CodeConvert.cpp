#include "../include/CodeConvert.hpp"
#include <algorithm>
#include <functional>

const std::string CCodeConvert::m_szAlphabet64  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char CCodeConvert::m_cPad                 = '=';
const char CCodeConvert::m_cNp                  = (char)std::string::npos;
const char CCodeConvert::m_Table64vals[128]     =
{
	62, m_cNp, m_cNp, m_cNp, 63, // '+', '/'
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, // 0-9
	m_cNp, m_cNp, m_cNp, m_cNp, m_cNp, m_cNp, m_cNp, 
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, // A-Z
	m_cNp, m_cNp, m_cNp, m_cNp, m_cNp, m_cNp, 
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 //a-z
};

CCodeConvert::CCodeConvert()
{
}

CCodeConvert::~CCodeConvert()
{
}

/********************************************************************
功能:	根据字符范围获取字符
参数: 	c 要获取的字符
返回:	获取的字符
*********************************************************************/
inline char CCodeConvert::TableBase64(const char c)
{
	return ((c < 43 || c > 122) ? m_cNp : m_Table64vals[c - 43]);
}

/********************************************************************
功能:	转换成base64
参数: 	data 要转换的char字符串
*		data_len char字符串长度
*		bLineFeed 是否补充换行
返回:	返回base64字符串
*********************************************************************/
std::string CCodeConvert::EncodeBase64(const unsigned char* data, uint32_t data_len, bool bLineFeed)
{
	std::string encoded;
	char c;
	size_t uiLineFeedLength = 0;

	//编码规则为（3*8=6*4）：每3字节共24位 => 6位一分共4段，6位对应一编码表里面一个字符，不够补0，编码后长度一定是4的整数倍
	const size_t uiEncodeLength = (data_len / 3 + 1) * 4;

	if (bLineFeed)
		encoded.reserve(uiEncodeLength + uiEncodeLength / 76 + 1);
	else
		encoded.reserve(uiEncodeLength);

	for (uint32_t i = 0; i < data_len; ++i)
	{
		c = static_cast<char>((data[i] >> 2) & 0x3f);
		encoded += m_szAlphabet64[c];

		c = static_cast<char>((data[i] << 4) & 0x3f);
		if (++i < data_len)
			c = static_cast<char>(c | static_cast<char>((data[i] >> 4) & 0x0f));
		encoded += m_szAlphabet64[c];

		if (i < data_len)
		{
			c = static_cast<char>((data[i] << 2) & 0x3c);
			if (++i < data_len)
				c = static_cast<char>(c | static_cast<char>((data[i] >> 6) & 0x03));
			encoded += m_szAlphabet64[c];
		}
		else
		{
			++i;
			encoded += m_cPad;
		}

		if (i < data_len)
		{
			c = static_cast<char>(data[i] & 0x3f);
			encoded += m_szAlphabet64[c];
		}
		else
		{
			encoded += m_cPad;
		}

		//每76字节换行，RFC822标准，但openssl里面是64字节
		if (bLineFeed && encoded.length() == (64 * (uiLineFeedLength + 1) + uiLineFeedLength))
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
功能:	转换成base64
参数: 	szStr 要转换的字符串
*		bLineFeed 是否补充换行
返回:    返回base64字符串
*********************************************************************/
std::string CCodeConvert::EncodeBase64(const std::string& szStr, bool bLineFeed)
{
	return EncodeBase64((const unsigned char*)szStr.c_str(), (uint32_t)szStr.length(), bLineFeed);
}

/********************************************************************
功能:	解码base64
参数: 	szBase64 要转换的字符串
返回:	返回正常字符串
*********************************************************************/
std::string CCodeConvert::DecodeBase64(const std::string& szBase64)
{
	std::string decoded;

	if (!szBase64.empty())
	{
		uint32_t strLength = (uint32_t)(szBase64.length() / 4 + 1) * 3;
		unsigned char *pResult = new unsigned char[strLength + 1]{ '\0' };

		if (nullptr != pResult)
		{
			uint32_t retLen = DecodeBase64(szBase64, pResult, strLength);

			if (retLen > 0)
			{
				decoded = std::string((const char*)pResult, retLen);
			}

			delete[] pResult;
		}
	}

	return std::move(decoded);
}

/********************************************************************
功能:	解码base64
参数: 	szBase64 要转换的字符串
*		buff 转换后存放buff
*		buff_len 转换后存放buff长度
返回:	转换后真实长度
*********************************************************************/
uint32_t CCodeConvert::DecodeBase64(const std::string& szBase64, unsigned char *buff, uint32_t buff_len)
{
	if (szBase64.empty() || nullptr == buff || 0 == buff_len)
	{
		return 0;
	}

	uint32_t ullOutLen = 0;
	std::string szTempBase64(szBase64);
	TrimString(szTempBase64, 1); //去除掉结尾可能存在的空格
	const uint32_t length = (uint32_t)szTempBase64.length();
	char c, d;

	for (uint32_t i = 0; i < length; ++i)
	{
		c = TableBase64(szTempBase64[i]);
		++i;
		d = TableBase64(szTempBase64[i]);
		c = (char)((c << 2) | ((d >> 4) & 0x3));
		buff[ullOutLen++] = c;

		//防止越界
		if (ullOutLen >= buff_len)
			break;

		if (++i < length)
		{
			c = szTempBase64[i];
			if (m_cPad == c)
			{
				break;
			}

			c = TableBase64(szTempBase64[i]);
			d = (char)(((d << 4) & 0xf0) | ((c >> 2) & 0xf));
			buff[ullOutLen++] = d;

			//防止越界
			if (ullOutLen >= buff_len)
				break;
		}

		if (++i < length)
		{
			d = szTempBase64[i];
			if (m_cPad == d)
			{
				break;
			}

			d = TableBase64(szTempBase64[i]);
			c = (char)(((c << 6) & 0xc0) | d);
			buff[ullOutLen++] = c;

			//防止越界
			if (ullOutLen >= buff_len)
				break;
		}
	}

	return ullOutLen;
}

/*********************************************************************
功能：	char*转16进制串，不加0x
参数：	data char*串
*		data_len char*串长度
*		isLowercase 是否小写字母
返回：	转后的串
修改：
*********************************************************************/
std::string CCodeConvert::EncodeHex(const unsigned char *data, uint32_t data_len, bool isLowercase)
{
	std::string szReturn;

	if (nullptr == data || 0 >= data_len)
		return std::move(szReturn);

	int n = 0;
	char *pStrTemp = new char[data_len * 2 + 1]{ '\0' };

	if (nullptr != pStrTemp)
	{
		for (uint32_t i = 0; i < data_len; ++i)
		{
			n += snprintf(pStrTemp + n, 3, (isLowercase ? "%02x" : "%02X"), data[i]);
		}

		pStrTemp[n] = '\0';
		szReturn = pStrTemp;
		delete[] pStrTemp;
	}

	return std::move(szReturn);
}

/*********************************************************************
功能：	string转16进制串，不加0x
参数：	szStr string串
*		isLowercase 是否小写字母
返回：	转后的串
修改：
*********************************************************************/
std::string CCodeConvert::EncodeHex(const std::string &szStr, bool isLowercase)
{
	return EncodeHex((const unsigned char*)szStr.c_str(), (uint32_t)szStr.length(), isLowercase);
}

/*********************************************************************
功能：	16进制串转char*，不包含0x
参数：	hex 16进制串
*		hex_len 16进制串长度
*		buff 转后的buff
*		buff_len 转后存放的buff长度
返回：	转后串的实际长度
修改：
*********************************************************************/
uint32_t CCodeConvert::DecodeHex(const char *hex, uint32_t hex_len, unsigned char *buff, uint32_t buff_len)
{
	if (nullptr == hex || 1 >= hex_len || nullptr == buff || 0 == buff_len)
		return 0;

	uint32_t out_len = 0;
	unsigned char h = 0, l = 0;

	for (uint32_t i = 0; i < hex_len; i += 2)
	{
		h = GetHexCharValue(hex[i]);
		l = GetHexCharValue(hex[i + 1]);
		buff[out_len++] = (h << 4) | l;

		//防止越界
		if (out_len >= buff_len)
			break;
	}

	return out_len;
}

/*********************************************************************
功能：	16进制串转char*，不包含0x
参数：	szStr 16进制串
*		buff 转后的buff
*		buff_len 转后存放的buff长度
返回：	转后串的实际长度
修改：
*********************************************************************/
uint32_t CCodeConvert::DecodeHex(const std::string &szStr, unsigned char *buff, uint32_t buff_len)
{
	return DecodeHex(szStr.c_str(), (uint32_t)szStr.length(), buff, buff_len);
}

/********************************************************************
功能:	删除字符串两端的空格、换行及制表符
参数: 	str 要处理的字符串
*		type 处理类型，0：两边，-1：左边，1：右边
返回:	无
*********************************************************************/
void CCodeConvert::TrimString(std::string& str, short type)
{
	if (!str.empty())
	{
		//该方法在有汉字时，可能会有问题
		//去除开头的
		if (type <= 0)
		str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun(::isspace))));

		//去除结尾的
		if (type >= 0)
		str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun(::isspace))).base(), str.end());
		
		//去除中间的
		str.erase(std::remove_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), '\n')), str.end());
	}
}

/********************************************************************
功能:	获取HEX字符值
参数: 	ch HEX字符
返回:	字符值，0-15
*********************************************************************/
inline unsigned char CCodeConvert::GetHexCharValue(const char &ch)
{
	switch (ch)
	{
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'a':
	case 'A':
		return 10;
	case 'b':
	case 'B':
		return 11;
	case 'c':
	case 'C':
		return 12;
	case 'd':
	case 'D':
		return 13;
	case 'e':
	case 'E':
		return 14;
	default:
		break;
	}

	return 15;
}