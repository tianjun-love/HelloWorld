#include "../include/snmpx_client.hpp"
#include "../include/snmpx_user_proccess.hpp"
#include "../include/cryptography_proccess.hpp"
#include "../include/snmpx_pack.hpp"
#include "../include/snmpx_unpack.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>
#else
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "OS/include/Ping.hpp"

//walk结束标识OID(测试时发现可以使用，本身是agent在snmpSet中使用)，第0个位置存放长度
static const oid walkEndOid[] = { 11, 1, 3, 6, 1, 6, 3, 1, 1, 6, 1, 0 };

std::mutex* CSnmpxClient::m_privUSMLock = new std::mutex();
std::map<string, CSnmpxClient::SUserUsmInfo*>* CSnmpxClient::m_privUSMMap = new std::map<string, CSnmpxClient::SUserUsmInfo*>();
int CSnmpxClient::m_msgID = 1;
std::mutex* CSnmpxClient::m_msgIDLock = new std::mutex();

/********************************************************************
功能：	初始化socket地址结构体及申请socket句柄
参数：	ip agent IP
*		port agent端口
*		fd socket句柄
*		timeout 超时，毫秒
*		client_addr 地址结构体
*		szError 错误信息
返回：	成功返回true
*********************************************************************/
static bool InitSocketFd(const std::string &ip, unsigned short port, long timeout, int& fd, sockaddr_in& client_addr, string& szError)
{
	//申请socket句柄，udp
	fd = (int)socket(AF_INET, SOCK_DGRAM, 0);

	//成功后设置超时
	if (fd > 0)
	{
#ifdef _WIN32
		//发送时限，不用设置，UDP一定能发出去
		//setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

		//接收时限
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
		//linux设置超时必须这样
		struct timeval tv;

		tv.tv_sec = timeout / 1000; //秒
		tv.tv_usec = (timeout % 1000) * 1000; //微秒

		//发送时限，不用设置，UDP一定能发出去
		//setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(struct timeval));

		//接收时限
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
#endif
	}
	else
	{
		szError = CErrorStatus::get_err_msg(errno, true, true);
		return false;
	}

	//设置agent地址信息
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);
	//client_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	if (ip.empty())
		client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		inet_pton(AF_INET, ip.c_str(), &client_addr.sin_addr);

	return true;
}

CSnmpxClient::SUserUsmInfo::SUserUsmInfo() : safeMode(0), msgAuthoritativeEngineID(NULL), msgAuthoritativeEngineID_len(0),
authPasswordPrivKey(NULL), authPasswordPrivKey_len(0), privPasswdPrivKey(NULL), privPasswdPrivKey_len(0)
{
}

CSnmpxClient::SUserUsmInfo::SUserUsmInfo(const userinfo_t* us) : 
	msgAuthoritativeEngineID(NULL), msgAuthoritativeEngineID_len(0), authPasswordPrivKey(NULL), 
	authPasswordPrivKey_len(0), privPasswdPrivKey(NULL), privPasswdPrivKey_len(0)
{
	memcpy(userName, us->userName, SNMPX_MAX_USER_NAME_LEN);
	memcpy(userAuthPassword, us->AuthPassword, SNMPX_MAX_USM_AUTH_KU_LEN);
	memcpy(userPrivPassword, us->PrivPassword, SNMPX_MAX_USM_PRIV_KU_LEN);
	safeMode = us->safeMode;
	authMode = us->AuthMode;
	privMode = us->PrivMode;

	if (us->msgAuthoritativeEngineID != NULL && us->msgAuthoritativeEngineID_len > 0)
	{
		msgAuthoritativeEngineID_len = us->msgAuthoritativeEngineID_len;
		msgAuthoritativeEngineID = (unsigned char*)malloc(msgAuthoritativeEngineID_len);
		memcpy(msgAuthoritativeEngineID, us->msgAuthoritativeEngineID, msgAuthoritativeEngineID_len);
	}

	if (us->authPasswordPrivKey != NULL && us->authPasswordPrivKey_len > 0)
	{
		authPasswordPrivKey_len = us->authPasswordPrivKey_len;
		authPasswordPrivKey = (unsigned char*)malloc(authPasswordPrivKey_len);
		memcpy(authPasswordPrivKey, us->authPasswordPrivKey, authPasswordPrivKey_len);
	}

	if (us->privPasswdPrivKey != NULL && us->privPasswdPrivKey_len > 0)
	{
		privPasswdPrivKey_len = us->privPasswdPrivKey_len;
		privPasswdPrivKey = (unsigned char*)malloc(privPasswdPrivKey_len);
		memcpy(privPasswdPrivKey, us->privPasswdPrivKey, privPasswdPrivKey_len);
	}
}

CSnmpxClient::SUserUsmInfo::~SUserUsmInfo()
{
	Clear();
}

void CSnmpxClient::SUserUsmInfo::Clear()
{
	if (NULL != msgAuthoritativeEngineID)
	{
		free(msgAuthoritativeEngineID);
		msgAuthoritativeEngineID = NULL;
		msgAuthoritativeEngineID_len = 0;
	}

	if (NULL != authPasswordPrivKey)
	{
		free(authPasswordPrivKey);
		authPasswordPrivKey = NULL;
		authPasswordPrivKey_len = 0;
	}

	if (NULL != privPasswdPrivKey)
	{
		free(privPasswdPrivKey);
		privPasswdPrivKey = NULL;
		privPasswdPrivKey_len = 0;
	}
}

bool CSnmpxClient::SUserUsmInfo::operator == (const userinfo_t& us) const
{
	if (0 == memcmp(userName, us.userName, SNMPX_MAX_USER_NAME_LEN)
		&& 0 == memcmp(userAuthPassword, us.AuthPassword, SNMPX_MAX_USM_AUTH_KU_LEN)
		&& 0 == memcmp(userPrivPassword, us.PrivPassword, SNMPX_MAX_USM_PRIV_KU_LEN)
		&& safeMode == us.safeMode
		&& authMode == us.AuthMode
		&& privMode == us.PrivMode
		&& msgAuthoritativeEngineID_len == us.msgAuthoritativeEngineID_len)
	{
		if (msgAuthoritativeEngineID_len >= 1)
		{
			if (0 != memcmp(msgAuthoritativeEngineID, us.msgAuthoritativeEngineID, msgAuthoritativeEngineID_len))
			{
				return false;
			}
		}
	}
	else
		return false;

	return true;
}

CSnmpxClient::CSnmpxClient(const std::string &ip, unsigned short port, long timeout, unsigned char retry_times, bool ping_on_timeout) : 
	m_szIP(ip), m_nPort(port), m_lTimeout(timeout), m_cRetryTimes(retry_times), m_bPingOnTimeout(ping_on_timeout), m_pUserInfo(NULL)
{
	if (ip.empty()){
		m_szIP = "0.0.0.0";
	}

	//超时最小500毫秒，不能是负值
	if (timeout < 500)
		m_lTimeout = 500;
	else if (timeout > 30000)
		m_lTimeout = 30000;
}

CSnmpxClient::~CSnmpxClient()
{
	if (NULL != m_pUserInfo)
	{
		free_userinfo_data(m_pUserInfo);
		m_pUserInfo = NULL;
	}
}

