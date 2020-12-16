/******************************************************
功能：	socket基类
作者：	田俊
时间：	2019-03-25
修改：
******************************************************/
#ifndef __SOCKET_BASE_HPP__
#define __SOCKET_BASE_HPP__

#include <string>
#include <cstring>
#include <ctime>
#include <cstdlib>

class CSocketBase
{
public:
	CSocketBase(const std::string &szIP, int iPort, int iTimeout);
	CSocketBase(const CSocketBase &Other);
	virtual ~CSocketBase();
	CSocketBase& operator=(const CSocketBase &Other);
	
	static unsigned int GetRandomNumber(unsigned int min = 0, unsigned int max = 4199999999);
	static bool GetRandomBytes(void *buf, unsigned int buf_len);
	static std::string GetErrorMsg(int sysErrCode);
	static std::string GetStrerror(int sysErrCode);
	static std::string DateTimeString(short nType = 0);
	static bool CheckIPFormat(const std::string &szIP, bool bIsIPv4 = true);
	static std::string GetHexString(const unsigned char *data, unsigned int data_len, bool bLowercase = false);
	static std::string GetHexString(const char *data, unsigned int data_len, bool bLowercase = false);
	static unsigned int FromHexString(const std::string& szHexString, char* data, unsigned int data_len);

protected:
	static char GetHexChar(const char &ch, bool isLowercase = false); //获取16进制的字符
	static char FromHexChar(const char &ch); //16进制的字符转数

protected:
	bool        m_bStatus;      //服务运行或连接状态
	bool        m_bInitWSA;     //是否已经初始化WSA，win32使用
	std::string m_szServerIP;   //服务IP
	int         m_iServerPort;  //服务端口
	int         m_iTimeOut;     //接收及发送数据超时时间，秒

};

#endif