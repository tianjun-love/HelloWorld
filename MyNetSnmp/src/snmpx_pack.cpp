#include "../include/snmpx_pack.hpp"
#include "../include/cryptography_proccess.hpp"
#include <stdlib.h>

CSnmpxPack::CSnmpxPack()
{
}

CSnmpxPack::~CSnmpxPack()
{
}

/*
 * TLV 中转换L长度为unsigned buf类型
 */
int CSnmpxPack::tag_len_pack(unsigned int taglen, unsigned char* buf)
{
	if (taglen > MAX_MSG_LEN)
	{
		m_szErrorMsg = format_err_msg("tag_len_pack wrong, TLV length can not more than %u.",
			MAX_MSG_LEN);
		return SNMPX_failure;
	}

	int rval = SNMPX_noError;

	if( taglen <= 127 ) //短格式
	{
		*buf = (unsigned char)taglen;
		rval = 1;
	}
	else
	{
		//长格式，最多也只有两个字节表示长度
		if (taglen <= 255)
		{
			*buf = 0x81;
			*(buf + 1) = (unsigned char)taglen;
			rval = 2;
		}
		else
		{
			*buf = 0x82;
			
			unsigned char t_buf[sizeof(unsigned int)] = { 0 };
			memcpy(t_buf, (unsigned char*)&taglen, sizeof(unsigned int));

			if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
			{
				*(buf + 1) = t_buf[1];
				*(buf + 2) = t_buf[0];
			}
			else
			{
				*(buf + 1) = t_buf[0];
				*(buf + 2) = t_buf[1];
			}

			rval = 3;
		}
	}

	return rval;
}	

/*
 * 输入tag ,tag_len , tag_buf
 * 转换输入tlv_buf
 * 返回转换后的长度
 */
int CSnmpxPack::tlv_pack(unsigned char tag, unsigned int tag_len, unsigned char* tag_buf, unsigned char* tlv_buf)
{
	int cnt = 0;
	int rval = SNMPX_noError;
	int real_len = 0;
	unsigned char t_buf[9600] = { 0 };

	*tlv_buf = tag; //TAG
	cnt = 1;

	switch (tag)
	{
	case ASN_INTEGER:
	case ASN_INTEGER64:
	case ASN_FLOAT:
	case ASN_DOUBLE:
		real_len = m_BerCode.asn_integer_code(tag, tag_len, tag_buf, t_buf);
		break;
	case ASN_TIMETICKS:
	case ASN_COUNTER:
	case ASN_GAUGE:
	case ASN_COUNTER64:
	case ASN_UNSIGNED64:
		real_len = m_BerCode.asn_unsigned_code(tag, tag_len, tag_buf, t_buf);
		break;
	case ASN_OBJECT_ID:
		real_len = m_BerCode.asn_oid_code(tag, tag_len, (oid*)tag_buf, t_buf);
		break;
	case ASN_IPADDRESS:
		real_len = m_BerCode.asn_ipaddress_code(tag_len, tag_buf, t_buf);
		break;
	default:
		real_len = tag_len;
		memcpy(t_buf, tag_buf, tag_len);
		break;
	}

	if (real_len < 0)
	{
		m_szErrorMsg = "tlv-pack value wrong: " + m_BerCode.GetErrorMsg();
		return SNMPX_failure;
	}

	unsigned char cvlen[8] = { 0 };  //LEN
	if ((rval = tag_len_pack(real_len, cvlen)) < 0)
	{
		m_szErrorMsg = "tlv-pack tlv len wrong: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	memcpy(tlv_buf + cnt, cvlen, rval);
	cnt = cnt + rval;

	if (real_len != 0)
	{
		memcpy(tlv_buf + 1 + rval, t_buf, real_len); //VALUE
		cnt = cnt + real_len;
	}

	return cnt;
}

/*
 * 组包v3 msgGlobalData段
 */
int CSnmpxPack::msgGlobalData_pack(struct snmpx_t* s, unsigned char* tlv_buf)
{
	if (s == NULL)
	{
		m_szErrorMsg = "msgGlobalData_pack wrong, input snmpx_t is NULL.";
		return SNMPX_failure;
	}

	unsigned char pkg_buf[256] = { 0 };
	unsigned int pkg_cnt = sizeof(pkg_buf);
	unsigned char t_buf[256] = { 0 };
	int rval = SNMPX_noError;

	//逆向去组包
	//msgSecurityModel
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->msgSecurityModel), (unsigned char*)&(s->msgSecurityModel), t_buf)) < 0)
	{
		m_szErrorMsg = "msgGlobalData_pack msgSecurityModel failed:" + m_szErrorMsg;
		return SNMPX_failure;
	}
	else
	{
		pkg_cnt = pkg_cnt - rval;
		memcpy(pkg_buf + pkg_cnt, t_buf, rval);
	}

	//msgFlags
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_OCTET_STR, sizeof(s->msgFlags), (unsigned char*)&s->msgFlags, t_buf)) < 0)
	{
		m_szErrorMsg = "msgGlobalData_pack msgFlags failed:" + m_szErrorMsg;
		return SNMPX_failure;
	}
	else
	{
		pkg_cnt = pkg_cnt - rval;
		memcpy(pkg_buf + pkg_cnt, t_buf, rval);
	}

	//msgMaxSize
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->msgMaxSize), (unsigned char*)&s->msgMaxSize, t_buf)) < 0)
	{
		m_szErrorMsg = "msgGlobalData_pack msgMaxSize failed:" + m_szErrorMsg;
		return SNMPX_failure;
	}
	else
	{
		pkg_cnt = pkg_cnt - rval;
		memcpy(pkg_buf + pkg_cnt, t_buf, rval);
	}

	//msgID
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->msgID), (unsigned char*)&s->msgID, t_buf)) < 0)
	{
		m_szErrorMsg = "msgGlobalData_pack msgID failed:" + m_szErrorMsg;
		return SNMPX_failure;
	}
	else
	{
		pkg_cnt = pkg_cnt - rval;
		memcpy(pkg_buf + pkg_cnt, t_buf, rval);
	}

	//组一次30
	//msgGlobalData_pack
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_SEQ, sizeof(pkg_buf) - pkg_cnt, (unsigned char*)pkg_buf + pkg_cnt, t_buf)) < 0)
	{
		m_szErrorMsg = "msgGlobalData_pack tag failed:" + m_szErrorMsg;
		return SNMPX_failure;
	}
	else
		memcpy(tlv_buf, t_buf, rval);

	return rval;
}