/********************************************************************
功能：	设置认证信息
参数：	version snmp版本
*		szUserName 用户或团体名
*		szError 错误信息
*		safeMode 安全模式，V3有效
*		authMode 认证hash算法，V3有效
*		szAuthPasswd 认证密码，V3有效
*		privMode 加密算法，V3有效
*		szPrivPasswd 加密密码，V3有效
返回：	成功返回true
*********************************************************************/
bool CSnmpxClient::SetAuthorizationInfo(unsigned char version, const std::string &szUserName, std::string &szError, unsigned char safeMode,
	unsigned char authMode, const std::string &szAuthPasswd, unsigned char privMode, const std::string &szPrivPasswd)
{
	bool bResult = true;

	if (SNMPX_VERSION_v1 == version || SNMPX_VERSION_v2c == version) //v1，v2c
	{
		if (szUserName.empty())
		{
			bResult = false;
			szError = "net-snmp v1/v2c团体名称不能为空！";
		}
		else
		{
			if (szUserName.length() > SNMPX_MAX_USER_NAME_LEN)
			{
				bResult = false;
				szError = "net-snmp v1/v2c团体名称不能大于：" + std::to_string(SNMPX_MAX_USER_NAME_LEN) + "字节！";
			}
			else
			{
				if (NULL != m_pUserInfo)
				{
					free_userinfo_data(m_pUserInfo);
					m_pUserInfo = NULL;
				}

				m_pUserInfo = (struct userinfo_t*)malloc(sizeof(struct userinfo_t));

				if (NULL != m_pUserInfo)
				{
					memset(m_pUserInfo, 0x00, sizeof(struct userinfo_t));

					m_pUserInfo->version = version;
					memcpy(m_pUserInfo->userName, szUserName.c_str(), szUserName.length());
				}
				else
				{
					bResult = false;
					szError = "创建用户认证信息对象失败，malloc返回NULL：" + CErrorStatus::get_err_msg(errno, true, true);
				}
			}
		}
	}
	else if (SNMPX_VERSION_v3 == version) //v3
	{
		if (szUserName.empty())
		{
			bResult = false;
			szError = "net-snmp v3用户名称不能为空！";
		}
		else
		{
			if (szUserName.length() > SNMPX_MAX_USER_NAME_LEN)
			{
				bResult = false;
				szError = "net-snmp v3用户名称不能大于：" + std::to_string(SNMPX_MAX_USER_NAME_LEN) + "字节！";
			}
			else
			{
				if (NULL != m_pUserInfo)
				{
					free_userinfo_data(m_pUserInfo);
					m_pUserInfo = NULL;
				}

				m_pUserInfo = (struct userinfo_t*)malloc(sizeof(struct userinfo_t));

				if (NULL != m_pUserInfo)
				{
					memset(m_pUserInfo, 0x00, sizeof(struct userinfo_t));

					m_pUserInfo->version = version;
					memcpy(m_pUserInfo->userName, szUserName.c_str(), szUserName.length());
				}
				else
				{
					bResult = false;
					szError = "创建用户认证信息对象失败，malloc返回NULL：" + CErrorStatus::get_err_msg(errno, true, true);
				}
			}
		}

		if (bResult)
		{
			if (SNMPX_SEC_LEVEL_noAuth == safeMode)
				m_pUserInfo->safeMode = safeMode;
			else if (SNMPX_SEC_LEVEL_authNoPriv == safeMode || SNMPX_SEC_LEVEL_authPriv == safeMode)
			{
				m_pUserInfo->safeMode = safeMode;

				//认证信息
				if (authMode > SNMPX_AUTH_SHA512)
				{
					bResult = false;
					szError = "不支持的认证hash算法：" + std::to_string(authMode) + ".";
				}
				else
					m_pUserInfo->AuthMode = authMode;

				if (bResult)
				{
					if (szAuthPasswd.empty())
					{
						bResult = false;
						szError = "net-snmp v3用户认证密码不能为空！";
					}
					else
					{
						if (szAuthPasswd.length() > SNMPX_MAX_USM_AUTH_KU_LEN)
						{
							bResult = false;
							szError = "net-snmp v3用户认证密码不能大于：" + std::to_string(SNMPX_MAX_USM_AUTH_KU_LEN) + "字节！";
						}
						else
							memcpy(m_pUserInfo->AuthPassword, szAuthPasswd.c_str(), szAuthPasswd.length());
					}
				}

				//加密信息
				if (bResult && SNMPX_SEC_LEVEL_authPriv == safeMode)
				{
					if (privMode > SNMPX_PRIV_AES256)
					{
						bResult = false;
						szError = "不支持的加密算法：" + std::to_string(privMode) + ".";
					}
					else
						m_pUserInfo->PrivMode = privMode;

					if (bResult)
					{
						if (szPrivPasswd.empty())
						{
							bResult = false;
							szError = "net-snmp v3用户加密密码不能为空！";
						}
						else
						{
							if (szPrivPasswd.length() > SNMPX_MAX_USM_PRIV_KU_LEN)
							{
								bResult = false;
								szError = "net-snmp v3用户加密密码不能大于：" + std::to_string(SNMPX_MAX_USM_PRIV_KU_LEN) + "字节！";
							}
							else
								memcpy(m_pUserInfo->PrivPassword, szPrivPasswd.c_str(), szPrivPasswd.length());
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

	//如果失败则释放对象
	if (!bResult && NULL != m_pUserInfo)
	{
		free_userinfo_data(m_pUserInfo);
		m_pUserInfo = NULL;
	}

	return bResult;
}

/********************************************************************
功能：	snmpx get数据
参数：	szOid get的oid信息
*		value 返回的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Get(const string& szOid, SSnmpxValue& value, string& szError)
{
	int irval = 0;
	std::vector<SSnmpxValue> valueVec;

	irval = SnmpxGetHandle(E_OID_STRING, &szOid, valueVec, szError);

	if (SNMPX_noError == irval)
	{
		if (valueVec.size() > 0)
			value = valueVec[0];
		else
		{
			irval = SNMPX_failure;
			szError = "获取结果为空！";
		}
	}

	return irval;
}

/********************************************************************
功能：	snmpx get数据
参数：	oidList get的oid信息
*		valueVec 返回的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Get(const list<string>& oidList, std::vector<SSnmpxValue>& valueVec, string& szError)
{
	return SnmpxGetHandle(E_OID_STRING_SET, &oidList, valueVec, szError);
}

/********************************************************************
功能：	snmpx get数据
参数：	pOid get的oid信息，OID的第一个位置存放的是长度
*		value 返回的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Get(const oid* pOid, SSnmpxValue& value, string& szError)
{
	int irval = 0;
	std::vector<SSnmpxValue> valueVec;

	irval = SnmpxGetHandle(E_OID_POINTER, pOid, valueVec, szError);

	if (SNMPX_noError == irval)
	{
		if (valueVec.size() > 0)
			value = valueVec[0];
		else
		{
			irval = SNMPX_failure;
			szError = "获取结果为空！";
		}
	}

	return irval;
}

/********************************************************************
功能：	snmpx get数据
参数：	oidList get的oid信息，OID的第一个位置存放的是长度
*		valueVec 返回的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Get(const list<const oid*>& oidList, std::vector<SSnmpxValue>& valueVec, string& szError)
{
	return SnmpxGetHandle(E_OID_POINTER_SET, &oidList, valueVec, szError);
}

/********************************************************************
功能：	snmpx get数据处理
参数：	oidType OID类型
*		pOid get的oid信息
*		valueVec 返回的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::SnmpxGetHandle(EOidType oidType, const void* pOid, std::vector<SSnmpxValue>& valueVec, string& szError)
{
	if (NULL == m_pUserInfo)
	{
		szError = "未设置认证信息！";
		return SNMPX_failure;
	}

	int irval = SNMPX_noError;
	struct snmpx_t snmpx_rc1; //第一发送数据结构体
	struct snmpx_t snmpx_sd1; //第一接收数据结构体
	struct snmpx_t snmpx_rc2; //第二次发送数据结构体
	struct snmpx_t snmpx_sd2; //第二次接收数据结构体

	/* 初始化 */
	init_snmpx_t(&snmpx_rc1);
	init_snmpx_t(&snmpx_sd1);
	init_snmpx_t(&snmpx_rc2);
	init_snmpx_t(&snmpx_sd2);

	InitPublicAttr(snmpx_sd1);
	snmpx_sd1.tag = SNMPX_MSG_GET;

	if (SNMPX_VERSION_v3 != m_pUserInfo->version) //V2版本
	{
		//请求
		irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, true, oidType, pOid, &valueVec, szError);

		if (SNMPX_noError != irval) {
			szError = "获取数据失败：" + szError;
		}
	}
	else //V3版本
	{
		//第一帧获取引擎ID
		irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, true, E_OID_NONE, NULL, NULL, szError, true);

		if (SNMPX_noError != irval) {
			szError = "获取引擎ID失败：" + szError;
		}
		else
		{
			//第二帧获取数据
			InitPublicAttr(snmpx_sd2, true);
			snmpx_sd2.tag = SNMPX_MSG_GET;

			//赋值引擎及随机数信息
			if (FillSnmpxEngineAndPrivacy(snmpx_rc1, snmpx_sd2, szError))
			{
				//计算密钥 , 这个比较占用CPU,建议如果第二次做GET操作的引擎ID也是一样,就不要再算第二次了
				InitUserPriv(snmpx_rc1);

				//请求
				irval = RequestSnmpx(snmpx_sd2, snmpx_rc2, true, oidType, pOid, &valueVec, szError);

				if (SNMPX_noError != irval) {
					szError = "获取数据失败：" + szError;
				}
			}
			else
				irval = SNMPX_failure;
		}
	}

	//清理
	free_snmpx_t(&snmpx_rc1); //释放内存
	free_snmpx_t(&snmpx_sd1); //释放内存
	free_snmpx_t(&snmpx_rc2); //释放内存
	free_snmpx_t(&snmpx_sd2); //释放内存

	//失败清除用户信息，下次重新生成
	if (irval != SNMPX_noError) {
		EraseUsmUser(m_szIP);
	}

	return irval;
}

/********************************************************************
功能：	snmpx getnext数据
参数：	szOid getnext的oid信息
*		oidValue 返回的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Getnext(const string& szOid, SSnmpxValue& value, string& szError)
{
	int irval = 0;
	std::vector<SSnmpxValue> valueVec;

	irval = SnmpxGetnextHandle(E_OID_POINTER, &szOid, valueVec, szError);

	if (SNMPX_noError == irval)
	{
		if (valueVec.size() > 0)
			value = valueVec[0];
		else
		{
			irval = SNMPX_failure;
			szError = "获取结果为空！";
		}
	}

	return irval;
}

/********************************************************************
功能：	snmpx getnext数据
参数：	pOid getnext的oid信息，OID的第一个位置存放的是长度
*		oidValue 返回的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Getnext(const oid* pOid, SSnmpxValue& value, string& szError)
{
	int irval = 0;
	std::vector<SSnmpxValue> valueVec;

	irval = SnmpxGetnextHandle(E_OID_POINTER, pOid, valueVec, szError);

	if (SNMPX_noError == irval)
	{
		if (valueVec.size() > 0)
			value = valueVec[0];
		else
		{
			irval = SNMPX_failure;
			szError = "获取结果为空！";
		}
	}

	return irval;
}

/********************************************************************
功能：	snmpx getnext数据处理
参数：	oidType OID类型
*		pOid getnext的oid信息
*		oidValue 返回的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::SnmpxGetnextHandle(EOidType oidType, const void* pOid, std::vector<SSnmpxValue>& valueVec, string& szError)
{
	if (NULL == m_pUserInfo)
	{
		szError = "未设置认证信息！";
		return SNMPX_failure;
	}

	int irval = SNMPX_noError;
	struct snmpx_t snmpx_rc1; //第一发送数据结构体
	struct snmpx_t snmpx_sd1; //第一接收数据结构体
	struct snmpx_t snmpx_rc2; //第二次发送数据结构体
	struct snmpx_t snmpx_sd2; //第二次接收数据结构体

	/* 初始化 */
	init_snmpx_t(&snmpx_rc1);
	init_snmpx_t(&snmpx_sd1);
	init_snmpx_t(&snmpx_rc2);
	init_snmpx_t(&snmpx_sd2);

	InitPublicAttr(snmpx_sd1);

	if (SNMPX_VERSION_v3 != m_pUserInfo->version) //V2版本
	{
		snmpx_sd1.tag = SNMPX_MSG_GETNEXT;

		//请求
		irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, true, oidType, pOid, &valueVec, szError);

		if (SNMPX_noError != irval) {
			szError = "获取数据失败：" + szError;
		}
	}
	else //V3版本
	{
		snmpx_sd1.tag = SNMPX_MSG_GET;

		//第一帧获取引擎ID
		irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, true, E_OID_NONE, NULL, NULL, szError, true);

		if (SNMPX_noError != irval) {
			szError = "获取引擎ID失败：" + szError;
		}
		else
		{
			//第二帧获取数据
			InitPublicAttr(snmpx_sd2, true);
			snmpx_sd2.tag = SNMPX_MSG_GETNEXT;

			//赋值引擎及随机数信息
			if (FillSnmpxEngineAndPrivacy(snmpx_rc1, snmpx_sd2, szError))
			{
				//计算密钥 , 这个比较占用CPU,建议如果第二次做GET操作的引擎ID也是一样,就不要再算第二次了
				InitUserPriv(snmpx_rc1);

				//请求
				irval = RequestSnmpx(snmpx_sd2, snmpx_rc2, true, oidType, pOid, &valueVec, szError);

				if (SNMPX_noError != irval) {
					szError = "获取数据失败：" + szError;
				}
			}
			else
				irval = SNMPX_failure;
		}
	}

	//清理
	free_snmpx_t(&snmpx_rc1); //释放内存
	free_snmpx_t(&snmpx_sd1); //释放内存
	free_snmpx_t(&snmpx_rc2); //释放内存
	free_snmpx_t(&snmpx_sd2); //释放内存

	//失败清除用户信息，下次重新生成
	if (SNMPX_noError != irval) {
		EraseUsmUser(m_szIP);
	}

	return irval;
}

