/******************************************************
���ܣ�	socket�������
���ߣ�	�￡
ʱ�䣺	2019-02-18
�޸ģ�
******************************************************/
#ifndef __SOCKET_SERVER_HPP__
#define __SOCKET_SERVER_HPP__

#include "SocketBase.hpp"
#include "ListenerThread.hpp"
#include "ClientHandleThread.hpp"
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
		unsigned int       uiCurrentCount;  //��ǰ��������
		unsigned long long ullTotalCount;   //��ʷ����������
		unsigned long long ullWeight;       //Ȩֵ��iCurrentCount * 7 + iTotalCount * 3

		SClientThreadInfo() : uiCurrentCount(0), ullTotalCount(0), ullWeight(0) {};
	};

	//�����߳̿ͻ�������
	void DealThreadClientCount(unsigned int uiThreadNO, bool bIsAdd);

	//��ȡ�����̱߳�ţ������㷨
	unsigned int GetDealThreadNO();

	//�ͻ������ݴ�������ʵ�֣����أ�<0��ʧ�ܣ�0���ɹ���û��ȡ�����ݣ�1���ɹ�����
	virtual short ClientDataHandle(bufferevent *bev, SClientInfo &clientInfo, SClientData &clientData);

	//���ͻ����Ƿ���������
	virtual bool CheckIsPersistentConnection(const std::string &szClientIP);

private:
	static void log_cb(int severity, const char* msg);
	static void fatal_cb(int err);

protected:
	CLogFile&       m_LogPrint;       //��־�����
	int             m_iMaxConnect;    //���������

private:
	SynQueue<SClientObject*>                   m_ConnectDeque;          //���Ӷ���
	CListenerThread                            m_ListenerThread;        //�����߳�
	const unsigned int                         m_uiHandleThreadNum;     //�ͻ��˴����߳���
	std::vector<CClientHandleThread*>          m_ClientHandleThreads;   //�ͻ��˴����߳�
	std::map<unsigned int, SClientThreadInfo*> m_ThreadClientCountMap;  //�����̵߳Ŀͻ���������Ϣ
	std::mutex                                 m_ThreadClientCountLock; //�����̵߳Ŀͻ���������

};

#endif