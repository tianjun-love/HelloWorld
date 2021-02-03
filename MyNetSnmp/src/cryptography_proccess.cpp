#include "../include/cryptography_proccess.hpp"
#include <openssl/aes.h>
#include <openssl/des.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <stdlib.h>
#include <time.h>

CCryptographyProccess::CCryptographyProccess()
{
}

CCryptographyProccess::~CCryptographyProccess()
{
}

/********************************************************************
功能：	aes cfb128加密
参数：	buf 待加密数据
*		buf_len 待加密数据长度
*		key 密码
*		iv IV
*		encode_buf 加密后的数据，调用才申请内存
返回：	加密后的数据长度，<0：失败
*********************************************************************/
int CCryptographyProccess::snmpx_aes_encode(const unsigned char* buf , unsigned int buf_len, const unsigned char* key, unsigned char* iv, 
	unsigned char* encode_buf)
{
	AES_KEY aes;
	int ret = 0;

	//设置key
	ret = AES_set_encrypt_key((unsigned char*)key, 128, &aes);
	if(ret < 0)
	{
		m_szErrorMsg = "AES_set_encrypt_key failed.";
		return ret;
	}

	//加密  
	int num = 0; //记录己处理的长度，每处理128位后，内部置0
	AES_cfb128_encrypt(buf, encode_buf, buf_len, &aes, iv, &num, AES_ENCRYPT);

	return (int)buf_len;
}

/********************************************************************
功能：	aes cfb128解密
参数：	buf 待解密数据
*		buf_len 待解密数据长度
*		key 密码
*		iv IV
*		decode_buf 解密后的数据，调用才申请内存
返回：	解密后的数据长度，<0：失败
*********************************************************************/
int CCryptographyProccess::snmpx_aes_decode(const unsigned char* buf, unsigned int buf_len, const unsigned char* key, unsigned char* iv,
	unsigned char* decode_buf)
{
	AES_KEY aes;
	int ret = 0;

	//设置key
	ret = AES_set_encrypt_key((unsigned char*)key, 128, &aes);
	if (ret < 0)
	{
		m_szErrorMsg = "AES_set_decrypt_key failed.";
		return ret;
	}

	//解密
	int num = 0; //记录己处理的长度，每处理128位后，内部置0
	AES_cfb128_encrypt(buf, decode_buf, buf_len, &aes, iv, &num, AES_DECRYPT);
	
	return (int)buf_len;
}

/********************************************************************
功能：	des cbc加密
参数：	buf 待加密数据
*		buf_len 待加密数据长度
*		key 密码
*		iv IV
*		ivlen IV长度
*		encode_buf 加密后的数据，调用才申请内存
返回：	加密后的数据长度，<0：失败
*********************************************************************/
int CCryptographyProccess::snmpx_des_encode(const unsigned char* buf, unsigned int buf_len, const unsigned char* key, unsigned char* iv, 
	unsigned int ivlen, unsigned char* encode_buf)
{
	unsigned char iv_temp[8] = { 0 };
	const unsigned int max_pad_size = 8;
	unsigned char pad_block[max_pad_size] = { 0 };
	unsigned int pad = 0, plast = 0;

	DES_key_schedule key_sched_store;
	DES_key_schedule *key_sch = &key_sched_store;
	DES_cblock key_struct;

	//计算填充
	pad = max_pad_size - (buf_len % max_pad_size);

	if (pad == max_pad_size)
	{
		pad = 0;
		plast = buf_len;
	}
	else
		plast = buf_len - (max_pad_size - pad);

	if (pad > 0)  /* copy data into pad block if needed */
	{
		memcpy(pad_block, buf + plast, max_pad_size - pad);
		memset(pad_block + (max_pad_size - pad), pad, pad);   /* filling in padblock */
	}

	//设置key
	memcpy(key_struct, key, sizeof(key_struct));
	(void)DES_key_sched(&key_struct, key_sch);

	//设置IV
	memcpy(iv_temp, iv, ivlen);
	
	//加密
	DES_ncbc_encrypt(buf, encode_buf, plast, key_sch, (DES_cblock *)iv_temp, DES_ENCRYPT);

	if (pad > 0)
	{
		//加密填充块
		DES_ncbc_encrypt(pad_block, encode_buf + plast, max_pad_size, key_sch, (DES_cblock *)iv_temp, DES_ENCRYPT);

		return plast + max_pad_size;
	}

	return plast;
}

