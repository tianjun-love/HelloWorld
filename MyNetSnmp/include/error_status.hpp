#ifndef __ERROR_STATUS_HPP__
#define __ERROR_STATUS_HPP__

#include "snmpx_types.hpp"

#define SNMPX_pingFailure             (-3)  //响应超时且ping失败
#define SNMPX_timeout                 (-2)  //自定义内部错误，超时
#define SNMPX_failure                 (-1)  //自定义内部错误，通用

#define SNMPX_noError                 (0x00)
#define SNMPX_tooBig                  (0x01)
#define SNMPX_noSuchName              (0x02)
#define SNMPX_badValue                (0x03)
#define SNMPX_readOnly                (0x04)
#define SNMPX_genError                (0x05)
#define SNMPX_noAccess                (0x06)  
#define SNMPX_wrongType               (0x07)
#define SNMPX_wrongLength             (0x08)
#define SNMPX_wrongEncoding           (0x09)
#define SNMPX_wrongValue              (0x0A)
#define SNMPX_noCreation              (0x0B)
#define SNMPX_inconsistentValue       (0x0C)
#define SNMPX_resourceUnavailable     (0x0D)
#define SNMPX_commitFailed            (0x0E)
#define SNMPX_undoFailed              (0x0F)
#define SNMPX_authorizationError      (0x10)
#define SNMPX_notWritable             (0x11)
#define SNMPX_inconsistentName        (0x12)

#define SNMPX_usmUnsupportedSecLevels (0x51)  //不支持的安全级别
#define SNMPX_usmNotInTimeWindows     (0x52)  //boots , times 需重试
#define SNMPX_usmUnknownUserNames     (0x53)  //用户名密码错    
#define SNMPX_usmUnknownEngineIDs     (0x54)  //不需重试	     
#define SNMPX_usmWrongDigests         (0x55)  //md5码认证错误	     
#define SNMPX_usmDecryptionErrors     (0x56)  //解密失败

class CErrorStatus
{
public:
	CErrorStatus();
	virtual ~CErrorStatus();

	//获取错误信息，需要的子类设置
	virtual const std::string& GetErrorMsg() const;

	//获取错误编号的详细信息
	static std::string get_err_msg(int errstat, bool is_errno, bool chinese_desc = false);

	//检查并设置snmpv3的usm错误状态
	static void set_usm_error_status(oid* oid_buf, unsigned int oid_buf_len, snmpx_t *s);

	//格式化错误信息
	static std::string format_err_msg(const char *format, ...);

private:
	static const char *m_error_string[2][19];    //snmp错误说明
	static const char *m_usm_error_string[2][6]; //usm错误

protected:
	std::string m_szErrorMsg; //错误信息，需要的子类使用

};

#endif