/*
 * 组oid binding段
 */
int CSnmpxPack::pack_variable_bindings(struct variable_bindings *tvb, unsigned char* tlv_buf)
{
	if (tvb == NULL) {
		return SNMPX_noError;
	}
	unsigned char pkg_buf[MAX_MSG_LEN] = { 0 };
	unsigned int pkg_cnt = sizeof(pkg_buf);
	unsigned char t_buf[MAX_MSG_LEN] = { 0 };
	int rval = SNMPX_noError;

	//逆向组包
	//OID对应的值域
	if ((rval = tlv_pack(tvb->val_tag, tvb->val_len, tvb->val_buf, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_variable_bindings value failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}
	else
	{
		pkg_cnt = pkg_cnt - rval;
		memcpy(pkg_buf + pkg_cnt, t_buf, rval);
	}

	//OID
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_OBJECT_ID, tvb->oid_buf_len, (unsigned char*)tvb->oid_buf, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_variable_bindings oid failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}
	else
	{
		pkg_cnt = pkg_cnt - rval;
		memcpy(pkg_buf + pkg_cnt, t_buf, rval);
	}

	//补充30
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_SEQ, sizeof(pkg_buf) - pkg_cnt, pkg_buf + pkg_cnt, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_variable_bindings tag failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	memcpy(tlv_buf, t_buf, rval);

	return rval;
}

/*
 * 对items进行组包
 */
int CSnmpxPack::pack_item(const std::list<variable_bindings*> *variable_bindings_list, unsigned char* tlv_buf)
{
	if (variable_bindings_list->empty()) { //如果队列为空,需填充0x30 , 0x00 
		*tlv_buf = 0x30;
		*(tlv_buf + 1) = 0x00;
		return 2;
	}

	unsigned char pkg_buf[MAX_MSG_LEN] = { 0 };
	unsigned int pkg_cnt = sizeof(pkg_buf);
	unsigned char t_buf[MAX_MSG_LEN] = { 0 };
	int rval = SNMPX_noError;
	std::list<variable_bindings*>::const_reverse_iterator iter;

	//逆向组包
	for (iter = variable_bindings_list->crbegin(); iter != variable_bindings_list->crend(); ++iter)
	{
		struct variable_bindings* tvb = *iter;
		if (tvb == NULL) {
			continue;
		}
		if (tvb->oid_buf == NULL) {
			continue;
		}
		if ((rval = pack_variable_bindings(tvb, t_buf)) < 0)
		{
			m_szErrorMsg = "pack_item variable failed: " + m_szErrorMsg;
			return SNMPX_failure;
		}

		pkg_cnt = pkg_cnt - rval;
		memcpy(pkg_buf + pkg_cnt, t_buf, rval);
		memset(t_buf, 0x00, sizeof(t_buf));
	}

	//填充30
	if ((rval = tlv_pack(ASN_SEQ, sizeof(pkg_buf) - pkg_cnt, pkg_buf + pkg_cnt, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_item tag failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	memcpy(tlv_buf, t_buf, rval);

	return rval;
}

/*
 * 对data段进行组包v1 get  v2 , v3共有
 */
int CSnmpxPack::pack_data(struct snmpx_t *s, unsigned char* tlv_buf)
{
	unsigned char pkg_buf[MAX_MSG_LEN] = { 0 };
	unsigned int pkg_cnt = sizeof(pkg_buf);
	unsigned char t_buf[MAX_MSG_LEN] = { 0 };
	int rval = SNMPX_noError;

	//逆向组包
	if ((rval = pack_item(s->variable_bindings_list, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_data item failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//error_index
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->error_index), (unsigned char*)&s->error_index, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_data error_index failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//error_status
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->error_status), (unsigned char*)&s->error_status, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_data error_status failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//request_id
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->request_id), (unsigned char*)&s->request_id, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_data request_id failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//TAG 操作段重新计算长度
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(s->tag, sizeof(pkg_buf) - pkg_cnt, pkg_buf + pkg_cnt, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_data tag failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	memcpy(tlv_buf, t_buf, rval);

	return rval;
}

/*
 * 组v1 TRAP data段
 */
int CSnmpxPack::pack_v1_trap_data(struct snmpx_t* s, unsigned char* tlv_buf)
{
	unsigned char pkg_buf[MAX_MSG_LEN] = { 0 };
	unsigned int pkg_cnt = sizeof(pkg_buf);
	unsigned char t_buf[MAX_MSG_LEN] = { 0 };
	int rval = SNMPX_noError;

	//逆向组包
	if ((rval = pack_item(s->variable_bindings_list, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_v1_trap_data item failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//time_stamp
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_TIMETICKS, sizeof(s->time_stamp), (unsigned char*)&s->time_stamp, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_v1_trap_data time_stamp failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//specific_trap
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->specific_trap), (unsigned char*)&s->specific_trap, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_v1_trap_data specific_trap failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//generic_trap
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->generic_trap), (unsigned char*)&s->generic_trap, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_v1_trap_data generic_trap failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//agent_addr
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_IPADDRESS, sizeof(s->agent_addr), (unsigned char*)&s->agent_addr, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_v1_trap_data agent_addr failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//enterprise_oid
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_OBJECT_ID, s->enterprise_oid_len, (unsigned char*)s->enterprise_oid, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_v1_trap_data enterprise_oid failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//TAG 操作段重新计算长度
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(s->tag, sizeof(pkg_buf) - pkg_cnt, pkg_buf + pkg_cnt, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_v1_trap_data tag failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	memcpy(tlv_buf, t_buf, rval);

	return rval;
}

/*
 * 对msgData进行组包 v3 独有
 */
int CSnmpxPack::pack_msgData(struct snmpx_t *s, unsigned char* tlv_buf)
{
	unsigned char pkg_buf[MAX_MSG_LEN] = { 0 };
	unsigned int pkg_cnt = sizeof(pkg_buf);
	unsigned char t_buf[MAX_MSG_LEN] = { 0 };
	int rval = SNMPX_noError;

	if ((rval = pack_data(s, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_msgData data failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//contextName
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_OCTET_STR, s->contextName_len, s->contextName, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_msgData contextName failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//contextEngineID
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_OCTET_STR, s->contextEngineID_len, s->contextEngineID, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_msgData contextEngineID failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//补充30到msgData段
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_SEQ, sizeof(pkg_buf) - pkg_cnt, pkg_buf + pkg_cnt, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_msgData tag failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	memcpy(tlv_buf, t_buf, rval);

	return rval;
}

/*
 * msgAuthoritativeEngineID,
 * msgAuthoritativeEngineBoots,
 * msgAuthoritativeEngineTime,
 * msgUserName,
 * msgAuthenticationParameters,
 * msgPrivacyParameters , 结构进行组包
 * 组包认证段
 */
int CSnmpxPack::pack_authData(struct snmpx_t *s, unsigned char* tlv_buf)
{
	unsigned char pkg_buf[MAX_MSG_LEN] = { 0 };
	unsigned int pkg_cnt = sizeof(pkg_buf);
	unsigned char t_buf[MAX_MSG_LEN] = { 0 };
	int rval = SNMPX_noError;

	//msgPrivacyParameters
	if ((rval = tlv_pack(ASN_OCTET_STR, s->msgPrivacyParameters_len, s->msgPrivacyParameters, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_authData failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	/* 判断是否需要验证，需要验证 ,先赋值12个0x00,组后包后,再重新赋值 */
	if ((s->msgFlags & 0x01) == 0x01)
	{
		s->msgAuthenticationParameters_len = 12;
		if (s->msgAuthenticationParameters != NULL) {
			free(s->msgAuthenticationParameters);
		}

		s->msgAuthenticationParameters = (unsigned char*)malloc(s->msgAuthenticationParameters_len * sizeof(unsigned char));
		memset(s->msgAuthenticationParameters, 0x00, sizeof(s->msgAuthenticationParameters_len));
	}

	//msgAuthenticationParameters
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_OCTET_STR, s->msgAuthenticationParameters_len, s->msgAuthenticationParameters, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_authData msgAuthenticationParameters failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//msgUserName
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_OCTET_STR, s->msgUserName_len, s->msgUserName, t_buf)) < 0)
	{ 
		m_szErrorMsg = "pack_authData msgUserName failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//msgAuthoritativeEngineTime
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->msgAuthoritativeEngineTime), (unsigned char*)&s->msgAuthoritativeEngineTime, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_authData msgAuthoritativeEngineTime failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//msgAuthoritativeEngineBoots
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->msgAuthoritativeEngineBoots), (unsigned char*)&s->msgAuthoritativeEngineBoots, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_authData msgAuthoritativeEngineBoots failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//msgAuthoritativeEngineID
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_OCTET_STR, s->msgAuthoritativeEngineID_len,
		s->msgAuthoritativeEngineID, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_authData msgAuthoritativeEngineID failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	pkg_cnt = pkg_cnt - rval;
	memcpy(pkg_buf + pkg_cnt, t_buf, rval);

	//组一个30
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_SEQ, sizeof(pkg_buf) - pkg_cnt, pkg_buf + pkg_cnt, t_buf)) < 0)
	{
		m_szErrorMsg = "pack_authData tag failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	//全部数据再组一个ASN_OCTET_STR
	memset(pkg_buf, 0x00, sizeof(pkg_buf));
	if ((pkg_cnt = tlv_pack(ASN_OCTET_STR, rval, t_buf, pkg_buf)) < 0)
	{
		m_szErrorMsg = "pack_authData OCTET_STR failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	memcpy(tlv_buf, pkg_buf, pkg_cnt);

	return pkg_cnt;
}

/*
 * 加密msgData段
 */
int CSnmpxPack::encrypt_msgData(struct snmpx_t* s, unsigned char* tlv_buf, const struct userinfo_t* user_info)
{
	unsigned char t_buf[MAX_MSG_LEN] = { 0 };
	int rval = SNMPX_noError;
	unsigned char* encrypt_buf = NULL;

	//msgData段
	if ((rval = pack_msgData(s, t_buf)) < 0)
	{
		m_szErrorMsg = "encrypt_msgData failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	if (s->msgPrivacyParameters == NULL || s->msgPrivacyParameters_len == 0)
	{
		m_szErrorMsg = "encrypt_msgData failed: msgPrivacyParameters is NULL.";
		return SNMPX_failure;
	}

	if (user_info == NULL)
	{
		m_szErrorMsg = "encrypt_msgData failed: user_info is NULL.";
		s->errcode = 2;
		return SNMPX_failure;
	}

	if (user_info->msgAuthoritativeEngineID == NULL || user_info->msgAuthoritativeEngineID_len == 0)
	{
		m_szErrorMsg = "encrypt_msgData failed: user_info->msgAuthoritativeEngineID is NULL.";
		return SNMPX_failure;
	}

	if (user_info->privPasswdPrivKey == NULL || user_info->privPasswdPrivKey_len == 0)
	{
		m_szErrorMsg = "encrypt_msgData failed: user_info->privPasswdPrivKey is NULL.";
		return SNMPX_failure;
	}

	encrypt_buf = (unsigned char*)malloc(rval * sizeof(unsigned char) + 8); //DES最多填充7
	unsigned int encrypt_buf_len = 0;

	//加密
	if (encrypt_pdu(t_buf, rval, user_info, s, encrypt_buf, &encrypt_buf_len) < 0)
	{
		if (encrypt_buf != NULL)
			free(encrypt_buf);
		m_szErrorMsg = "encrypt_msgData encrypted data failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	//加密段
	memset(t_buf, 0x00, sizeof(t_buf));
	if ((rval = tlv_pack(ASN_OCTET_STR, encrypt_buf_len, encrypt_buf, t_buf)) < 0)
	{
		if (encrypt_buf != NULL)
			free(encrypt_buf);
		m_szErrorMsg = "encrypt_msgData encrypted failed: " + m_szErrorMsg;
		return SNMPX_failure;
	}

	memcpy(tlv_buf, t_buf, rval);

	if (encrypt_buf != NULL)
		free(encrypt_buf);

	return rval;
}

/*
 * 验证数据
 */
int CSnmpxPack::check_sd_snmpd_data(struct snmpx_t* s, const struct userinfo_t* user_info)
{
	if (s->msgVersion == 0x03)
	{
		if (s->tag == SNMPX_MSG_GET && s->msgAuthoritativeEngineID == NULL) {

		}
		else if (s->tag == SNMPX_MSG_REPORT) { //回复引擎ID的时候,不需要检查用户名

		}
		else //需认证用户名跟请求消息是否一致
		{
			if (s->msgUserName == NULL || s->msgUserName_len == 0) {
				m_szErrorMsg = "check_sd_snmpd_data failed: s->msgUserName is NULL.";
				return SNMPX_failure;
			}

			if (user_info == NULL) {
				s->errcode = 2;
				m_szErrorMsg = "check_sd_snmpd_data failed: user_info is NULL.";
				return SNMPX_failure;
			}

			if (memcmp(user_info->userName, s->msgUserName, strlen(user_info->userName)) != 0) {
				s->errcode = 2;
				m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed, user_info->userName[%s] s->msgUserName[%s] not math.",
					user_info->userName, s->msgUserName);
				return SNMPX_failure;
			}

			if (user_info->version != 3) {
				m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed, user_info->version[0x%02X] is not v3.",
					user_info->version);
				return SNMPX_failure;
			}

			if (user_info->safeMode == 0) {
				if ((s->msgFlags & 0x03) != 0x00) {
					m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed, msgFlags set wrong, user_info->safeMode[0x%02X] s->msgFlags[0x%02X].",
						user_info->safeMode, s->msgFlags);
					return SNMPX_failure;
				}
			}
			else if (user_info->safeMode == 1) {
				if ((s->msgFlags & 0x03) != 0x01) {
					m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed, msgFlags set wrong, user_info->safeMode[0x%02X] s->msgFlags[0x%02X].",
						user_info->safeMode, s->msgFlags);
					return SNMPX_failure;
				}
			}
			else {
				if ((s->msgFlags & 0x03) != 0x03) {
					m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed, msgFlags set wrong, user_info->safeMode[0x%02X] s->msgFlags[0x%02X].",
						user_info->safeMode, s->msgFlags);
					return SNMPX_failure;
				}
			}

			//判断引擎ID是否一致
			if (s->msgAuthoritativeEngineID == NULL || s->msgAuthoritativeEngineID_len == 0) {
				m_szErrorMsg = "check_sd_snmpd_data failed: s->msgAuthoritativeEngineID is NULL.";
				return SNMPX_failure;
			}

			if (user_info->msgAuthoritativeEngineID == NULL || user_info->msgAuthoritativeEngineID_len == 0) {
				m_szErrorMsg = "check_sd_snmpd_data failed: user_info->msgAuthoritativeEngineID is NULL.";
				return SNMPX_failure;
			}

			if (user_info->msgAuthoritativeEngineID_len != s->msgAuthoritativeEngineID_len) {
				m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed: msgAuthoritativeEngineID_len is not match, user[%u] s[%u].",
					user_info->msgAuthoritativeEngineID_len, s->msgAuthoritativeEngineID_len);
				return SNMPX_failure;
			}

			if (memcmp(user_info->msgAuthoritativeEngineID, s->msgAuthoritativeEngineID, user_info->msgAuthoritativeEngineID_len) != 0) {
				m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed: msgAuthoritativeEngineID is not match, user[0x%s] s[0x%s].",
					user_info->msgAuthoritativeEngineID_len, s->msgAuthoritativeEngineID_len);
				get_hex_string(user_info->msgAuthoritativeEngineID, user_info->msgAuthoritativeEngineID_len).c_str();
				get_hex_string(s->msgAuthoritativeEngineID, s->msgAuthoritativeEngineID_len).c_str();
				return SNMPX_failure;
			}
		}

		//整体认证
		if (s->msgID == 0) {
			m_szErrorMsg = "check_sd_snmpd_data failed: s->msgID is 0.";
			return SNMPX_failure;
		}

		if (s->msgMaxSize == 0) {
			m_szErrorMsg = "check_sd_snmpd_data failed: s->msgMaxSize is 0.";
			return SNMPX_failure;
		}

		if (s->request_id == 0) {
			m_szErrorMsg = "check_sd_snmpd_data failed: s->request_id is 0.";
			return SNMPX_failure;
		}

		if (s->msgSecurityModel != 0x03) { //usm
			m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed: s->msgSecurityModel[0x%02X] is not 0x03.",
				s->msgSecurityModel);
			return SNMPX_failure;
		}

		if (s->tag == 0) {
			m_szErrorMsg = "check_sd_snmpd_data failed: s->tag is 0.";
			return SNMPX_failure;
		}
	}
	else if (s->msgVersion == 0x01 || s->msgVersion == 0x00) //v1 v2c
	{
		if (s->community == NULL || s->community_len == 0) {
			m_szErrorMsg = "check_sd_snmpd_data failed: s->community is NULL.";
			return SNMPX_failure;
		}

		if (user_info == NULL) {
			m_szErrorMsg = "check_sd_snmpd_data failed: user_info is NULL.";
			return SNMPX_failure;
		}

		if (s->msgVersion != user_info->version) {
			m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed: version mot match, user[0x%02X] s[0x%02X].",
				user_info->version, s->msgVersion);
			return SNMPX_failure;
		}

		if (memcmp(user_info->userName, s->community, strlen(user_info->userName)) != 0) {
			m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed: community mot match, user[%s] s[%s].",
				user_info->userName, s->community);
			return SNMPX_failure;
		}

		if (s->msgVersion == 0x00 && s->tag == SNMPX_MSG_TRAP) { //v1 trap特有的字段

		}
		else {
			if (s->request_id == 0) {
				m_szErrorMsg = "check_sd_snmpd_data failed: s->request_id is 0.";
				return SNMPX_failure;
			}
		}

		if (s->tag == 0) {
			m_szErrorMsg = "check_sd_snmpd_data failed: s->tag is 0.";
			return SNMPX_failure;
		}
	}
	else
	{
		m_szErrorMsg = format_err_msg("check_sd_snmpd_data failed: not support version[0x%02X].", s->msgVersion);
		return SNMPX_failure;
	}

	return SNMPX_noError;
}

