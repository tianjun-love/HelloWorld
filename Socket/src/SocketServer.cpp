#include "../include/SocketServer.hpp"
#include <cstring>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

CSocketServer::CSocketServer(const std::string &szIPAddress, unsigned short nPort, int iMaxConnections) : 
	CBaseAttributes(szIPAddress, nPort), m_iMaxConnections(iMaxConnections)
{
}

CSocketServer::~CSocketServer()
{
}

/*****************************************************
功能：	初始化环境
参数：	szError 错误信息
返回：	成功返回true
*****************************************************/
bool CSocketServer::InitEnv(std::string &szError)
{
	bool bResult = true;

	if (!m_bInitWSA)
	{
#ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		{
			bResult = false;
			szError = "WSAStartup init failed !";
		}	
#endif

		if (bResult)
		{
			m_bInitWSA = true;
		}
	}

	return bResult;
}

/*****************************************************
功能：	清理环境资源
参数：	无
返回：	无
*****************************************************/
void CSocketServer::FreeEnv()
{
	if (m_bState)
	{
		Stop();
	}

#ifdef _WIN32
	if (m_bInitWSA)
	{
		WSACleanup();
		m_bInitWSA = false;
	}
#endif

	return;
}

/*****************************************************
功能：	开启socket服务
参数：	szError 错误信息
返回：	错误返回false，成功会阻塞
*****************************************************/
bool CSocketServer::Start(std::string &szError)
{
	if (m_bState)
	{
		szError = "Server is already started.";
		return false;
	}

	if (!m_bInitWSA)
	{
		szError = "InitEnv must be called first.";
		return false;
	}

	//创建句柄
	SClientObject lis;

	lis.bIsListen = true;
	lis.szIP = m_szServerIP;
	lis.nPort = m_nServerPort;

	lis.iFD = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
	if (INVALID_SOCKET == lis.iFD)
	{
		szError = "Create socket handle failed:" + GetErrorMsg(MY_SOCKET_ERRNO);
		return false;
	}
#else
	if (lis.iFD <= 0)
	{
		szError = "Create socket handle failed:" + GetErrorMsg(MY_SOCKET_ERRNO);
		return false;
	}
#endif

	//设置成非阻塞
	SetSocketBolckState(lis.iFD, false);

	//设置端口复用
	int iReuseable = 1;
	setsockopt(lis.iFD, SOL_SOCKET, SO_REUSEADDR, (const char*)(&iReuseable), (int)sizeof(iReuseable));

	//地址
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));

	sin.sin_family = AF_INET;
	if (lis.szIP.empty())
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		inet_pton(AF_INET, lis.szIP.c_str(), &sin.sin_addr);
	sin.sin_port = htons(lis.nPort);

	//绑定
	if (0 > bind(lis.iFD, (const sockaddr*)&sin, (socklen_t)sizeof(sin)))
	{
		szError = "Bind socket address failed:" + GetErrorMsg(MY_SOCKET_ERRNO);
		CloseSocket(lis.iFD);
		return false;
	}

	//监听
	if (0 > listen(lis.iFD, m_iMaxConnections))
	{
		szError = "Listen for client connections failed:" + GetErrorMsg(MY_SOCKET_ERRNO);
		CloseSocket(lis.iFD);
		return false;
	}
	else
	{
		m_bState = true;
		m_SocketHandleSet.push_back(lis);
	}

	std::list<SClientObject> tempClientList;
	fd_set rfd;
	struct timeval tv;
	MY_SOCKET_TYPE maxFD = 0;
	int ret = 0;

	tv.tv_sec = 0;
	tv.tv_usec = 200000; //200毫秒

	//循环处理
	while (m_bState)
	{
		FD_ZERO(&rfd);
		maxFD = 0;

		for (const auto &fd : m_SocketHandleSet)
		{
			FD_SET(fd.iFD, &rfd);

			if (fd.iFD > maxFD)
				maxFD = fd.iFD;
		}

#ifdef _WIN32
		ret = select(0, &rfd, NULL, NULL, &tv);
#else
		ret = select(maxFD + 1, &rfd, NULL, NULL, &tv);
#endif
		if (ret > 0)
		{
			for (std::list<SClientObject>::iterator iter = m_SocketHandleSet.begin(); iter != m_SocketHandleSet.end(); )
			{
				if (FD_ISSET(iter->iFD, &rfd))
				{
					if (iter->bIsListen) //客户端连接
					{
						SClientObject client;

						if (RecvClientConnection(*iter, client))
						{
							tempClientList.push_back(client);
						}

						++iter;
					}
					else //客户端数据读取
					{
						if (!ReadClientData(*iter))
						{
							CloseSocket(iter->iFD);
							iter = m_SocketHandleSet.erase(iter);
						}
						else
							++iter;
					}
				}
				else
					++iter;
			}

			for (auto &client : tempClientList)
			{
				m_SocketHandleSet.push_back(client);
			}

			tempClientList.clear();
		}
		else if (ret == 0)
		{
			//超时
			continue;
		}
		else
		{
			PrintfLog(0, "Server call select() failed:%s", GetErrorMsg(MY_SOCKET_ERRNO).c_str());
		}
	}

	return true;
}

