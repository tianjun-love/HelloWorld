#include "../include/snmpx_user_proccess.hpp"
#include "../include/cryptography_proccess.hpp"
#include <stdlib.h>

CUserProccess::CUserProccess()
{
}

CUserProccess::~CUserProccess()
{
}

/********************************************************************
功能：	计算用户的私有的加密认证密钥
参数：	user_info 用户信息对象
返回：	成功返回0
*********************************************************************/
int CUserProccess::snmpx_user_init(struct userinfo_t *user_info)
{
	if (user_info == NULL) {
		m_szErrorMsg = "snmpx_user_init failed: user_info is NULL.";
		return SNMPX_failure;
	}

	if (strlen(user_info->userName) == 0) {
		m_szErrorMsg = "snmpx_user_init failed: user_info->userName length is 0.";
		return SNMPX_failure;
	}

	if (user_info->version <= 2) {
		//v1,v2c只需要团体名
	}
	else if (user_info->version == 3) 
	{
		if (user_info->safeMode == 0) {
			//不认证，不加密
		}
		else if (user_info->safeMode == 1) {
			if (calc_hash_user_auth(user_info) < 0) {
				m_szErrorMsg = "snmpx_user_init failed: " + m_szErrorMsg;
				return SNMPX_failure;
			}
		}
		else if (user_info->safeMode == 2) {
			if (calc_hash_user_auth(user_info) < 0) {
				m_szErrorMsg = "snmpx_user_init failed: " + m_szErrorMsg;
				return SNMPX_failure;
			}

			if (calc_hash_user_auth_priv(user_info) < 0) {
				m_szErrorMsg = "snmpx_user_init failed: " + m_szErrorMsg;
				return SNMPX_failure;
			}
		}
		else {
			m_szErrorMsg = format_err_msg("snmpx_user_init failed: not support safeMode[0x%02X].", 
				user_info->safeMode);
			return SNMPX_failure;
		}

	}
	else {
		m_szErrorMsg = format_err_msg("snmpx_user_init failed: not support version[0x%02X].",
			user_info->version);
		return SNMPX_failure;
	}

	return SNMPX_noError;
}


/********************************************************************
功能：	计算hash用户认证码
参数：	user_info 用户信息对象
返回：	成功返回0
*********************************************************************/
int CUserProccess::calc_hash_user_auth(struct userinfo_t *user_info)
{
	unsigned char userKey[32] = { 0 }; //net-snmp源码包定义：最小：USM_AUTH_KU_LEN
	unsigned int userKeyLen = 32;
	unsigned char privKey[512] = { 0 }; //net-snmp源码包定义：最小：USM_LENGTH_KU_HASHBLOCK，最大：SNMP_MAXBUF_SMALL
	unsigned int privKeyLen = 512;
	CCryptographyProccess crypto;

	if (user_info->AuthMode == 0 || user_info->AuthMode == 1)
	{
		if (strlen(user_info->AuthPassword) == 0) {
			m_szErrorMsg = "calc_hash_user_auth failed: user_info->AuthPassword is NULL.";
			return SNMPX_failure;
		}

		if (user_info->msgAuthoritativeEngineID == NULL || user_info->msgAuthoritativeEngineID_len == 0) {
			m_szErrorMsg = "calc_hash_user_auth failed: user_info->msgAuthoritativeEngineID is NULL.";
			return SNMPX_failure;
		}

		//不为空先释放
		if (user_info->authPasswordPrivKey != NULL && user_info->authPasswordPrivKey_len > 0) {
			free(user_info->authPasswordPrivKey);
			user_info->authPasswordPrivKey = NULL;
			user_info->authPasswordPrivKey_len = 0;
		}

		//计算
		if (crypto.get_user_hash_ku((const unsigned char*)user_info->AuthPassword, (unsigned int)strlen(user_info->AuthPassword), 
			user_info->AuthMode, userKey, &userKeyLen) != 0) {
			m_szErrorMsg = "calc_hash_user_auth failed: " + crypto.GetErrorMsg();
			return SNMPX_failure;
		}

		/* 再使用msgAuthoritativeEngineID 进行二次加密 */
		if (crypto.get_user_hash_kul(user_info->msgAuthoritativeEngineID, user_info->msgAuthoritativeEngineID_len, user_info->AuthMode,
			userKey, userKeyLen, privKey, &privKeyLen) != 0) {
			m_szErrorMsg = "calc_hash_user_auth failed: " + crypto.GetErrorMsg();
			return SNMPX_failure;
		}

		user_info->authPasswordPrivKey = (unsigned char*)malloc(privKeyLen * sizeof(unsigned char));
		user_info->authPasswordPrivKey_len = privKeyLen;
		memcpy(user_info->authPasswordPrivKey, privKey, privKeyLen);

	}
	else {
		m_szErrorMsg = format_err_msg("calc_hash_user_auth failed: unsupport authMode[0x%02X].",
			user_info->AuthMode);
		return SNMPX_failure;
	}

	return SNMPX_noError;
}

