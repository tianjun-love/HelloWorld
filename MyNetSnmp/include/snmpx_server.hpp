/********************************************************************
名称:	自定义snmpx服务类
功能:	snmpx服务
作者:	田俊
时间:	2021-01-26
注意:	windows环境使用前，先调用init_win32_socket_env
*********************************************************************/
#ifndef __SNMPX_SERVER_HPP__
#define __SNMPX_SERVER_HPP__

#include "error_status.hpp"
#include <mutex>

using std::list;
using std::string;

class CSnmpxServer
{
public:
	CSnmpxServer(const string &ip, unsigned short port, bool is_trapd_server, bool port_reuseable = true);
	virtual ~CSnmpxServer();
	
	bool StartServer(string &szError);
	void StopServer();
	bool AddUserAuthorization(short version, const std::string &szUserName, std::string &szError, unsigned char safeMode = 2, 
		unsigned char authMode = 0, const std::string &szAuthPasswd = "", unsigned char privMode = 0,
		const std::string &szPrivPasswd = "", const unsigned char *engineID = NULL, unsigned int engineIdLen = 0);

	//检查端口是否被占用
	static bool CheckSocketPortUsed(const string &ip, unsigned short port, bool is_udp = true);

protected:
	virtual void Trap_Handle(const string &agentIP, unsigned short agentPort, const string &enterprise_oid, int generic_trap, 
		int specific_trap, unsigned int time_stamp, const list<SSnmpxValue*> &vb_list, size_t max_oid_string_len);
	virtual void Trap2_Handle(const string &agentIP, unsigned short agentPort, const string &enterprise_oid, unsigned int sysuptime,
		const list<SSnmpxValue*> &vb_list, size_t max_oid_string_len);
	virtual void Request_Handle(const string &clientIP, unsigned short clientPort, unsigned char tag, const list<SSnmpxValue*> &rvb_list, 
		int &error_stat, int &error_index, list<SSnmpxValue> &svb_list, size_t max_oid_string_len);
	virtual void PrintLogMsg(unsigned short nLevel, const string &szMsg);

	static string GetItemsPrintString(unsigned int *sysuptime, const string *enterprise_id, const list<SSnmpxValue*> &vb_list,
		size_t max_oid_string_len = 29);

private:
	void RecvDataDealThread(const string client_ip, unsigned short client_port, char *data, unsigned int data_len, bool free_data);

#ifdef _WIN32
#define MAX_FDP_NUM	 512    //select最大监听数
#else
#define MAX_FDP_NUM	 4096   //epoll最大监听数
#endif

	//这里只是为了在接收v3 trap信息时加快解码速度
	static std::pair<std::map<std::string, userinfo_t*>*, std::mutex*> *m_UserAuthInfoCache; //agent用户信息，trap使用，<IP，用户信息>
	std::map<string, userinfo_t*>  m_UserTable;  //用户表<用户名+"_"+版本号, 用户信息对象>，如<"public_V2c", obj*>

protected:
	bool           m_bIsRun;         //服务运行状态
	std::string    m_szIP;           //服务IP
	unsigned short m_nPort;          //服务端口
	bool           m_bIsTrapd;       //true:trapd服务，false:agent服务
	bool           m_bPortReuseable; //是否端口复用
};



#endif//__QTUM_SNMP_TRAPD_HPP__



