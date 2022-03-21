#include "../include/SocketServer.hpp"
#include <event2/thread.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

static CLogFile* g_pLogPrint = nullptr; //��־���������ĺ���ʹ��
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
���ܣ�	��ʼ��socket����
������	szError ������Ϣ�����
���أ�	�ɹ�����true
�޸ģ�
*********************************************************************/
bool CSocketServer::Init(std::string& szError)
{
	//���IP��ʽ
	if (!CheckIPFormat(m_szServerIP))
	{
		szError = "Init check server IP format wrong !";
		return false;
	}

	//���˿�
	if (m_iServerPort <= 0 || m_iServerPort >= 65536)
	{
		szError = "Init check server port wrong !";
		return false;
	}

	//��鴦���߳���
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

	//����������־���
	if (nullptr == g_pLogPrint)
	{
		g_pLogPrint = &m_LogPrint;
	}

	//���ô�����־��ӡ�ص�����
	event_set_log_callback(log_cb);

	//������������ص�����
	event_set_fatal_callback(fatal_cb);

	//��Ӵ����߳�
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
���ܣ�	��������
������	szError ������Ϣ
���أ�	����ʧ�ܷ���false���ɹ������ѭ�������᷵��
�޸ģ�
*********************************************************************/
bool CSocketServer::Start(std::string& szError)
{
	if (!m_bStatus)
	{
		m_bStatus = true;
		
		//���������߳�
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

		//���������߳�
		if (!m_ListenerThread.Start(szError))
		{
			szError = "Start socket listen thread error:" + szError;
			return false;
		}

		CClientObject *client = nullptr;
		ev_uint64_t ullIndex = 0;
		ev_uint64_t ullNO = 0;

		//���߳�ѭ��
		while (m_bStatus)
		{
			m_ConnectDeque.Get(client);

			if (nullptr != client)
			{
				client->pHandleObject = (void*)this;

				if (CheckIsPersistentConnection(client->Session.szClientIP)) //�����Ӵ���ÿ������һ���߳�
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

							//��ӿͻ��˴���
							t->AddClient(client);

							//���̶߳��󱣴�
							m_KeepAliveClientLock.lock();
							m_KeepAliveClientThreads.push_back(t);
							m_KeepAliveClientLock.unlock();
						}
					}
				}
				else //�����Ӵ���
				{
					//��ȡ�����߳�
					ullIndex = GetDealThreadNO();

					//����
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
���ܣ�	ֹͣ����
������	��
���أ�	��
�޸ģ�
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
���ܣ�	�ͷ��������Դ
������	��
���أ�	��
�޸ģ�
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

	//�ͷ�libevent
	libevent_global_shutdown();
}

/*********************************************************************
���ܣ�	��ȡsocket��Ϣ
������	fd socket���
*		pIP IP��ַ�����������Ҫ��NULL
*		pPort �˿ڣ����������Ҫ��NULL
���أ�	IP��ַ:�˿��ַ���
�޸ģ�
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
���ܣ�	�����߳̿ͻ�������
������	ullThreadNO �̱߳��
*		bIsAdd true:���ӣ�false:����
���أ�	��
�޸ģ�
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

		//���¼���Ȩֵ
		iter->second->ullWeight = iter->second->uiCurrentCount * 7 + iter->second->ullTotalCount * 3;
	}

	m_ThreadClientCountLock.unlock();
}

/*********************************************************************
���ܣ�	��ȡ�����̱߳�ţ������㷨
������	��
���أ�	�����̱߳��
�޸ģ�
*********************************************************************/
ev_uint64_t CSocketServer::GetDealThreadNO()
{
	ev_uint64_t uiIndex = 0;
	uint64_t ullWeight = 0;
	std::map<ev_uint64_t, SClientThreadInfo*>::const_iterator iter;

	m_ThreadClientCountLock.lock();

	//�������ٻ���һ����¼
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
					//�ҵ�Ȩֵ��С��
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
���ܣ�	�ͻ������ݴ���
������	szClientStr �ͻ�����Ϣ������
*		bev bufferevent������
*		clientSession �ͻ��˻Ự������
*		clientData �ͻ������ݣ����
���أ�	<0��ʧ�ܣ�0���ɹ������ݲ�ȫ��1���ɹ�����
�޸ģ�
*********************************************************************/
short CSocketServer::ClientDataHandle(bufferevent *bev, CClientSession &session, CClientData &data)
{
	size_t iMaxReadLength = bufferevent_get_max_single_read(bev);

	if (nullptr == data.pMsgBuffer)
	{
		data.pMsgBuffer = new unsigned char[iMaxReadLength + 1]{ '\0' };
	}

	//��ȡ����
	data.uiMsgTotalLength = (unsigned int)bufferevent_read(bev, data.pMsgBuffer, iMaxReadLength);

	//���
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

		//������ͬ����
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
���ܣ�	���ͻ����Ƿ���������
������	szClientIP �ͻ���IP
���أ�	�������ӷ���true
�޸ģ�
*********************************************************************/
bool CSocketServer::CheckIsPersistentConnection(const std::string &szClientIP)
{
	return false;
}

/*********************************************************************
���ܣ�	�ͻ��˶Ͽ�����ʱ�Ļص���������Ҫ������Ự�ڵ�����
������	clientSession �ͻ�����Ϣ
���أ�	��
�޸ģ�
*********************************************************************/
void CSocketServer::OnClientDisconnect(CClientSession &clientSession)
{
	return;
}

/*********************************************************************
���ܣ�	�ڲ��̻߳ص�����
������	��
���أ�	��
�޸ģ�
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
���ܣ�	��IP�Ͷ˿�ת������
������	szIP IP���磺"169.202.51.212"
*		iPort �˿�
���أ�	ת��������
�޸ģ�
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
���ܣ�	������־����
������	severity ��־�ȼ�
*		msg ��־��Ϣ
���أ�	��
�޸ģ�
*********************************************************************/
void CSocketServer::log_cb(int severity, const char* msg)
{
	//��־�������
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
���ܣ�	����������Ĭ����exit(1)
������	err �����
���أ�	��
�޸ģ�
*********************************************************************/
void CSocketServer::fatal_cb(int err)
{
	//����������
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