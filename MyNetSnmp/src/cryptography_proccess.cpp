#include "../include/cryptography_proccess.hpp"
#include <openssl/aes.h>
#include <openssl/des.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <stdlib.h>
#include <time.h>

static const EVP_MD* get_auth_hash_function(unsigned char authMode)
{
	const EVP_MD *hashfn = NULL;

	switch (authMode)
	{
	case SNMPX_AUTH_MD5:
		hashfn = EVP_md5();
		break;
	case SNMPX_AUTH_SHA:
		hashfn = EVP_sha1();
		break;
	case SNMPX_AUTH_SHA224:
		hashfn = EVP_sha224();
		break;
	case SNMPX_AUTH_SHA256:
		hashfn = EVP_sha256();
		break;
	case SNMPX_AUTH_SHA384:
		hashfn = EVP_sha384();
		break;
	case SNMPX_AUTH_SHA512:
		hashfn = EVP_sha512();
		break;
	default:
		hashfn = EVP_md5(); //默认MD5
		break;
	}

	return hashfn;
}

static const EVP_CIPHER* get_priv_cipher(unsigned char privMode)
{
	const EVP_CIPHER *cipher = NULL;

	switch (privMode)
	{
	case SNMPX_PRIV_DES:
		cipher = EVP_des_cbc();
		break;
	case SNMPX_PRIV_AES:
		cipher = EVP_aes_128_cfb();
		break;
	case SNMPX_PRIV_AES192:
		cipher = EVP_aes_192_cfb();
		break;
	case SNMPX_PRIV_AES256:
		cipher = EVP_aes_256_cfb();
		break;
	default:
		cipher = EVP_aes_128_cfb(); //默认AES
		break;
	}

	return cipher;
}

CCryptographyProccess::CCryptographyProccess()
{
}

CCryptographyProccess::~CCryptographyProccess()
{
}

/********************************************************************
功能：	aes cfb加密
参数：	buf 待加密数据
*		buf_len 待加密数据长度
*		key 密码
*		iv IV
*		priv_mode 加密算法
*		encode_buf 加密后的数据，调用才申请内存
返回：	加密后的数据长度，<0：失败
*********************************************************************/
int CCryptographyProccess::snmpx_aes_encode(const unsigned char* buf , unsigned int buf_len, const unsigned char* key, unsigned char* iv,
	unsigned char priv_mode, unsigned char* encode_buf)
{
	int ret = SNMPX_failure;
	const EVP_CIPHER *cipher = get_priv_cipher(priv_mode);
	EVP_CIPHER_CTX *ctx = NULL;

	ctx = EVP_CIPHER_CTX_new();
	if (NULL != ctx)
	{
		//成功返回1，失败返回0
		if (1 == EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv))
		{
			int enclen = 0, outl = 0;

			if (1 == EVP_EncryptUpdate(ctx, encode_buf, &outl, buf, (int)buf_len))
			{
				enclen = outl;

				if (1 == EVP_EncryptFinal_ex(ctx, encode_buf + outl, &outl))
				{
					enclen += outl;
					ret = enclen;
				}
				else
					m_szErrorMsg = "snmpx_aes_encode failed: EVP_EncryptFinal_ex return 0.";
			}
			else
				m_szErrorMsg = "snmpx_aes_encode failed: EVP_EncryptUpdate return 0.";
		}
		else
			m_szErrorMsg = "snmpx_aes_encode failed: EVP_EncryptInit_ex return 0.";

		EVP_CIPHER_CTX_free(ctx);
	}
	else
		m_szErrorMsg = "snmpx_aes_encode failed: EVP_CIPHER_CTX_new return NULL.";

	return ret;
}

