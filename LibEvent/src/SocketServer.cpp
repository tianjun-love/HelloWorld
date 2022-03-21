#include "../include/SocketServer.hpp"
#include <event2/thread.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

static CLogFile* g_pLogPrint = nullptr; //日志输出，类外的函数使用
const std::string g_szLibeventLog = "LibeventError.log";

CSocketServer::CSocketServer(CLogFile& log, const std::string& szIP, int iPort, int iMaxConnect, int iReadTimeOut, unsigned int uiClientHandleThreadNum) :
CSocketBase(szIP, iPort, iReadTimeOut), m_ConnectDeque(iMaxConnect, nullptr), m_ListenerThread(szIP, iPort, iMaxConnect, log, m_ConnectDeque), 
m_uiHandleThreadNum(uiClientHandleThreadNum), m_pKeepAliveDealThread(nullptr), m_LogPrint(log), m_iMaxConnect(iMaxConnect)
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

	//检查处理线程数
	if (0 == m_uiHandleThreadNum || m_uiHandleThreadNum > 100)
	{
		szError = "Client handle thread count must between 1 and 100 !";
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

	//添加处理线程
	for (unsigned int i = 0, j = 0; i < m_uiHandleThreadNum; ++i)
	{
		CClientHandleThread *t = new CClientHandleThread(j, m_LogPrint, false);

		if (nullptr != t)
		{
			SClientThreadInfo *c = new SClientThreadInfo();

			if (nullptr != c)
			{
				m_ClientHandleThreads.push_back(t);
				m_ThreadClientCountMap.insert(std::make_pair(j, c));
				++j;
			}
			else
			{
				delete t;
				t = nullptr;
				szError = "New client handle thread [" + std::to_string(j) + "] client connect count info object return nullptr !";
				return false;
			}
		}
		else
		{
			szError = "New client handle thread [" + std::to_string(j) + "] object return nullptr !";
			return false;
		}
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
		
		//启动处理线程
		for (auto &t : m_ClientHandleThreads)
		{
			if (nullptr != t)
			{
				if (!t->Start(szError))
				{
					szError = "Start client handle thread [" + std::to_string(t->GetThreadNO()) 
						+ "] error:" + szError;
					return false;
				}
			}
		}

		m_pKeepAliveDealThread = new std::thread(std::bind(&CSocketServer::InnerThreadHandle, this));
		if (nullptr == m_pKeepAliveDealThread)
		{
			szError = "Start keep-alive client handle thread return nullptr !";
			return false;
		}

		//启动监听线程
		if (!m_ListenerThread.Start(szError))
		{
			szError = "Start socket listen thread error:" + szError;
			return false;
		}

		CClientObject *client = nullptr;
		ev_uint64_t ullIndex = 0;
		ev_uint64_t ullNO = 0;

		//主线程循环
		while (m_bStatus)
		{
			m_ConnectDeque.Get(client);

			if (nullptr != client)
			{
				client->pHandleObject = (void*)this;

				if (CheckIsPersistentConnection(client->Session.szClientIP)) //长连接处理，每个连接一个线程
				{
					ullNO = ConvertIPAndPortToNum(client->Session.szClientIP, client->Session.uiClientPort);
					CClientHandleThread *t = new CClientHandleThread(ullNO, m_LogPrint, true);

					if (nullptr == t)
					{
						m_LogPrint << E_LOG_LEVEL_SERIOUS << "New keep-alive client handle thread [" << ullNO 
							<< "] object return nullptr !" << logendl;
					}
					else
					{
						m_LogPrint << E_LOG_LEVEL_PROMPT << "New keep-alive client handle thread [" << ullNO
							<< "] object success !" << logendl;

						if (!t->Start(szError))
						{
							m_LogPrint << E_LOG_LEVEL_SERIOUS << "Start keep-alive client handle thread [" << ullNO
								<< "] error:" << szError << logendl;
						}
						else
						{
							m_LogPrint << E_LOG_LEVEL_PROMPT << "Start keep-alive client handle thread [" << ullNO
								<< "] success !" << logendl;

							//添加客户端处理
							t->AddClient(client);

							//将线程对象保存
							m_KeepAliveClientLock.lock();
							m_KeepAliveClientThreads.push_back(t);
							m_KeepAliveClientLock.unlock();
						}
					}
				}
				else //短连接处理
				{
					//获取处理线程
					ullIndex = GetDealThreadNO();

					//处理
					m_ClientHandleThreads[ullIndex]->AddClient(client);
				}
			}
		}
	}
	else
	{
		szError = "Socket server is already started.";
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
		m_bStatus = false;

		m_ListenerThread.Stop();

		m_ConnectDeque.Put(nullptr);

		for (auto &t : m_ClientHandleThreads)
		{
			if (nullptr != t)
			{
				t->Stop();
				delete t;
				t = nullptr;
			}
		}

		if (nullptr != m_pKeepAliveDealThread)
		{
			m_pKeepAliveDealThread->join();
			delete m_pKeepAliveDealThread;
			m_pKeepAliveDealThread = nullptr;
		}
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
#ifdef _WIN32
	if (m_bInitWSA)
	{
		WSACleanup();
		m_bInitWSA = false;
	}
#endif

	m_ClientHandleThreads.clear();

	m_ThreadClientCountLock.lock();

	for (auto &c : m_ThreadClientCountMap)
	{
		delete c.second;
		c.second = nullptr;
	}

	m_ThreadClientCountMap.clear();

	m_ThreadClientCountLock.unlock();

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
	std::string szReturn;

	getpeername(fd, (struct sockaddr *)&sin, &len);
	szReturn = inet_ntoa(sin.sin_addr);

	if (nullptr != pIP)
		*pIP = szReturn;

	if (nullptr != pPort)
		*pPort = sin.sin_port;

	szReturn.append(std::string(":") + std::to_string(sin.sin_port));

	return std::move(szReturn);
}

/*********************************************************************
功能：	处理线程客户端数量
参数：	ullThreadNO 线程编号
*		bIsAdd true:增加，false:减少
返回：	无
修改：
*********************************************************************/
void CSocketServer::DealThreadClientCount(ev_uint64_t ullThreadNO, bool bIsAdd)
{
	std::map<ev_uint64_t, SClientThreadInfo*>::iterator iter;

	m_ThreadClientCountLock.lock();

	iter = m_ThreadClientCountMap.find(ullThreadNO);
	if (iter != m_ThreadClientCountMap.end())
	{
		if (bIsAdd)
		{
			++(iter->second->uiCurrentCount);
			++(iter->second->ullTotalCount);
		}
		else
			--(iter->second->uiCurrentCount);

		//重新计算权值
		iter->second->ullWeight = iter->second->uiCurrentCount * 7 + iter->second->ullTotalCount * 3;
	}

	m_ThreadClientCountLock.unlock();
}

/*********************************************************************
功能：	获取处理线程编号，负载算法
参数：	无
返回：	处理线程编号
修改：
*********************************************************************/
ev_uint64_t CSocketServer::GetDealThreadNO()
{
	ev_uint64_t uiIndex = 0;
	uint64_t ullWeight = 0;
	std::map<ev_uint64_t, SClientThreadInfo*>::const_iterator iter;

	m_ThreadClientCountLock.lock();

	//这里至少会有一个记录
	iter = m_ThreadClientCountMap.cbegin();
	if (0 == iter->second->ullWeight)
		uiIndex = iter->first;
	else
	{
		uiIndex = iter->first;
		ullWeight = iter->second->ullWeight;
		++iter;

		if (iter != m_ThreadClientCountMap.cend())
		{
			for (; iter != m_ThreadClientCountMap.cend(); ++iter)
			{
				if (0 == iter->second->ullWeight)
				{
					uiIndex = iter->first;
					break;
				}
				else
				{
					//找到权值最小的
					if (iter->second->ullWeight < ullWeight)
					{
						uiIndex = iter->first;
						ullWeight = iter->second->ullWeight;
					}
				}
			}
		}
	}

	m_ThreadClientCountLock.unlock();

	return uiIndex;
}

/*********************************************************************
功能：	客户端数据处理
参数：	szClientStr 客户端信息，输入
*		bev bufferevent，输入
*		clientSession 客户端会话，输入
*		clientData 客户端数据，输出
返回：	<0：失败，0：成功但数据不全，1：成功处理
修改：
*********************************************************************/
short CSocketServer::ClientDataHandle(bufferevent *bev, CClientSession &session, CClientData &data)
{
	size_t iMaxReadLength = bufferevent_get_max_single_read(bev);

	if (nullptr == data.pMsgBuffer)
	{
		data.pMsgBuffer = new unsigned char[iMaxReadLength + 1]{ '\0' };
	}

	//读取数据
	data.uiMsgTotalLength = (unsigned int)bufferevent_read(bev, data.pMsgBuffer, iMaxReadLength);

	//输出
	if (0 == data.uiMsgTotalLength)
	{
		m_LogPrint << E_LOG_LEVEL_PROMPT << "Client handle thread [" << session.ullThreadNO << "] recv from ["
			<< session.szClientInfo << "] data_len:0 !" << logendl;
		return 0;
	}
	else
	{
		m_LogPrint << E_LOG_LEVEL_PROMPT << "Client handle thread [" << session.ullThreadNO << "] recv from ["
			<< session.szClientInfo << "] data_len:" << data.uiMsgTotalLength << " data:\n0x"
			<< GetHexString(data.pMsgBuffer, data.uiMsgTotalLength) << logendl;

		//返回相同数据
		if (0 == bufferevent_write(bev, data.pMsgBuffer, data.uiMsgTotalLength))
		{
			m_LogPrint << E_LOG_LEVEL_PROMPT << "Client handle thread [" << session.ullThreadNO << "] send to ["
				<< session.szClientInfo << "] same data success !" << logendl;
			return 1;
		}
		else
		{
			m_LogPrint << E_LOG_LEVEL_PROMPT << "Client handle thread [" << session.ullThreadNO << "] send to ["
				<< session.szClientInfo << "] same data failed:" << evutil_socket_error_to_string(evutil_socket_geterror(bufferevent_getfd(bev)))
				<< logendl;
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
功能：	客户端断开连接时的回调方法，主要是清除会话内的数据
参数：	clientSession 客户端信息
返回：	无
修改：
*********************************************************************/
void CSocketServer::OnClientDisconnect(CClientSession &clientSession)
{
	return;
}

/*********************************************************************
功能：	内部线程回调方法
参数：	无
返回：	无
修改：
*********************************************************************/
void CSocketServer::InnerThreadHandle()
{
	const unsigned int uiSeconds = 15;
	std::list<CClientHandleThread*>::iterator iter;

	while (m_bStatus)
	{
		std::this_thread::sleep_for(std::chrono::seconds(uiSeconds));

		m_KeepAliveClientLock.lock();

		for (iter = m_KeepAliveClientThreads.begin(); iter != m_KeepAliveClientThreads.end();)
		{
			if ((*iter)->GetRunFlag())
				++iter;
			else
			{
				(*iter)->Stop();

				m_LogPrint << E_LOG_LEVEL_PROMPT << "Stop keep-alive client handle thread [" << (*iter)->GetThreadNO()
					<< "] success." << logendl;

				delete (*iter);
				iter = m_KeepAliveClientThreads.erase(iter);
			}
		}

		m_KeepAliveClientLock.unlock();
	}
}

/*********************************************************************
功能：	将IP和端口转成数字
参数：	szIP IP，如："169.202.51.212"
*		iPort 端口
返回：	转换的数字
修改：
*********************************************************************/
ev_uint64_t CSocketServer::ConvertIPAndPortToNum(const std::string &szIP, unsigned int iPort)
{
	char buf[32]{ '\0' };
	unsigned int a, b, c, d;

#ifdef _WIN32
	sscanf_s(szIP.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d);
#else
	sscanf(szIP.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d);
#endif

	snprintf(buf, sizeof(buf), "%u%u%u%u%u", a, b, c, d, iPort);

	return std::strtoull(buf, nullptr, 10);
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