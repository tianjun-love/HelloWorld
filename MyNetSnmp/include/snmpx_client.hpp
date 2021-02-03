/********************************************************************
名称:	自定义snmpx客户端类
功能:	snmp的get,set,table等功能
作者:	田俊
时间:	2021-01-14
注意:	oid*的数组，第一个位置存放OID长度，类不是线程安全的，所有不要在多个线程中使用一个客户端
*		windows环境使用前，先调用init_win32_socket_env
*********************************************************************/
#ifndef __SNMPX_CLIENT_HPP__
#define __SNMPX_CLIENT_HPP__

#include "error_status.hpp"
#include <mutex>

using std::list;
using std::string;

class CSnmpxClient
{
public:
	CSnmpxClient(const std::string &ip, unsigned short port, long timeout = 1500, unsigned char retry_times = 3);
	virtual ~CSnmpxClient();

	//设置认证信息，创建对象后必须先调用
	bool SetAuthorizationInfo(unsigned char version, const std::string &szUserName, std::string &szError, unsigned char safeMode = 2,
		unsigned char authMode = 0, const std::string &szAuthPasswd = "", unsigned char privMode = 0,
		const std::string &szPrivPasswd = "");

	//具体方法
	int Get(const string& szOid, SSnmpxValue& value, string& szError);
	int Get(const list<string>& oidList, std::vector<SSnmpxValue>& valueVec, string& szError);
	int Get(const oid* pOid, SSnmpxValue& value, string& szError);
	int Get(const list<const oid*>& oidList, std::vector<SSnmpxValue>& valueVec, string& szError);
	int Getnext(const string& szOid, SSnmpxValue& value, string& szError);
	int Getnext(const oid* pOid, SSnmpxValue& value, string& szError);
	int Set(const SSnmpxValue& value, string& szError);
	int Set(const list<SSnmpxValue>& valueList, string& szError);
	int Set(const std::pair<const oid*, SSnmpxValue>& value, string& szError);
	int Set(const list<std::pair<const oid*, SSnmpxValue>>& valueList, string& szError);
	int Table(const string& szTableOid, TTableResultType& valueMap, string& szError, int iReplications = 20);
	int Table(const oid* pTableOid, TTableResultType& valueMap, string& szError, int iReplications = 20);
	int Walk(const string& szWalkOid, list<SSnmpxValue>& valueList, string& szError);
	int Walk(const oid* pWalkOid, list<SSnmpxValue>& valueList, string& szError);
	int Trap(int generic_trap, int specific_trap, unsigned int time_stamp, const list<SSnmpxValue> &vb_list, string& szError);
	int Trap2(const list<SSnmpxValue> &vb_list, string& szError);
	int Inform(const list<SSnmpxValue> &svb_list, list<SSnmpxValue> &rvb_list, string& szError);

	static void EraseUsmUser(const string& szAgentIP);
	static void EraseAllUsmUser();

private:
	//OID类型
	enum EOidType
	{
		E_OID_NONE        = 0, //未知OID类型
		E_OID_STRING      = 1, //一个OID字符串
		E_OID_STRING_SET  = 2, //多个OID字符串
		E_OID_POINTER     = 3, //一个OID数组指针，第一个位置放oid长度
		E_OID_POINTER_SET = 4  //多个OID数组指针，每个的第一个位置放oid长度
	};

	//用户的USM信息
	struct SUserUsmInfo
	{
		char userName[MAX_USER_INFO_LEN + 1];
		char userAuthPassword[MAX_USER_INFO_LEN + 1];
		char userPrivPassword[MAX_USER_INFO_LEN + 1];
		unsigned char safeMode;
		unsigned char authMode;
		unsigned char privMode;

		unsigned char* msgAuthoritativeEngineID;  //引擎ID
		unsigned int   msgAuthoritativeEngineID_len; //引擎ID长度
		unsigned char* authPasswordPrivKey;  //ku 转换
		unsigned int   authPasswordPrivKey_len;
		unsigned char* privPasswdPrivKey;   //ku转换
		unsigned int   privPasswdPrivKey_len;

		SUserUsmInfo();
		SUserUsmInfo(const userinfo_t* us);
		~SUserUsmInfo();

		SUserUsmInfo(const SUserUsmInfo& Other) = delete;
		SUserUsmInfo operator = (const SUserUsmInfo& Other) = delete;
		bool operator == (const userinfo_t& us) const;

		void Clear();
	};

	void InitPublicAttr(snmpx_t& snmpx, bool is_second_frame = false);
	void InitUserPriv(const snmpx_t& snmpx);
	
	int SnmpxGetHandle(EOidType oidType, const void* pOid, std::vector<SSnmpxValue>& valueVec, string& szError);
	int SnmpxGetnextHandle(EOidType oidType, const void* pOid, std::vector<SSnmpxValue>& valueVec, string& szError);
	int SnmpxSetHandle(EOidType oidType, const void* pValue, string& szError);
	int SnmpxTableHandle(EOidType oidType, const void* pTableOid, TTableResultType& valueMap,  string& szError, int iReplications);
	int SnmpxWalkHandle(EOidType oidType, const void* pWalkOid, list<SSnmpxValue>& valueList, string& szError);
	int RequestSnmpx(snmpx_t& snmpx_sd, snmpx_t& snmpx_rc, bool bIsGet, EOidType oidType, const void* pOidList, 
		std::vector<SSnmpxValue>* pValueVec, string& szError, bool bIsGetEngineID = false);

	static int GetSnmpxMsgID();
	static bool FillSnmpxEngineAndPrivacy(const snmpx_t& snmpxSrc, snmpx_t& snmpxDst, string& szError);
	static bool FillSnmpxOidInfo(bool bIsGet, EOidType oidType, const void* pOidList, snmpx_t& snmpx, string& szError);
	static bool FillSnmpxTableResultData(const snmpx_t& snmpx, const oid* srcTableOid, oid** pTableOid, TTableResultType& valueMap);
	static bool FillSnmpxWalkResultData(const snmpx_t& snmpx, const oid* srcTableOid, oid** pTableOid, list<SSnmpxValue>& valueList,
		bool& bIsGetOrGetnext);
	static int GetSetOidValueBytes(const SSnmpxValue& value, unsigned char* tag, unsigned char** data, string& szError);
	static int CompareOidBuf(const oid* pSrcBuf, const oid* pCompareBuf, unsigned int iCompareBufLen, bool bIsWalkEnd = false);

private:
	static std::mutex                      *m_privUSMLock;  //用户USM信息锁
	static std::map<string, SUserUsmInfo*> *m_privUSMMap;   //用户USM信息<IP， usm信息>
	static int                             m_msgID;         //消息ID
	static std::mutex                      *m_msgIDLock;    //消息ID锁

protected:
	std::string       m_szIP;        //agent IP
	unsigned short    m_nPort;       //agent端口
	long              m_lTimeout;    //接收超时，毫秒，最小0.5秒，最大30秒
	unsigned char     m_cRetryTimes; //重试次数
	struct userinfo_t *m_pUserInfo;  //认证信息对象

};

#endif