/*
 * 加密数据段
 */
int CSnmpxPack::encrypt_pdu(const unsigned char* in, unsigned int in_len, const userinfo_t* user_info, const snmpx_t *s,
	unsigned char *out, unsigned int *out_len)
{
	int rval = SNMPX_noError;
	CCryptographyProccess crypto;
	unsigned char iv[32] = { 0 }; /* 计算iv偏移量的值 */

	if (user_info->PrivMode == 0) //AES
	{
		unsigned int msgAuthoritativeEngineBootsHl = convert_to_nl(s->msgAuthoritativeEngineBoots);
		unsigned int msgAuthoritativeEngineTimeHl = convert_to_nl(s->msgAuthoritativeEngineTime);
		memcpy(iv, &msgAuthoritativeEngineBootsHl, 4);
		memcpy(iv + 4, &msgAuthoritativeEngineTimeHl, 4);
		memcpy(iv + 8, s->msgPrivacyParameters, s->msgPrivacyParameters_len);

		rval = crypto.snmpx_aes_encode(in, in_len, user_info->privPasswdPrivKey, iv, out);

		if (rval < 0)
			m_szErrorMsg = "encrypt_pdu aes failed: " + crypto.GetErrorMsg();
		else
			*out_len = (unsigned int)rval;
	}
	else if (user_info->PrivMode == 1) //DES
	{
		const unsigned int ivLen = 8;

		for (int i = 0; i < (int)ivLen; i++) {
			iv[i] = s->msgPrivacyParameters[i] ^ user_info->privPasswdPrivKey[ivLen + i];
		}

		rval = crypto.snmpx_des_encode(in, in_len, user_info->privPasswdPrivKey, iv, ivLen, out);

		if (rval < 0)
			m_szErrorMsg = "encrypt_pdu des failed: " + crypto.GetErrorMsg();
		else
			*out_len = (unsigned int)rval;
	}
	else
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("encrypt_msgData failed: not support privMode[0x%02X].",
			user_info->PrivMode);
	}

	return rval;
}