/********************************************************************
功能：	snmpx set数据
参数：	value 设置的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Set(const SSnmpxValue& value, string& szError)
{
	return SnmpxSetHandle(E_OID_STRING, &value, szError);
}

/********************************************************************
功能：	snmpx set数据
参数：	valueList 设置的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Set(const list<SSnmpxValue>& valueList, string& szError)
{
	return SnmpxSetHandle(E_OID_STRING_SET, &valueList, szError);
}

/********************************************************************
功能：	snmpx set数据
参数：	value 设置的oid值信息，OID的第一个位置存放的是长度
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Set(const std::pair<const oid*, SSnmpxValue>& value, string& szError)
{
	return SnmpxSetHandle(E_OID_POINTER, &value, szError);
}

/********************************************************************
功能：	snmpx set数据
参数：	valueList 设置的oid值信息，OID的第一个位置存放的是长度
*		szError 错误信息
返回：	成功返回true
*********************************************************************/
int CSnmpxClient::Set(const list<std::pair<const oid*, SSnmpxValue>>& valueList, string& szError)
{
	return SnmpxSetHandle(E_OID_POINTER_SET, &valueList, szError);
}

/********************************************************************
功能：	snmpx set数据
参数：	pValue 设置的oid值信息
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::SnmpxSetHandle(EOidType oidType, const void* pValue, string& szError)
{
	if (NULL == m_pUserInfo)
	{
		szError = "未设置认证信息！";
		return SNMPX_failure;
	}

	int irval = SNMPX_noError;
	struct snmpx_t snmpx_rc1; //第一发送数据结构体
	struct snmpx_t snmpx_sd1; //第一接收数据结构体
	struct snmpx_t snmpx_rc2; //第二次发送数据结构体
	struct snmpx_t snmpx_sd2; //第二次接收数据结构体

	/* 初始化 */
	init_snmpx_t(&snmpx_rc1);
	init_snmpx_t(&snmpx_sd1);
	init_snmpx_t(&snmpx_rc2);
	init_snmpx_t(&snmpx_sd2);

	InitPublicAttr(snmpx_sd1);
	snmpx_sd1.tag = SNMPX_MSG_SET;

	if (SNMPX_VERSION_v3 != m_pUserInfo->version) //V2版本
	{
		//请求
		irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, false, oidType, pValue, NULL, szError);

		if (SNMPX_noError != irval) {
			szError = "下发数据失败：" + szError;
		}
	}
	else //V3版本
	{
		snmpx_sd1.tag = SNMPX_MSG_GET;

		//第一帧获取引擎ID
		irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, true, E_OID_NONE, NULL, NULL, szError, true);

		if (SNMPX_noError != irval) {
			szError = "获取引擎ID失败：" + szError;
		}
		else
		{
			//第二帧获取数据
			InitPublicAttr(snmpx_sd2, true);
			snmpx_sd2.tag = SNMPX_MSG_SET;
			
			//赋值引擎及随机数信息
			if (FillSnmpxEngineAndPrivacy(snmpx_rc1, snmpx_sd2, szError))
			{
				//计算密钥 , 这个比较占用CPU,建议如果第二次做GET操作的引擎ID也是一样,就不要再算第二次了
				InitUserPriv(snmpx_rc1);

				//请求
				irval = RequestSnmpx(snmpx_sd2, snmpx_rc2, false, oidType, pValue, NULL, szError);

				if (SNMPX_noError != irval) {
					szError = "下发数据失败：" + szError;
				}
			}
			else
				irval = SNMPX_failure;
		}
	}

	//清理
	free_snmpx_t(&snmpx_rc1); //释放内存
	free_snmpx_t(&snmpx_sd1); //释放内存
	free_snmpx_t(&snmpx_rc2); //释放内存
	free_snmpx_t(&snmpx_sd2); //释放内存

	//失败清除用户信息，下次重新生成
	if (SNMPX_noError != irval) {
		EraseUsmUser(m_szIP);
	}

	return irval;
}

