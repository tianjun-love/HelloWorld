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

	if (user_info->version <= SNMPX_VERSION_v2c) {
		//v1,v2c只需要团体名
	}
	else if (user_info->version == SNMPX_VERSION_v3) 
	{
		if (user_info->safeMode == SNMPX_SEC_LEVEL_noAuth) {
			//不认证，不加密
		}
		else if (user_info->safeMode == SNMPX_SEC_LEVEL_authNoPriv) {
			if (calc_hash_user_auth(user_info) < 0) {
				m_szErrorMsg = "snmpx_user_init failed: " + m_szErrorMsg;
				return SNMPX_failure;
			}
		}
		else if (user_info->safeMode == SNMPX_SEC_LEVEL_authPriv) {
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
	unsigned char userKey[64] = { 0 }; //最大64
	unsigned int userKeyLen = 64;
	unsigned char privKey[64] = { 0 }; //最大不会超过64
	unsigned int privKeyLen = 64;
	CCryptographyProccess crypto;

	if (user_info->AuthMode <= SNMPX_AUTH_SHA512)
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
	unsigned char userKey[64] = { 0 }; //最大64
	unsigned int userKeyLen = 64;
	unsigned char privKey[64] = { 0 }; //最大不会超过64
	unsigned int privKeyLen = 64;
	CCryptographyProccess crypto;

	if (user_info->AuthMode <= SNMPX_AUTH_SHA512)
	{
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

		//再使用msgAuthoritativeEngineID 进行二次加密
		if (crypto.get_user_hash_kul(user_info->msgAuthoritativeEngineID, user_info->msgAuthoritativeEngineID_len, user_info->AuthMode,
			userKey, userKeyLen, privKey, &privKeyLen) != 0) {
			m_szErrorMsg = "calc_hash_user_auth_priv failed: " + crypto.GetErrorMsg();
			return SNMPX_failure;
		}

		//判断密钥长度是否足够
		const unsigned int priv_key_len = get_priv_key_length(user_info->PrivMode);

		if (priv_key_len > privKeyLen)
		{
			unsigned char hash[64] = { 0 }; //最大64
			unsigned int hash_len = 64;

			//计算
			if (crypto.gen_data_HASH(privKey, privKeyLen, user_info->AuthMode, hash, &hash_len) != 0) {
				m_szErrorMsg = "calc_hash_user_auth_priv failed: " + crypto.GetErrorMsg();
				return SNMPX_failure;
			}

			user_info->privPasswdPrivKey_len = priv_key_len;
			user_info->privPasswdPrivKey = (unsigned char*)malloc(user_info->privPasswdPrivKey_len * sizeof(unsigned char));
			memcpy(user_info->privPasswdPrivKey, privKey, privKeyLen);

			//目前最长密钥(AES256)是32，所以两次hash肯定足够了
			memcpy(user_info->privPasswdPrivKey + privKeyLen, hash, (user_info->privPasswdPrivKey_len - privKeyLen));
		}
		else
		{
			user_info->privPasswdPrivKey_len = priv_key_len;
			user_info->privPasswdPrivKey = (unsigned char*)malloc(user_info->privPasswdPrivKey_len * sizeof(unsigned char));
			memcpy(user_info->privPasswdPrivKey, privKey, user_info->privPasswdPrivKey_len);
		}
	}
	else {
		m_szErrorMsg = format_err_msg("calc_hash_user_auth_priv failed: unsupport authMode[0x%02X].",
			user_info->AuthMode);
		return SNMPX_failure;
	}

	return SNMPX_noError;
}