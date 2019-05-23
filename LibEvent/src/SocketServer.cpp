#include "../include/SocketServer.hpp"
#include <event2/thread.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

static CLogFile* g_pLogPrint = nullptr; //日志输出，类外的函数使用
const std::string g_szLibeventLog = "LibeventError.log";

CSocketServer::CSocketServer(CLogFile& log, const std::string& szIP, int iPort, int iMaxConnect, int iReadTimeOut) : 
CSocketBase(szIP, iPort, iReadTimeOut), m_LogPrint(log), m_iMaxConnect(iMaxConnect), m_iCurrConnect(0), m_iListenFd(-1),
m_pEventBase(nullptr), m_pEventListener(nullptr)
{
}

CSocketServer::~CSocketServer()
{
}

/*********************************************************************
功能：	初始化socket服务
参数：	szError 错误信息，输出
返回：	成功返回true
修改：
*********************************************************************/
bool CSocketServer::Init(std::string& szError)
{
	//检查IP格式
	if (!CheckIPFormat(m_szServerIP))
	{
		szError = "Init check server IP format wrong !";
		return false;
	}

	//检查端口
	if (m_iServerPort <= 0 || m_iServerPort >= 65536)
	{
		szError = "Init check server port wrong !";
		return false;
	}

	
#ifdef _WIN32
	if (!m_bInitWSA)
	{
		WSADATA wsa_data;
		if (0 != WSAStartup(MAKEWORD(2, 2), &wsa_data))
		{
			szError = "Init WSAStartup failed !";
			return false;
		}
		else
			m_bInitWSA = true;
	}
#endif

	//设置类外日志输出
	if (nullptr == g_pLogPrint)
	{
		g_pLogPrint = &m_LogPrint;
	}

	//设置错误日志打印回调函数
	event_set_log_callback(log_cb);

	//设置致命错误回调函数
	event_set_fatal_callback(fatal_cb);

	//设置使用线程
	int iRet = 0;
#ifdef _WIN32
	iRet = evthread_use_windows_threads();
#else
	iRet = evthread_use_pthreads();
#endif

	if (0 != iRet)
	{
		szError = "Use libevent thread failed !";
		return false;
	}

	//创建socket句柄
	m_iListenFd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (m_iListenFd <= 0)
	{
		szError = "Create socket fd failed !";
		return false;
	}

	evutil_make_listen_socket_reuseable(m_iListenFd);
	evutil_make_socket_nonblocking(m_iListenFd);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	if (m_szServerIP.empty())
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		evutil_inet_pton(AF_INET, m_szServerIP.c_str(), &sin.sin_addr);
	sin.sin_port = htons(m_iServerPort);

	//绑定地址
	if (bind(m_iListenFd, (const struct sockaddr*)&sin, sizeof(sin)) < 0)
	{
		szError = "Bind socket address failed !";
		return false;
	}

	//监听
	if (listen(m_iListenFd, m_iMaxConnect) < 0)
	{
		szError = "Socket server listen failed !";
		return false;
	}

	//创建event_base
	m_pEventBase = event_base_new();
	if (nullptr == m_pEventBase)
	{
		szError = "Create event_base fd failed !";
		return false;
	}
	
	//创建监听事件
	m_pEventListener = event_new(m_pEventBase, m_iListenFd, EV_READ | EV_PERSIST, do_accept, this);
	if (nullptr == m_pEventListener)
	{
		szError = "Create listen event failed !";
		return false;
	}

	//添加监听事件
	if (0 > event_add(m_pEventListener, nullptr))
	{
		szError = "Add event to event_base failed !";
		event_free(m_pEventListener);
		return false;
	}

	return true;
}

/*********************************************************************
功能：	启动服务
参数：	szError 错误信息
返回：	启动失败返回false，成功会进入循环，不会返回
修改：
*********************************************************************/
bool CSocketServer::Start(std::string& szError)
{
	if (!m_bStatus)
	{
		m_bStatus = true;
		
		if (nullptr != m_pEventBase)
		{
			m_LogPrint << E_LOG_LEVEL_ALL << "Server socket start on [" << m_szServerIP << ":" 
				<< m_iServerPort << "] ..." << logendl;

			//启动事件处理循环
			if (0 > event_base_dispatch(m_pEventBase))
			{
				m_bStatus = false;
				szError = "Socket server start failed !";
				return false;
			}
		}
		else
		{
			szError = "Socket server not be inited !";
			return false;
		}
	}
	else
	{
		szError = "Socket server is start now.";
		return false;
	}

	return true;
}

