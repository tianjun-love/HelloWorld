/******************************************************
功能：	socket服务端类
作者：	田俊
时间：	2019-02-18
修改：
******************************************************/
#ifndef __SOCKET_SERVER_HPP__
#define __SOCKET_SERVER_HPP__

#include "SocketBase.hpp"
#include "event.h"
#include "LogFile/include/LogFile.hpp"

class CSocketServer : public CSocketBase
{
public:
	CSocketServer(CLogFile& log, const std::string& szIP, int iPort, int iMaxConnect = 100, int iReadTimeOut = 0);
	CSocketServer(const CSocketServer& Other) = delete;
	virtual ~CSocketServer();
	CSocketServer& operator=(const CSocketServer& Other) = delete;

	static std::string GetSocketInfo(evutil_socket_t fd, std::string *pIP = nullptr, int *pPort = nullptr);

protected:
	bool Init(std::string& szError);
	bool Start(std::string& szError);
	void Stop();
	void Free();

	//客户端信息
	struct SClientInfo
	{
		std::string szClientInfo;         //IP + ":" + Port
		std::string szClientIP;           //IP
		bool        bLogined;             //是否己登陆

		SClientInfo() : bLogined(false) {};
		~SClientInfo() {};
	};

	//客户端连接数据
	struct SClientData
	{
		bool               bSecurityTunnel;      //是否安全通道
		bool               bReadSignLength;      //是否读取签名长度
		unsigned int       uiMsgTotalLength;     //消息总长度
		unsigned char      *pMsgBuffer;          //消息缓存，子类不用释放
		unsigned int       uiMsgReadedLength;    //消息己读取长度
		unsigned char      sessionKeyBuffer[16]; //会话密钥
		unsigned long long ullMsgID;             //消息ID

		SClientData() : bSecurityTunnel(false), bReadSignLength(false), uiMsgTotalLength(0), pMsgBuffer(nullptr), 
			uiMsgReadedLength(0), ullMsgID(0){
			memset(sessionKeyBuffer, 0, 16);
		};
		SClientData(const SClientData& Other) = delete;
		~SClientData() { Clear(); };
		SClientData& operator=(const SClientData& Other) = delete;
		void Clear() {
			uiMsgTotalLength = 0;
			uiMsgReadedLength = 0;

			if (nullptr != pMsgBuffer) {
				delete[] pMsgBuffer;
				pMsgBuffer = nullptr;
			}
		}
	};

	//客户端对象
	struct SClientObject
	{
		CSocketServer *pServerObject;  //服务对象
		SClientInfo   ClientInfo;      //客户端信息
		SClientData   ClientData;      //客户端数据

		SClientObject() : pServerObject(nullptr) {};
		SClientObject(const SClientObject& Other) = delete;
		~SClientObject() { Clear(); };
		SClientObject& operator=(const SClientObject& Other) = delete;
		void Clear() { ClientData.Clear(); };
	};

	//客户端数据处理，子类实现，返回：<0：失败，0：成功但没读取到数据，1：成功处理
	virtual short ClientDataHandle(bufferevent *bev, SClientInfo &clientInfo, SClientData &clientData);

	//检查客户端是否允许长连接
	virtual bool CheckIsPersistentConnection(const std::string &szClientIP);

private:
	static void do_accept(evutil_socket_t listenFd, short evFlags, void* arg);
	static void read_cb(bufferevent *bev, void* arg);
	static void write_cb(bufferevent *bev, void* arg);
	static void error_cb(bufferevent *bev, short evFlags, void* arg);
	static void log_cb(int severity, const char* msg);
	static void fatal_cb(int err);

protected:
	CLogFile&       m_LogPrint;       //日志输出类
	int             m_iMaxConnect;    //最大连接数
	int             m_iCurrConnect;   //当前连接数

private:
	evutil_socket_t m_iListenFd;      //监听的socket句柄
	event_base*     m_pEventBase;     //event_base
	struct event*   m_pEventListener; //监听事件

};

#endif