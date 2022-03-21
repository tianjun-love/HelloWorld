/******************************************************
���ܣ�	socket�������
���ߣ�	�￡
ʱ�䣺	2019-02-18
�޸ģ�
******************************************************/
#ifndef __SOCKET_SERVER_HPP__
#define __SOCKET_SERVER_HPP__

#include "SocketBase.hpp"
#include "../include/ListenerThread.hpp"
#include "../include/ClientHandleThread.hpp"
#include <map>

class CSocketServer : public CSocketBase
{
public:
	CSocketServer(CLogFile& log, const std::string& szIP, int iPort, int iMaxConnect = 100, int iReadTimeOut = 0,
		unsigned int uiClientHandleThreadNum = 5);
	CSocketServer(const CSocketServer& Other) = delete;
	virtual ~CSocketServer();
	CSocketServer& operator=(const CSocketServer& Other) = delete;

	static std::string GetSocketInfo(evutil_socket_t fd, std::string *pIP = nullptr, int *pPort = nullptr);

protected:
	bool Init(std::string& szError);
	bool Start(std::string& szError);
	void Stop();
	void Free();

	friend class CClientHandleThread;

	//ÿ���̴߳���ͻ���������Ϣ
	struct SClientThreadInfo
	{
		unsigned int   uiCurrentCount;  //��ǰ��������
		uint64_t       ullTotalCount;   //��ʷ����������
		uint64_t       ullWeight;       //Ȩֵ��iCurrentCount * 7 + iTotalCount * 3

		SClientThreadInfo() : uiCurrentCount(0), ullTotalCount(0), ullWeight(0) {};
	};

	//�����߳̿ͻ�������
	void DealThreadClientCount(ev_uint64_t ullThreadNO, bool bIsAdd);

	//��ȡ�����̱߳�ţ������㷨
	ev_uint64_t GetDealThreadNO();

	//�ͻ������ݴ�������ʵ�֣����أ�<0��ʧ�ܣ�0���ɹ���û��ȡ�����ݣ�1���ɹ�����
	virtual short ClientDataHandle(bufferevent *bev, CClientSession &session, CClientData &data);

	//���ͻ����Ƿ���������
	virtual bool CheckIsPersistentConnection(const std::string &szClientIP);

	//�ͻ��˶Ͽ�ʱ�Ĵ�����
	virtual void OnClientDisconnect(CClientSession &clientSession);

private:
	void InnerThreadHandle();
	static ev_uint64_t ConvertIPAndPortToNum(const std::string &szIP, unsigned int iPort);
	static void log_cb(int severity, const char* msg);
	static void fatal_cb(int err);

	SynQueue<CClientObject*>                   m_ConnectDeque;          //���Ӷ���
	CListenerThread                            m_ListenerThread;        //�����߳�
	const unsigned int                         m_uiHandleThreadNum;     //�ͻ��˴����߳���
	std::vector<CClientHandleThread*>          m_ClientHandleThreads;   //�ͻ��˴����߳�
	std::map<ev_uint64_t, SClientThreadInfo*>  m_ThreadClientCountMap;  //�����̵߳Ŀͻ���������Ϣ
	std::mutex                                 m_ThreadClientCountLock; //�����̵߳Ŀͻ���������
	std::thread                                *m_pKeepAliveDealThread; //�����Ӵ����߳�
	std::list<CClientHandleThread*>            m_KeepAliveClientThreads;//�����ӿͻ��˴����߳�
	std::mutex                                 m_KeepAliveClientLock;   //�����ӿͻ��˴����߳���

protected:
	CLogFile&     m_LogPrint;    //��־�����
	int           m_iMaxConnect; //���������

};

#endif