/*
* 组包函数处理，agent使用
*/
int CSnmpxPack::snmpx_pack(unsigned char*buf, struct snmpx_t* rc, struct snmpx_t* sd, const struct userinfo_t* user_info)
{
	if (sd->msgVersion == 0) {
		sd->msgVersion = rc->msgVersion;
	}
	if (sd->msgVersion == 0x01 || sd->msgVersion == 0x00) { // v1 v2c
		if (rc->community == NULL) {
			m_szErrorMsg = "snmpx_pack failed, community is NULL.";
			return SNMPX_failure;
		}
		sd->community_len = rc->community_len;
		sd->community = (unsigned char*)malloc(rc->community_len * sizeof(unsigned char));
		memcpy(sd->community, rc->community, rc->community_len);
	}
	if (sd->msgVersion == 0x03) { //v3
		if (sd->msgUserName == NULL && rc->msgUserName != NULL) {
			sd->msgUserName_len = rc->msgUserName_len;
			sd->msgUserName = (unsigned char*)malloc(rc->msgUserName_len * sizeof(unsigned char));
			memcpy(sd->msgUserName, rc->msgUserName, rc->msgUserName_len);
		}
		if (sd->msgID == 0) {
			sd->msgID = rc->msgID;
		}
		if (sd->msgMaxSize == 0) {
			sd->msgMaxSize = rc->msgMaxSize;
		}
		if ((rc->errcode == 2) || (rc->errcode == 3)) {
			sd->msgFlags = 0x00;
		}
		else {
			sd->msgFlags = (rc->msgFlags & 0x03); //去掉Reportable标志位
		}
		if (sd->msgSecurityModel == 0) {
			sd->msgSecurityModel = rc->msgSecurityModel;
		}
		if (sd->msgAuthoritativeEngineID == NULL && rc->msgAuthoritativeEngineID != NULL) {
			sd->msgAuthoritativeEngineID_len = rc->msgAuthoritativeEngineID_len;
			sd->msgAuthoritativeEngineID = (unsigned char*)malloc(rc->msgAuthoritativeEngineID_len * sizeof(unsigned char));
			memcpy(sd->msgAuthoritativeEngineID, rc->msgAuthoritativeEngineID, rc->msgAuthoritativeEngineID_len);
		}

		if ((sd->msgFlags & 0x02) == 0x02) { //判断是否需要加密
			if (sd->msgUserName == NULL) {
				m_szErrorMsg = "snmpx_pack failed, msgUserName is NULL.";
				return SNMPX_failure;
			}
			//加密需生成随机数
			CCryptographyProccess crypto;
			sd->msgPrivacyParameters_len = 8;
			sd->msgPrivacyParameters = (unsigned char*)malloc(sd->msgPrivacyParameters_len * sizeof(unsigned char));
			crypto.gen_msgPrivacyParameters(sd->msgPrivacyParameters, sd->msgPrivacyParameters_len);
		}
		if (sd->msgAuthoritativeEngineBoots == 0) {
			sd->msgAuthoritativeEngineBoots = rc->msgAuthoritativeEngineBoots;
		}
		if (sd->msgAuthoritativeEngineTime == 0) {
			sd->msgAuthoritativeEngineTime = rc->msgAuthoritativeEngineTime;
		}
		if (sd->contextEngineID == NULL && sd->msgAuthoritativeEngineID != NULL) {
			sd->contextEngineID_len = sd->msgAuthoritativeEngineID_len;
			sd->contextEngineID = (unsigned char*)malloc(sd->msgAuthoritativeEngineID_len * sizeof(unsigned char));
			memcpy(sd->contextEngineID, sd->msgAuthoritativeEngineID, sd->msgAuthoritativeEngineID_len);
		}
	}
	if (sd->request_id == 0) {
		sd->request_id = rc->request_id;
	}

	//v3情况下 若请求引擎ID,不能直接回复SNMP_MSG_RESPONSE
	if (sd->msgVersion == 0x03 && rc->msgUserName == NULL) {
		sd->tag = SNMPX_MSG_REPORT;
	}
	else {
		sd->tag = SNMPX_MSG_RESPONSE;
	}

	return snmpx_group_pack(buf, sd, user_info);
}

