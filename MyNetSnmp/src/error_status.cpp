#include "../include/error_status.hpp"
#include <stdarg.h>
#include <stdio.h>

#ifndef MAX_ERR_BUFF_LEN
#define MAX_ERR_BUFF_LEN (81920) //错误消息最大长度
#endif

static const oid usmStatsUnsupportedSecLevelsOid[] = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 1, 0 };
static const oid usmStatsNotInTimeWindowsOid[]     = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 2, 0 };
static const oid usmStatsUnknownUserNamesOid[]     = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 3, 0 };
static const oid usmStatsUnknownEngineIDsOid[]     = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 4, 0 };
static const oid usmStatsWrongDigestsOid[]         = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 5, 0 };
static const oid usmStatsDecryptionErrorsOid[]     = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 6, 0 };

const char* CErrorStatus::m_error_string[2][19] = {
	{
		"(noError) No Error.",
		"(tooBig) Response message would have been too large.",
		"(noSuchName) There is no such variable name in this MIB.",
		"(badValue) The value given has the wrong type or length.",
		"(readOnly) The two parties used do not have access to use the specified SNMP PDU.",
		"(genError) A general failure occured.",
		"noAccess.",
		"wrongType (The set datatype does not match the data type the agent expects).",
		"wrongLength (The set value has an illegal length from what the agent expects).",
		"wrongEncoding.",
		"wrongValue (The set value is illegal or unsupported in some way).",
		"noCreation (That table does not support row creation or that object can not ever be created).",
		"inconsistentValue (The set value is illegal or unsupported in some way).",
		"resourceUnavailable (This is likely a out-of-memory failure within the agent).",
		"commitFailed.",
		"undoFailed.",
		"authorizationError (access denied to that object).",
		"notWritable (That object does not support modification).",
		"inconsistentName (That object can not currently be created)."
	},
	{
		"成功！"
		"返回结果集太大！",
		"在MIB中不存在该变量名！",
		"数据值损坏，长度或类型错误！",
		"对象只读！",
		"generate_Ku错误！",
		"对象拒绝访问！",
		"数据类型与agent不匹配！",
		"数据长度与agent不匹配！",
		"数据编码错误！",
		"数据值异常！",
		"该OID不存在或未创建！",
		"数据值不支持写或未创建！",
		"agent繁忙，资源紧张！",
		"提交失败！",
		"回滚失败！",
		"授权失败，拒绝访问该对象！",
		"该OID无写权限！",
		"不一致的名称，对象当前未创建！"
	}
};

/* USM的错误码 */
const char* CErrorStatus::m_usm_error_string[2][6] = {
	{
		"Unsupported security level.",       /* SNMPERR_UNSUPPORTED_SEC_LEVEL */
		"Not in time window.",       /* SNMPERR_NOT_IN_TIME_WINDOW */
		"Unknown user name.",        /* SNMPERR_UNKNOWN_USER_NAME */
		"Unknown engine ID.",        /* SNMPERR_UNKNOWN_ENG_ID */
		"Authentication failure (incorrect password, community or key).",    /* SNMPERR_AUTHENTICATION_FAILURE */
		"Decryption error."         /* SNMPERR_DECRYPTION_ERR */
	},
	{
		"安全模式错误！",
		"时间窗口错误！",
		"用户名不存在！",
		"引擎ID错误！",
		"认证失败，用户密码或团体名错误！",
		"解密失败！"
	}
};

CErrorStatus::CErrorStatus()
{
}

CErrorStatus::~CErrorStatus()
{
}

const std::string& CErrorStatus::GetErrorMsg() const
{
	return m_szErrorMsg;
}

