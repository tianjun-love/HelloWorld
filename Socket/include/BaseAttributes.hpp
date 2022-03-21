/*************************************************
功能:	socket基础属性类
作者:	田俊
时间:	2022-03-16
修改:
*************************************************/
#ifndef __BASE_ATTRIBUTES_HPP__
#define __BASE_ATTRIBUTES_HPP__

#include "ClientObject.hpp"

#ifdef _WIN32
#define MY_SOCKET_ERRNO WSAGetLastError()
#else
#define MY_SOCKET_ERRNO errno
#endif

//时间字符串类型
enum ETimeStringType
{
	E_DATE_TIME = 0,          //日期时间串
	E_DATE,                   //日期串
	E_TIME,                   //时间串
	E_TIME_MILLISECOND,       //带毫秒时间串
	E_DATE_TIME_MILLISECOND   //带毫秒日期时间串
};

class CBaseAttributes
{
public:
	CBaseAttributes(const std::string &szServerIP, unsigned short nServerPort);
	virtual ~CBaseAttributes();

	//简单输出日志
	virtual void PrintfLog(int level, const char *fmt, ...);

	static void SetSocketBolckState(MY_SOCKET_TYPE socket, bool block);
	static void CloseSocket(MY_SOCKET_TYPE socket);
	static std::string GetTimeString(ETimeStringType eType = E_DATE_TIME);
	static std::string GetErrorMsg(int errCode);

protected:
	std::string    m_szServerIP;     //服务器IP
	unsigned short m_nServerPort;    //服务器端口

	bool           m_bInitWSA;       //是否初始化WSA，windows生效
	bool           m_bState;         //服务状态

};

#endif