/*********************************************************************
功能：	停止服务
参数：	无
返回：	无
修改：
*********************************************************************/
void CSocketServer::Stop()
{
	if (m_bStatus)
	{
		timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;

		m_bStatus = false;

		//停止事件处理循环
		event_base_loopexit(m_pEventBase, &tv);
	}
}

/*********************************************************************
功能：	释放申请的资源
参数：	无
返回：	无
修改：
*********************************************************************/
void CSocketServer::Free()
{
	//关闭监听的socket句柄
	if (m_iListenFd > 0)
	{
		evutil_closesocket(m_iListenFd);
		m_iListenFd = -1;
	}

	//关闭监听事件
	if (m_pEventListener != nullptr)
	{
		event_free(m_pEventListener);
		m_pEventListener = nullptr;
	}

	//关闭event_base
	if (m_pEventBase != nullptr)
	{
		event_base_free(m_pEventBase);
		m_pEventBase = nullptr;
	}

#ifdef _WIN32
	if (m_bInitWSA)
	{
		WSACleanup();
		m_bInitWSA = false;
	}
#endif

	//释放libevent
	libevent_global_shutdown();
}

/*********************************************************************
功能：	获取socket信息
参数：	fd socket句柄
*		pIP IP地址，输出，不需要填NULL
*		pPort 端口，输出，不需要填NULL
返回：	IP地址:端口字符串
修改：
*********************************************************************/
std::string CSocketServer::GetSocketInfo(evutil_socket_t fd, std::string *pIP, int *pPort)
{
	sockaddr_in sin;
	socklen_t len = sizeof(sin);
	char buf[64]{ '\0' };
	std::string szReturn;

	getpeername(fd, (struct sockaddr *)&sin, &len);
	evutil_inet_ntop(AF_INET, &sin.sin_addr, buf, sizeof(buf));
	szReturn = buf;

	if (nullptr != pIP)
		*pIP = szReturn;

	if (nullptr != pPort)
		*pPort = sin.sin_port;

	szReturn.append(std::string(":") + std::to_string(sin.sin_port));

	return std::move(szReturn);
}

/*********************************************************************
功能：	客户端数据处理
参数：	szClientStr 客户端信息，输入
*		bev bufferevent，输入
*		clientInfo 客户端信息，输入
*		clientData 客户端数据，输出
返回：	<0：失败，0：成功但数据不全，1：成功处理
修改：
*********************************************************************/
short CSocketServer::ClientDataHandle(bufferevent *bev, SClientInfo &clientInfo, SClientData &clientData)
{
	size_t iMaxReadLength = bufferevent_get_max_single_read(bev);

	if (nullptr == clientData.pMsgBuffer)
	{
		clientData.pMsgBuffer = new unsigned char[iMaxReadLength + 1]{ '\0' };
	}

	//读取数据
	clientData.uiMsgTotalLength = (unsigned int)bufferevent_read(bev, clientData.pMsgBuffer, iMaxReadLength);

	//输出
	if (0 == clientData.uiMsgTotalLength)
	{
		m_LogPrint << E_LOG_LEVEL_PROMPT << "Recv from [" << clientInfo.szClientInfo << "] data_len:0 !"
			<< logendl;
		return 0;
	}
	else
	{
		m_LogPrint << E_LOG_LEVEL_PROMPT << "Recv from [" << clientInfo.szClientInfo << "] data_len:"
			<< clientData.uiMsgTotalLength << " data:\n0x" << GetHexString(clientData.pMsgBuffer, clientData.uiMsgTotalLength)
			<< logendl;

		//返回相同数据
		if (0 == bufferevent_write(bev, clientData.pMsgBuffer, clientData.uiMsgTotalLength))
		{
			m_LogPrint << E_LOG_LEVEL_PROMPT << "Send to [" << clientInfo.szClientInfo << "] same data success !"
				<< logendl;
			return 1;
		}
		else
		{
			m_LogPrint << E_LOG_LEVEL_PROMPT << "Send to [" << clientInfo.szClientInfo << "] same data failed:"
				<< evutil_socket_error_to_string(evutil_socket_geterror(bufferevent_getfd(bev))) << logendl;
			return -1;
		}
	}
}

