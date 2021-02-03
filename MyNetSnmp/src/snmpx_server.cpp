#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <thread>

#include "../include/snmpx_server.hpp"
#include "../include/snmpx_unpack.hpp"
#include "../include/snmpx_user_proccess.hpp"

#ifdef _WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

//系统启动时长OID标识，在item第一个绑定
static const char* trapSystemUpTime = ".1.3.6.1.2.1.1.3.0";

//企业OID标识,在item第二个绑定，RFC1157会item在最后绑定：".1.3.6.1.6.3.1.1.4.3.0"
static const char* trapEnterpriseID = ".1.3.6.1.6.3.1.1.4.1.0";

std::pair<std::map<std::string, userinfo_t*>*, std::mutex*>* CSnmpxServer::m_UserAuthInfoCache =
	new std::pair<std::map<std::string, userinfo_t*>*, std::mutex*>
{ 
	new std::map<std::string, userinfo_t*>(), 
	new std::mutex() 
};

/********************************************************************
功能：	设置socket非阻塞
参数：	sockfd socket句柄
返回：	无
*********************************************************************/
static void SetSocketNoblocking(int sock_fd)
{
#ifdef _WIN32
	u_long ulBlock = 1; //0：阻塞，非0：不阻塞
	ioctlsocket(sock_fd, FIONBIO, &ulBlock);
#else
	int flags = fcntl(sock_fd, F_GETFD, 0);

	if (-1 != flags) {
		fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
	}
#endif
}

/********************************************************************
功能：	初始化socket句柄及地址服务
参数：	ip 服务IP
*		port 服务端口
*		addr 服务地址
*		szError 错误信息
返回：	无
*********************************************************************/
static bool InitSocketFd(const std::string &ip, unsigned short port, int& fd, sockaddr_in& addr, string &szError)
{
	//socket
	fd = (int)socket(AF_INET, SOCK_DGRAM, 0);
	if (fd <= 0)
	{
		szError = "socket创建失败：" + CErrorStatus::get_err_msg(errno, true, true);
		return false;
	}
	else
		SetSocketNoblocking(fd); //设置非阻塞

	//设置地址信息
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	//addr.sin_addr.s_addr = inet_addr(ip.c_str());
	if (ip.empty())
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

	//绑定地址
	if (::bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1)
	{
		szError = "socket绑定地址失败：" + CErrorStatus::get_err_msg(errno, true, true);
		close_socket_fd(fd);
		return false;
	}	

	return true;
}

CSnmpxServer::CSnmpxServer(const string &ip, unsigned short port, bool is_trapd_server) : m_bIsRun(false), m_szIP(ip), 
	m_nPort(port), m_bIsTrapd(is_trapd_server)
{
}

CSnmpxServer::~CSnmpxServer()
{
	if (!m_UserTable.empty())
	{
		CUserProccess::snmpx_user_map_free(m_UserTable);
		m_UserTable.clear();
	}
}

