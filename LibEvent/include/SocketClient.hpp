/***********************************************************
功能：socket客户端
作者：田俊
时间：2019-02-21
修改：
***********************************************************/
#ifndef __SOCKET_CLIENT_HPP__
#define __SOCKET_CLIENT_HPP__

#include "SocketBase.hpp"

class CSocketClient : public CSocketBase
{
public:
	CSocketClient(const std::string& szServerIP, int iServerPort, int iTimeOut = 15);
	CSocketClient(const CSocketClient& Other) = delete;
	virtual ~CSocketClient();

	CSocketClient& operator = (const CSocketClient& Other) = delete;

	bool Init(std::string &szError); //初始化
	void Free(); //释放
	bool Connect(std::string& szError); //连接
	bool Reconnect(std::string& szError); //重新连接
	void DisConnect(); //断开连接

	int SendMsg(const char* data, int data_len, std::string& szError);//发送
	int RecvMsg(char* data, int buf_len, std::string& szError); //接收

	bool SendMsgWithEOF_Zero(const std::string &szData, std::string& szError); //发送带'\0'为结束符的消息
	bool RecvMsgWithEOF_Zero(std::string &szData, std::string& szError); //接收带'\0'为结束符的消息

private:
	long        m_lConnectId;   //socket句柄
	

};

#endif