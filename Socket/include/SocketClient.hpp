/*************************************************
功能:	socket客户端对象
作者:	田俊
时间:	2022-03-16
修改:
*************************************************/
#ifndef __SOCKET_CLIENT_HPP__
#define __SOCKET_CLIENT_HPP__

#include "BaseAttributes.hpp"

class CSocketClient : public CBaseAttributes
{
public:
	CSocketClient(const std::string &szIPAddress, unsigned short nPort, int iTimeout);
	CSocketClient(const CSocketClient &Other) = delete;
	virtual ~CSocketClient();

	bool InitEnv(std::string &szError); //初始化
	void FreeEnv(); //释放
	bool Connect(std::string& szError); //连接
	bool Reconnect(std::string& szError); //重新连接
	void DisConnect(); //断开连接

	bool SendMsg(const std::string &szMsg, std::string& szError);//发送
	bool RecvMsg(std::string &szMsg, std::string& szError); //接收

private:
	int            m_iTimeout;   //超时

	MY_SOCKET_TYPE m_iSocketFD;

};

#endif