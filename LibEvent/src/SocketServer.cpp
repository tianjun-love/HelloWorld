#include "../include/SocketServer.hpp"
#include <event2/thread.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

static CLogFile* g_pLogPrint = nullptr; //日志输出，类外的函数使用
const std::string g_szLibeventLog = "LibeventError.log";

CSocketServer::CSocketServer(CLogFile& log, const std::string& szIP, int iPort, int iMaxConnect, int iReadTimeOut, unsigned int uiClientHandleThreadNum) :
CSocketBase(szIP, iPort, iReadTimeOut), m_LogPrint(log), m_iMaxConnect(iMaxConnect), m_ConnectDeque(iMaxConnect, nullptr), m_ListenerThread(szIP, iPort, iMaxConnect, log, m_ConnectDeque),
m_uiHandleThreadNum(uiClientHandleThreadNum)
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
		CClientHandleThread *t = new CClientHandleThread(j, m_LogPrint);

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

		//启动监听线程
		if (!m_ListenerThread.Start(szError))
		{
			szError = "Start socket listen thread error:" + szError;
			return false;
		}

		SClientObject *client = nullptr;
		unsigned int uiIndex = 0;

		//主线程循环
		while (m_bStatus)
		{
			m_ConnectDeque.Get(client);

			if (nullptr != client)
			{
				client->pHandleObject = (void*)this;

				//获取处理线程
				uiIndex = GetDealThreadNO();

				//处理
				m_ClientHandleThreads[uiIndex]->AddClient(client);
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
功能：	处理线程客户端数量
参数：	uiThreadNO 线程编号
*		bIsAdd true:增加，false:减少
返回：	无
修改：
*********************************************************************/
void CSocketServer::DealThreadClientCount(unsigned int uiThreadNO, bool bIsAdd)
{
	std::map<unsigned int, SClientThreadInfo*>::iterator iter;

	m_ThreadClientCountLock.lock();

	iter = m_ThreadClientCountMap.find(uiThreadNO);
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
unsigned int CSocketServer::GetDealThreadNO()
{
	unsigned int uiIndex = 0;
	unsigned long long ullWeight = 0;
	std::map<unsigned int, SClientThreadInfo*>::const_iterator iter;

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
		m_LogPrint << E_LOG_LEVEL_PROMPT << "Client handle thread [" << clientInfo.uiThreadNO << "] recv from [" << clientInfo.szClientInfo 
			<< "] data_len:0 !" << logendl;
		return 0;
	}
	else
	{
		m_LogPrint << E_LOG_LEVEL_PROMPT << "Client handle thread [" << clientInfo.uiThreadNO << "] recv from [" << clientInfo.szClientInfo
			<< "] data_len:" << clientData.uiMsgTotalLength << " data:\n0x" << GetHexString(clientData.pMsgBuffer, clientData.uiMsgTotalLength)
			<< logendl;

		//返回相同数据
		if (0 == bufferevent_write(bev, clientData.pMsgBuffer, clientData.uiMsgTotalLength))
		{
			m_LogPrint << E_LOG_LEVEL_PROMPT << "Client handle thread [" << clientInfo.uiThreadNO << "] send to [" << clientInfo.szClientInfo
				<< "] same data success !" << logendl;
			return 1;
		}
		else
		{
			m_LogPrint << E_LOG_LEVEL_PROMPT << "Client handle thread [" << clientInfo.uiThreadNO << "] send to [" << clientInfo.szClientInfo
				<< "] same data failed:" << evutil_socket_error_to_string(evutil_socket_geterror(bufferevent_getfd(bev))) << logendl;
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