/********************************************************************
功能：	des cbc解密
参数：	buf 待解密数据
*		buf_len 待解密数据长度
*		key 密码
*		iv IV
*		ivlen IV长度
*		decode_buf 解密后的数据，调用才申请内存
返回：	解密后的数据长度，<0：失败
*********************************************************************/
int CCryptographyProccess::snmpx_des_decode(const unsigned char* buf, unsigned int buf_len, const unsigned char* key, unsigned char* iv,
	unsigned int ivlen, unsigned char* decode_buf)
{
	if (0 != (buf_len % 8))
	{
		m_szErrorMsg = format_err_msg("Des decode failed, buf_len must integral multiple of 8, it length[%u].",
			buf_len);
		return SNMPX_failure;
	}

	unsigned char iv_temp[8] = { 0 };

	DES_cblock key_struct;
	DES_key_schedule key_sched_store;
	DES_key_schedule *key_sch = &key_sched_store;

	//设置key
	memcpy(key_struct, key, sizeof(key_struct));
	(void)DES_key_sched(&key_struct, key_sch);

	//设置IV
	memcpy(iv_temp, iv, ivlen);

	//解密
	DES_cbc_encrypt(buf, decode_buf, buf_len, key_sch, (DES_cblock *)iv_temp, DES_DECRYPT);

	return (int)buf_len;
}

/********************************************************************
功能：	随机数生成算法
参数：	msgPrivacyParameters 存放buff
*		msgPrivacyParameters_len 长度，必须是8
返回：	成功返回0
*********************************************************************/
int CCryptographyProccess::gen_msgPrivacyParameters(unsigned char* msgPrivacyParameters, unsigned int msgPrivacyParameters_len)
{
	if (NULL == msgPrivacyParameters) {
		m_szErrorMsg = "msgPrivacyParameters buffer is null.";
		return SNMPX_failure;
	}

	if (msgPrivacyParameters_len != 8) {
		m_szErrorMsg = "msgPrivacyParameters_len must be 8.";
		return SNMPX_failure;
	}

	for (unsigned int i = 0; i < msgPrivacyParameters_len; ++i) 
	{
		//注意全局srand()初始化
		msgPrivacyParameters[i] = (unsigned char)(rand() % 255);
	}

	return SNMPX_noError;
}

/********************************************************************
功能：	生成消息的HMAC
参数：	authkey 认证密码
*		authkey_len 认证密码长度
*		msgAuthoritativeEngineID 对端引擎ID
*		msgAuthoritativeEngineID_len 对端引擎ID长度
*		authMode 认证类型，0：MD5，1：SHA1
*		message 组好的包数据
*		msglen 组好的包数据长度
*		cur_msgAuthenticationParameters 生成的认证参数
*		cur_msgAuthenticationParameters_len 生成的认证参数长度
返回：	成功返回值0
*********************************************************************/
int CCryptographyProccess::gen_msgHMAC(unsigned char* authkey, unsigned int authkey_len,
	unsigned char* msgAuthoritativeEngineID, const unsigned int msgAuthoritativeEngineIDLen,
	unsigned char authMode, const unsigned char* message, const unsigned int msglen,
	unsigned char *msgAuthenticationParameters, unsigned int* msgAuthenticationParameters_len)
{
	/* hash算法取前96位 */
	if (*msgAuthenticationParameters_len != 12)
	{
		m_szErrorMsg = format_err_msg("gen_msgHMAC failed, msgAuthenticationParameters_len must 12, it provide[%u].",
			*msgAuthenticationParameters_len);
		return SNMPX_failure;
	}

	unsigned char buf[512] = { 0 };
	unsigned int buf_len = 512;

	if (0 == authMode)
		HMAC(EVP_md5(), authkey, authkey_len, message, msglen, buf, &buf_len);
	else if (1 == authMode)
		HMAC(EVP_sha1(), authkey, authkey_len, message, msglen, buf, &buf_len);
	else
	{
		m_szErrorMsg = format_err_msg("gen_msgHMAC failed, not support authMode[0x%02X].",
			authMode);
		return SNMPX_failure;
	}

	if (*msgAuthenticationParameters_len > buf_len)
		*msgAuthenticationParameters_len = buf_len;

	memcpy(msgAuthenticationParameters, buf, *msgAuthenticationParameters_len);

	return SNMPX_noError;
}

