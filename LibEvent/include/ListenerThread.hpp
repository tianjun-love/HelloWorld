/******************************************************
功能：	监听线程类
作者：	田俊
时间：	2020-06-01
修改：
******************************************************/
#ifndef __LISTENER_THREAD_HPP__
#define __LISTENER_THREAD_HPP__

#include "event.h"
#include "event2/listener.h"
#include "ClientObject.hpp"
#include "LogFile/include/LogFile.hpp"
#include "OS/include/SynQueue.hpp"
#include <thread>

class CListenerThread
{
public:
	CListenerThread(const std::string &szIP, int iPort, int iMaxConn, CLogFile &log, SynQueue<CClientObject*> &connDeque);
	CListenerThread(const CListenerThread &Other) = delete;
	~CListenerThread();
	CListenerThread& operator=(const CListenerThread &Other) = delete;

	bool Start(std::string &szError);
	void Stop();

private:
	void HandleThread();
	static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int sa_len, void *user_data);
	static void error_cb(struct evconnlistener *listener, void *user_data);

private:
	bool                     m_bRunFlag;        //运行标志
	std::string              m_szServerIP;      //服务IP
	int                      m_iServerPort;     //服务端口
	int                      m_iMaxConnect;     //最大连接数
	CLogFile                 &m_LogFile;        //日志对象
	SynQueue<CClientObject*> &m_ConnectDeque;   //客户端连接队列

	std::thread              *m_pThread;        //监听线程
	evutil_socket_t          m_iListenFd;       //监听的socket句柄
	struct event_base        *m_pEventBase;     //event_base
	struct evconnlistener    *m_pEventListener; //event监听对象

};

#endif