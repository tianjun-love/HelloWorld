/********************************************************************
功能:	编码转换类
作者:	田俊
时间:	2015-03-21
修改:	2020-11-19 田俊  修改名称及增加HEX编解码
*********************************************************************/
#ifndef __CODE_CONVERT_HPP__
#define __CODE_CONVERT_HPP__

#include <string>

class CCodeConvert
{
public:
	CCodeConvert();
	~CCodeConvert();

	//Base64
	static std::string EncodeBase64(const unsigned char* data, uint32_t data_len, bool bLineFeed = false);
	static std::string EncodeBase64(const std::string& szStr, bool bLineFeed = false);
	static std::string DecodeBase64(const std::string& szBase64);
	static uint32_t DecodeBase64(const std::string& szBase64, unsigned char *buff, uint32_t buff_len);

	//Hex
	static std::string EncodeHex(const unsigned char *data, uint32_t data_len, bool isLowercase = false);
	static std::string EncodeHex(const std::string &szStr, bool isLowercase = false);
	static uint32_t DecodeHex(const char *hex, uint32_t hex_len, unsigned char *buff, uint32_t buff_len);
	static uint32_t DecodeHex(const std::string &szStr, unsigned char *buff, uint32_t buff_len);

private:
	static void TrimString(std::string& str, short type = 0);
	static inline char TableBase64(const char c); //根据字符范围获取字符
	static inline unsigned char GetHexCharValue(const char &ch); //获取HEX字符值
	
private:
	static const std::string m_szAlphabet64;     //base64编码字符范围
	static const char        m_cPad;             //base64编码填充字符
	static const char        m_cNp;              //base64解码时，表示找不到该字符，即不是合法的base64字符
	static const char        m_Table64vals[128]; //base64解码表

};

#endif