/*
 * 组包函数处理
 */
int CSnmpxPack::snmpx_group_pack(unsigned char *buf, struct snmpx_t* s, const struct userinfo_t* user_info)
{
	unsigned char pkg_buf[MAX_MSG_LEN] = { 0 };
	int pkg_cnt = (int)sizeof(pkg_buf);
	unsigned char t_buf[MAX_MSG_LEN] = { 0 };
	int rval = SNMPX_noError;

	if (check_sd_snmpd_data(s, user_info) < 0) {
		if (s->errcode == 2) {
			//用户名错误，继续组包
		}
		else {
			m_szErrorMsg = "snmpx_group_pack failed: " + m_szErrorMsg;
			return SNMPX_failure;
		}
	}

	if (s->msgVersion == 0x03) //v3
	{
		//msgData段，判断是否需要加密
		if (((s->msgFlags & 0x02) == 0x02) && (s->errcode != 2) && (s->errcode != 3)) {
			if ((rval = encrypt_msgData(s, t_buf, user_info)) < 0) {
				m_szErrorMsg = "snmpx_group_pack failed: " + m_szErrorMsg;
				return SNMPX_failure;
			}
		}
		else {

			if ((rval = pack_msgData(s, t_buf)) < 0) {
				m_szErrorMsg = "snmpx_group_pack failed: " + m_szErrorMsg;
				return SNMPX_failure;
			}
		}

		pkg_cnt = pkg_cnt - rval;
		if (pkg_cnt < 0)
		{
			m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_pack failed: MAX_MSG_LEN is %u Bytes.",
				MAX_MSG_LEN);
			return SNMPX_failure;
		}
		else
			memcpy(pkg_buf + pkg_cnt, t_buf, rval);

		//组认证段包
		if ((rval = pack_authData(s, t_buf)) < 0) {
			m_szErrorMsg = "snmpx_group_pack authData failed: " + m_szErrorMsg;
			return SNMPX_failure;
		}

		pkg_cnt = pkg_cnt - rval;
		if (pkg_cnt < 0)
		{
			m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_pack failed: MAX_MSG_LEN is %u Bytes.",
				MAX_MSG_LEN);
			return SNMPX_failure;
		}
		else
			memcpy(pkg_buf + pkg_cnt, t_buf, rval);

		//组msgGlobalData段
		memset(t_buf, 0x00, sizeof(t_buf));
		if ((rval = msgGlobalData_pack(s, t_buf)) < 0) { 
			m_szErrorMsg = "snmpx_group_pack msgGlobalData failed: " + m_szErrorMsg;
			return SNMPX_failure;
		}

		pkg_cnt = pkg_cnt - rval;
		if (pkg_cnt < 0)
		{
			m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_pack failed: MAX_MSG_LEN is %u Bytes.",
				MAX_MSG_LEN);
			return SNMPX_failure;
		}
		else
			memcpy(pkg_buf + pkg_cnt, t_buf, rval);

		//msgVersion
		memset(t_buf, 0x00, sizeof(t_buf));
		if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->msgVersion), &s->msgVersion, t_buf)) < 0) {
			m_szErrorMsg = "snmpx_group_pack version failed: " + m_szErrorMsg;
			return SNMPX_failure;
		}

		pkg_cnt = pkg_cnt - rval;
		if (pkg_cnt < 0)
		{
			m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_pack failed: MAX_MSG_LEN is %u Bytes.",
				MAX_MSG_LEN);
			return SNMPX_failure;
		}
		else
			memcpy(pkg_buf + pkg_cnt, t_buf, rval);

		//组一次30
		memset(t_buf, 0x00, sizeof(t_buf));
		if ((rval = tlv_pack(ASN_SEQ, sizeof(pkg_buf) - pkg_cnt, pkg_buf + pkg_cnt, t_buf)) < 0) { //allmsg
			m_szErrorMsg = "snmpx_group_pack tag failed: " + m_szErrorMsg;
			return SNMPX_failure;
		}

		//验回填验证信息
		if (((s->msgFlags & 0x01) == 0x01) && (s->errcode != 2) && (s->errcode != 3))
		{
			if (user_info->authPasswordPrivKey == NULL || user_info->authPasswordPrivKey_len == 0) {
				m_szErrorMsg = "snmpx_group_pack failed: user_info->authPasswordPrivKey is NULL.";
				return SNMPX_failure;
			}

			if (user_info->msgAuthoritativeEngineID == NULL || user_info->msgAuthoritativeEngineID_len == 0) {
				m_szErrorMsg = "snmpx_group_pack failed: user_info->msgAuthoritativeEngineID is NULL.";
				return SNMPX_failure;
			}

			CCryptographyProccess crypto;
			unsigned int real_msgAuthenticationParameters_len = s->msgAuthenticationParameters_len;
			unsigned char* real_msgAuthenticationParameters = (unsigned char*)malloc(real_msgAuthenticationParameters_len * sizeof(unsigned char));
			memset(real_msgAuthenticationParameters, 0x00, sizeof(real_msgAuthenticationParameters_len));

			int lac = crypto.gen_pkg_HMAC(user_info->authPasswordPrivKey, user_info->authPasswordPrivKey_len,
				user_info->msgAuthoritativeEngineID, user_info->msgAuthoritativeEngineID_len,
				user_info->AuthMode, t_buf, rval,
				s->msgAuthenticationParameters, s->msgAuthenticationParameters_len,
				real_msgAuthenticationParameters, real_msgAuthenticationParameters_len);

			if (lac <= 0)
			{
				m_szErrorMsg = "snmpx_group_pack failed: " + crypto.GetErrorMsg();
				free(real_msgAuthenticationParameters);
				return SNMPX_failure;
			}

			memcpy(t_buf + lac, real_msgAuthenticationParameters, real_msgAuthenticationParameters_len);
			free(real_msgAuthenticationParameters);
		}

		//获取引擎ID时，不用判断长度
		if (0 != user_info->agentMaxMsg_len && rval > user_info->agentMaxMsg_len)
		{
			m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_pack failed: agent support max msg length is %d Bytes, it packed %d.",
				user_info->agentMaxMsg_len, rval);
			return SNMPX_failure;
		}
		else
			memcpy(buf, t_buf, rval);

		return rval;
	}
	else if (s->msgVersion == 0x01 || s->msgVersion == 0x00) //v1 v2c
	{
		if (s->msgVersion == 0x00 && s->tag == SNMPX_MSG_TRAP) { //v1 trap 特有的字段
			if ((rval = pack_v1_trap_data(s, t_buf)) < 0) {
				m_szErrorMsg = "snmpx_group_pack v1-trap failed: " + m_szErrorMsg;
				return SNMPX_failure;
			}
		}
		else {
			if ((rval = pack_data(s, t_buf)) < 0) { //组data段
				m_szErrorMsg = "snmpx_group_pack item failed: " + m_szErrorMsg;
				return SNMPX_failure;
			}
		}

		pkg_cnt = pkg_cnt - rval;
		if (pkg_cnt < 0)
		{
			m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_pack failed: MAX_MSG_LEN is %u Bytes.",
				MAX_MSG_LEN);
			return SNMPX_failure;
		}
		else
			memcpy(pkg_buf + pkg_cnt, t_buf, rval);

		//community
		memset(t_buf, 0x00, sizeof(t_buf));
		if ((rval = tlv_pack(ASN_OCTET_STR, s->community_len, s->community, t_buf)) < 0)
		{
			m_szErrorMsg = "snmpx_group_pack community failed: " + m_szErrorMsg;
			return SNMPX_failure;
		}

		pkg_cnt = pkg_cnt - rval;
		if (pkg_cnt < 0)
		{
			m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_pack failed: MAX_MSG_LEN is %u Bytes.",
				MAX_MSG_LEN);
			return SNMPX_failure;
		}
		else
			memcpy(pkg_buf + pkg_cnt, t_buf, rval);

		//version
		memset(t_buf, 0x00, sizeof(t_buf));
		if ((rval = tlv_pack(ASN_INTEGER, sizeof(s->msgVersion), (unsigned char*)&s->msgVersion, t_buf)) < 0)
		{
			m_szErrorMsg = "snmpx_group_pack version failed: " + m_szErrorMsg;
			return SNMPX_failure;
		}

		pkg_cnt = pkg_cnt - rval;
		if (pkg_cnt < 0)
		{
			m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_pack failed: MAX_MSG_LEN is %u Bytes.",
				MAX_MSG_LEN);
			return SNMPX_failure;
		}
		else
			memcpy(pkg_buf + pkg_cnt, t_buf, rval);

		//组一次30
		memset(t_buf, 0x00, sizeof(t_buf));
		if ((rval = tlv_pack(0x30, sizeof(pkg_buf) - pkg_cnt, pkg_buf + pkg_cnt, t_buf)) < 0)
		{
			m_szErrorMsg = "snmpx_group_pack tag failed: " + m_szErrorMsg;
			return SNMPX_failure;
		}

		if (rval > MAX_MSG_LEN)
		{
			m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_pack failed: max msg length is %d Bytes, it packed %d.",
				MAX_MSG_LEN, rval);
			return SNMPX_failure;
		}
		else
			memcpy(buf, t_buf, rval);

		return rval;
	}
	else
	{
		m_szErrorMsg = format_err_msg("snmpx_group_pack failed, not support version[0x%02X].", s->msgVersion);
		return SNMPX_failure;
	}

	return SNMPX_noError;
}

