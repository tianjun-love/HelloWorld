/********************************************************************
功能:	加解密类
作者:	田俊
时间:	2021-01-07
修改:
*********************************************************************/
#ifndef __CRYPTOGRAPHY_PROCCESS_HPP__
#define __CRYPTOGRAPHY_PROCCESS_HPP__

#include "error_status.hpp"

class CCryptographyProccess : public CErrorStatus
{
public:
	CCryptographyProccess();
	~CCryptographyProccess();

	//aes
	int snmpx_aes_encode(const unsigned char* buf, unsigned int buf_len, const unsigned char* key, unsigned char* iv, 
		unsigned char* encode_buf);
	int snmpx_aes_decode(const unsigned char* buf, unsigned int buf_len, const unsigned char* key, unsigned char* iv, 
		unsigned char* decode_buf);

	//des
	int snmpx_des_encode(const unsigned char* buf, unsigned int buf_len, const unsigned char* key, unsigned char* iv, 
		unsigned int ivlen, unsigned char* encode_buf);
	int snmpx_des_decode(const unsigned char* buf, unsigned int buf_len, const unsigned char* key, unsigned char* iv, 
		unsigned int ivlen, unsigned char* decode_buf);

	//生成加密随机数参数
	int gen_msgPrivacyParameters(unsigned char* msgPrivacyParameters, unsigned int msgPrivacyParameters_len);

	//生成消息HMAC
	int gen_msgHMAC(unsigned char* authkey, unsigned int authkey_len, 
		unsigned char* msgAuthoritativeEngineID, const unsigned int msgAuthoritativeEngineIDLen, 
		unsigned char authMode, const unsigned char* message, const unsigned int msglen,
		unsigned char *msgAuthenticationParameters, unsigned int* msgAuthenticationParameters_len);

	//计算包的HMAC
	int gen_pkg_HMAC(unsigned char* authkey, unsigned int authkey_len,
		unsigned char* msgAuthoritativeEngineID, const unsigned int msgAuthoritativeEngineID_len,
		unsigned char authMode, const unsigned char* message, const unsigned int msglen,
		const unsigned char* cur_msgAuthenticationParameters, const unsigned int cur_msgAuthenticationParameters_len,
		unsigned char* real_msgAuthenticationParameters, unsigned int real_msgAuthenticationParameters_len);

	//对用户的认证密码进行第一次加密
	int get_user_hash_ku(const unsigned char* authPasswd, unsigned int authPasswd_len, unsigned char authMode, 
		unsigned char *Ku, unsigned int *Ku_len);

	//对用户的认证密码进行第二次加密
	int get_user_hash_kul(const unsigned char* engineID, unsigned int engineID_len, unsigned char authMode, 
		const unsigned char* Ku, unsigned int Ku_len, unsigned char* Kul, unsigned int* Kul_len);

};

#endif