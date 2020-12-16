#include "../include/ClientHandleThread.hpp"
#include "../include/SocketServer.hpp"
#include "event2/thread.h"

#ifndef _WIN32
#include <arpa/inet.h>
#endif

CClientHandleThread::CClientHandleThread(ev_uint64_t ullNO, CLogFile &log, bool bKeepAlive) : m_bRunFlag(false), m_ullThreadNO(ullNO), 
m_LogFile(log), m_bKeepAlive(bKeepAlive), m_pThread(nullptr), m_pEvent(nullptr), m_pEventBase(nullptr)
{
	timerclear(&m_BeginTime);
}

CClientHandleThread::~CClientHandleThread()
{
	Stop();
}

/*********************************************************************
功能：	启动服务
参数：	szError 错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool CClientHandleThread::Start(std::string &szError)
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

	//添加一个定时器，防止没有客户端连接时退出循环
	m_pEvent = event_new(m_pEventBase, -1, EV_PERSIST, timeout_cb, (void*)this);
	if (nullptr == m_pEvent)
	{
		szError = "Create timer event fd failed, return nullptr !";
		return false;
	}

	struct timeval tv;
	tv.tv_sec = 60;
	tv.tv_usec = 0;

	//添加定时事件
	if (0 > event_add(m_pEvent, &tv))
	{
		szError = "Add event to event_base failed !";
		return false;
	}

	try
	{
		m_pThread = new std::thread(std::bind(&CClientHandleThread::HandleThread, this));

		if (nullptr != m_pThread)
			m_bRunFlag = true;
		else
		{
			szError = "New client handle thread return nullptr !";
			return false;
		}
	}
	catch (const std::exception& ex)
	{
		szError = std::string("Open client handle thread failed:") + ex.what();
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
void CClientHandleThread::Stop()
{
	if (m_bRunFlag)
	{
		//修改运行标志
		m_bRunFlag = false;
	}

	//停止循环
	if (nullptr != m_pEventBase)
	{
		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 50;

		event_base_loopexit(m_pEventBase, &tv);

		m_LogFile << E_LOG_LEVEL_ALL << "Client handle thread [" << m_ullThreadNO << "] stop dispatch !"
			<< logendl;
	}

	if (nullptr != m_pThread)
	{
		m_pThread->join();
		delete m_pThread;
		m_pThread = nullptr;
	}

	if (nullptr != m_pEvent)
	{
		event_free(m_pEvent);
		m_pEvent = nullptr;
	}

	if (nullptr != m_pEventBase)
	{
		event_base_free(m_pEventBase);
		m_pEventBase = nullptr;
	}
}

/*********************************************************************
功能：	添加客户端
参数：	client 客户端
返回：	无
修改：
*********************************************************************/
void CClientHandleThread::AddClient(CClientObject *client)
{
	if (nullptr == client)
		return;

	CSocketServer *pServer = (CSocketServer*)(client->pHandleObject);

	//设置不阻塞
	evutil_make_socket_nonblocking(client->iClientFd);

	//创建事件
	struct bufferevent *bev = bufferevent_socket_new(m_pEventBase, client->iClientFd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);

	if (nullptr != bev)
	{
		//读写超时
		if (pServer->m_iTimeOut > 0 && !pServer->CheckIsPersistentConnection(client->Session.szClientIP))
		{
			timeval tvRead, tvWrite;

			tvRead.tv_sec = pServer->m_iTimeOut;
			tvRead.tv_usec = 0;

			tvWrite.tv_sec = 3; //默认
			tvWrite.tv_usec = 0;

			bufferevent_set_timeouts(bev, &tvRead, nullptr);
		}

		//设置回调
		client->pOther = (void*)this;
		client->Session.ullThreadNO = m_ullThreadNO;
		bufferevent_setcb(bev, read_cb, nullptr, error_cb, (void*)client);
		bufferevent_enable(bev, EV_READ | EV_PERSIST);

		//增加数量
		if (!m_bKeepAlive)
			pServer->DealThreadClientCount(m_ullThreadNO, true);

		m_LogFile << E_LOG_LEVEL_PROMPT << "Client handle thread [" << m_ullThreadNO << "] add client ["
			<< client->Session.szClientInfo << "] success !" << logendl;
	}
	else
	{
		m_LogFile << E_LOG_LEVEL_WARN << "Client handle thread [" << m_ullThreadNO << "] add client ["
			<< client->Session.szClientInfo << "] failed: create new socket bufferevent failed !" << logendl;

		delete client;
		client = nullptr;
	}
}

/*********************************************************************
功能：	获取处理线程编号
参数：	无
返回：	线程编号
修改：
*********************************************************************/
ev_uint64_t CClientHandleThread::GetThreadNO() const
{
	return m_ullThreadNO;
}

/*********************************************************************
功能：	获取运行标志，当长连接时判断是否己结束
参数：	无
返回：	运行标志
修改：
*********************************************************************/
bool CClientHandleThread::GetRunFlag() const
{
	return m_bRunFlag;
}

/*********************************************************************
功能：	处理线程
参数：	无
返回：	无
修改：
*********************************************************************/
void CClientHandleThread::HandleThread()
{
	m_LogFile << E_LOG_LEVEL_ALL << "Client handle thread [" << m_ullThreadNO << "] start dispatch !"
		<< logendl;

	evutil_gettimeofday(&m_BeginTime, nullptr);
	event_base_dispatch(m_pEventBase);
}

