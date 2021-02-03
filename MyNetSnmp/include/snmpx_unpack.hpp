#ifndef __SNMPX_UNPACK_HPP__
#define __SNMPX_UNPACK_HPP__

#include "ber_code.hpp"

class CSnmpxUnpack : public CErrorStatus
{
public:
	CSnmpxUnpack();
	~CSnmpxUnpack();

	//解码，当是多用户时，用户表map<用户名/团体名+"_"+版本, 用户对象>，如：map<"public_V2c", obj*>
	//在做服务端解码时，user_auth_info_cache存放agent的认证信息缓存，pair<map<ip, userinto_t*>*, mutex*>
	int snmpx_group_unpack(unsigned char* buf, unsigned int buf_len, struct snmpx_t *r, bool is_get_v3_engineID, 
		const void *user_info, bool is_user_info_multi, const void *user_auth_info_cache = NULL);

	//绑定的值转用户对象
	static void snmpx_get_vb_value(const variable_bindings* variable_binding, SSnmpxValue* value);

private:
	int tlv_unpack(unsigned char* tlv, unsigned int tlv_len, std::list<struct tlv_data*> &mglist);
	int unpack_oid_value(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s);
	int unpack_item(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s);
	int unpack_cmd(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s);
	int unpack_trap_v1_cmd(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s);
	int unpack_v3_msgGlobalData(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s);
	int unpack_v3_msgData(unsigned char*buf, unsigned buf_len, struct snmpx_t* s);
	int unpack_v3_authData(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s);
	int check_rc_snmpx_data(struct snmpx_t* r, const userinfo_t* t_user, unsigned char* buf, unsigned int buf_len);
	int decrypt_pdu(const unsigned char* in, unsigned int in_len, const userinfo_t* u, const snmpx_t *r, 
		unsigned char *out, unsigned int *out_len);

	static bool check_octet_str_printable(unsigned char* str, unsigned int len, bool check_chinese = false);
	static const userinfo_t* generate_agent_user_info(const userinfo_t *t_user, const struct snmpx_t* r, const void *user_authinfo_cache,
		std::string &error);

private:
	CBerCode    m_BerCode;    //BER-code对象

};

#endif