/********************************************************************
功能：	aes cfb解密
参数：	buf 待解密数据
*		buf_len 待解密数据长度
*		key 密码
*		iv IV
*		priv_mode 加密算法
*		decode_buf 解密后的数据，调用才申请内存
返回：	解密后的数据长度，<0：失败
*********************************************************************/
int CCryptographyProccess::snmpx_aes_decode(const unsigned char* buf, unsigned int buf_len, const unsigned char* key, unsigned char* iv,
	unsigned char priv_mode, unsigned char* decode_buf)
{
	int ret = SNMPX_failure;
	const EVP_CIPHER *cipher = get_priv_cipher(priv_mode);
	EVP_CIPHER_CTX *ctx = NULL;

	ctx = EVP_CIPHER_CTX_new();
	if (NULL != ctx)
	{
		//成功返回1，失败返回0
		if (1 == EVP_DecryptInit_ex(ctx, cipher, NULL, key, iv))
		{
			int declen = 0, outl = 0;

			if (1 == EVP_DecryptUpdate(ctx, decode_buf, &outl, buf, (int)buf_len))
			{
				declen = outl;

				if (1 == EVP_DecryptFinal_ex(ctx, decode_buf + outl, &outl))
				{
					declen += outl;
					ret = declen;
				}
				else
					m_szErrorMsg = "snmpx_aes_decode failed: EVP_EncryptFinal_ex return 0.";
			}
			else
				m_szErrorMsg = "snmpx_aes_decode failed: EVP_EncryptUpdate return 0.";
		}
		else
			m_szErrorMsg = "snmpx_aes_decode failed: EVP_EncryptInit_ex return 0.";

		EVP_CIPHER_CTX_free(ctx);
	}
	else
		m_szErrorMsg = "snmpx_aes_decode failed: EVP_CIPHER_CTX_new return NULL.";

	return ret;
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

	if (msgPrivacyParameters_len != SNMPX_PRIVACY_PARAM_LEN) {
		m_szErrorMsg = format_err_msg("msgPrivacyParameters_len must be %d.", SNMPX_PRIVACY_PARAM_LEN);
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
*		authMode 认证类型，0：MD5，1：SHA，2:SHA224，3:SHA256，4:SHA384，5:SHA512
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
	const unsigned int auth_hash_length = get_auth_hmac_length(authMode);

	if (*msgAuthenticationParameters_len != auth_hash_length)
	{
		m_szErrorMsg = format_err_msg("gen_msgHMAC failed, msgAuthenticationParameters_len must be %u, it provide[%u].",
			auth_hash_length, *msgAuthenticationParameters_len);
		return SNMPX_failure;
	}

	unsigned char buf[64] = { 0 };
	unsigned int buf_len = 64;

	//计算
	HMAC(get_auth_hash_function(authMode), authkey, authkey_len, message, msglen, buf, &buf_len);

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
*		authMode 认证类型，0：MD5，1：SHA，2:SHA224，3:SHA256，4:SHA384，5:SHA512
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
功能：	生成数据的hash
参数：	data 数据
*		data_len 数据长度
*		hashType hash类型，0：MD5，1：SHA，2:SHA224，3:SHA256，4:SHA384，5:SHA512
*		hash 生成的hash数据
*		hash_len 生成的hash数据长度
返回：	成功返回0
*********************************************************************/
int CCryptographyProccess::gen_data_HASH(const unsigned char* data, unsigned int data_len, unsigned char hashType, unsigned char* hash,
	unsigned int* hash_len)
{
	if (NULL == data || 0 == data_len || hashType > SNMPX_AUTH_SHA512 || NULL == hash || NULL == hash_len)
	{
		m_szErrorMsg = "gen_data_HASH failed: parameter wrong.";
		return SNMPX_failure;
	}

	int ret = SNMPX_failure;
	const EVP_MD *hashfn = get_auth_hash_function(hashType);
	EVP_MD_CTX *ctx = NULL;

	ctx = EVP_MD_CTX_create();
	if (NULL != ctx)
	{
		//成功返回1，失败返回0
		if (1 == EVP_DigestInit_ex(ctx, hashfn, NULL))
		{
			if (1 == EVP_DigestUpdate(ctx, data, data_len))
			{
				if (1 == EVP_DigestFinal_ex(ctx, hash, hash_len))
					ret = SNMPX_noError;
				else
					m_szErrorMsg = "get_user_md5_ku failed: EVP_DigestFinal_ex return 0.";
			}
			else
				m_szErrorMsg = "gen_data_HASH failed: EVP_DigestUpdate return 0.";
		}
		else
			m_szErrorMsg = "gen_data_HASH failed: EVP_DigestInit_ex return 0.";

		EVP_MD_CTX_destroy(ctx);
	}
	else
		m_szErrorMsg = "gen_data_HASH failed: EVP_MD_CTX_create return NULL.";

	return ret;
}

/********************************************************************
功能：	对用户的认证密码进行第一次加密，生成hash
参数：	authPasswd 认证密码
*		authPasswd_len 认证密码长度
*		authMode 认证类型，0：MD5，1：SHA，2:SHA224，3:SHA256，4:SHA384，5:SHA512
*		Ku 加密后数据
*		Ku_len 加密后数据长度
返回：	成功返回0
*********************************************************************/
int CCryptographyProccess::get_user_hash_ku(const unsigned char* authPasswd, unsigned int authPasswd_len, unsigned char authMode, 
	unsigned char *Ku, unsigned int *Ku_len)
{
	int ret = SNMPX_failure;
	const EVP_MD *hashfn = get_auth_hash_function(authMode);
	EVP_MD_CTX *ctx = NULL;
	
	ctx = EVP_MD_CTX_create();
	if (NULL != ctx)
	{
		//成功返回1，失败返回0
		if (1 == EVP_DigestInit_ex(ctx, hashfn, NULL))
		{
			bool updateFlag = true;
			int nbytes = 1024 * 1024; //net-snmp源码包里的定义：USM_LENGTH_EXPANDED_PASSPHRASE
			unsigned int i, pindex = 0;
			unsigned char buf[64], *bufp; //net-snmp源码包里的定义：USM_LENGTH_KU_HASHBLOCK

			//net-snmp源码包里的定义
			while (nbytes > 0)
			{
				bufp = buf;

				for (i = 0; i < 64; i++) {
					*bufp++ = authPasswd[pindex++ % authPasswd_len];
				}

				if (1 == EVP_DigestUpdate(ctx, buf, 64))
					nbytes -= 64;
				else
				{
					updateFlag = false;
					m_szErrorMsg = "get_user_md5_ku failed: EVP_DigestUpdate return 0.";
					break;
				}
			}

			if (updateFlag)
			{
				if (1 == EVP_DigestFinal_ex(ctx, Ku, Ku_len))
					ret = SNMPX_noError;
				else
					m_szErrorMsg = "get_user_md5_ku failed: EVP_DigestFinal_ex return 0.";
			}
		}
		else
			m_szErrorMsg = "get_user_md5_ku failed: EVP_DigestInit_ex return 0.";

		EVP_MD_CTX_destroy(ctx);
	}
	else
		m_szErrorMsg = "get_user_md5_ku failed: EVP_MD_CTX_create return NULL.";

	return ret;
}

/********************************************************************
功能：	对用户的认证密码进行第二次加密，生成hash
参数：	engineID 引擎ID
*		engineID_len 引擎ID长度
*		authMode 认证类型，0：MD5，1：SHA，2:SHA224，3:SHA256，4:SHA384，5:SHA512
*		Ku 第一次生成的hash
*		Ku_len 第一次生成的hash长度
*		Kul 第二次生成的hash
*		Kul_len 第二次生成的hash长度
返回：	成功返回0
*********************************************************************/
int CCryptographyProccess::get_user_hash_kul(const unsigned char* engineID, unsigned int engineID_len, unsigned char authMode,
	const unsigned char* Ku, unsigned int Ku_len, unsigned char* Kul, unsigned int* Kul_len)
{
	int ret = SNMPX_failure;
	unsigned int nbytes = 0;
	unsigned char buf[256] = { 0 }; //足够使用了

	//net-snmp源码包里的定义
	memcpy(buf, Ku, Ku_len);
	nbytes += Ku_len;
	memcpy(buf + nbytes, engineID, engineID_len);
	nbytes += engineID_len;
	memcpy(buf + nbytes, Ku, Ku_len);
	nbytes += Ku_len;

	//计算
	ret = gen_data_HASH(buf, nbytes, authMode, Kul, Kul_len);

	return ret;
}