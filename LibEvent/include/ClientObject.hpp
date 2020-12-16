/******************************************************
功能：	客户端对象类
作者：	田俊
时间：	2020-06-01
修改：
******************************************************/
#ifndef __CLIENT_OBJECT_HPP__
#define __CLIENT_OBJECT_HPP__

#include <string>
#include <cstring>
#include <map>
#include <vector>
#include "event2/util.h"

//客户端会话
class CClientSession
{
public:
	CClientSession() : uiClientPort(0), ullThreadNO(0), bLogined(false) {};
	~CClientSession() {};

public:
	std::string          szClientInfo;         //IP + ":" + Port
	std::string          szClientIP;           //IP
	ev_uint32_t          uiClientPort;         //端口
	ev_uint64_t          ullThreadNO;          //所属处理线程编号
	std::string          szUserName;           //登陆用户名称
	bool                 bLogined;             //登陆标志
	std::map<int, void*> UserDataCache;        //用户数据缓存，如果需要，注意在客户端断开连接时手工释放
	std::vector<void*>   UserAttrCache;        //用户属性缓存，如果需要，注意在客户端断开连接时手工释放

};

//客户端连接数据
class CClientData
{
public:
	CClientData() : bSecurityTunnel(false), bReadSignLength(false), uiMsgTotalLength(0), pMsgBuffer(nullptr),
		uiMsgReadedLength(0), ullMsgID(0) {
		memset(sessionKeyBuffer, 0, 16);
	};
	CClientData(const CClientData& Other) = delete;
	~CClientData() { Clear(); };
	CClientData& operator=(const CClientData& Other) = delete;

	void Clear() {
		uiMsgTotalLength = 0;
		uiMsgReadedLength = 0;

		if (nullptr != pMsgBuffer) {
			delete[] pMsgBuffer;
			pMsgBuffer = nullptr;
		}
	}

public:
	bool               bSecurityTunnel;      //是否安全通道
	bool               bReadSignLength;      //是否读取签名长度
	unsigned int       uiMsgTotalLength;     //消息总长度
	unsigned char      *pMsgBuffer;          //消息缓存，子类不用释放
	unsigned int       uiMsgReadedLength;    //消息己读取长度
	unsigned char      sessionKeyBuffer[16]; //会话密钥
	unsigned long long ullMsgID;             //消息ID

};

//客户端对象
class CClientObject
{
public:
	CClientObject() : iClientFd(-1), pHandleObject(nullptr), pOther(nullptr) {};
	CClientObject(const CClientObject& Other) = delete;
	~CClientObject() { Clear(); };
	CClientObject& operator=(const CClientObject& Other) = delete;

	void Clear() { Data.Clear(); };

public:
	evutil_socket_t iClientFd;       //客户端连接句柄
	void            *pHandleObject;  //数据处理服务对象
	void            *pOther;         //其它对象
	CClientSession	Session;         //客户端会话
	CClientData     Data;            //客户端数据

};

#endif