/******************************************************
功能：	数字编码，适应大小端
作者：	田俊
时间：	2019-03-15
修改：
******************************************************/
#ifndef __DIGIT_CODING_HPP__
#define __DIGIT_CODING_HPP__

#include <cstring>
#include <string>

#define my_int8_len  (1U) //1个字节
#define my_int16_len (2U) //2个字节
#define my_int32_len (4U) //4个字节
#define my_int64_len (8U) //8个字节

#define my_int8_   char
#define my_uint8_  unsigned char
#define my_int16_  short
#define my_uint16_ unsigned short
#define my_int32_  int
#define my_uint32_ unsigned int
#define my_int64_  long long
#define my_uint64_ unsigned long long

class CDigitCoding
{
public:
	//定义多字节类型的union，转大小端使用
	union INT16_UNION
	{
		my_int16_ i16;
		my_uint8_ b[my_int16_len];
	};

	union UINT16_UNION
	{
		my_uint16_ u16;
		my_uint8_  b[my_int16_len];
	};

	union INT32_UNION
	{
		my_int32_ i32;
		my_uint8_ b[my_int32_len];
	};

	union UINT32_UNION
	{
		my_uint32_ u32;
		my_uint8_ b[my_int32_len];
	};

	union INT64_UNION
	{
		my_int64_ i64;
		my_uint8_ b[my_int64_len];
	};

	union UINT64_UNION
	{
		my_uint64_ u64;
		my_uint8_  b[my_int64_len];
	};

public:
	CDigitCoding();
	~CDigitCoding();

	static void SetEndianType(bool bEndian); //设置大小端类型
	static bool GetEndianType(); //获取大小端类型
	static bool CheckCpuEndian(); //检查本机大小端类型

	static char* put_int8(char *buf, my_int8_ data);
	static const char* get_int8(const char *buf, my_int8_ *data);

	static char* put_uint8(char *buf, my_uint8_ data);
	static const char* get_uint8(const char *buf, my_uint8_ *data);

	static char* put_int16(char *buf, my_int16_ data);
	static const char* get_int16(const char *buf, my_int16_ *data);

	static char* put_uint16(char *buf, my_uint16_ data);
	static const char* get_uint16(const char *buf, my_uint16_ *data);

	static char* put_int32(char *buf, my_int32_ data);
	static const char* get_int32(const char *buf, my_int32_ *data);

	static char* put_uint32(char *buf, my_uint32_ data);
	static const char* get_uint32(const char *buf, my_uint32_ *data);

	static char* put_int64(char *buf, my_int64_ data);
	static const char* get_int64(const char *buf, my_int64_ *data);

	static char* put_uint64(char *buf, my_uint64_ data);
	static const char* get_uint64(const char *buf, my_uint64_ *data);

	static char* put_str(char *buf, const char *in, unsigned int in_len);
	static char* put_str(char *buf, const unsigned char *in, unsigned int in_len);
	static const char* get_str(const char *buf, char *out, unsigned int out_len);
	static const char* get_str(const char *buf, unsigned char *out, unsigned int out_len);

	static unsigned int from_hex(const char *hex, unsigned int hex_len, char *data, unsigned int data_len); //16进制串转char*，不包含0x
	static std::string get_hex(const char *data, unsigned int data_len, bool isLowercase = false); //char*转16进制串，不加0x
	static std::string get_hex(const unsigned char *data, unsigned int data_len, bool isLowercase = false); //char*转16进制串，不加0x
	static std::string get_hex(my_int16_ nData, bool isLowercase = false); //short转16进制，不加0x
	static std::string get_hex(my_int32_ iData, bool isLowercase = false); //int转16进制，不加0x
	static std::string get_hex(my_int64_ llData, bool isLowercase = false); //long long转16进制，不加0x
	static std::string get_hex(my_uint16_ unData, bool isLowercase = false); //short转16进制，不加0x
	static std::string get_hex(my_uint32_ uiData, bool isLowercase = false); //int转16进制，不加0x
	static std::string get_hex(my_uint64_ ullData, bool isLowercase = false); //long long转16进制，不加0x

	static char from_hex(const std::string sz2BytesHex); //两字节的16进制串转数字,不包含0x
	static std::string get_hex(const unsigned char data); //数字转两字节16进制串，不加0x

private:
	static char GetHexChar(const char &ch, bool isLowercase = false); //获取16进制的字符
	static char FromHexChar(const char &ch); //16进制的字符转数

private:
	static bool m_bIsBigEndian; //是否大端，默认大端
	static bool m_bCPUEndian;   //CPU的大小端

};

#endif