/********************************************************************
功能：	snmpx table数据
参数：	szTableOid 表名OID
*		valueMap 结果容器<行，<列，数据对象>>
*		szError 错误信息
*		iReplications 每次回复条数
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Table(const string& szTableOid, TTableResultType& valueMap, string& szError, int iReplications)
{
	return SnmpxTableHandle(E_OID_STRING, &szTableOid, valueMap, szError, iReplications);
}

/********************************************************************
功能：	snmpx table数据
参数：	pTableOid 表名OID，第一个位置存放oid长度
*		valueMap 结果容器<行，<列，数据对象>>
*		szError 错误信息
*		iReplications 每次回复条数
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Table(const oid* pTableOid, TTableResultType& valueMap, string& szError, int iReplications)
{
	return SnmpxTableHandle(E_OID_POINTER, pTableOid, valueMap, szError, iReplications);
}

/********************************************************************
功能：	snmpx table处理
参数：	oidType oid类型
*		pTableOid 表名OID，根据oidType处理
*		valueMap 结果容器<行，<列，数据对象>>
*		szError 错误信息
*		iReplications 每次回复条数
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::SnmpxTableHandle(EOidType oidType, const void* pTableOid, TTableResultType& valueMap, string& szError, int iReplications)
{
	if (NULL == m_pUserInfo)
	{
		szError = "未设置认证信息！";
		return SNMPX_failure;
	}

	if (SNMPX_VERSION_v1 == m_pUserInfo->version)
	{
		szError = "snmp V1版本不支持get-bulk操作！";
		return SNMPX_failure;
	}

	if (iReplications < 1 || iReplications > SNMPX_MAX_BULK_REPETITIONS)
	{
		szError = "最大回复条数设置错误，不能小于1条且不能大于" + std::to_string(SNMPX_MAX_BULK_REPETITIONS) + "条！" ;
		return SNMPX_failure;
	}

	int irval = SNMPX_noError;
	bool bExit = false;
	oid* pCurrTableOid = NULL;
	oid tableOidBuf[SNMPX_MAX_OID_LEN + 1] = { 0 };

	//将oid都转为oid*类型，方便处理
	if (E_OID_STRING == oidType)
	{
		if (!parse_oid_string(*((const string*)pTableOid), tableOidBuf, szError))
		{
			szError = "转换OID失败：" + szError;
			return SNMPX_failure;
		}
		else
		{
			//第一个位置存放OID长度
			pCurrTableOid = (oid*)malloc(sizeof(oid) * (tableOidBuf[0] + 1));
			pCurrTableOid[0] = tableOidBuf[0];
			memcpy(pCurrTableOid + 1, tableOidBuf + 1, sizeof(oid) * tableOidBuf[0]);
		}
	}
	else
	{
		oid tableOidBufLen = ((const oid*)pTableOid)[0];
		pCurrTableOid = (oid*)malloc(sizeof(oid) * (tableOidBufLen + 1));
		memcpy(pCurrTableOid, pTableOid, sizeof(oid) * (tableOidBufLen + 1));
		memcpy(tableOidBuf, (const oid*)pTableOid, sizeof(oid) * (tableOidBufLen + 1));
	}

	struct snmpx_t snmpx_rc1; //第一发送数据结构体
	struct snmpx_t snmpx_sd1; //第一接收数据结构体
	struct snmpx_t snmpx_rc2; //第二次发送数据结构体
	struct snmpx_t snmpx_sd2; //第二次接收数据结构体

	/* 初始化 */
	init_snmpx_t(&snmpx_rc1);
	init_snmpx_t(&snmpx_sd1);
	init_snmpx_t(&snmpx_rc2);
	init_snmpx_t(&snmpx_sd2);

	//清空结果集
	valueMap.clear();

	//初始化公用属性
	InitPublicAttr(snmpx_sd1);

	if (SNMPX_VERSION_v3 != m_pUserInfo->version) //V2版本
	{
		snmpx_sd1.tag = SNMPX_MSG_GETBULK;
		snmpx_sd1.error_index = iReplications; //赋值请求返回最大的行数，复用

		//table处理
		do
		{
			//请求
			irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, true, E_OID_POINTER, pCurrTableOid, NULL, szError);

			if (SNMPX_noError != irval)
			{
				szError = "获取数据失败：" + szError;
				bExit = true;
			}
			else
			{
				//处理
				bExit = FillSnmpxTableResultData(snmpx_rc1, tableOidBuf, &pCurrTableOid, valueMap);
			}

			clear_snmpx_t(&snmpx_rc1); //释放内存，准备下一次使用
		} while (!bExit);
	}
	else //V3版本
	{
		snmpx_sd1.tag = SNMPX_MSG_GET;

		//第一帧获取引擎ID
		irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, true, E_OID_NONE, NULL, NULL, szError, true);

		if (SNMPX_noError != irval) {
			szError = "获取引擎ID失败：" + szError;
		}
		else
		{
			//第二帧获取数据
			InitPublicAttr(snmpx_sd2, true);
			snmpx_sd2.tag = SNMPX_MSG_GETBULK;
			snmpx_sd2.error_index = iReplications; //赋值请求返回最大的行数，复用
			
			//赋值引擎及随机数信息
			if (FillSnmpxEngineAndPrivacy(snmpx_rc1, snmpx_sd2, szError))
			{
				//计算密钥 , 这个比较占用CPU,建议如果第二次做GET操作的引擎ID也是一样,就不要再算第二次了
				InitUserPriv(snmpx_rc1);

				//table处理
				do
				{
					//请求
					irval = RequestSnmpx(snmpx_sd2, snmpx_rc2, true, E_OID_POINTER, pCurrTableOid, NULL, szError);

					if (SNMPX_noError != irval)
					{
						szError = "获取数据失败：" + szError;
						bExit = true;
					}
					else
					{
						//处理
						bExit = FillSnmpxTableResultData(snmpx_rc2, tableOidBuf, &pCurrTableOid, valueMap);
					}

					clear_snmpx_t(&snmpx_rc2); //释放内存，准备下一次使用
				} while (!bExit);
			}
			else
				irval = SNMPX_failure;
		}
	}

	//清理
	free_snmpx_t(&snmpx_rc1); //释放内存
	free_snmpx_t(&snmpx_sd1); //释放内存
	free_snmpx_t(&snmpx_rc2); //释放内存
	free_snmpx_t(&snmpx_sd2); //释放内存

	if (NULL != pCurrTableOid) {
		free(pCurrTableOid);
	}

	//失败清除用户信息，下次重新生成
	if (SNMPX_noError != irval) {
		EraseUsmUser(m_szIP);
	}

	return irval;
}

/********************************************************************
功能：	snmpx walk
参数：	szWalkOid oid串
*		valueList 结果容器
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Walk(const string& szWalkOid, list<SSnmpxValue>& valueList, string& szError)
{
	return SnmpxWalkHandle(E_OID_STRING, &szWalkOid, valueList, szError);
}

/********************************************************************
功能：	snmpx walk
参数：	pWalkOid oid指针，第一个位置存放oid长度
*		valueList 结果容器
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::Walk(const oid* pWalkOid, list<SSnmpxValue>& valueList, string& szError)
{
	return SnmpxWalkHandle(E_OID_POINTER, pWalkOid, valueList, szError);
}

/********************************************************************
功能：	snmpx walk处理
参数：	oidType oid类型
*		pWalkOid oid，根据oidType处理
*		valueList 结果容器
*		szError 错误信息
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::SnmpxWalkHandle(EOidType oidType, const void* pWalkOid, list<SSnmpxValue>& valueList, string& szError)
{
	if (NULL == m_pUserInfo)
	{
		szError = "未设置认证信息！";
		return SNMPX_failure;
	}

	int irval = SNMPX_noError;
	bool bExit = false;
	bool bGetOrGetnext = true; //true:getnext, false:get
	oid* pCurrTableOid = NULL;
	oid tableOidBuf[SNMPX_MAX_OID_LEN + 1] = { 0 };

	//将oid都转为oid*类型，方便处理
	if (E_OID_STRING == oidType)
	{
		if (!parse_oid_string(*((const string*)pWalkOid), tableOidBuf, szError))
		{
			szError = "转换OID失败：" + szError;
			return SNMPX_failure;
		}
		else
		{
			//第一个位置存放OID长度
			pCurrTableOid = (oid*)malloc(sizeof(oid) * (tableOidBuf[0] + 1));
			pCurrTableOid[0] = tableOidBuf[0];
			memcpy(pCurrTableOid + 1, tableOidBuf + 1, sizeof(oid) * tableOidBuf[0]);
		}
	}
	else
	{
		oid tableOidBufLen = ((const oid*)pWalkOid)[0];
		pCurrTableOid = (oid*)malloc(sizeof(oid) * (tableOidBufLen + 1));
		memcpy(pCurrTableOid, pWalkOid, sizeof(oid) * (tableOidBufLen + 1));
		memcpy(tableOidBuf, (const oid*)pWalkOid, sizeof(oid) * (tableOidBufLen + 1));
	}

	struct snmpx_t snmpx_rc1; //第一发送数据结构体
	struct snmpx_t snmpx_sd1; //第一接收数据结构体
	struct snmpx_t snmpx_rc2; //第二次发送数据结构体
	struct snmpx_t snmpx_sd2; //第二次接收数据结构体

	/* 初始化 */
	init_snmpx_t(&snmpx_rc1);
	init_snmpx_t(&snmpx_sd1);
	init_snmpx_t(&snmpx_rc2);
	init_snmpx_t(&snmpx_sd2);

	//清空结果集
	valueList.clear();

	//初始化公用属性
	InitPublicAttr(snmpx_sd1);

	if (SNMPX_VERSION_v3 != m_pUserInfo->version) //V2版本
	{
		//walk处理
		do
		{
			if (bGetOrGetnext)
			{
				snmpx_sd1.tag = SNMPX_MSG_GETNEXT;

				//请求
				irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, true, E_OID_POINTER, pCurrTableOid, NULL, szError);

				if (SNMPX_noError != irval)
				{
					szError = "获取数据失败：" + szError;
					bExit = true;
				}
				else
				{
					//处理
					bExit = FillSnmpxWalkResultData(snmpx_rc1, tableOidBuf, &pCurrTableOid, valueList, bGetOrGetnext);
				}
			}
			else
			{
				snmpx_sd1.tag = SNMPX_MSG_GET;

				//请求，最后一次获取walk的OID本身，成功说明有值，失败不处理
				if (SNMPX_noError == RequestSnmpx(snmpx_sd1, snmpx_rc1, true, E_OID_POINTER, pCurrTableOid, NULL, szError))
				{
					//处理
					FillSnmpxWalkResultData(snmpx_rc1, tableOidBuf, &pCurrTableOid, valueList, bGetOrGetnext);
				}

				bExit = true;
			}

			clear_snmpx_t(&snmpx_rc1); //释放内存，准备下一次使用
		} while (!bExit);
	}
	else //V3版本
	{
		snmpx_sd1.tag = SNMPX_MSG_GET;

		//第一帧获取引擎ID
		irval = RequestSnmpx(snmpx_sd1, snmpx_rc1, true, E_OID_NONE, NULL, NULL, szError, true);

		if (SNMPX_noError != irval) {
			szError = "获取引擎ID失败：" + szError;
		}
		else
		{
			//第二帧获取数据
			InitPublicAttr(snmpx_sd2, true);

			//赋值引擎及随机数信息
			if (FillSnmpxEngineAndPrivacy(snmpx_rc1, snmpx_sd2, szError))
			{
				//计算密钥 , 这个比较占用CPU,建议如果第二次做GET操作的引擎ID也是一样,就不要再算第二次了
				InitUserPriv(snmpx_rc1);

				//walk处理
				do
				{
					if (bGetOrGetnext)
					{
						snmpx_sd2.tag = SNMPX_MSG_GETNEXT;

						//请求
						irval = RequestSnmpx(snmpx_sd2, snmpx_rc2, true, E_OID_POINTER, pCurrTableOid, NULL, szError);

						if (SNMPX_noError != irval)
						{
							szError = "获取数据失败：" + szError;
							bExit = true;
						}
						else
						{
							//处理
							bExit = FillSnmpxWalkResultData(snmpx_rc2, tableOidBuf, &pCurrTableOid, valueList, bGetOrGetnext);
						}
					}
					else
					{
						snmpx_sd2.tag = SNMPX_MSG_GET;

						//请求，最后一次获取walk的OID本身，成功说明有值，失败不处理
						if (SNMPX_noError == RequestSnmpx(snmpx_sd2, snmpx_rc2, true, E_OID_POINTER, pCurrTableOid, NULL, szError))
						{
							//处理
							FillSnmpxWalkResultData(snmpx_rc2, tableOidBuf, &pCurrTableOid, valueList, bGetOrGetnext);
						}

						bExit = true;
					}

					clear_snmpx_t(&snmpx_rc2); //释放内存，准备下一次使用
				} while (!bExit);
			}
			else
				irval = SNMPX_failure;
		}
	}

	//清理
	free_snmpx_t(&snmpx_rc1); //释放内存
	free_snmpx_t(&snmpx_sd1); //释放内存
	free_snmpx_t(&snmpx_rc2); //释放内存
	free_snmpx_t(&snmpx_sd2); //释放内存

	if (NULL != pCurrTableOid) {
		free(pCurrTableOid);
	}

	//失败清除用户信息，下次重新生成
	if (SNMPX_noError != irval) {
		EraseUsmUser(m_szIP);
	}

	return irval;
}

