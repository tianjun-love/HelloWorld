/*************************************************
功能:	socket原生服务类
作者:	田俊
时间:	2022-03-15
修改:
*************************************************/
#ifndef __SOCKET_SERVER_HPP__
#define __SOCKET_SERVER_HPP__

#include "BaseAttributes.hpp"
#include <list>

class CSocketServer : public CBaseAttributes
{
public:
	CSocketServer(const std::string &szIPAddress, unsigned short nPort, int iMaxConnections);
	CSocketServer(const CSocketServer &Other) = delete;
	virtual ~CSocketServer();

	bool InitEnv(std::string &szError);
	void FreeEnv();
	bool Start(std::string &szError);
	void Stop();

protected:
	virtual bool ReadClientData(const SClientObject &client);

private:
	bool RecvClientConnection(const SClientObject &lisenSocket, SClientObject &client);

private:
	int                      m_iMaxConnections; //最大连接数
	std::list<SClientObject> m_SocketHandleSet; //socket句柄列表

};

#endif