/********************************************************************
功能：	组好包后,生成md5码，函数从要加密的msg,查找到已在包生成的md5码,并置为0x00,再做一次md5码生成算法
参数：	authkey 认证密码
*		authkey_len 认证密码长度
*		msgAuthoritativeEngineID 对端引擎ID
*		msgAuthoritativeEngineID_len 对端引擎ID长度
*		authMode 认证类型，0：MD5，1：SHA1
*		message 组好的包数据
*		msglen 组好的包数据长度
*		cur_msgAuthenticationParameters 当前认证参数
*		cur_msgAuthenticationParameters_len 当前认证参数长度
*		real_msgAuthenticationParameters 生成的认证参数
*		real_msgAuthenticationParameters_len 生成的认证参数长度
返回：	成功返回值为md5码所在包的位置
*********************************************************************/
int CCryptographyProccess::gen_pkg_HMAC(unsigned char* authkey, unsigned int authkey_len,
	unsigned char* msgAuthoritativeEngineID , const unsigned int msgAuthoritativeEngineID_len ,
	unsigned char authMode, const unsigned char* message, const unsigned int msglen,
	const unsigned char* cur_msgAuthenticationParameters , const unsigned int cur_msgAuthenticationParameters_len ,
	unsigned char* real_msgAuthenticationParameters  , unsigned int real_msgAuthenticationParameters_len)
{
	int rval = 0;
	unsigned char* count_md5_buf = (unsigned char*)malloc(msglen * sizeof(unsigned char*));
	memcpy(count_md5_buf, message, msglen);

	int lac = find_str_lac(count_md5_buf, msglen, cur_msgAuthenticationParameters, cur_msgAuthenticationParameters_len);
	if(lac <= 0)
	{
		m_szErrorMsg = "gen_pkg_real_HMAC failed: not found msgAuthenticationParameters.";
		rval = -1;
	}
	else
	{
		memset(count_md5_buf + lac, 0x00, cur_msgAuthenticationParameters_len);

		if (gen_msgHMAC(authkey, authkey_len, msgAuthoritativeEngineID, msgAuthoritativeEngineID_len, authMode, count_md5_buf, 
			msglen, real_msgAuthenticationParameters, &real_msgAuthenticationParameters_len) < 0)
		{
			m_szErrorMsg = "gen_pkg_real_HMAC failed: " + m_szErrorMsg;
			rval = -1;
		}
		else
			rval = lac;
	}

	//清理
	free(count_md5_buf);

	return rval;
}