/********************************************************************
功能：	snmp v1 trap
参数：	generic_trap trap类型
*		specific_trap trap特别码
*		time_stamp 系统启动时间
*		vb_list 要上报的绑定信息
*		szError 错误信息
返回：	成功返回true
*********************************************************************/
int CSnmpxClient::Trap(int generic_trap, int specific_trap, unsigned int time_stamp, const list<SSnmpxValue> &vb_list, string& szError)
{
	szError = "暂时不支持！";

	return SNMPX_failure;
}

/********************************************************************
功能：	snmp v2 trap
参数：	vb_list 要上报的绑定信息
*		szError 错误信息
返回：	成功返回true
*********************************************************************/
int CSnmpxClient::Trap2(const list<SSnmpxValue> &vb_list, string& szError)
{
	szError = "暂时不支持！";

	return SNMPX_failure;
}

/********************************************************************
功能：	snmp inform
参数：	svb_list 要上报的绑定信息
*		rvb_list 返回的绑定信息
*		szError 错误信息
返回：	成功返回true
*********************************************************************/
int CSnmpxClient::Inform(const list<SSnmpxValue> &svb_list, list<SSnmpxValue> &rvb_list, string& szError)
{
	szError = "暂时不支持！";

	return SNMPX_failure;
}

/********************************************************************
功能：	清除指定usm用户信息
参数：	szAgentIP IP
返回：	无
*********************************************************************/
void CSnmpxClient::EraseUsmUser(const string& szAgentIP)
{
	std::map<string, SUserUsmInfo*>::iterator usmIter;

	m_privUSMLock->lock();

	usmIter = m_privUSMMap->find(szAgentIP);
	if (usmIter != m_privUSMMap->end())
	{
		usmIter->second->Clear();
		delete usmIter->second;
		m_privUSMMap->erase(usmIter);
	}

	m_privUSMLock->unlock();
}

/********************************************************************
功能：	清除所有usm用户信息
参数：	无
返回：	无
*********************************************************************/
void CSnmpxClient::EraseAllUsmUser()
{
	m_privUSMLock->lock();

	for (auto& usm : *m_privUSMMap)
	{
		usm.second->Clear();
		delete usm.second;
	}

	m_privUSMMap->clear();

	m_privUSMLock->unlock();
}

/********************************************************************
功能：	初始化snmpx结构体公共信息
参数：	snmpx snmpx结构体
*		us 用户加密信息
*		is_second_frame 是否是第二帧数据结构体，v3使用
返回：	无
*********************************************************************/
void CSnmpxClient::InitPublicAttr(snmpx_t& snmpx, bool is_second_frame)
{
	snmpx.msgVersion = m_pUserInfo->version;
	snmpx.msgMaxSize = SNMPX_MAX_MSG_LEN; //固定值便可

	/*版本号  0x00:v1 , 0x01:v2c , 0x03:v3 */
	if (SNMPX_VERSION_v3 == snmpx.msgVersion)
	{
		snmpx.msgID = m_pUserInfo->msgID;
		snmpx.msgSecurityModel = SNMPX_SEC_MODEL_USM; /* 固定 usm模式 */
		snmpx.msgUserName_len = (unsigned int)strlen(m_pUserInfo->userName);
		snmpx.msgUserName = (unsigned char*)malloc(snmpx.msgUserName_len + 1); //需要多加一个字节'\0'
		memset(snmpx.msgUserName, 0x00, snmpx.msgUserName_len + 1); //一定要初始化
		memcpy(snmpx.msgUserName, m_pUserInfo->userName, snmpx.msgUserName_len);

		//第一帧获取引擎ID时，是使用明文
		if (is_second_frame)
		{
			snmpx.msgID = m_pUserInfo->msgID;
			snmpx.request_id = 2;

			if (SNMPX_SEC_LEVEL_noAuth == m_pUserInfo->safeMode)
				snmpx.msgFlags = SNMPX_MSG_FLAG_RPRT_BIT;
			else if (SNMPX_SEC_LEVEL_authNoPriv == m_pUserInfo->safeMode)
				snmpx.msgFlags = SNMPX_MSG_FLAG_AUTH_BIT | SNMPX_MSG_FLAG_RPRT_BIT;
			else
				snmpx.msgFlags = SNMPX_MSG_FLAG_AUTH_BIT | SNMPX_MSG_FLAG_PRIV_BIT | SNMPX_MSG_FLAG_RPRT_BIT;
		}
		else
		{
			m_pUserInfo->msgID = GetSnmpxMsgID();
			snmpx.msgID = m_pUserInfo->msgID;
			snmpx.request_id = 1;
			snmpx.msgFlags = SNMPX_MSG_FLAG_RPRT_BIT;
		}
	}
	else
	{
		snmpx.community_len = (unsigned int)strlen(m_pUserInfo->userName);
		snmpx.community = (unsigned char*)malloc(snmpx.community_len + 1);
		memset(snmpx.community, 0x00, snmpx.community_len + 1); //一定要初始化
		memcpy(snmpx.community, m_pUserInfo->userName, snmpx.community_len);
	}
}

/********************************************************************
功能：	初始化V3的用户加密信息
参数：	snmpx V3第一帧请求的返回信息
*		us snmpx的用户结构体
返回：	无
*********************************************************************/
void CSnmpxClient::InitUserPriv(const snmpx_t& snmpx)
{
	//V3才处理，snmpx内部也是这么处理的
	if (SNMPX_VERSION_v3 != m_pUserInfo->version || SNMPX_SEC_LEVEL_noAuth == m_pUserInfo->safeMode)
		return;

	//agent能支持的最大消息长度
	m_pUserInfo->agentMaxMsg_len = snmpx.msgMaxSize;

	//引擎ID
	if (m_pUserInfo->msgAuthoritativeEngineID_len != snmpx.msgAuthoritativeEngineID_len
		|| 0 != memcmp(m_pUserInfo->msgAuthoritativeEngineID, snmpx.msgAuthoritativeEngineID, m_pUserInfo->msgAuthoritativeEngineID_len))
	{
		if (NULL != m_pUserInfo->msgAuthoritativeEngineID)
		{
			free(m_pUserInfo->msgAuthoritativeEngineID);
			m_pUserInfo->msgAuthoritativeEngineID = NULL;
		}

		m_pUserInfo->msgAuthoritativeEngineID = (unsigned char*)malloc(snmpx.msgAuthoritativeEngineID_len);
		m_pUserInfo->msgAuthoritativeEngineID_len = snmpx.msgAuthoritativeEngineID_len;
		memcpy(m_pUserInfo->msgAuthoritativeEngineID, snmpx.msgAuthoritativeEngineID, m_pUserInfo->msgAuthoritativeEngineID_len);
	}

	std::map<string, SUserUsmInfo*>::iterator usmIter;
	CUserProccess userProc;

	//生成认证及加密密钥
	m_privUSMLock->lock();

	usmIter = m_privUSMMap->find(m_szIP);
	if (usmIter != m_privUSMMap->end())
	{
		if (*(usmIter->second) == *m_pUserInfo)
		{
			//认证密码
			if (NULL == m_pUserInfo->authPasswordPrivKey)
			{
				m_pUserInfo->authPasswordPrivKey_len = usmIter->second->authPasswordPrivKey_len;
				m_pUserInfo->authPasswordPrivKey = (unsigned char*)malloc(m_pUserInfo->authPasswordPrivKey_len);
				memcpy(m_pUserInfo->authPasswordPrivKey, usmIter->second->authPasswordPrivKey, m_pUserInfo->authPasswordPrivKey_len);
			}

			//加密密码
			if (SNMPX_SEC_LEVEL_authPriv == m_pUserInfo->safeMode)
			{
				if (NULL == m_pUserInfo->privPasswdPrivKey)
				{
					m_pUserInfo->privPasswdPrivKey_len = usmIter->second->privPasswdPrivKey_len;
					m_pUserInfo->privPasswdPrivKey = (unsigned char*)malloc(m_pUserInfo->privPasswdPrivKey_len);
					memcpy(m_pUserInfo->privPasswdPrivKey, usmIter->second->privPasswdPrivKey, m_pUserInfo->privPasswdPrivKey_len);
				}
			}
		}
		else
		{
			//删除重新生成
			usmIter->second->Clear();
			delete usmIter->second;
			m_privUSMMap->erase(usmIter);

			userProc.snmpx_user_init(m_pUserInfo); //比较耗时，只做一次
			m_privUSMMap->insert(std::make_pair(m_szIP, new SUserUsmInfo(m_pUserInfo)));
		}
	}
	else
	{
		userProc.snmpx_user_init(m_pUserInfo); //比较耗时，只做一次
		m_privUSMMap->insert(std::make_pair(m_szIP, new SUserUsmInfo(m_pUserInfo)));
	}

	m_privUSMLock->unlock();

	return;
}