/*********************************************************************
功能：	检查客户端是否允许长连接
参数：	szClientIP 客户端IP
返回：	允许长连接返回true
修改：
*********************************************************************/
bool CSocketServer::CheckIsPersistentConnection(const std::string &szClientIP)
{
	return false;
}

/*********************************************************************
功能：	接收客户端连接处理
参数：	listen_fd 监听的socket句柄
*		evFlags 事件标志位
*		arg 参数
返回：	无
修改：
*********************************************************************/
void CSocketServer::do_accept(evutil_socket_t listen_fd, short evFlags, void* arg)
{
	CSocketServer* classBase = (CSocketServer*)arg;
	struct sockaddr_in sin;
	socklen_t sin_len = sizeof(sin);

	//接收连接
	evutil_socket_t client_fd = ::accept(listen_fd, (struct sockaddr*)&sin, &sin_len);
	if (client_fd <= 0)
	{
		classBase->m_LogPrint << E_LOG_LEVEL_SERIOUS << "Socket server accept connect failed, return value <= 0 !"
			<< logendl;
	}
	else
	{
		char buf[64]{ '\0' };
		SClientObject* pClient = new SClientObject();

		evutil_inet_ntop(AF_INET, &sin.sin_addr, buf, sizeof(buf));
		pClient->ClientInfo.szClientIP = buf;
		pClient->ClientInfo.szClientInfo = pClient->ClientInfo.szClientIP + std::string(":") + std::to_string(sin.sin_port);
		pClient->pServerObject = classBase;

		classBase->m_LogPrint << E_LOG_LEVEL_PROMPT << "Client [" << pClient->ClientInfo.szClientInfo << "] connected !"
			<< logendl;

		//设置不阻塞
		evutil_make_socket_nonblocking(client_fd);

		//创建事件
		struct bufferevent *bev = bufferevent_socket_new(classBase->m_pEventBase, client_fd, 
			BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
		if (nullptr != bev)
		{
			//读写超时
			if (classBase->m_iTimeOut > 0 && !classBase->CheckIsPersistentConnection(pClient->ClientInfo.szClientIP))
			{
				timeval tvRead, tvWrite;

				tvRead.tv_sec = classBase->m_iTimeOut;
				tvRead.tv_usec = 0;

				tvWrite.tv_sec = 3; //默认
				tvWrite.tv_usec = 0;

				bufferevent_set_timeouts(bev, &tvRead, nullptr);
			}

			//设置回调
			bufferevent_setcb(bev, read_cb, nullptr, error_cb, (void*)pClient);
			bufferevent_enable(bev, EV_READ | EV_PERSIST);

			classBase->m_LogPrint << E_LOG_LEVEL_SERIOUS << "Socket server current connected client count:" 
				<< ++(classBase->m_iCurrConnect) << logendl;
		}
		else
		{
			classBase->m_LogPrint << E_LOG_LEVEL_SERIOUS << "Client [" << pClient->ClientInfo.szClientInfo
				<< "] create new socket event failed !" << logendl;
			
			delete pClient;
			pClient = nullptr;
		}
	}
}

/*********************************************************************
功能：	读事件处理
参数：	bev bufferevent
*		arg 参数
返回：	无
修改：
*********************************************************************/
void CSocketServer::read_cb(struct bufferevent *bev, void* arg)
{
	SClientObject* client = (SClientObject*)arg;
	int iRet = 1;

	//循环读取，尽量多读，防止多个包一起发时，只会收到前面一个
	do
	{
		//处理客户端数据
		iRet = client->pServerObject->ClientDataHandle(bev, client->ClientInfo, client->ClientData);

		if (0 == iRet) //没读到，退出，等待下一次的数据到来
			break;
		else if (1 == iRet) //处理成功，清理数据
			client->ClientData.Clear();
		else //错误，断开客户端的连接，防止接收脏数据
		{
			client->pServerObject->m_LogPrint << E_LOG_LEVEL_WARN << "Recv from [" << client->ClientInfo.szClientInfo
				<< "] msg read and handle failed, server active disconnected !" << logendl;

			client->pServerObject->m_LogPrint << E_LOG_LEVEL_SERIOUS << "Socket server current connected client count:"
				<< --(client->pServerObject->m_iCurrConnect) << logendl;

			client->Clear();
			delete client;
			client = nullptr;
			bufferevent_free(bev);
			
			break;
		}
	} while (1 == iRet);
}

/*********************************************************************
功能：	写事件处理
参数：	bev bufferevent
*		arg 参数
返回：	无
修改：
*********************************************************************/
void CSocketServer::write_cb(struct bufferevent *bev, void* arg)
{
	SClientObject* client = (SClientObject*)arg;

	client->pServerObject->m_LogPrint << E_LOG_LEVEL_PROMPT << "Client [" << client->ClientInfo.szClientInfo
		<< "] msg write completed." << logendl;
}

/*********************************************************************
功能：	错误事件处理
参数：	bev bufferevent
*		evFlags 事件标志位
*		arg 参数
返回：	无
修改：
*********************************************************************/
void CSocketServer::error_cb(struct bufferevent *bev, short evFlags, void* arg)
{
	SClientObject* client = (SClientObject*)arg;

	//错误处理
	if (evFlags & BEV_EVENT_EOF)
	{
		client->pServerObject->m_LogPrint << E_LOG_LEVEL_WARN << "Client [" << client->ClientInfo.szClientInfo
			<< "] normal disconnected." << logendl;
	}
	else if (evFlags & BEV_EVENT_ERROR)
	{
		client->pServerObject->m_LogPrint << E_LOG_LEVEL_WARN << "Client [" << client->ClientInfo.szClientInfo
			<< "] unnormal disconnected, error:" << evutil_socket_error_to_string(evutil_socket_geterror(bufferevent_getfd(bev)))
			<< logendl;
	}
	else if (evFlags & BEV_EVENT_TIMEOUT)
	{
		client->pServerObject->m_LogPrint << E_LOG_LEVEL_WARN << "Client [" << client->ClientInfo.szClientInfo
			<< "] timeout disconnected." << logendl;
	}
	else
	{
		client->pServerObject->m_LogPrint << E_LOG_LEVEL_WARN << "Client [" << client->ClientInfo.szClientInfo
			<< "] abnormal disconnected, evFlags:" << evFlags << logendl;
	}

	//输出未处理的信息
	if (client->ClientData.uiMsgTotalLength > 0 && nullptr != client->ClientData.pMsgBuffer)
	{
		client->pServerObject->m_LogPrint << E_LOG_LEVEL_WARN << "Client [" << client->ClientInfo.szClientInfo
			<< "] has untreated data:\n0x" << GetHexString(client->ClientData.pMsgBuffer, client->ClientData.uiMsgTotalLength)
			<< logendl;
	}

	client->pServerObject->m_LogPrint << E_LOG_LEVEL_SERIOUS << "Socket server current connected client count:"
		<< --(client->pServerObject->m_iCurrConnect) << logendl;

	//释放资源
	client->Clear();
	delete client;
	client = nullptr;
	bufferevent_free(bev);
}

/*********************************************************************
功能：	错误日志处理
参数：	severity 日志等级
*		msg 日志消息
返回：	无
修改：
*********************************************************************/
void CSocketServer::log_cb(int severity, const char* msg)
{
	//日志输出处理
	if (nullptr != g_pLogPrint)
	{
		(*g_pLogPrint) << E_LOG_LEVEL_SERIOUS << "Libevent inner error, level:" << severity
			<< " msg:" << msg << logendl;
	}
	else
	{
		char buf[1001] = { '\0' };
		snprintf(buf, 1000, "echo \"%s >> [SERIOUS]: Libevent inner error, level:%d msg:%s\" >> %s", 
			DateTimeString().c_str(), severity, msg, g_szLibeventLog.c_str());
		system(buf);
	}
}

/*********************************************************************
功能：	致命错误处理，默认是exit(1)
参数：	err 错误号
返回：	无
修改：
*********************************************************************/
void CSocketServer::fatal_cb(int err)
{
	//致命错误处理
	if (nullptr != g_pLogPrint)
	{
		(*g_pLogPrint) << E_LOG_LEVEL_SERIOUS << "Libevent fatal error, active call exit(), errno:"
			<< err << logendl;
	}
	else
	{
		char buf[1001] = { '\0' };
		snprintf(buf, 1000, "echo \"%s >> [SERIOUS]: Libevent fatal error, active call exit(), errno:%d\" >> %s",
			DateTimeString().c_str(), err, g_szLibeventLog.c_str());
		system(buf);
	}

	exit(1);
}