/********************************************************************
功能：	对用户的认证密码进行第一次加密，生成hash
参数：	authPasswd 认证密码
*		authPasswd_len 认证密码长度
*		authMode 认证类型，0：MD5，1：SHA
*		Ku 加密后数据
*		Ku_len 加密后数据长度
返回：	成功返回0
*********************************************************************/
int CCryptographyProccess::get_user_hash_ku(const unsigned char* authPasswd, unsigned int authPasswd_len, unsigned char authMode, 
	unsigned char *Ku, unsigned int *Ku_len)
{
	int nbytes = 1024 * 1024; //net-snmp源码包里的定义：USM_LENGTH_EXPANDED_PASSPHRASE
	unsigned int i, pindex = 0;
	unsigned char buf[64], *bufp; //net-snmp源码包里的定义：USM_LENGTH_KU_HASHBLOCK
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();

	if (NULL != ctx)
	{
		//成功返回1，失败返回0
		if (0 == authMode)
		{
			if (0 == EVP_DigestInit(ctx, EVP_md5())) {
				EVP_MD_CTX_destroy(ctx);
				m_szErrorMsg = "get_user_md5_ku failed: EVP_DigestInit md5 failed.";
				return SNMPX_failure;
			}
		}
		else
		{
			if (0 == EVP_DigestInit(ctx, EVP_sha1())) {
				EVP_MD_CTX_destroy(ctx);
				m_szErrorMsg = "get_user_md5_ku failed: EVP_DigestInit sha1 failed.";
				return SNMPX_failure;
			}
		}

		//net-snmp源码包里的定义
		while (nbytes > 0)
		{
			bufp = buf;

			for (i = 0; i < 64; i++) {
				*bufp++ = authPasswd[pindex++ % authPasswd_len];
			}

			EVP_DigestUpdate(ctx, buf, 64);
			nbytes -= 64;
		}

		unsigned int tmp_len = *Ku_len;
		EVP_DigestFinal(ctx, Ku, &tmp_len);
		*Ku_len = tmp_len;

		EVP_MD_CTX_destroy(ctx);
	}
	else
	{
		m_szErrorMsg = "get_user_md5_ku failed: EVP_MD_CTX_create return NULL.";
		return SNMPX_failure;
	}

	return SNMPX_noError;
}

/********************************************************************
功能：	对用户的认证密码进行第二次加密，生成hash
参数：	engineID 引擎ID
*		engineID_len 引擎ID长度
*		authMode 认证类型，0：MD5，1：SHA
*		Ku 第一次生成的hash
*		Ku_len 第一次生成的hash长度
*		Kul 第二次生成的hash
*		Kul_len 第二次生成的hash长度
返回：	成功返回0
*********************************************************************/
int CCryptographyProccess::get_user_hash_kul(const unsigned char* engineID, unsigned int engineID_len, unsigned char authMode,
	const unsigned char* Ku, unsigned int Ku_len, unsigned char* Kul, unsigned int* Kul_len)
{
	const unsigned int properlength = (0 == authMode ? 16 : 20); //第一次生成的hash长度，字节
	unsigned int nbytes = 0;
	unsigned char buf[1024];

	memcpy(buf, Ku, properlength);
	nbytes += properlength;
	memcpy(buf + nbytes, engineID, engineID_len);
	nbytes += engineID_len;
	memcpy(buf + nbytes, Ku, properlength);
	nbytes += properlength;

	EVP_MD_CTX *ctx = EVP_MD_CTX_create();

	if (NULL != ctx)
	{
		//成功返回1，失败返回0
		if (0 == authMode)
		{
			if (0 == EVP_DigestInit(ctx, EVP_md5())) {
				EVP_MD_CTX_destroy(ctx);
				m_szErrorMsg = "get_user_md5_kul failed: EVP_DigestInit md5 failed.";
				return SNMPX_failure;
			}
		}
		else
		{
			if (0 == EVP_DigestInit(ctx, EVP_sha1())) {
				EVP_MD_CTX_destroy(ctx);
				m_szErrorMsg = "get_user_md5_kul failed: EVP_DigestInit sha1 failed.";
				return SNMPX_failure;
			}
		}

		EVP_DigestUpdate(ctx, buf, nbytes);

		unsigned int tmp_len = *Kul_len;
		EVP_DigestFinal(ctx, Kul, &tmp_len);
		*Kul_len = tmp_len;

		EVP_MD_CTX_destroy(ctx);
	}
	else
	{
		m_szErrorMsg = "get_user_md5_kul failed: EVP_MD_CTX_create return NULL.";
		return SNMPX_failure;
	}

	return SNMPX_noError;
}