/********************************************************************
功能：	获取消息ID
参数：	无
返回：	消息ID
*********************************************************************/
int CSnmpxClient::GetSnmpxMsgID()
{
	int msgID = 0;

	m_msgIDLock->lock();

	msgID = m_msgID;

	if (m_msgID == 0x7FFFFFFF)
		m_msgID = 1; //达到最大值，归1
	else
		++m_msgID;

	m_msgIDLock->unlock();

	return msgID;
}

/********************************************************************
功能：	填充snmpx引擎及随机数信息,V3才会使用
参数：	snmpxSrc 源信息结构体
*		snmpxDst 目标信息结构体
*		szError 错误信息
返回：	成功返回true
*********************************************************************/
bool CSnmpxClient::FillSnmpxEngineAndPrivacy(const snmpx_t& snmpxSrc, snmpx_t& snmpxDst, string& szError)
{
	//生成随机数，加密才需要
	if (NULL != snmpxDst.msgPrivacyParameters)
	{
		free(snmpxDst.msgPrivacyParameters);
		snmpxDst.msgPrivacyParameters = NULL;
		snmpxDst.msgPrivacyParameters_len = 0;
	}

	if ((snmpxDst.msgFlags & SNMPX_MSG_FLAG_PRIV_BIT) == SNMPX_MSG_FLAG_PRIV_BIT)
	{
		CCryptographyProccess crypto;

		snmpxDst.msgPrivacyParameters_len = SNMPX_PRIVACY_PARAM_LEN; //固定值
		snmpxDst.msgPrivacyParameters = (unsigned char*)malloc(snmpxDst.msgPrivacyParameters_len);
		if (crypto.gen_msgPrivacyParameters(snmpxDst.msgPrivacyParameters, snmpxDst.msgPrivacyParameters_len) < 0)
		{
			szError = "填充请求数据，生成随机数失败：" + crypto.GetErrorMsg();
			return false;
		}
	}

	//赋值引擎信息
	if (NULL != snmpxDst.msgAuthoritativeEngineID)
	{
		snmpxDst.msgAuthoritativeEngineID_len = 0;
		free(snmpxDst.msgAuthoritativeEngineID);
		snmpxDst.msgAuthoritativeEngineID = NULL;
	}

	if (snmpxSrc.msgAuthoritativeEngineID_len > 0)
	{
		snmpxDst.msgAuthoritativeEngineID_len = snmpxSrc.msgAuthoritativeEngineID_len;
		snmpxDst.msgAuthoritativeEngineID = (unsigned char*)malloc(snmpxSrc.msgAuthoritativeEngineID_len);
		memcpy(snmpxDst.msgAuthoritativeEngineID, snmpxSrc.msgAuthoritativeEngineID, snmpxSrc.msgAuthoritativeEngineID_len);
	}

	snmpxDst.msgAuthoritativeEngineBoots = snmpxSrc.msgAuthoritativeEngineBoots;
	snmpxDst.msgAuthoritativeEngineTime = snmpxSrc.msgAuthoritativeEngineTime;

	if (snmpxDst.contextEngineID == NULL && snmpxDst.msgAuthoritativeEngineID != NULL)
	{
		snmpxDst.contextEngineID_len = snmpxDst.msgAuthoritativeEngineID_len;
		snmpxDst.contextEngineID = (unsigned char*)malloc(snmpxDst.msgAuthoritativeEngineID_len);
		memcpy(snmpxDst.contextEngineID, snmpxDst.msgAuthoritativeEngineID, snmpxDst.msgAuthoritativeEngineID_len);
	}

	return true;
}

/********************************************************************
功能：	填充snmpx OID信息
参数：	bIsGet 是否是get
*		oidType oid类型
*		pOidList OID容器
*		snmpx 填充对象
*		szError 错误信息
返回：	无
*********************************************************************/
bool CSnmpxClient::FillSnmpxOidInfo(bool bIsGet, EOidType oidType, const void* pOidList, snmpx_t& snmpx, string& szError)
{
	bool bResult = true;
	oid oidTemp[SNMPX_MAX_OID_LEN + 1] = { 0 };
	int rval = 0;
	unsigned char tag, *data = NULL;
	CSnmpxPack pack;

	//先清空一下oid信息，table时，可能会多次调用
	free_variable_glist_data(snmpx.variable_bindings_list);

	switch (oidType)
	{
	case CSnmpxClient::E_OID_STRING:
	{
		if (bIsGet)
		{
			const string* pOid = (const string*)pOidList;

			if (parse_oid_string(*pOid, oidTemp, szError))
			{
				if (pack.snmpx_set_vb_list(snmpx.variable_bindings_list, oidTemp + 1, sizeof(oid) * oidTemp[0], ASN_NULL, NULL, 0) < 0)
				{
					bResult = false;
					szError = "设置绑定OID及值失败：" + pack.GetErrorMsg();
				}
			}
			else
			{
				bResult = false;
				szError = "解析OID失败：" + szError;
			}
		}
		else
		{
			const SSnmpxValue* value = (const SSnmpxValue*)pOidList;

			if (0 == value->OidBuf[0]) //第一个位置存放OID长度
			{
				//防止OID格式错误
				if (!parse_oid_string(value->szOid, oidTemp, szError))
				{
					bResult = false;
					szError = "解析OID失败：" + szError;
				}
			}
			else
				memcpy(oidTemp, value->OidBuf, (value->OidBuf[0] + 1) * sizeof(oid));
			
			if (bResult)
			{
				rval = GetSetOidValueBytes(*value, &tag, &data, szError);

				if (rval >= 0)
				{
					if (pack.snmpx_set_vb_list(snmpx.variable_bindings_list, oidTemp + 1, sizeof(oid) * oidTemp[0], tag, data, rval) < 0)
					{
						bResult = false;
						szError = "设置绑定OID及值失败：" + pack.GetErrorMsg();
					}
				}
				else
				{
					bResult = false;
					szError = "获取下发的oid绑定数据字节失败：" + szError;
				}

				if (data != NULL)
				{
					free(data);
					data = NULL;
				}
			}
		}
	}
	break;
	case CSnmpxClient::E_OID_STRING_SET:
	{
		if (bIsGet)
		{
			const list<string>* pOid = (const list<string>*)pOidList;

			for (const auto& szOid : *pOid)
			{
				//防止OID格式错误
				if (parse_oid_string(szOid, oidTemp, szError))
				{
					if (pack.snmpx_set_vb_list(snmpx.variable_bindings_list, oidTemp + 1, sizeof(oid) * oidTemp[0], ASN_NULL, NULL, 0) < 0)
					{
						bResult = false;
						szError = "设置绑定OID及值失败：" + pack.GetErrorMsg();
					}
				}
				else
				{
					bResult = false;
					szError = "解析OID失败：" + szError;
				}

				if (!bResult)
					break;
			}
		}
		else
		{
			const list<SSnmpxValue>* valueSet = (const list<SSnmpxValue>*)pOidList;

			for (const auto& value : *valueSet)
			{
				if (0 == value.OidBuf[0]) //第一个位置存放OID长度
				{
					//防止OID格式错误
					if (!parse_oid_string(value.szOid, oidTemp, szError))
					{
						bResult = false;
						szError = "解析OID失败：" + szError;
						break;
					}
				}
				else
					memcpy(oidTemp, value.OidBuf, (value.OidBuf[0] + 1) * sizeof(oid));

				if (bResult)
				{
					rval = GetSetOidValueBytes(value, &tag, &data, szError);

					if (rval >= 0)
					{
						if (pack.snmpx_set_vb_list(snmpx.variable_bindings_list, oidTemp + 1, sizeof(oid) * oidTemp[0], tag, data, rval) < 0)
						{
							bResult = false;
							szError = "设置绑定OID及值失败：" + pack.GetErrorMsg();
						}
					}
					else
					{
						bResult = false;
						szError = "获取下发的oid绑定数据字节失败：" + szError;
					}

					if (data != NULL)
					{
						free(data);
						data = NULL;
					}

					if (!bResult)
						break;
				}
			}
		}
	}
	break;
	case CSnmpxClient::E_OID_POINTER:
	{
		if (bIsGet)
		{
			const oid* pOid = (const oid*)pOidList;

			if (pack.snmpx_set_vb_list(snmpx.variable_bindings_list, pOid + 1, sizeof(oid) * pOid[0], ASN_NULL, NULL, 0) < 0)
			{
				bResult = false;
				szError = "设置绑定OID及值失败：" + pack.GetErrorMsg();
			}
		}
		else
		{
			const std::pair<const oid*, SSnmpxValue>* value = (const std::pair<const oid*, SSnmpxValue>*)pOidList;

			rval = GetSetOidValueBytes(value->second, &tag, &data, szError);

			if (rval >= 0)
			{
				if (pack.snmpx_set_vb_list(snmpx.variable_bindings_list, value->first + 1, sizeof(oid) * value->first[0], tag, data, rval) < 0)
				{
					bResult = false;
					szError = "设置绑定OID及值失败：" + pack.GetErrorMsg();
				}
			}
			else
			{
				bResult = false;
				szError = "获取下发的oid绑定数据字节失败：" + szError;
			}

			if (data != NULL)
			{
				free(data);
				data = NULL;
			}
		}
	}
	break;
	case CSnmpxClient::E_OID_POINTER_SET:
	{
		if (bIsGet)
		{
			const list<const oid*>* pOidSet = (const list<const oid*>*)pOidList;

			for (const auto& pOid : *pOidSet)
			{
				if (pack.snmpx_set_vb_list(snmpx.variable_bindings_list, pOid + 1, sizeof(oid) * pOid[0], ASN_NULL, NULL, 0) < 0)
				{
					bResult = false;
					szError = "设置绑定OID及值失败：" + pack.GetErrorMsg();
					break;
				}
			}
		}
		else
		{
			const list<std::pair<const oid*, SSnmpxValue>>* valueSet = (const list<std::pair<const oid*, SSnmpxValue>>*)pOidList;

			for (const auto& value : *valueSet)
			{
				rval = GetSetOidValueBytes(value.second, &tag, &data, szError);

				if (rval >= 0)
				{
					if (pack.snmpx_set_vb_list(snmpx.variable_bindings_list, value.first + 1, sizeof(oid) * value.first[0], tag, data, rval) < 0)
					{
						bResult = false;
						szError = "设置绑定OID及值失败：" + pack.GetErrorMsg();
					}
				}
				else
				{
					bResult = false;
					szError = "获取下发的oid绑定数据字节失败：" + szError;
				}

				if (data != NULL)
				{
					free(data);
					data = NULL;
				}

				if (!bResult)
					break;
			}
		}
	}
	break;
	default:
		bResult = false;
		szError = "OID类型错误：" + std::to_string(oidType);
	}

	return bResult;
}

