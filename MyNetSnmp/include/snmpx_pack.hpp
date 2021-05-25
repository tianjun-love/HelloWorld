#ifndef __SNMPX_PACK_HPP__
#define __SNMPX_PACK_HPP__

#include "ber_code.hpp"

class CSnmpxPack : public CErrorStatus
{
public:
	CSnmpxPack();
	~CSnmpxPack();

	//组包，agent使用
	int snmpx_pack(unsigned char*buf, struct snmpx_t* rc, struct snmpx_t* sd, const struct userinfo_t* user_info);

	//组包
	int snmpx_group_pack(unsigned char* buf, struct snmpx_t* s, const struct userinfo_t* user_info);

	//设置绑定值
	int snmpx_set_vb_list(std::list<variable_bindings*> *variable_bindings_list, const oid* rs_oid, unsigned int rs_oid_len,
		unsigned char tag, const unsigned char* buf, unsigned int len);

private:
	int tag_len_pack(unsigned int taglen, unsigned char* buf);
	int tlv_pack(unsigned char tag, unsigned int tag_len, unsigned char*  tag_buf, unsigned char* tlv_buf);
	int msgGlobalData_pack(struct snmpx_t* s, unsigned char* tlv_buf);
	int pack_variable_bindings(struct variable_bindings *tvb, unsigned char* tlv_buf);
	int pack_item(const std::list<variable_bindings*> *variable_bindings_list, unsigned char* tlv_buf);
	int pack_data(struct snmpx_t *s, unsigned char* tlv_buf);
	int pack_v1_trap_data(struct snmpx_t* s, unsigned char* tlv_buf);
	int pack_msgData(struct snmpx_t *s, unsigned char* tlv_buf);
	int pack_authData(struct snmpx_t *s, unsigned char* tlv_buf, const struct userinfo_t* user_info);
	int encrypt_msgData(struct snmpx_t* s, unsigned char* tlv_buf, const struct userinfo_t* user_info);
	int check_sd_snmpd_data(struct snmpx_t* s, const struct userinfo_t* user_info);
	int encrypt_pdu(const unsigned char* in, unsigned int in_len, const userinfo_t* user_info, const snmpx_t *s, 
		unsigned char *out, unsigned int *out_len);

private:
	CBerCode    m_BerCode;    //BER-code对象

};

#endif