std::string CErrorStatus::get_err_msg(int errstat, bool is_errno, bool chinese_desc)
{
	std::string szResult;

	if (is_errno)
	{
		switch (errstat)
		{
		case EPERM:
			if (chinese_desc)
				szResult = "不允许的操作！";
			else
				szResult = "Operation not permitted.";
			break;
		case EAGAIN:
			if (chinese_desc)
				szResult = "资源暂时不可用！";
			else
				szResult = "Resource temporarily unavailable.";
			break;
		case ENOMEM:
			if (chinese_desc)
				szResult = "无法申请内存！";
			else
				szResult = "Cannot allocate memory.";
			break;
		case EACCES:
			if (chinese_desc)
				szResult = "权限错误！";
			else
				szResult = "Permission denied.";
			break;
		case EFAULT:
			if (chinese_desc)
				szResult = "地址损坏！";
			else
				szResult = "Bad address.";
			break;
		case EBUSY:
			if (chinese_desc)
				szResult = "设备或资源繁忙！";
			else
				szResult = "Device or resource busy.";
			break;
		case ENFILE:
			if (chinese_desc)
				szResult = "系统中打开文件过多！";
			else
				szResult = "Too many open files in system.";
			break;
		case EMFILE:
			if (chinese_desc)
				szResult = "同时打开文件过多！";
			else
				szResult = "Too many open files at same time.";
			break;
		case ENOSPC:
			if (chinese_desc)
				szResult = "设备空间不足！";
			else
				szResult = "No space left on device.";
			break;
		case ENOTSOCK:
			if (chinese_desc)
				szResult = "在无效的socket句柄上操作！";
			else
				szResult = "Socket operation on non-socket.";
			break;
		case ETIMEDOUT:
			if (chinese_desc)
				szResult = "连接超时！";
			else
				szResult = "Connection timed out.";
			break;
		case ECONNREFUSED:
			if (chinese_desc)
				szResult = "拒绝连接，服务可能未启动！";
			else
				szResult = "Connection refused.";
			break;
		case EHOSTUNREACH:
			if (chinese_desc)
				szResult = "主机不可达，IP路由不存在！";
			else
				szResult = "No route to host.";
			break;
		default:
		{
			char buf[2048] = { '\0' };

#ifdef _WIN32
			if (0 == strerror_s(buf, 2047, errstat))
				szResult = buf;
			else
			{
				if (chinese_desc)
					szResult = "strerror_s获取错误信息失败, 原始错误状态码：" + std::to_string(errstat);
				else
					szResult = "strerror_s work wrong, source error code: " + std::to_string(errstat);
			}
#else
			char *p_error = strerror_r(errstat, buf, 2047);
			if (nullptr != p_error)
				szResult = p_error;
			else
			{
				if (chinese_desc)
					szResult = "strerror_r获取错误信息失败, 原始错误状态码：" + std::to_string(errstat);
				else
					szResult = "strerror_r work wrong, source error code: " + std::to_string(errstat);
			}
#endif
		}
			break;
		}
	}
	else
	{
		if (errstat <= SNMPX_inconsistentName && errstat >= SNMPX_noError) {
			szResult = m_error_string[(chinese_desc ? 1 : 0)][errstat];
		}
		else if ((errstat >= SNMPX_usmUnsupportedSecLevels) && (errstat - SNMPX_usmUnsupportedSecLevels < 6)) {
			szResult = m_usm_error_string[(chinese_desc ? 1 : 0)][errstat - SNMPX_usmUnsupportedSecLevels];
		}
		else {
			if (chinese_desc)
				szResult = "未知的snmpx错误状态码：" + std::to_string(errstat);
			else
				szResult = "Unknow snmpx error_state: " + std::to_string(errstat);
		}
	}
	
	return std::move(szResult);
}

void CErrorStatus::set_usm_error_status(oid* oid_buf, unsigned int oid_buf_len, snmpx_t *s)
{
	if ((unsigned int)sizeof(usmStatsUnsupportedSecLevelsOid) == oid_buf_len)
	{
		//在返回模式下,解包需要判断一下,根据返回的oid判断是否存在错误
		if ((s->msgFlags & 0x04) == 0x00 && s->tag == SNMPX_MSG_REPORT) //0x04表示获取agent引擎ID
		{
			if (s->error_status == 0)
			{
				if (memcmp(oid_buf, usmStatsUnsupportedSecLevelsOid, oid_buf_len) == 0) {
					s->error_status = SNMPX_usmUnsupportedSecLevels;
				}
				if (memcmp(oid_buf, usmStatsNotInTimeWindowsOid, oid_buf_len) == 0) {
					s->error_status = SNMPX_usmNotInTimeWindows;
				}
				if (memcmp(oid_buf, usmStatsUnknownUserNamesOid, oid_buf_len) == 0) {
					s->error_status = SNMPX_usmUnknownUserNames;
				}
				if (memcmp(oid_buf, usmStatsUnknownEngineIDsOid, oid_buf_len) == 0) {
					s->error_status = SNMPX_usmUnknownEngineIDs;
				}
				if (memcmp(oid_buf, usmStatsWrongDigestsOid, oid_buf_len) == 0) {
					s->error_status = SNMPX_usmWrongDigests;
				}
				if (memcpy(oid_buf, usmStatsDecryptionErrorsOid, oid_buf_len) == 0) {
					s->error_status = SNMPX_usmDecryptionErrors;
				}
			}
		}
	}

	return;
}

std::string CErrorStatus::format_err_msg(const char *format, ...)
{
	std::string szRet;

	if (NULL != format)
	{
		char buff[MAX_ERR_BUFF_LEN + 1] = { '\0' };
		int iRet = 0;
		va_list ap;
		va_start(ap, format);

		//格式化
#ifdef _WIN32
		iRet = vsprintf_s(buff, MAX_ERR_BUFF_LEN, format, ap);
#else
		iRet = vsnprintf(buff, MAX_ERR_BUFF_LEN, format, ap);
#endif

		va_end(ap);

		if (iRet >= 0)
			szRet = buff;
		else
			szRet = "format_err_msg call vsnprintf failed.";
	}

	return std::move(szRet);
}