/********************************************************************
功能：	填充snmpx获取table的结果信息
参数：	snmpx 结果信息，agent返回
*		srcTableOid 原始的table的OID
*		pTableOid table的oid
*		valueMap 结果容器，己分行<行，<列，数据对象>>
返回：	己获取完成，返回true
*********************************************************************/
bool CSnmpxClient::FillSnmpxTableResultData(const snmpx_t& snmpx, const oid* srcTableOid, oid** pTableOid, TTableResultType& valueMap)
{
	bool bResult = false;
	const struct variable_bindings* rctvb = NULL;
	TTableResultType_iter iter;
	oid iRow = 0, iCol = 0;
	std::list<variable_bindings*>::const_iterator variter;

	for (variter = snmpx.variable_bindings_list->cbegin(); variter != snmpx.variable_bindings_list->cend(); ++variter)
	{
		SSnmpxValue valueTemp;
		rctvb = *variter;

		//退出标志，table的节点OID长度一定大于table本身OID长度
		if (srcTableOid[0] * sizeof(oid) >= rctvb->oid_buf_len
			|| 0 != CompareOidBuf(srcTableOid, rctvb->oid_buf, rctvb->oid_buf_len, false))
		{
			bResult = true;
			break;
		}

		//处理数据，按行分好
		CSnmpxUnpack::snmpx_get_vb_value(rctvb, &valueTemp);
		iRow = rctvb->oid_buf[rctvb->oid_buf_len / sizeof(oid) - 1];
		iCol = rctvb->oid_buf[rctvb->oid_buf_len / sizeof(oid) - 2];
		iter = valueMap.find(iRow);
		if (iter != valueMap.end())
		{
			iter->second.push_back(std::make_pair(iCol, valueTemp));
		}
		else
		{
			std::vector<std::pair<oid, SSnmpxValue>> temp = { {iCol, valueTemp} };
			valueMap.insert(std::make_pair(iRow, temp));
		}
	}

	if (!bResult) //还未完成，设置下一次的oid
	{
		free(*pTableOid);

		//rctvb不会为空
		*pTableOid = (oid*)malloc(rctvb->oid_buf_len + sizeof(oid));
		if (NULL != *pTableOid)
		{
			(*pTableOid)[0] = rctvb->oid_buf_len / sizeof(oid);
			memcpy(*pTableOid + 1, rctvb->oid_buf, rctvb->oid_buf_len);
		}
		else
		{
			//有异常，认为处理完成
			bResult = true;
		}
	}

	return bResult;
}

/********************************************************************
功能：	填充snmpx获取walk的结果信息
参数：	snmpx 结果信息，agent返回
*		srcTableOid 原始的table的OID
*		pTableOid table的oid
*		valueList 结果容器
*		bIsGetOrGetnext 是否是get，true:getnext, false:get
返回：	己获取完成，返回true
*********************************************************************/
bool CSnmpxClient::FillSnmpxWalkResultData(const snmpx_t& snmpx, const oid* srcTableOid, oid** pTableOid, list<SSnmpxValue>& valueList,
	bool& bIsGetOrGetnext)
{
	bool bResult = false;
	const struct variable_bindings* rctvb = NULL;
	std::list<variable_bindings*>::const_iterator variter;

	for (variter = snmpx.variable_bindings_list->cbegin(); variter != snmpx.variable_bindings_list->cend(); ++variter)
	{
		SSnmpxValue valueTemp;
		rctvb = *variter;

		if (0 == CompareOidBuf(srcTableOid, rctvb->oid_buf, rctvb->oid_buf_len, true))
		{
			//该OID节点不存在，退出
			if (ASN_NO_SUCHOBJECT == rctvb->val_tag || ASN_NO_SUCHOBJECT1 == rctvb->val_tag || ASN_NO_SUCHOBJECT2 == rctvb->val_tag)
			{
				bResult = true;
			}
			else
			{
				CSnmpxUnpack::snmpx_get_vb_value(rctvb, &valueTemp);

				if (bIsGetOrGetnext)
					valueList.push_back(valueTemp);
				else
					valueList.push_front(valueTemp);
			}
		}
		else
		{
			//退出标志
			if (0 == CompareOidBuf(walkEndOid, rctvb->oid_buf, rctvb->oid_buf_len, true)
				|| ASN_NO_SUCHOBJECT == rctvb->val_tag || ASN_NO_SUCHOBJECT1 == rctvb->val_tag || ASN_NO_SUCHOBJECT2 == rctvb->val_tag)
				bResult = true;
			else //尝试get操作，get后也要退出
				bIsGetOrGetnext = false;

			break;
		}
	}

	if (!bResult) //还未完成，设置下一次的oid
	{
		free(*pTableOid);

		if (bIsGetOrGetnext)
			*pTableOid = (oid*)malloc(rctvb->oid_buf_len + sizeof(oid)); //rctvb不会为空
		else
			*pTableOid = (oid*)malloc((srcTableOid[0] + 1) * sizeof(oid));

		if (NULL != *pTableOid)
		{
			if (bIsGetOrGetnext)
			{
				(*pTableOid)[0] = rctvb->oid_buf_len / sizeof(oid);
				memcpy(*pTableOid + 1, rctvb->oid_buf, rctvb->oid_buf_len);
			}
			else //获取一下walk的oid本身
				memcpy(*pTableOid, srcTableOid, (srcTableOid[0] + 1) * sizeof(oid));
		}
		else
		{
			//有异常，认为处理完成
			bResult = true;
		}
	}

	return bResult;
}

