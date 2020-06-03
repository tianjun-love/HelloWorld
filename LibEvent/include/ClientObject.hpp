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
#include "event2/util.h"

//客户端信息
struct SClientInfo
{
	std::string  szClientInfo;         //IP + ":" + Port
	std::string  szClientIP;           //IP
	bool         bLogined;             //是否己登陆
	unsigned int uiThreadNO;           //所属处理线程编号

	SClientInfo() : bLogined(false), uiThreadNO(0) {};
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
		uiMsgReadedLength(0), ullMsgID(0) {
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
	void            *pHandleObject;  //用户参数，数据处理服务对象
	void            *pOtherObject;   //用户参数，其它对象
	evutil_socket_t iClientFd;       //客户端连接句柄
	SClientInfo     ClientInfo;      //客户端信息
	SClientData     ClientData;      //客户端数据

	SClientObject() : pHandleObject(nullptr), pOtherObject(nullptr), iClientFd(-1) {};
	SClientObject(const SClientObject& Other) = delete;
	~SClientObject() { Clear(); };
	SClientObject& operator=(const SClientObject& Other) = delete;
	void Clear() { ClientData.Clear(); };
};

#endif