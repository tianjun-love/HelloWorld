/******************************************************
功能：	socket服务端类
作者：	田俊
时间：	2019-02-18
修改：
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

	//每个线程处理客户端数量信息
	struct SClientThreadInfo
	{
		unsigned int       uiCurrentCount;  //当前连接数量
		unsigned long long ullTotalCount;   //历史总连接数量
		unsigned long long ullWeight;       //权值，iCurrentCount * 7 + iTotalCount * 3

		SClientThreadInfo() : uiCurrentCount(0), ullTotalCount(0), ullWeight(0) {};
	};

	//处理线程客户端数量
	void DealThreadClientCount(unsigned int uiThreadNO, bool bIsAdd);

	//获取处理线程编号，负载算法
	unsigned int GetDealThreadNO();

	//客户端数据处理，子类实现，返回：<0：失败，0：成功但没读取到数据，1：成功处理
	virtual short ClientDataHandle(bufferevent *bev, SClientInfo &clientInfo, SClientData &clientData);

	//检查客户端是否允许长连接
	virtual bool CheckIsPersistentConnection(const std::string &szClientIP);

private:
	static void log_cb(int severity, const char* msg);
	static void fatal_cb(int err);

protected:
	CLogFile&       m_LogPrint;       //日志输出类
	int             m_iMaxConnect;    //最大连接数

private:
	SynQueue<SClientObject*>                   m_ConnectDeque;          //连接队列
	CListenerThread                            m_ListenerThread;        //监听线程
	const unsigned int                         m_uiHandleThreadNum;     //客户端处理线程数
	std::vector<CClientHandleThread*>          m_ClientHandleThreads;   //客户端处理线程
	std::map<unsigned int, SClientThreadInfo*> m_ThreadClientCountMap;  //处理线程的客户端数量信息
	std::mutex                                 m_ThreadClientCountLock; //处理线程的客户端数量锁

};

#endif