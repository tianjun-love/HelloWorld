/******************************************************
功能：	客户端数据处理类
作者：	田俊
时间：	2020-06-01
修改：
******************************************************/
#ifndef __CLIENT_HANDLE_HPP__
#define __CLIENT_HANDLE_HPP__

#include "event.h"
#include "ClientObject.hpp"
#include "LogFile/include/LogFile.hpp"
#include <thread>

class CClientHandleThread
{
public:
	CClientHandleThread(ev_uint64_t ullNO, CLogFile &log, bool bKeepAlive);
	CClientHandleThread(const CClientHandleThread &Other) = delete;
	~CClientHandleThread();
	CClientHandleThread& operator=(const CClientHandleThread &Other) = delete;

	bool Start(std::string &szError);
	void Stop();

	void AddClient(CClientObject *client);
	ev_uint64_t GetThreadNO() const;
	bool GetRunFlag() const;

private:
	void HandleThread();
	static void timeout_cb(evutil_socket_t fd, short evFlags, void *arg);
	static void read_cb(bufferevent *bev, void* arg);
	static void write_cb(bufferevent *bev, void* arg);
	static void error_cb(bufferevent *bev, short evFlags, void* arg);

private:
	bool               m_bRunFlag;        //运行标志
	const ev_uint64_t  m_ullThreadNO;     //线程编号
	CLogFile           &m_LogFile;        //日志对象
	const bool         m_bKeepAlive;      //是否是长连接
					   
	std::thread        *m_pThread;        //循环线程
	struct timeval     m_BeginTime;       //开始时间
	struct event       *m_pEvent;         //定时事件
	struct event_base  *m_pEventBase;     //event_base

};

#endif