/********************************************************************
功能：	计算hash用户通信密码
参数：	user_info 用户信息对象
返回：	成功返回0
*********************************************************************/
int CUserProccess::calc_hash_user_auth_priv(struct userinfo_t *user_info)
{
	unsigned char userKey[32] = { 0 }; //net-snmp源码包定义：最小：USM_AUTH_KU_LEN
	unsigned int userKeyLen = 32;
	unsigned char privKey[512] = { 0 }; //net-snmp源码包定义：最小：USM_LENGTH_KU_HASHBLOCK，最大：SNMP_MAXBUF_SMALL
	unsigned int privKeyLen = 512;
	CCryptographyProccess crypto;

	if (strlen(user_info->AuthPassword) == 0 || strlen(user_info->PrivPassword) == 0) {
		m_szErrorMsg = "calc_hash_user_auth_priv failed: user_info->AuthPassword or user_info->PrivPassword is NULL.";
		return SNMPX_failure;
	}

	if (user_info->msgAuthoritativeEngineID == NULL || user_info->msgAuthoritativeEngineID_len == 0) {
		m_szErrorMsg = "calc_hash_user_auth_priv failed: user_info->msgAuthoritativeEngineID is NULL.";
		return SNMPX_failure;
	}

	//不为空先释放
	if (user_info->privPasswdPrivKey != NULL && user_info->privPasswdPrivKey_len > 0) {
		free(user_info->privPasswdPrivKey);
		user_info->privPasswdPrivKey = NULL;
		user_info->privPasswdPrivKey_len = 0;
	}

	//计算
	if (crypto.get_user_hash_ku((const unsigned char *)user_info->PrivPassword,
		(unsigned int)strlen(user_info->PrivPassword), user_info->AuthMode, userKey, &userKeyLen) != 0) {
		m_szErrorMsg = "calc_hash_user_auth_priv failed: " + crypto.GetErrorMsg();
		return SNMPX_failure;
	}

	/* 再使用msgAuthoritativeEngineID 进行二次加密 */
	if (crypto.get_user_hash_kul(user_info->msgAuthoritativeEngineID, user_info->msgAuthoritativeEngineID_len, user_info->AuthMode, 
		userKey, userKeyLen, privKey, &privKeyLen) != 0) {
		m_szErrorMsg = "calc_hash_user_auth_priv failed: " + crypto.GetErrorMsg();
		return SNMPX_failure;
	}

	user_info->privPasswdPrivKey = (unsigned char*)malloc(privKeyLen * sizeof(unsigned char));
	memcpy(user_info->privPasswdPrivKey, privKey, privKeyLen);
	user_info->privPasswdPrivKey_len = privKeyLen;

	return SNMPX_noError;
}

/********************************************************************
功能：	释放用户信息
参数：	user_info 用户信息对象
返回：	无
*********************************************************************/
void CUserProccess::snmpx_user_free(struct userinfo_t* user_info)
{
	if (NULL != user_info)
	{
		if (user_info->msgAuthoritativeEngineID != NULL) {
			free(user_info->msgAuthoritativeEngineID);
			user_info->msgAuthoritativeEngineID = NULL;
		}

		if (user_info->authPasswordPrivKey != NULL) {
			free(user_info->authPasswordPrivKey);
			user_info->authPasswordPrivKey = NULL;
		}

		if (user_info->privPasswdPrivKey != NULL) {
			free(user_info->privPasswdPrivKey);
			user_info->privPasswdPrivKey = NULL;
		}

		user_info->msgAuthoritativeEngineID_len = 0;
		user_info->authPasswordPrivKey_len = 0;
		user_info->privPasswdPrivKey_len = 0;

		free(user_info);
		user_info = NULL;
	}
}

/********************************************************************
功能：	释放用户信息列表
参数：	user_info_list 用户信息列表
返回：	无
*********************************************************************/
void CUserProccess::snmpx_user_map_free(std::map<std::string, userinfo_t*> &user_info_map)
{
	std::map<std::string, userinfo_t*>::iterator iter;

	for (iter = user_info_map.begin(); iter != user_info_map.end(); ++iter)
	{
		snmpx_user_free(iter->second);
		iter->second = NULL;
	}

	user_info_map.clear();
}