/*
 *  设置链表数据
 */
int CSnmpxPack::snmpx_set_vb_list(std::list<variable_bindings*> *variable_bindings_list, const oid* rs_oid, unsigned int rs_oid_len,
	unsigned char tag, const unsigned char* buf, unsigned int len)
{
	struct variable_bindings* tvb = (struct variable_bindings*)malloc(sizeof(struct variable_bindings));

	if (tvb != NULL)
	{
		memset(tvb, 0x00, sizeof(struct variable_bindings));

		tvb->val_tag = tag;
		tvb->oid_buf_len = rs_oid_len;
		tvb->oid_buf = (oid*)malloc(rs_oid_len);

		if (tvb->oid_buf == NULL)
		{
			m_szErrorMsg = "snmpx_set_vb_list failed: malloc oid_buf return NULL.";
			free(tvb);
			return SNMPX_failure;
		}

		memcpy(tvb->oid_buf, rs_oid, rs_oid_len);
		tvb->val_len = len;

		if (tvb->val_len >= 1)
		{
			tvb->val_buf = (unsigned char*)malloc(tvb->val_len * sizeof(unsigned char));

			if (tvb->val_buf == NULL)
			{
				m_szErrorMsg = "snmpx_set_vb_list failed: malloc val_buf return NULL.";
				free(tvb->oid_buf);
				free(tvb);
				return SNMPX_failure;
			}

			memcpy(tvb->val_buf, buf, tvb->val_len);
		}

		variable_bindings_list->push_back(tvb);
	}
	else
	{
		m_szErrorMsg = "snmpx_set_vb_list failed: malloc variable_bindings return NULL.";
		return SNMPX_failure;
	}
	
	return SNMPX_noError;
}