/********************************************************************
功能：	snmpx请求及返回处理
参数：	us 用户加密信息
*		snmpx_sd 发送数据结构体
*		snmpx_rc 接收数据结构体
*		bIsGet 是否是get
*		oidType oid类型
*		pOidList OID容器
*		pValueVec 结果值容器
*		szError 错误信息
*		bIsGetEngineID 是否获取引擎ID
返回：	成功返回0
*********************************************************************/
int CSnmpxClient::RequestSnmpx(snmpx_t& snmpx_sd, snmpx_t& snmpx_rc, bool bIsGet, EOidType oidType, const void* pOidList, 
	std::vector<SSnmpxValue>* pValueVec, string& szError, bool bIsGetEngineID)
{
	int fd = 0, sento_result = 0, recvbuf_len = 0, retryTimes = 0, iErrnoTemp = 0;
	unsigned char sendbuf[SNMPX_MAX_MSG_LEN] = { 0 }; /* upd包最大只可以是65535 */
	unsigned char recvbuf[SNMPX_MAX_MSG_LEN] = { 0 };
	const int iRetryEngineTime = 2;
	CSnmpxPack pack;
	CSnmpxUnpack unpack;

	//赋值请求的OID
	if (NULL != pOidList)
	{
		if (!FillSnmpxOidInfo(bIsGet, oidType, pOidList, snmpx_sd, szError)) {
			return SNMPX_failure;
		}
	}

	//个别类型设备获取引擎ID时，不能正确返回引擎时间，最多重发2次
	for (int i = 0; i < iRetryEngineTime; ++i)
	{
		retryTimes = m_cRetryTimes;

		//接收请求返回，要求重试
		do
		{
			//组包
			memset(sendbuf, 0x00, sizeof(sendbuf));
			int sendbuf_len = pack.snmpx_group_pack(sendbuf, &snmpx_sd, m_pUserInfo);
			if (sendbuf_len <= 0)
			{
				szError = "发送前组包失败：" + pack.GetErrorMsg();
				return SNMPX_failure;
			}

			struct sockaddr_in client_addr;
			socklen_t cli_len = sizeof(client_addr);

			//初始化socket，每次都是新的，保证成功率
			if (!InitSocketFd(m_szIP, m_nPort, m_lTimeout, fd, client_addr, szError))
			{
				szError = "初始化socket句柄失败：" + szError;
				return SNMPX_failure;
			}

			//发送请求，UDP一定能发出去
			sento_result = sendto(fd, (const char*)sendbuf, sendbuf_len, 0, (struct sockaddr *)&client_addr, cli_len);
			if (sento_result <= 0)
			{
				szError = CErrorStatus::get_err_msg(errno, true, true);
				close_socket_fd(fd);
				return SNMPX_failure;
			}
			else
			{
				memset(recvbuf, 0x00, sizeof(recvbuf));
				recvbuf_len = recvfrom(fd, (char*)recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&client_addr, &cli_len);
				if (recvbuf_len <= 0) //接收失败则重试
				{
					iErrnoTemp = errno;
					--retryTimes;
				}
				else
					retryTimes = 0;

				close_socket_fd(fd);
			}

		} while (retryTimes > 0);

		if (recvbuf_len <= 0)
		{
			//团体名或加密密码错误会接收超时，用ping测试
			if (EAGAIN == iErrnoTemp || 0 == iErrnoTemp)
			{
				if (m_bPingOnTimeout)
				{
					if (CPing::ping(m_szIP, szError))
					{
						if (3 == m_pUserInfo->version)
							szError = "接收返回超时，agent未启动或通信密码错误！";
						else
							szError = "接收返回超时，agent未启动或团体名错误！";
					}
					else
						szError = "ping失败！";
				}
				else
					szError = "接收返回超时！";

				return SNMPX_timeout;
			}
			else
			{
				szError = CErrorStatus::get_err_msg(iErrnoTemp, true, true);
				return SNMPX_failure;
			}
		}
		else
		{
			memcpy(snmpx_rc.ip, m_szIP.c_str(), m_szIP.length());
			snmpx_rc.remote_port = m_nPort;
		}

		//返回数据解包
		if (unpack.snmpx_group_unpack(recvbuf, recvbuf_len, &snmpx_rc, bIsGetEngineID, m_pUserInfo, false) < 0)
		{
			szError = "接收后解包失败：" + unpack.GetErrorMsg();
			return SNMPX_failure;
		}

		snmpx_sd.request_id++;

		//判断返回错误码
		if (0 == snmpx_rc.error_status)
			break;
		else if (bIsGetEngineID 
			&& SNMPX_usmUnknownEngineIDs == snmpx_rc.error_status
			&& snmpx_rc.msgAuthoritativeEngineID_len > 0)
			break;
		else
		{
			//最后失败前重试
			if (i != (iRetryEngineTime - 1)
				&& SNMPX_usmNotInTimeWindows == snmpx_rc.error_status)
			{
				//重新赋值引擎时间
				snmpx_sd.msgAuthoritativeEngineBoots = snmpx_rc.msgAuthoritativeEngineBoots;
				snmpx_sd.msgAuthoritativeEngineTime = snmpx_rc.msgAuthoritativeEngineTime;

				free_snmpx_t(&snmpx_rc); //释放内存

				continue;
			}

			szError = CErrorStatus::get_err_msg(snmpx_rc.error_status, false, true);
			return snmpx_rc.error_status;
		}
	}

	//生成数据
	if (NULL != pValueVec)
	{
		std::list<variable_bindings*>::iterator iter;

		for (iter = snmpx_rc.variable_bindings_list->begin(); iter != snmpx_rc.variable_bindings_list->end(); ++iter)
		{
			SSnmpxValue valueTemp;

			CSnmpxUnpack::snmpx_get_vb_value(*iter, &valueTemp);
			pValueVec->push_back(valueTemp);
		}
	}

	return SNMPX_noError;
}

/********************************************************************
功能：	获取下发OID的值数据
参数：	value 要下发的值，调用者传入
*		tag 数据类型，传出
*		data 要下发的数据，传出，调用者释放
*		szError 错误信息，传出
返回：	成功返回 >= 0的值
*********************************************************************/
int CSnmpxClient::GetSetOidValueBytes(const SSnmpxValue& value, unsigned char* tag, unsigned char** data, string& szError)
{
	int rval = SNMPX_failure;

	if (*data != NULL)
	{
		free(*data);
		*data = NULL;
	}

	switch (value.cValType)
	{
	case SNMPX_ASN_INTEGER:
	{
		rval = (int)sizeof(value.Val.num.i);
		*data = (unsigned char*)malloc(rval * sizeof(unsigned char));
		*tag = ASN_INTEGER;

		if (NULL != *data)
			memcpy(*data, &value.Val.num.i, rval);
		else
			szError = "malloc数据缓存失败：" + CErrorStatus::get_err_msg(errno, true, true);
	}
		break;
	case SNMPX_ASN_NULL:
		rval = 0;
		*data = NULL;
		*tag = ASN_NULL;
		break;
	case SNMPX_ASN_UNSIGNED:
	{
		rval = (int)sizeof(value.Val.num.u);
		*data = (unsigned char*)malloc(rval * sizeof(unsigned char));
		*tag = ASN_GAUGE;

		if (NULL != *data)
			memcpy(*data, &value.Val.num.u, rval);
		else
			szError = "malloc数据缓存失败：" + CErrorStatus::get_err_msg(errno, true, true);
	}
		break;
	case SNMPX_ASN_IPADDRESS:
	{
		rval = (int)sizeof(value.Val.num.i);
		*data = (unsigned char*)malloc(rval * sizeof(unsigned char));
		*tag = ASN_IPADDRESS;

		if (NULL != *data)
			memcpy(*data, &value.Val.num.i, rval);
		else
			szError = "malloc数据缓存失败：" + CErrorStatus::get_err_msg(errno, true, true);
	}
		break;
	case SNMPX_ASN_INTEGER64:
	{
		rval = (int)sizeof(value.Val.num.ll);
		*data = (unsigned char*)malloc(rval * sizeof(unsigned char));
		*tag = ASN_INTEGER64;

		if (NULL != *data)
			memcpy(*data, &value.Val.num.ll, rval);
		else
			szError = "malloc数据缓存失败：" + CErrorStatus::get_err_msg(errno, true, true);
	}
		break;
	case SNMPX_ASN_UNSIGNED64:
	{
		rval = (int)sizeof(value.Val.num.ull);
		*data = (unsigned char*)malloc(rval * sizeof(unsigned char));
		*tag = ASN_UNSIGNED64;

		if (NULL != *data)
			memcpy(*data, &value.Val.num.ull, rval);
		else
			szError = "malloc数据缓存失败：" + CErrorStatus::get_err_msg(errno, true, true);
	}
		break;
	case SNMPX_ASN_FLOAT:
	{
		rval = (int)sizeof(value.Val.num.f);
		*data = (unsigned char*)malloc(rval * sizeof(unsigned char));
		*tag = ASN_FLOAT;

		if (NULL != *data)
			memcpy(*data, &value.Val.num.f, rval);
		else
			szError = "malloc数据缓存失败：" + CErrorStatus::get_err_msg(errno, true, true);
	}
		break;
	case SNMPX_ASN_DOUBLE:
	{
		rval = (int)sizeof(value.Val.num.d);
		*data = (unsigned char*)malloc(rval * sizeof(unsigned char));
		*tag = ASN_DOUBLE;

		if (NULL != *data)
			memcpy(*data, &value.Val.num.d, rval);
		else
			szError = "malloc数据缓存失败：" + CErrorStatus::get_err_msg(errno, true, true);
	}
		break;
	default:
	{
		*tag = ASN_OCTET_STR;

		if (value.Val.str.empty())
		{
			rval = 0;
			*data = NULL;
		}
		else
		{
			rval = (int)value.Val.str.length();
			*data = (unsigned char*)malloc(rval * sizeof(unsigned char));

			if (NULL != *data)
				memcpy(*data, value.Val.str.c_str(), rval);
			else
				szError = "malloc数据缓存失败：" + CErrorStatus::get_err_msg(errno, true, true);
		}
	}
		break;
	}
	
	return rval;
}

/********************************************************************
功能：	比较OID是否相等
参数：	pSrcBuf 原始OID，第一个位置存放要比较的长度
*		pCompareBuf 要比较的oid
*		iCompareBufLen 要比较的oid长度，按字节算的
*		bIsWalkEnd 是否是判断walk的结束OID
返回：	相同返回0，不同返回-1
*********************************************************************/
int CSnmpxClient::CompareOidBuf(const oid* pSrcBuf, const oid* pCompareBuf, unsigned int iCompareBufLen, bool bIsWalkEnd)
{
	if (pSrcBuf[0] <= (oid)(iCompareBufLen / (unsigned int)sizeof(oid)))
	{
		oid iLen = (bIsWalkEnd ? pSrcBuf[0] : (pSrcBuf[0] - 1));

		for (oid i = 0; i < iLen; ++i)
		{
			if (pSrcBuf[i + 1] != pCompareBuf[i])
				return SNMPX_failure;
		}
	}
	else
		return SNMPX_failure;

	return SNMPX_noError;
}