/********************************************************************
功能：	snmpx服务开始
参数：	szError 错误信息
返回：	失败返回false
*********************************************************************/
bool CSnmpxServer::StartServer(string &szError)
{
	bool bResult = true;
	int server_fd = 0;
	sockaddr_in server_addr;

	//初始化socket句柄，绑定地址
	if (InitSocketFd(m_szIP, m_nPort, server_fd, server_addr, szError))
	{
		char *data = NULL; //由处理线程释放
		char recv_buff[MAX_MSG_LEN + 1];
		int recv_len = 0;
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = 0;
		string client_ip;
		unsigned short client_port;
		char ip_buff[32] = { '\0' };

#ifdef _WIN32
		struct fd_set rfd;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 100000; //100毫秒
		int nfds = 0;

		m_bIsRun = true;
		PrintLogMsg(0, "snmpx_server start on [" + m_szIP + ":" + std::to_string(m_nPort) + "]...");

		while (m_bIsRun)
		{
			FD_ZERO(&rfd);
			FD_SET(server_fd, &rfd);

			nfds = select(server_fd + 1, &rfd, NULL, NULL, &tv);

			if (nfds < 0)
			{
				PrintLogMsg(0, "snmpx_server check client data for read failed, select return error: "
					+ CErrorStatus::get_err_msg(errno, true, true));
			}
			else if (nfds > 0)
			{
				if (FD_ISSET(server_fd, &rfd))
				{
					client_addr_len = (socklen_t)sizeof(client_addr);
					memset(&client_addr, 0, client_addr_len);

					recv_len = recvfrom(server_fd, recv_buff, MAX_MSG_LEN, 0, (struct sockaddr*)&client_addr, &client_addr_len);
					if (recv_len > 0)
					{
						//agent_ip = inet_ntoa(client_addr.sin_addr);
						memset(ip_buff, 0, sizeof(ip_buff));
						inet_ntop(AF_INET, &client_addr.sin_addr, ip_buff, sizeof(ip_buff));
						client_ip = ip_buff;
						client_port = ntohs(client_addr.sin_port);

						if (m_bIsTrapd)
						{
							//此处会耗时,从此处开启处理线程，处理线程释放
							data = (char*)malloc(recv_len);

							if (NULL != data)
							{
								memcpy(data, recv_buff, recv_len);
								std::thread t(&CSnmpxServer::RecvDataDealThread, this, client_ip, client_port, data, recv_len, true);
								t.detach();
							}
							else
							{
								PrintLogMsg(0, "snmpx_server recvfrom client[" + client_ip + ":" + std::to_string(client_port) 
									+ "] data deal failed, malloc data cache return NULL: " 
									+ CErrorStatus::get_err_msg(errno, true, true));
							}
						}
						else
						{
							//agent服务顺序处理
							RecvDataDealThread(client_ip, client_port, recv_buff, recv_len, false);
						}
					}
					else
					{
						PrintLogMsg(1, "snmpx_server recvfrom client data failed, recvfrom return failed: "
							+ CErrorStatus::get_err_msg(errno, true, true));
					}
				}
			}
			else
			{
				/* 返回0表示已经超时 */
			}
		}

		FD_CLR(server_fd, &rfd);
		FD_ZERO(&rfd);
#else
		/* 创建 epoll 句柄，把监听 socket 加入到 epoll 集合里 */
		int kdp_fd = epoll_create(MAX_FDP_NUM + 1);
		if (kdp_fd > 0)
		{
			struct epoll_event ev;

			ev.events = EPOLLIN; //UDP不要使用ET模式，数据太多来不及读占满队列后，不会再触发事件
			ev.data.fd = server_fd;

			if (epoll_ctl(kdp_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0)
			{
				bResult = false;
				szError = "epoll add socket fd failed: " + CErrorStatus::get_err_msg(errno, true, true);
			}
			else
			{
				m_bIsRun = true;
				int i = 0, nfds = 0;
				const int timeout = 100; //毫秒
				struct epoll_event events[MAX_FDP_NUM];

				PrintLogMsg(0, "snmpx_server start on [" + m_szIP + ":" + std::to_string(m_nPort) + "]...");

				while (m_bIsRun)
				{
					/* 等待有事件发生 */
					nfds = epoll_wait(kdp_fd, events, MAX_FDP_NUM, timeout);

					if (nfds < 0)
					{
						PrintLogMsg(0, "snmpx_server check client data for read failed, epoll_wait return error: " 
							+ CErrorStatus::get_err_msg(errno, true, true));
					}
					else if (nfds > 0)
					{
						/* 处理所有事件 */
						for (i = 0; i < nfds; ++i)
						{
							if (events[i].events & EPOLLIN && events[i].data.fd == server_fd)
							{
								client_addr_len = (socklen_t)sizeof(client_addr);
								memset(&client_addr, 0, client_addr_len);

								recv_len = recvfrom(server_fd, recv_buff, MAX_MSG_LEN, 0, (struct sockaddr*)&client_addr, 
									&client_addr_len);
								if (recv_len > 0)
								{
									//agent_ip = inet_ntoa(client_addr.sin_addr);
									memset(ip_buff, 0, sizeof(ip_buff));
									inet_ntop(AF_INET, &client_addr.sin_addr, ip_buff, sizeof(ip_buff));
									client_ip = ip_buff;
									client_port = ntohs(client_addr.sin_port);

									if (m_bIsTrapd)
									{
										//此处会耗时,从此处开启处理线程，处理线程释放
										data = (char*)malloc(recv_len);

										if (NULL != data)
										{
											memcpy(data, recv_buff, recv_len);
											std::thread t(&CSnmpxServer::RecvDataDealThread, this, client_ip, client_port, data, 
												recv_len, true);
											t.detach();
										}
										else
										{
											PrintLogMsg(0, "snmpx_server recvfrom client[" + client_ip + ":" + std::to_string(client_port) 
												+ "] data deal failed, malloc data cache return NULL: " 
												+ CErrorStatus::get_err_msg(errno, true, true));
										}
									}
									else
									{
										//agent服务顺序处理
										RecvDataDealThread(client_ip, client_port, recv_buff, recv_len, false);
									}
								}
								else
								{
									PrintLogMsg(1, "snmpx_server recvfrom client data failed, recvfrom return failed: "
										+ CErrorStatus::get_err_msg(errno, true, true));
								}
							}
						}
					}
					else
					{
						/* 返回0表示已经超时 */
					}
				}

				epoll_ctl(kdp_fd, EPOLL_CTL_DEL, server_fd, &ev);
			}

			close(kdp_fd);
		}
		else
		{
			bResult = false;
			szError = "create epoll fd failed: " + CErrorStatus::get_err_msg(errno, true, true);
		}
#endif

		PrintLogMsg(0, "snmpx_server stopped on [" + m_szIP + ":" + std::to_string(m_nPort) + "].");
		close_socket_fd(server_fd);
	}
	else
		bResult = false;

	return bResult;
}

/********************************************************************
功能：	关闭snmpx服务
参数：	无
返回：	无
*********************************************************************/
void CSnmpxServer::StopServer()
{
	m_bIsRun = false;
}

/********************************************************************
功能：	设置认证信息
参数：	version snmp版本，1：v1，2：v2c，3：v3
*		szUserName 用户或团体名
*		szError 错误信息
*		safeMode 安全模式，V3有效
*		authMode 认证hash算法，V3有效
*		szAuthPasswd 认证密码，V3有效
*		privMode 加密算法，V3有效
*		szPrivPasswd 加密密码，V3有效
*		engineID 引擎ID，V3有效
*		engineIdLen 引擎ID长度，V3有效
返回：	成功返回true
*********************************************************************/
bool CSnmpxServer::AddUserAuthorization(short version, const std::string &szUserName, std::string &szError, unsigned char safeMode, 
	unsigned char authMode, const std::string &szAuthPasswd, unsigned char privMode, const std::string &szPrivPasswd, 
	const unsigned char *engineID, unsigned int engineIdLen)
{
	bool bResult = true;
	std::string user_id;
	struct userinfo_t *pUserInfo = (struct userinfo_t *)malloc(sizeof(struct userinfo_t));
	memset(pUserInfo, 0, sizeof( struct userinfo_t ));
	
	if (pUserInfo != nullptr)
	{
		if (1 == version || 2 == version) //v1，v2c
		{
			if (szUserName.empty())
			{
				bResult = false;
				szError = "net-snmp v1/v2c团体名称不能为空！";
			}
			else
			{
				if (szUserName.length() > MAX_USER_INFO_LEN)
				{
					bResult = false;
					szError = "net-snmp v1/v2c团体名称不能大于：" + std::to_string(MAX_USER_INFO_LEN) + "字节！";
				}
				else
				{
					user_id = szUserName + (1 == version ? "_V1" : "_V2c");
					pUserInfo->version = version - 1;
					memcpy(pUserInfo->userName, szUserName.c_str(), szUserName.length());
				}
			}
		}
		else if (3 == version) //v3
		{
			if (szUserName.empty())
			{
				bResult = false;
				szError = "net-snmp v3用户名称不能为空！";
			}
			else
			{
				if (szUserName.length() > MAX_USER_INFO_LEN)
				{
					bResult = false;
					szError = "net-snmp v3用户名称不能大于：" + std::to_string(MAX_USER_INFO_LEN) + "字节！";
				}
				else
				{
					user_id = szUserName + "_V3";
					pUserInfo->version = 0x03;
					memcpy(pUserInfo->userName, szUserName.c_str(), szUserName.length());
				}
			}

			if (bResult)
			{
				if (0 == safeMode)
					pUserInfo->safeMode = safeMode;
				else if (1 == safeMode || 2 == safeMode)
				{
					pUserInfo->safeMode = safeMode;

					//认证信息
					if (authMode > 1)
					{
						bResult = false;
						szError = "不支持的认证hash算法：" + std::to_string(authMode) + ".";
					}
					else
						pUserInfo->AuthMode = authMode;

					if (bResult)
					{
						if (szAuthPasswd.empty())
						{
							bResult = false;
							szError = "net-snmp v3用户认证密码不能为空！";
						}
						else
						{
							if (szAuthPasswd.length() > MAX_USER_INFO_LEN)
							{
								bResult = false;
								szError = "net-snmp v3用户认证密码不能大于：" + std::to_string(MAX_USER_INFO_LEN) + "字节！";
							}
							else
								memcpy(pUserInfo->AuthPassword, szAuthPasswd.c_str(), szAuthPasswd.length());
						}
					}

					//加密信息
					if (bResult && 2 == safeMode)
					{
						if (privMode > 1)
						{
							bResult = false;
							szError = "不支持的加密算法：" + std::to_string(privMode) + ".";
						}
						else
							pUserInfo->PrivMode = privMode;

						if (bResult)
						{
							if (szPrivPasswd.empty())
							{
								bResult = false;
								szError = "net-snmp v3用户加密密码不能为空！";
							}
							else
							{
								if (szPrivPasswd.length() > MAX_USER_INFO_LEN)
								{
									bResult = false;
									szError = "net-snmp v3用户加密密码不能大于：" + std::to_string(MAX_USER_INFO_LEN) + "字节！";
								}
								else
									memcpy(pUserInfo->PrivPassword, szPrivPasswd.c_str(), szPrivPasswd.length());
							}
						}
					}

					//生成认证信息
					if (bResult)
					{
						if (NULL != engineID && engineIdLen > 0)
						{
							if (engineIdLen < 5 || engineIdLen > 32)
							{
								bResult = false;
								szError = "引擎ID长度错误，只能是5-32字节！";
							}
							else
							{
								pUserInfo->msgAuthoritativeEngineID_len = engineIdLen;
								pUserInfo->msgAuthoritativeEngineID = (unsigned char*)malloc(pUserInfo->msgAuthoritativeEngineID_len);

								if (NULL != pUserInfo->msgAuthoritativeEngineID)
								{
									CUserProccess user_proc;
									memcpy(pUserInfo->msgAuthoritativeEngineID, engineID, pUserInfo->msgAuthoritativeEngineID_len);

									if (SNMPX_noError != user_proc.snmpx_user_init(pUserInfo))
									{
										bResult = false;
										szError = "生成用户认证信息失败：" + user_proc.GetErrorMsg();
									}
								}
								else
								{
									bResult = false;
									szError = "创建引擎ID缓存失败，malloc return NULL: " + CErrorStatus::get_err_msg(errno, true, true);
								}
							}
						}
					}
				}
				else
				{
					bResult = false;
					szError = "不支持的安全模式：" + std::to_string(safeMode) + ".";
				}
			}
		}
		else
		{
			bResult = false;
			szError = "不支持的版本号：" + std::to_string(version) + ".";
		}

		//成功插入，失败则释放对象
		if (bResult)
			m_UserTable.insert(std::make_pair(user_id, pUserInfo));
		else
			CUserProccess::snmpx_user_free(pUserInfo);
	}
	else
	{
		bResult = false;
		szError = "创建用户对象失败，malloc return NULL: " + CErrorStatus::get_err_msg(errno, true, true);
	}

	return bResult;
}

/********************************************************************
功能：	检查socket端口是否被占用
参数：	ip IP
*		port 端口
*		is_udp 协议类型，true:UDP, false:TCP
返回：	占用返回true
*********************************************************************/
bool CSnmpxServer::CheckSocketPortUsed(const string &ip, unsigned short port, bool is_udp)
{
	std::string szError;

	if (!init_win32_socket_env(szError)) {
		return false;
	}

	bool bRes = false;
	int fd = 0;

	if (is_udp)
		fd = (int)socket(AF_INET, SOCK_DGRAM, 0);
	else
		fd = (int)socket(AF_INET, SOCK_STREAM, 0);

	if (fd > 0)
	{
		struct sockaddr_in addr;

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (ip.empty())
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
		else
			inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

		if (0 > ::bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
			bRes = true;
		}

		close_socket_fd(fd);
	}

	free_win32_socket_env();

	return bRes;
}

/********************************************************************
功能：	获取绑定数据字符串
参数：	sysuptime 系统启动时间
*		enterprise_id 企业OID
*		vb_list 绑定数据
*		max_oid_string_len 字符串OID最大长度，输出使用，最小22，最大64
返回：	绑定数据字符串
*********************************************************************/
string CSnmpxServer::GetItemsPrintString(unsigned int *sysuptime, const string *enterprise_id, const list<SSnmpxValue*> &vb_list,
	size_t max_oid_string_len)
{
	if (max_oid_string_len > 128)
		max_oid_string_len = 128;

	string szResult;
	const string oid_format = "%-" + std::to_string(max_oid_string_len) + "s";
	const unsigned int buff_len = 512;
	char buff[buff_len] = { '\0' };

	if (nullptr != sysuptime)
	{
		snprintf(buff, buff_len, (oid_format + " => %10s : %s\n").c_str(), trapSystemUpTime, "sysuptime", 
			get_timeticks_string(*sysuptime).c_str());
		szResult = buff;
	}

	if (nullptr != enterprise_id)
	{
		memset(buff, 0, buff_len);
		snprintf(buff, buff_len, (oid_format + " => %10s : %s\n").c_str(), trapEnterpriseID, "enterprise", enterprise_id->c_str());
		if (szResult.empty())
			szResult = buff;
		else
			szResult.append(buff);
	}

	for (const auto &p : vb_list)
	{
		memset(buff, 0, buff_len);

		switch (p->cValType)
		{
		case SNMPX_ASN_INTEGER:
			snprintf(buff, buff_len, (oid_format + " => %10s : %d\n").c_str(), p->szOid.c_str(), "ingeter", p->Val.num.i);
			break;
		case SNMPX_ASN_NULL:
			snprintf(buff, buff_len, (oid_format + " => %10s : \n").c_str(), p->szOid.c_str(), "null");
			break;
		case SNMPX_ASN_UNSIGNED:
			snprintf(buff, buff_len, (oid_format + " => %10s : %u\n").c_str(), p->szOid.c_str(), "unsigned", p->Val.num.u);
			break;
		case SNMPX_ASN_INTEGER64:
			snprintf(buff, buff_len, (oid_format + " => %10s : %lld\n").c_str(), p->szOid.c_str(), "ingeter64", p->Val.num.ll);
			break;
		case SNMPX_ASN_UNSIGNED64:
			snprintf(buff, buff_len, (oid_format + " => %10s : %llu\n").c_str(), p->szOid.c_str(), "unsigned64", p->Val.num.ull);
			break;
		case SNMPX_ASN_FLOAT:
			snprintf(buff, buff_len, (oid_format + " => %10s : %0.3f\n").c_str(), p->szOid.c_str(), "float", p->Val.num.f);
			break;
		case SNMPX_ASN_DOUBLE:
			snprintf(buff, buff_len, (oid_format + " => %10s : %0.3lf\n").c_str(), p->szOid.c_str(), "double", p->Val.num.d);
			break;
		case SNMPX_ASN_OCTET_STR:
			if (p->Val.hex_str)
				snprintf(buff, buff_len, (oid_format + " => %10s : %s\n").c_str(), p->szOid.c_str(), "hex-string", p->Val.str.c_str());
			else
				snprintf(buff, buff_len, (oid_format + " => %10s : %s\n").c_str(), p->szOid.c_str(), "string", p->Val.str.c_str());
			break;
		case SNMPX_ASN_IPADDRESS:
			snprintf(buff, buff_len, (oid_format + " => %10s : %s\n").c_str(), p->szOid.c_str(), "ipaddress", 
				get_ipaddress_string(p->Val.num.i).c_str());
			break;
		case SNMPX_ASN_UNSUPPORT:
			snprintf(buff, buff_len, (oid_format + " => %10s : %s\n").c_str(), p->szOid.c_str(), "unsupport", p->Val.str.c_str());
			break;
		case SNMPX_ASN_NO_SUCHOBJECT:
			snprintf(buff, buff_len, (oid_format + " => invalid : no such object.\n").c_str(), p->szOid.c_str());
			break;
		default:
			snprintf(buff, buff_len, (oid_format + " => tag 0x%02X : unknow print.\n").c_str(), p->szOid.c_str(), p->cValType);
			break;
		}

		if (szResult.empty())
			szResult = buff;
		else
			szResult.append(buff);
	}

	return std::move(szResult);
}

/********************************************************************
功能：	接收数据处理
参数：	client_ip
*		client_port
*		data 数据
*		data_len 数据长度
*		free_data 是否释放data
返回：	无
*********************************************************************/
void CSnmpxServer::RecvDataDealThread(const string client_ip, unsigned short client_port, char *data, unsigned int data_len, 
	bool free_data)
{
	struct snmpx_t snmpx_rc;
	CSnmpxUnpack unpack;

	init_snmpx_t(&snmpx_rc);
	memcpy(snmpx_rc.ip, client_ip.c_str(), client_ip.length());
	snmpx_rc.remote_port = client_port;

	//解包
	if (unpack.snmpx_group_unpack((unsigned char*)data, data_len, &snmpx_rc, false, &m_UserTable, true, 
		m_UserAuthInfoCache) < 0)
	{
		PrintLogMsg(0, "snmpx_server recvfrom client[" + client_ip + ":" + std::to_string(client_port) + "] data unpack failed: "
			+ unpack.GetErrorMsg() + " data:\n" + get_hex_print_string((unsigned char*)data, data_len, 0));
	}
	else
	{
		//解析值
		string szEnterpriseID;
		unsigned int uiSystemUpTime = 0;
		size_t max_oid_string_len = 0;
		std::list<SSnmpxValue*> itemList;
		std::list<struct variable_bindings*>::const_iterator variter;
		SSnmpxValue *pSnmpxValue = nullptr;

		if (SNMPX_MSG_TRAP == snmpx_rc.tag)
		{
			//v1 trap的企业OID
			szEnterpriseID = get_oid_string(snmpx_rc.enterprise_oid, snmpx_rc.enterprise_oid_len);
			max_oid_string_len = szEnterpriseID.length();
		}

		for (variter = snmpx_rc.variable_bindings_list->cbegin(); variter != snmpx_rc.variable_bindings_list->cend(); ++variter)
		{
			pSnmpxValue = new SSnmpxValue();

			CSnmpxUnpack::snmpx_get_vb_value(*variter, pSnmpxValue);

			if (pSnmpxValue->szOid.length() > max_oid_string_len)
				max_oid_string_len = pSnmpxValue->szOid.length();

			if (SNMPX_MSG_TRAP != snmpx_rc.tag)
			{
				if (11 == pSnmpxValue->OidBuf[0]
					&& SNMPX_ASN_OCTET_STR == pSnmpxValue->cValType
					&& trapEnterpriseID == pSnmpxValue->szOid)
				{
					szEnterpriseID = pSnmpxValue->Val.str; //获取企业OID
					delete pSnmpxValue;
					pSnmpxValue = nullptr;
				}
				else if (9 == pSnmpxValue->OidBuf[0]
					&& SNMPX_ASN_UNSIGNED == pSnmpxValue->cValType
					&& trapSystemUpTime == pSnmpxValue->szOid)
				{
					uiSystemUpTime = pSnmpxValue->Val.num.u; //获取系统启动时间
					delete pSnmpxValue;
					pSnmpxValue = nullptr;
				}
				else
					itemList.push_back(pSnmpxValue);
			}
			else
				itemList.push_back(pSnmpxValue);
		}

		//处理
		if (m_bIsTrapd)
		{
			if (SNMPX_MSG_TRAP == snmpx_rc.tag)
				Trap_Handle(client_ip, client_port, szEnterpriseID, snmpx_rc.generic_trap, snmpx_rc.specific_trap, 
					snmpx_rc.time_stamp, itemList, max_oid_string_len);
			else if (SNMPX_MSG_TRAP2 == snmpx_rc.tag)
				Trap2_Handle(client_ip, client_port, szEnterpriseID, uiSystemUpTime, itemList, max_oid_string_len);
			else
			{
				PrintLogMsg(1, "snmpx_server recvfrom client[" + client_ip + ":" + std::to_string(client_port) + "] data handle failed: "
					"is a trapd server, tag: 0x" + get_hex_string(&snmpx_rc.tag, 1) + " data:\n" 
					+ get_hex_print_string((unsigned char*)data, data_len, 4));
			}
		}
		else
		{
			if (SNMPX_MSG_TRAP != snmpx_rc.tag && SNMPX_MSG_TRAP2 != snmpx_rc.tag)
			{
				std::list<SSnmpxValue> itemList_sd;

				//注意error_status和error_index复用
				Request_Handle(client_ip, client_port, snmpx_rc.tag, itemList, snmpx_rc.error_status, snmpx_rc.error_index, itemList_sd,
					max_oid_string_len);

				//对itemList_sd处理
				//组包发送
				//...
			}
			else
			{
				PrintLogMsg(1, "snmpx_server recvfrom client[" + client_ip + ":" + std::to_string(client_port) + "] data handle failed: "
					"is not a trapd server, tag: 0x" + get_hex_string(&snmpx_rc.tag, 1) + " data:\n" 
					+ get_hex_print_string((unsigned char*)data, data_len, 4));
			}
		}

		for (auto &p : itemList)
		{
			delete p;
			p = nullptr;
		}

		itemList.clear();
	}

	free_snmpx_t(&snmpx_rc);

	if (free_data)
		free(data); //这里释放

	return;
}

/********************************************************************
功能：	trap v1版本处理
参数：	agent_ip
*		agent_port
*		enterprise_oid 企业OID
*		generic_trap trap 类型
*		generic_trap 特别码
*		time_stamp agent运行时间，timeticks
*		vb_list 绑定数据
*		max_oid_string_len 收到的item中oid转字符串后最大长度，输出使用
返回：	无
*********************************************************************/
void CSnmpxServer::Trap_Handle(const string &agentIP, unsigned short agentPort, const string &enterprise_oid, int generic_trap,
	int specific_trap, unsigned int time_stamp, const list<SSnmpxValue*> &vb_list, size_t max_oid_string_len)
{
	PrintLogMsg(2, "snmpx_server recvfrom agent[" + agentIP + ":" + std::to_string(agentPort) + "] v1 trap data: \n"
		+ GetItemsPrintString(&time_stamp, &enterprise_oid, vb_list, max_oid_string_len));
}

/********************************************************************
功能：	trap v2c版本处理
参数：	agent_ip
*		agent_port
*		enterprise_oid 企业OID
*		vb_list 绑定数据
*		max_oid_string_len 收到的item中oid转字符串后最大长度，输出使用
返回：	无
*********************************************************************/
void CSnmpxServer::Trap2_Handle(const string &agentIP, unsigned short agentPort, const string &enterprise_oid, 
	unsigned int sysuptime, const list<SSnmpxValue*> &vb_list, size_t max_oid_string_len)
{
	PrintLogMsg(2, "snmpx_server recvfrom agent[" + agentIP + ":" + std::to_string(agentPort) + "] v2 trap data: \n"
		+ GetItemsPrintString(&sysuptime, &enterprise_oid, vb_list, max_oid_string_len));
}

/********************************************************************
功能：	其它PDU类型处理
参数：	client_ip
*		client_port
*		tag 请求类型
*		vb_list 绑定数据
*		error_stat 错误状态，当是getbluk时，复用
*		error_index 错误索引，当是getbluk时，复用
*		svb_list 返回绑定数据
*		max_oid_string_len 收到的item中oid转字符串后最大长度，输出使用
返回：	无
*********************************************************************/
void CSnmpxServer::Request_Handle(const string &clientIP, unsigned short clientPort, unsigned char tag, 
	const list<SSnmpxValue*> &rvb_list, int &error_stat, int &error_index, list<SSnmpxValue> &svb_list, size_t max_oid_string_len)
{
	PrintLogMsg(2, "snmpx_server recvfrom client[" + clientIP + ":" + std::to_string(clientPort) + "] request, tag: 0x" 
		+ get_hex_string(&tag, 1) + " data: \n" + GetItemsPrintString(nullptr, nullptr, rvb_list, max_oid_string_len));
}

/********************************************************************
功能：	输出日志消息
参数：	nLevel 日志级别，0：ALL，1：WARN，2：PROMPT，3：DEBUG
*		szMsg 日志
返回：	无
*********************************************************************/
void CSnmpxServer::PrintLogMsg(unsigned short nLevel, const string &szMsg)
{
	if (!szMsg.empty())
	{
		printf("%s >> level[%hu]: %s\n", get_current_time_string().c_str(), nLevel, szMsg.c_str());
	}
}