#include "../include/ListenerThread.hpp"
#include "event2/thread.h"

#ifndef _WIN32
#include <arpa/inet.h>
#endif

CListenerThread::CListenerThread(const std::string &szIP, int iPort, int iMaxConn, CLogFile &log, SynQueue<CClientObject*> &connDeque) :
	m_bRunFlag(false), m_szServerIP(szIP), m_iServerPort(iPort), m_iMaxConnect(iMaxConn), m_LogFile(log), m_ConnectDeque(connDeque),
	m_pThread(nullptr), m_iListenFd(-1), m_pEventBase(nullptr), m_pEventListener(nullptr)
{
}

CListenerThread::~CListenerThread()
{
	Stop();
}

/*********************************************************************
功能：	启动服务
参数：	szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CListenerThread::Start(std::string &szError)
{
	//设置线程安全
	int iRet = 0;
#ifdef _WIN32
	iRet = evthread_use_windows_threads();
#else
	iRet = evthread_use_pthreads();
#endif

	if (0 != iRet)
	{
		szError = "Set libevent thread safe failed !";
		return false;
	}

	//创建event_base
	m_pEventBase = event_base_new();
	if (nullptr == m_pEventBase)
	{
		szError = "Create event_base fd failed !";
		return false;
	}

	//设置绑定地址
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	if (m_szServerIP.empty())
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		evutil_inet_pton(AF_INET, m_szServerIP.c_str(), &sin.sin_addr);
	sin.sin_port = htons(m_iServerPort);

	//监听
	m_pEventListener = evconnlistener_new_bind(m_pEventBase, listener_cb, (void *)this, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE,
		m_iMaxConnect, (struct sockaddr*)&sin, sizeof(sin));

	if (nullptr == m_pEventListener)
	{
		szError = "Create evconnlistener fd failed !";
		return false;
	}

	//错误回调
	evconnlistener_set_error_cb(m_pEventListener, error_cb);

	//开启线程
	try
	{
		m_pThread = new std::thread(std::bind(&CListenerThread::HandleThread, this));

		if (nullptr != m_pThread)
			m_bRunFlag = true;
		else
		{
			szError = "New socket listener thread return nullptr !";
			return false;
		}
	}
	catch (const std::exception& ex)
	{
		szError = std::string("Open socket listener thread failed:") + ex.what();
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
void CListenerThread::Stop()
{
	if (m_bRunFlag)
	{
		m_bRunFlag = false;

		//停止监听
		if (nullptr != m_pEventListener)
			evconnlistener_disable(m_pEventListener);

		//停止循环
		if (nullptr != m_pEventBase)
		{
			timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 50;

			event_base_loopexit(m_pEventBase, &tv);

			m_LogFile << E_LOG_LEVEL_ALL << "Server socket listen thread stop ! " << logendl;
		}
	}

	if (nullptr != m_pThread)
	{
		m_pThread->join();
		delete m_pThread;
		m_pThread = nullptr;
	}

	if (nullptr != m_pEventListener)
	{
		evconnlistener_free(m_pEventListener);
		m_pEventListener = nullptr;
	}

	if (nullptr != m_pEventBase)
	{
		event_base_free(m_pEventBase);
		m_pEventBase = nullptr;
	}
}

/*********************************************************************
功能：	处理线程
参数：	无
返回：	无
修改：
*********************************************************************/
void CListenerThread::HandleThread()
{
	m_LogFile << E_LOG_LEVEL_ALL << "Server socket listen thread start on [" << m_szServerIP << ":"
		<< m_iServerPort << "] ..." << logendl;

	event_base_dispatch(m_pEventBase);
}

/*********************************************************************
功能：	监听回调方法
参数：	listener 监听对象
*		fd 新连接句柄
*		sa 连接地址
*		socklen sa长度
*		user_data 用户数据
返回：	无
修改：
*********************************************************************/
void CListenerThread::listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int sa_len, void *user_data)
{
	CListenerThread* pServer = (CListenerThread*)user_data;

	if (nullptr == pServer)
		return;

	std::string szClientIP = inet_ntoa(((struct sockaddr_in*)sa)->sin_addr);
	USHORT nClientPort = ((struct sockaddr_in*)sa)->sin_port;
	CClientObject *client = new CClientObject();

	if (nullptr == client)
	{
		pServer->m_LogFile << E_LOG_LEVEL_SERIOUS << "Client [" + szClientIP << ":" << nClientPort << "] connect, but new client object return nullptr !"
			<< logendl;
		return;
	}
	else
		pServer->m_LogFile << E_LOG_LEVEL_PROMPT << "Client [" + szClientIP << ":" << nClientPort << "] connected !" << logendl;

	client->iClientFd = fd;
	client->Session.szClientIP = szClientIP;
	client->Session.uiClientPort = nClientPort;
	client->Session.szClientInfo = szClientIP + ":" + std::to_string(nClientPort);

	//插入队列
	pServer->m_ConnectDeque.Put(client);
}

/*********************************************************************
功能：	错误回调方法
参数：	listener 监听对象
*		user_data 用户数据
返回：	无
修改：
*********************************************************************/
void CListenerThread::error_cb(struct evconnlistener *listener, void *user_data)
{
	CListenerThread* pServer = (CListenerThread*)user_data;

	if (nullptr != pServer)
	{
		pServer->m_LogFile << E_LOG_LEVEL_SERIOUS << "Server socket listen thread occured error !"
			<< logendl;
	}
}