/*********************************************************************
功能：	定时回调方法
参数：	fd 句柄或信号
*		evFlags 事件标志
*		arg 用户数据
返回：	无
修改：
*********************************************************************/
void CClientHandleThread::timeout_cb(evutil_socket_t fd, short evFlags, void *arg)
{
	CClientHandleThread *pServer = (CClientHandleThread*)arg;
	long lDiv = 0, lMod = 0;

	if (nullptr != pServer)
	{
		struct timeval CurrTime, difference;
		std::string szOut;

		evutil_gettimeofday(&CurrTime, nullptr);
		evutil_timersub(&CurrTime, &pServer->m_BeginTime, &difference);

		szOut = std::to_string(difference.tv_usec / 1000) + " 毫秒 !";
		lDiv = difference.tv_sec / 60; //分钟
		lMod = difference.tv_sec % 60; //秒

		if (lMod > 0)
			szOut = std::to_string(lMod) + " 秒 " + szOut;

		if (lDiv > 0)
		{
			lMod = lDiv % 60;
			lDiv = lDiv / 60; //小时

			if (lMod > 0)
				szOut = std::to_string(lMod) + " 分钟 " + szOut;

			if (lDiv > 0)
			{
				lMod = lDiv % 24;
				lDiv = lDiv / 24;

				if (lMod > 0)
					szOut = std::to_string(lMod) + " 小时 " + szOut;

				if (lDiv > 0)
					szOut = std::to_string(lDiv) + " 天 " + szOut;
			}
		}


		pServer->m_LogFile << E_LOG_LEVEL_PROMPT << "Client handle thread [" << pServer->m_ullThreadNO
			<< "] timeout_cb called, total run " << szOut << logendl;
	}
}

/*********************************************************************
功能：	读事件处理
参数：	bev bufferevent
*		arg 参数
返回：	无
修改：
*********************************************************************/
void CClientHandleThread::read_cb(struct bufferevent *bev, void* arg)
{
	CClientObject* client = (CClientObject*)arg;
	CSocketServer* pServer = (CSocketServer*)(client->pHandleObject);
	CClientHandleThread *pSelf = (CClientHandleThread*)(client->pOther);
	int iRet = 1;

	//循环读取，尽量多读，防止多个包一起发时，只会收到前面一个
	do
	{
		//处理客户端数据
		iRet = pServer->ClientDataHandle(bev, client->Session, client->Data);

		if (0 == iRet) //没读到，退出，等待下一次的数据到来
			break;
		else if (1 == iRet) //处理成功，清理数据
			client->Data.Clear();
		else //错误，断开客户端的连接，防止接收脏数据
		{
			pServer->m_LogPrint << E_LOG_LEVEL_WARN << "Client handle thread [" << pSelf->m_ullThreadNO << "] recv from ["
				<< client->Session.szClientInfo << "] msg read and handle failed, server active disconnected !"
				<< logendl;

			//减少数量
			if (!pSelf->m_bKeepAlive)
				pServer->DealThreadClientCount(pSelf->m_ullThreadNO, false);

			pServer->OnClientDisconnect(client->Session);
			client->Clear();
			delete client;
			client = nullptr;
			bufferevent_free(bev);

			//修改运行标志
			if (pSelf->m_bKeepAlive)
				pSelf->m_bRunFlag = false;

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
void CClientHandleThread::write_cb(struct bufferevent *bev, void* arg)
{
	CClientObject* client = (CClientObject*)arg;
	CSocketServer* pServer = (CSocketServer*)(client->pHandleObject);
	CClientHandleThread *pSelf = (CClientHandleThread*)(client->pOther);

	pServer->m_LogPrint << E_LOG_LEVEL_PROMPT << "Client handle thread [" << pSelf->m_ullThreadNO << "] client ["
		<< client->Session.szClientInfo << "] msg write completed." << logendl;
}

/*********************************************************************
功能：	错误事件处理
参数：	bev bufferevent
*		evFlags 事件标志位
*		arg 参数
返回：	无
修改：
*********************************************************************/
void CClientHandleThread::error_cb(struct bufferevent *bev, short evFlags, void* arg)
{
	CClientObject* client = (CClientObject*)arg;
	CSocketServer* pServer = (CSocketServer*)(client->pHandleObject);
	CClientHandleThread *pSelf = (CClientHandleThread*)(client->pOther);
	std::string szError;

	//错误处理
	if (evFlags & BEV_EVENT_EOF)
		szError = "normal disconnected.";
	else if (evFlags & BEV_EVENT_ERROR)
	{
		szError = std::string("unnormal disconnected, error:") 
			+ evutil_socket_error_to_string(evutil_socket_geterror(bufferevent_getfd(bev)));
	}
	else if (evFlags & BEV_EVENT_TIMEOUT)
		szError = "timeout disconnected.";
	else
		szError = "abnormal disconnected, evFlags:" + std::to_string(evFlags);

	pServer->m_LogPrint << E_LOG_LEVEL_WARN << "Client handle thread [" << pSelf->m_ullThreadNO << "] client ["
		<< client->Session.szClientInfo << "] " << szError << logendl;

	//输出未处理的信息
	if (client->Data.uiMsgTotalLength > 0 && nullptr != client->Data.pMsgBuffer)
	{
		pServer->m_LogPrint << E_LOG_LEVEL_WARN << "Client handle thread [" << pSelf->m_ullThreadNO << "] client ["
			<< client->Session.szClientInfo << "] has untreated data:\n0x"
			<< pServer->GetHexString(client->Data.pMsgBuffer, client->Data.uiMsgTotalLength) 
			<< logendl;
	}

	//减少数量
	if (!pSelf->m_bKeepAlive)
		pServer->DealThreadClientCount(pSelf->m_ullThreadNO, false);

	//释放资源
	pServer->OnClientDisconnect(client->Session);
	client->Clear();
	delete client;
	client = nullptr;
	bufferevent_free(bev);

	//修改运行标志
	if (pSelf->m_bKeepAlive)
		pSelf->m_bRunFlag = false;
}