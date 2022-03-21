/*************************************************
功能:	socket客户端对象
作者:	田俊
时间:	2021-02-22
修改:
*************************************************/
#ifndef __CLIENT_OBJECT_HPP__
#define __CLIENT_OBJECT_HPP__

#include <string>

#ifdef _WIN32
#define MY_SOCKET_TYPE unsigned long long
#else
#define MY_SOCKET_TYPE int
#endif

//客户端信息对象
struct SClientObject
{
	std::string    szIP;
	unsigned short nPort;
	MY_SOCKET_TYPE iFD;
	bool           bIsListen;

	SClientObject() : nPort(0), iFD(0), bIsListen(false) {};
};

#endif