/*****************************************************
功能：	停止socket服务
参数：	无
返回：	无
*****************************************************/
void CSocketServer::Stop()
{
	if (m_bState)
	{
		m_bState = false;

		for (const auto &fd : m_SocketHandleSet)
		{
			CloseSocket(fd.iFD);
		}

		m_SocketHandleSet.clear();
	}

	return;
}

/*****************************************************
功能：	处理客户端连接
参数：	lisenSocket 监听对象
*		client 客户端对象
返回：	成功返回true
*****************************************************/
bool CSocketServer::RecvClientConnection(const SClientObject &lisenSocket, SClientObject &client)
{
	sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	char clientAddrBuff[INET_ADDRSTRLEN] = { '\0' };

	memset(&clientAddr, 0, sizeof(clientAddr));
	client.iFD = accept(lisenSocket.iFD, (sockaddr*)&clientAddr, &clientAddrLen);

#ifdef _WIN32
	if (INVALID_SOCKET == client.iFD)
	{
		printf("Recv client connect failed:%s\n", GetErrorMsg(MY_SOCKET_ERRNO).c_str());
		return false;
	}
#else
	if (0 >= client.iFD)
	{
		printf("Recv client connect failed:%s\n", GetErrorMsg(MY_SOCKET_ERRNO).c_str());
		return false;
	}
#endif

	client.szIP = inet_ntop(AF_INET, &clientAddr.sin_addr, clientAddrBuff, sizeof(clientAddrBuff));
	client.nPort = clientAddr.sin_port;

	//设置不阻塞
	SetSocketBolckState(client.iFD, false);

	PrintfLog(1, "Client %s:%hu connected.", client.szIP.c_str(), client.nPort);

	return true;
}

/*****************************************************
功能：	处理客户端数据
参数：	client 客户端对象
返回：	成功返回true
*****************************************************/
bool CSocketServer::ReadClientData(const SClientObject &client)
{
	char buff[10240] = { '\0' };
	int iRet = 0;

	iRet = recv(client.iFD, buff, 10240, 0);

	if (0 == iRet)
	{
		PrintfLog(1, "Client %s:%hu normal disconnected.", client.szIP.c_str(), client.nPort);
		return false;
	}
	else if (0 > iRet)
	{
		iRet = MY_SOCKET_ERRNO;

		if (ECONNRESET == iRet 
#ifdef _WIN32
			|| WSAECONNRESET == iRet
			|| WSAECONNABORTED == iRet
#endif
			)
		{
			PrintfLog(1, "Client %s:%hu unnormal disconnected.", client.szIP.c_str(), client.nPort);
			return false;
		}
		else
		{
			if (EINTR != iRet 
				&& EAGAIN != iRet 
#ifdef _WIN32
				&& EWOULDBLOCK != iRet
#endif
				)
			{
				PrintfLog(1, "Recv from client %s:%hu msg error:%s.", client.szIP.c_str(), client.nPort,
					GetErrorMsg(iRet).c_str());
				return false;
			}
		}
	}
	else
	{
		PrintfLog(2, "Recv from client %s:%hu msg:%s.", client.szIP.c_str(), client.nPort, buff);

		std::string szEcho = std::string("Server recved:") + buff;

		iRet = send(client.iFD, szEcho.c_str(), (int)szEcho.length(), 0);

		if (iRet <= 0)
		{
			PrintfLog(1, "Send to client %s:%hu failed:%s", client.szIP.c_str(), client.nPort, GetErrorMsg(MY_SOCKET_ERRNO));
		}
	}

	return true;
}