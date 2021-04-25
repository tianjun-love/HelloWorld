#include "../include/snmpx_unpack.hpp"
#include "../include/cryptography_proccess.hpp"
#include "../include/snmpx_user_proccess.hpp"
#include <malloc.h>
#include <math.h>
#include <errno.h>
#include <mutex>

CSnmpxUnpack::CSnmpxUnpack()
{
}

CSnmpxUnpack::~CSnmpxUnpack()
{
}

/*
 * 解析TLV编码串
 */
int CSnmpxUnpack::tlv_unpack(unsigned char* tlv, unsigned int tlv_len, std::list<struct tlv_data*> &mglist)
{
	if (tlv == NULL || tlv_len < 2)
	{
		m_szErrorMsg = "tlv_unpack data wrong, value is NULL or length less than 2 !";
		return SNMPX_failure;
	}

	/* SNMP BER合法的TAG只有一个字节 */
	for (unsigned int cnt = 0; cnt < tlv_len; cnt++)
	{
		struct tlv_data *m_tlv_data = (struct tlv_data *)malloc(sizeof(struct tlv_data));

		if (NULL == m_tlv_data)
		{
			m_szErrorMsg = "tlv_unpack wrong, malloc tlv_data return NULL !";
			return SNMPX_failure;
		}

		m_tlv_data->value = NULL;
		m_tlv_data->value_len = 0;
		m_tlv_data->tag = *tlv++;

		/* 获取len 的长度 判断是短格式还是长格式 */
		if ((*tlv & 0x80) != 0x80) //短格式
		{
			m_tlv_data->value_len = *tlv++;
			cnt++;

			if (m_tlv_data->value_len > tlv_len - cnt - 1) //长度不符合要求,停止解码
			{
				free_tlv_data(m_tlv_data);
				m_szErrorMsg = format_err_msg("tlv_unpack data length wrong, value len[%u] remained len[%u].",
					m_tlv_data->value_len, tlv_len - cnt - 1);
				return SNMPX_failure;
			}

			/* 直接取value的值 */
			m_tlv_data->value = (unsigned char*)malloc(m_tlv_data->value_len * sizeof(unsigned char));

			memcpy(m_tlv_data->value, tlv, m_tlv_data->value_len);
			tlv += m_tlv_data->value_len;
			cnt += m_tlv_data->value_len;
		}
		else //长格式 ,SNMP 长度最多支持65535 ,若超过65535则认为组包不合规则  */
		{
			unsigned int num_len = (*tlv & 0x7F);

			if (num_len > 2) //不允许超过65535，即用两个字节就可表示长度
			{
				free_tlv_data(m_tlv_data);
				m_szErrorMsg = format_err_msg("tlv_unpack data length wrong, value len need one or two Bytes, it provide[%u].",
					num_len);
				return SNMPX_failure;
			}

			if (num_len > tlv_len - cnt - 1) //长度不符合要求,停止解码
			{
				free_tlv_data(m_tlv_data);
				m_szErrorMsg = format_err_msg("tlv_unpack data length wrong, value len[%u] remained len[%u].",
					num_len, tlv_len - cnt - 1);
				return SNMPX_failure;
			}

			cnt++;
			tlv++; //偏移到真正的长度

			for (unsigned int i = 0; i < num_len; i++)
			{
				cnt++;
				unsigned char v = *(tlv++);
				m_tlv_data->value_len = m_tlv_data->value_len + v * (unsigned int)pow(2, 8 * (num_len - i - 1));
			}

			if (m_tlv_data->value_len > tlv_len - cnt - 1) //长度不符合要求,停止解码
			{
				free_tlv_data(m_tlv_data);
				m_szErrorMsg = format_err_msg("tlv_unpack data length wrong, value len[%u] remained len[%u].",
					m_tlv_data->value_len, tlv_len - cnt - 1);
				return SNMPX_failure;
			}

			/* 获取value的长度 */
			m_tlv_data->value = (unsigned char*)malloc(m_tlv_data->value_len * sizeof(unsigned char));

			memcpy(m_tlv_data->value, tlv, m_tlv_data->value_len);
			tlv += m_tlv_data->value_len;
			cnt += m_tlv_data->value_len;
		}

		mglist.push_back(m_tlv_data);
	}

	return SNMPX_noError;
}

/*
 * 解释oid对应value的值v1, v2c, v3共用
 */
int CSnmpxUnpack::unpack_oid_value( unsigned char* buf, unsigned int buf_len, struct snmpx_t *s)
{
	int rval = SNMPX_noError;
	std::list<struct tlv_data*> mlist;
	std::list<struct tlv_data*>::iterator iter;
	struct tlv_data* tlv = NULL;

	//解一对oid和值
	if (tlv_unpack(buf, buf_len, mlist) != 0)
	{
		m_szErrorMsg = "unpack_oid_value failed: " + m_szErrorMsg;
		free_tlv_glist_data(mlist);
		return SNMPX_failure;
	}
	
	if (mlist.size() != 2)
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("unpack_oid_value length wrong, parse result length must be 2, it provide[%llu].",
			mlist.size());
	}
	else
	{
		//oid
		iter = mlist.begin();
		tlv = *iter;

		if (tlv->tag != ASN_OBJECT_ID)
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_oid_value tag wrong, tag must be OBJECT_ID, it provide[0x%02X].",
				tlv->tag);
		}
		else
		{
			oid t_buf[MAX_OID_LEN] = { 0 };
			struct variable_bindings* vb = (struct variable_bindings*)malloc(sizeof(struct variable_bindings));
			memset(vb, 0x00, sizeof(struct variable_bindings));

			rval = m_BerCode.asn_oid_decode(tlv->value, tlv->value_len, t_buf);
			if (rval < 0)
			{
				free(vb);
				m_szErrorMsg = "unpack_oid_value oid wrong: " + m_BerCode.GetErrorMsg();
			}
			else
			{
				vb->oid_buf_len = rval;
				vb->oid_buf = (oid*)malloc(vb->oid_buf_len);
				memcpy(vb->oid_buf, t_buf, rval);

				//oid -> value
				++iter;
				tlv = *iter;

				vb->val_tag = tlv->tag;

				if (tlv->value_len != 0)
				{
					vb->val_len = tlv->value_len;
					vb->val_buf = (unsigned char*)malloc(tlv->value_len * sizeof(unsigned char));
					memcpy(vb->val_buf, tlv->value, vb->val_len);
				}

				s->variable_bindings_list->push_back(vb);
				CErrorStatus::set_usm_error_status(vb->oid_buf, vb->oid_buf_len, s);
			}
		}
	}

	//释放内存
	free_tlv_glist_data(mlist);

	return rval;
}

/*
 * 翻译item段v1, v2c , v3 共用
 */
int CSnmpxUnpack::unpack_item(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s)
{
	int rval = SNMPX_noError;
	std::list<struct tlv_data*> mlist;
	std::list<struct tlv_data*>::iterator iter;
	struct tlv_data* tlv = NULL;

	//解item，包含多个或一个OID和值
	if (tlv_unpack(buf, buf_len, mlist) == 0)
	{
		for (iter = mlist.begin(); iter != mlist.end(); ++iter)
		{
			tlv = *iter;

			if (unpack_oid_value(tlv->value, tlv->value_len, s) < 0)
			{
				rval = SNMPX_failure;
				m_szErrorMsg = "unpack_item failed: " + m_szErrorMsg;
				break;
			}
		}
	}
	else
	{
		rval = SNMPX_failure;
		m_szErrorMsg = "unpack_item failed: " + m_szErrorMsg;
	}

	//释放内存
	free_tlv_glist_data(mlist);

	return rval;
}

/*
 * 操作段处理v1 get ,  v2c , v3共用
 * 请求ID | 错误状态 | 差错索引 | item段
 */
int CSnmpxUnpack::unpack_cmd(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s)
{
	std::list<struct tlv_data*> mlist;
	std::list<struct tlv_data*>::iterator iter;
	struct tlv_data* tlv = NULL;
	int rval = SNMPX_noError;

	//解data段，包含请求ID，错误状态，错误索引，item段
	if (tlv_unpack(buf, buf_len, mlist) != 0)
	{
		m_szErrorMsg = "unpack_cmd failed: " + m_szErrorMsg;
		free_tlv_glist_data(mlist);
		return SNMPX_failure;
	}

	if (mlist.size() != 4)
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("unpack_cmd size wrong, parse result length must be 4, it provide[%llu].",
			mlist.size());
	}
	else
	{
		//请求ID
		iter = mlist.begin();
		tlv = *iter;

		if (tlv->tag == ASN_INTEGER)
			s->request_id = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_cmd tag wrong, request-id tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		//错误状态
		++iter;
		tlv = *iter;

		if (tlv->tag == ASN_INTEGER)
			s->error_status = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_cmd tag wrong, error-status tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		//错误索引
		++iter;
		tlv = *iter;

		if (tlv->tag == ASN_INTEGER)
			s->error_index = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_cmd tag wrong, error-index tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		//item段
		++iter;
		tlv = *iter;

		if (tlv->tag == ASN_SEQ)
		{
			if (tlv->value_len > 0)
			{
				rval = unpack_item(tlv->value, tlv->value_len, s);
				if (rval < 0) {
					m_szErrorMsg = "unpack_cmd failed: " + m_szErrorMsg;
				}
			}
		}
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_cmd tag wrong, item tag must be [0x%02X], it provide[0x%02X].",
				ASN_SEQ, tlv->tag);
		}
	}

g_end:
	//释放内存
	free_tlv_glist_data(mlist);

	return rval;
}

/*
 * 操作段处理v1 trap处理
 * enterprise_oid | agent IP | trap类型(0-6) | 特定代码 | 时间戳 | item段
 */
int CSnmpxUnpack::unpack_trap_v1_cmd(unsigned char* buf , unsigned int buf_len , struct snmpx_t *s)
{
	std::list<struct tlv_data*> mlist;
	std::list<struct tlv_data*>::iterator iter;
	struct tlv_data* tlv = NULL;
	int rval = SNMPX_noError;

	//解data段
	if (tlv_unpack(buf, buf_len, mlist) != 0)
	{
		m_szErrorMsg = "unpack_trap_v1_cmd failed: " + m_szErrorMsg;
		free_tlv_glist_data(mlist);
		return SNMPX_failure;
	}
	
	if (mlist.size() != 6)
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("unpack_trap_v1_cmd size wrong, parse result length must be 6, it provide[%llu].",
			mlist.size());
	}
	else
	{
		iter = mlist.begin();
		tlv = *iter; //enterprise_oid

		if (tlv->tag == ASN_OBJECT_ID)
		{
			oid t_buf[MAX_OID_LEN] = { 0 };
			int t_len = m_BerCode.asn_oid_decode(tlv->value, tlv->value_len, t_buf);

			if (t_len <= 0)
			{
				rval = SNMPX_failure;
				m_szErrorMsg = "unpack_trap_v1_cmd wrong, get enterprise_oid error: " + m_BerCode.GetErrorMsg();
			}

			s->enterprise_oid_len = t_len;
			s->enterprise_oid = (oid*)malloc(s->enterprise_oid_len);
			memcpy(s->enterprise_oid, t_buf, t_len);
		}
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_trap_v1_cmd tag wrong, enterprise_oid tag must be OBJECT_ID, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //agent_addr

		if (tlv->tag == ASN_IPADDRESS)
			s->agent_addr = m_BerCode.asn_ipaddress_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_trap_v1_cmd tag wrong, agent_address tag must be IPAddress, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //generic_trap

		if (tlv->tag == ASN_INTEGER)
			s->generic_trap = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_trap_v1_cmd tag wrong, generic_trap tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //specific_trap

		if (tlv->tag == ASN_INTEGER)
			s->specific_trap = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_trap_v1_cmd tag wrong, specific_trap tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //time_stamp

		if (tlv->tag == ASN_TIMETICKS)
			s->time_stamp = m_BerCode.asn_unsigned_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_trap_v1_cmd tag wrong, time_stamp tag must be TIMETICKS, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //item

		if (tlv->tag == ASN_SEQ)
		{
			if (tlv->value_len > 0)
			{
				rval = unpack_item(tlv->value, tlv->value_len, s);
				if (rval < 0) {
					m_szErrorMsg = "unpack_trap_v1_cmd failed: " + m_szErrorMsg;
				}
			}
		}
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_trap_v1_cmd tag wrong, item tag must be [0x%02X], it provide[0x%02X].",
				ASN_SEQ, tlv->tag);
		}
	}

g_end:
	//清理内存
	free_tlv_glist_data(mlist);

	return rval;
}

/*
 * 翻译标志段
 * msgID | msgMaxSize | msgFlags | msgSecurityModel
 */
int CSnmpxUnpack::unpack_v3_msgGlobalData(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s)
{
	std::list<struct tlv_data*> mlist;
	std::list<struct tlv_data*>::iterator iter;
	struct tlv_data* tlv = NULL;
	int rval = SNMPX_noError;

	//解data段
	if (tlv_unpack(buf, buf_len, mlist) != 0)
	{
		m_szErrorMsg = "unpack_v3_msgGlobalData failed: " + m_szErrorMsg;
		free_tlv_glist_data(mlist);
		return SNMPX_failure;
	}

	if (mlist.size() < 4)
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("unpack_v3_msgGlobalData size wrong, oid-value pair at least need 4, it provide[%llu].",
			mlist.size());
	}
	else
	{
		iter = mlist.begin();
		tlv = *iter; //msgID

		if (tlv->tag == ASN_INTEGER)
			s->msgID = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_msgGlobalData tag wrong, msgID tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //msgMaxSize

		if (tlv->tag == ASN_INTEGER)
			s->msgMaxSize = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_msgGlobalData tag wrong, msgMaxSize tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //msgFlags

		if (tlv->tag == ASN_OCTET_STR)
			s->msgFlags = *tlv->value;
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_msgGlobalData tag wrong, msgFlags tag must be OCTET_STR, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //msgSecurityModel

		if (tlv->tag == ASN_INTEGER)
			s->msgSecurityModel = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_msgGlobalData tag wrong, msgSecurityModel tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
		}
	}

g_end:
	//清理内存
	free_tlv_glist_data(mlist);

	return rval;
}

/*
 * 处理数据段
 * ContextEngineID | ContextName | PDU
 */
int CSnmpxUnpack::unpack_v3_msgData(unsigned char*buf, unsigned buf_len, struct snmpx_t* s)
{
	std::list<struct tlv_data*> mlist;
	std::list<struct tlv_data*>::iterator iter;
	struct tlv_data* tlv = NULL;
	int rval = SNMPX_noError;

	//解数据层
	if (tlv_unpack(buf, buf_len, mlist) != 0)
	{
		m_szErrorMsg = "unpack_v3_msgData failed: " + m_szErrorMsg;
		free_tlv_glist_data(mlist);
		return SNMPX_failure;
	}

	if (mlist.size() < 3)
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("unpack_v3_msgData size wrong, oid-value pair at least need 3, it provide[%llu].",
			mlist.size());
	}
	else
	{
		iter = mlist.begin();
		tlv = *iter; //contextEngineID

		if (tlv->tag == ASN_OCTET_STR)
		{
			if (tlv->value_len > 0)
			{
				s->contextEngineID_len = tlv->value_len;
				s->contextEngineID = (unsigned char*)malloc(tlv->value_len * sizeof(unsigned char));
				memcpy(s->contextEngineID, tlv->value, tlv->value_len);
			}
		}
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_msgData tag wrong, ContextEngineID tag must be OCTET_STR, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //contextName

		if (tlv->tag == ASN_OCTET_STR)
		{
			if (tlv->value_len > 0)
			{
				s->contextName_len = tlv->value_len;
				s->contextName = (unsigned char*)malloc(tlv->value_len * sizeof(unsigned char));
				memcpy(s->contextName, tlv->value, tlv->value_len);
			}
		}
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_msgData tag wrong, ContextName tag must be OCTET_STR, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //command

		s->tag = tlv->tag;
		rval = unpack_cmd(tlv->value, tlv->value_len, s);
		if (rval < 0) {
			m_szErrorMsg = "unpack_v3_msgData failed: " + m_szErrorMsg;
		}
	}

g_end:
	//清理内存
	free_tlv_glist_data(mlist);

	return rval;
}

/*
 * 翻译认证段
 * authEngineID | authEngineBoots | authEngineTime | UserName | authParam | privParam
 */
int CSnmpxUnpack::unpack_v3_authData(unsigned char* buf, unsigned int buf_len, struct snmpx_t *s)
{
	std::list<struct tlv_data*> mlist;
	std::list<struct tlv_data*>::iterator iter;
	struct tlv_data* tlv = NULL;
	int rval = SNMPX_noError;

	//解开第一层0x30
	if (tlv_unpack(buf, buf_len, mlist) != 0)
	{
		m_szErrorMsg = "unpack_v3_authData failed: " + m_szErrorMsg;
		free_tlv_glist_data(mlist);
		return SNMPX_failure;
	}
	else
	{
		if (mlist.empty())
		{
			m_szErrorMsg = "unpack_v3_authData size wrong, oid-value pair at least need 1, it provide[0].";
			return SNMPX_failure;
		}
	}

	iter = mlist.begin();
	tlv = *iter; //解开数据层

	if (tlv_unpack(tlv->value, tlv->value_len, mlist) != 0)
	{
		m_szErrorMsg = "unpack_v3_authData data failed: " + m_szErrorMsg;
		free_tlv_glist_data(mlist);
		return SNMPX_failure;
	}
	
	if (mlist.size() < 7)
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("unpack_v3_authData size wrong, oid-value pair at least need 7, it provide[%llu].",
			mlist.size());
	}
	else
	{
		//开始数据解释
		++iter;
		tlv = *iter; //msgAuthoritativeEngineID

		if (tlv->tag == ASN_OCTET_STR)
		{
			s->msgAuthoritativeEngineID_len = tlv->value_len;

			if (s->msgAuthoritativeEngineID_len > 0)
			{
				s->msgAuthoritativeEngineID = (unsigned char*)malloc(tlv->value_len * sizeof(unsigned char));
				memcpy(s->msgAuthoritativeEngineID, tlv->value, tlv->value_len);
			}
		}
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_authData tag wrong, msgAuthoritativeEngineID tag must be OCTET_STR, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //msgAuthoritativeEngineBoots

		if (tlv->tag == ASN_INTEGER)
			s->msgAuthoritativeEngineBoots = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_authData tag wrong, msgAuthoritativeEngineBoots tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //msgAuthoritativeEngineTime

		if (tlv->tag == ASN_INTEGER)
			s->msgAuthoritativeEngineTime = m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_authData tag wrong, msgAuthoritativeEngineTime tag must be INTEGER, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //msgUserName

		if (tlv->tag == ASN_OCTET_STR)
		{
			if (tlv->value_len > 0)
			{
				s->msgUserName_len = tlv->value_len;
				s->msgUserName = (unsigned char*)malloc(tlv->value_len * sizeof(unsigned char) + 1); //该字段需字符串处理,需要添加空字符
				memcpy(s->msgUserName, tlv->value, tlv->value_len);
				s->msgUserName[s->msgUserName_len] = '\0';
			}
		}
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_authData tag wrong, msgUserName tag must be OCTEX_STR, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //msgAuthenticationParameters

		if (tlv->tag == ASN_OCTET_STR)
		{
			if (tlv->value_len > 0)
			{
				s->msgAuthenticationParameters_len = tlv->value_len;
				s->msgAuthenticationParameters = (unsigned char*)malloc(tlv->value_len * sizeof(unsigned char));
				memcpy(s->msgAuthenticationParameters, tlv->value, tlv->value_len);
			}
		}
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_authData tag wrong, msgAuthenticationParameters tag must be OCTEX_STR, it provide[0x%02X].",
				tlv->tag);
			goto g_end;
		}

		++iter;
		tlv = *iter; //msgPrivacyParameters

		if (tlv->tag == ASN_OCTET_STR)
		{
			if (tlv->value_len > 0)
			{
				s->msgPrivacyParameters_len = tlv->value_len;
				s->msgPrivacyParameters = (unsigned char*)malloc(tlv->value_len * sizeof(unsigned char));
				memcpy(s->msgPrivacyParameters, tlv->value, tlv->value_len);
			}
		}
		else
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("unpack_v3_authData tag wrong, msgPrivacyParameters tag must be OCTEX_STR, it provide[0x%02X].",
				tlv->tag);
		}
	}

g_end:
	//清理内存
	free_tlv_glist_data(mlist);

	return rval;
}

/*
 *重新检查数据合法性
 */
int CSnmpxUnpack::check_rc_snmpx_data(struct snmpx_t* r, const userinfo_t* t_user, unsigned char* buf, unsigned int buf_len,
	bool is_get_v3_engineID)
{
	if (r->msgVersion == 0x03)
	{
		if (r->tag == SNMPX_MSG_GET && (r->msgAuthoritativeEngineID == NULL || r->msgAuthoritativeEngineID_len == 0)) {
			//获取v3引擎ID请求不用校验
		}
		else if (r->tag == SNMPX_MSG_REPORT || (is_get_v3_engineID && r->tag == SNMPX_MSG_RESPONSE)) {
			//收到引擎ID的时候不需要检查，有的设备是以get-response返回
			if (is_get_v3_engineID && r->tag == SNMPX_MSG_RESPONSE)
			{
				if (r->msgAuthoritativeEngineID == NULL || r->msgAuthoritativeEngineID_len == 0)
				{
					m_szErrorMsg = "check_rc_snmpx_data failed, get agent AuthoritativeEngineID NULL.";
					return SNMPX_failure;
				}
			}
		}
		else //需认证请求消息是否一致
		{
			if (r->msgVersion != t_user->version)
			{
				m_szErrorMsg = format_err_msg("check_rc_snmpx_data failed, user version[0x%02X], packge version[0x%02X].",
					t_user->version, r->msgVersion);
				return SNMPX_failure;
			}

			if (t_user->safeMode == 0)
			{
				//noAuthNoPriv
				if ((r->msgFlags & 0x03) != 0x00) 
				{
					//当用户名错误，md5码错误时,组包时将msgFlags设置为0x00
					if (r->msgFlags != 0x00)
					{
						m_szErrorMsg = format_err_msg("check_rc_snmpx_data failed, user safeMode is noAuthNoPriv, packge msgFlags[0x%02X].",
							r->msgFlags);
						return SNMPX_failure;
					}
				}
			}
			else if (t_user->safeMode == 1)
			{
				//authNoPriv
				if ((r->msgFlags & 0x03) != 0x01)
				{
					//当用户名错误，md5码错误时,组包时将msgFlags设置为0x00
					if (r->msgFlags != 0x00)
					{
						m_szErrorMsg = format_err_msg("check_rc_snmpx_data failed, user safeMode is authNoPriv, packge msgFlags[0x%02X].",
							r->msgFlags);
						return SNMPX_failure;
					}

				}
			}
			else
			{
				//authPriv
				if ((r->msgFlags & 0x03) != 0x03)
				{
					//当用户名错误，md5码错误时,组包时将msgFlags设置为0x00
					if (r->msgFlags != 0x00)
					{
						m_szErrorMsg = format_err_msg("check_rc_snmpx_data failed, user safeMode is authPriv, packge msgFlags[0x%02X].",
							r->msgFlags);
						return SNMPX_failure;
					}
				}
			}

			//判断引擎ID是否一致
			if (r->msgAuthoritativeEngineID == NULL || r->msgAuthoritativeEngineID_len == 0) {
				m_szErrorMsg = "check_rc_snmpx_data failed, recv msgAuthoritativeEngineID is null.";
				return SNMPX_failure;
			}
			if (t_user->msgAuthoritativeEngineID == NULL || t_user->msgAuthoritativeEngineID_len == 0) {
				m_szErrorMsg = "check_rc_snmpx_data failed, user msgAuthoritativeEngineID is null.";
				return SNMPX_failure;
			}
			if (t_user->msgAuthoritativeEngineID_len != r->msgAuthoritativeEngineID_len) {
				m_szErrorMsg = format_err_msg("check_rc_snmpx_data AuthoritativeEngineID failed, user length[0x%02X], recv[0x%02X].",
					t_user->msgAuthoritativeEngineID_len, r->msgAuthoritativeEngineID_len);
				return SNMPX_failure;
			}
			if (memcmp(t_user->msgAuthoritativeEngineID, r->msgAuthoritativeEngineID, t_user->msgAuthoritativeEngineID_len) != 0) {
				m_szErrorMsg = format_err_msg("check_rc_snmpx_data AuthoritativeEngineID failed, user ID[0x%s], recv [0x%s].",
					get_hex_string(t_user->msgAuthoritativeEngineID, t_user->msgAuthoritativeEngineID_len).c_str(), 
					get_hex_string(r->msgAuthoritativeEngineID, r->msgAuthoritativeEngineID_len).c_str());
				return SNMPX_failure;
			}

			//判断用户是否需要验证，加密
			if ((r->msgFlags & 0x01) == 0x01)
			{
				//通过查找两次判断准确性,不可能出现在开头位置
				if (r->msgAuthenticationParameters == NULL || r->msgAuthenticationParameters_len == 0) {
					m_szErrorMsg = "check_rc_snmpx_data failed, msgAuthenticationParameters is null.";
					return SNMPX_failure;
				}

				int lac = find_str_lac(buf, buf_len, r->msgAuthoritativeEngineID, r->msgAuthoritativeEngineID_len);
				if (lac <= 0)
				{
					m_szErrorMsg = "check_rc_snmpx_data failed, can not found msgAuthoritativeEngineID.";
					return SNMPX_failure;
				}

				lac = lac + find_str_lac(buf + lac, buf_len - lac, r->msgUserName, r->msgUserName_len);
				if (lac <= 0)
				{
					m_szErrorMsg = "check_rc_snmpx_data failed, can not found msgUserName.";
					return SNMPX_failure;
				}

				lac = lac + find_str_lac(buf + lac, buf_len - lac, r->msgAuthenticationParameters, r->msgAuthenticationParameters_len);
				if (lac <= 0)
				{
					m_szErrorMsg = "check_rc_snmpx_data failed, can not found msgAuthenticationParameters.";
					return SNMPX_failure;
				}

				CCryptographyProccess crypto;
				unsigned char* count_md5_buf = (unsigned char*)malloc(buf_len);
				unsigned int real_msgAuthenticationParameters_len = r->msgAuthenticationParameters_len;
				unsigned char* real_msgAuthenticationParameters = (unsigned char*)malloc(real_msgAuthenticationParameters_len * sizeof(unsigned char));

				memcpy(count_md5_buf, buf, buf_len);
				memset(count_md5_buf + lac, 0x00, r->msgAuthenticationParameters_len); //置12个0x00

				//计算HMAC
				if (crypto.gen_msgHMAC(t_user->authPasswordPrivKey, t_user->authPasswordPrivKey_len, r->msgAuthoritativeEngineID, 
					r->msgAuthoritativeEngineID_len, t_user->AuthMode, count_md5_buf, buf_len, real_msgAuthenticationParameters,
					&real_msgAuthenticationParameters_len) < 0)
				{
					m_szErrorMsg = format_err_msg("check_rc_snmpx_data msgAuthenticationParameters failed, real_msgAuthenticationParameters[0x%s].",
						get_hex_string(real_msgAuthenticationParameters, real_msgAuthenticationParameters_len).c_str());

					if (count_md5_buf != NULL) {
						free(count_md5_buf);
					}

					if (real_msgAuthenticationParameters != NULL) {
						free(real_msgAuthenticationParameters);
					}

					return SNMPX_failure;
				}

				//比较
				if (memcmp(real_msgAuthenticationParameters, r->msgAuthenticationParameters,
					r->msgAuthenticationParameters_len) != 0) {
					r->errcode = 3;//hash报错码
				}
				if (count_md5_buf != NULL) {
					free(count_md5_buf);
				}

				if (real_msgAuthenticationParameters != NULL) {
					free(real_msgAuthenticationParameters);
				}
			}
		}

		//整体认证
		if (r->msgID == 0)
		{
			m_szErrorMsg = "check_rc_snmpx_data failed, msgID is 0.";
			return SNMPX_failure;
		}
		if (r->msgMaxSize == 0)
		{
			m_szErrorMsg = "check_rc_snmpx_data failed, msgMaxSize is 0.";
			return SNMPX_failure;
		}
		
		//当用户名错误时，会报SNMPX_MSG_REPORT
		if (r->tag != SNMPX_MSG_REPORT && r->tag != SNMPX_MSG_TRAP2 && r->request_id == 0)
		{
			m_szErrorMsg = "check_rc_snmpx_data failed, request ID is 0.";
			return SNMPX_failure;
		}

		//usm
		if (r->msgSecurityModel != 0x03)
		{
			m_szErrorMsg = format_err_msg("check_rc_snmpx_data failed, USM type is not 0x03, it provide[0x%02X].",
				r->msgSecurityModel);
			return SNMPX_failure;
		}

		if (r->tag == 0)
		{
			m_szErrorMsg = "check_rc_snmpx_data failed, PDU type is 0x00.";
			return SNMPX_failure;
		}
	}
	else if (r->msgVersion == 0x01 || r->msgVersion == 0x00) //v1,v2c
	{
		if (r->msgVersion == 0x00 && r->tag == SNMPX_MSG_TRAP) { //检查v1数据合法性

		}
		else
		{
			if (r->tag != SNMPX_MSG_TRAP2 && r->request_id == 0)
			{
				m_szErrorMsg = "check_rc_snmpx_data failed, request ID is 0.";
				return SNMPX_failure;
			}
		}

		if (r->msgVersion != t_user->version)
		{
			m_szErrorMsg = format_err_msg("check_rc_snmpx_data failed, user version[0x%02X], packge version[0x%02X].",
				t_user->version, r->msgVersion);
			return SNMPX_failure;
		}

		if (r->tag == 0)
		{
			m_szErrorMsg = "check_rc_snmpx_data failed, PDU type is 0x00.";
			return SNMPX_failure;
		}
	}
	else
	{
		m_szErrorMsg = format_err_msg("check_rc_snmpx_data failed, unsupport snmp version[0x%02X].",
			r->msgVersion);
		return SNMPX_failure;
	}

	return SNMPX_noError;
}

/*
 * 转成结构体
 */
int CSnmpxUnpack::snmpx_group_unpack(unsigned char* buf, unsigned int buf_len, struct snmpx_t *r, bool is_get_v3_engineID, 
	const void *user_info, bool is_user_info_multi, const void *user_auth_info_cache)
{
	std::list<struct tlv_data*> mlist;
	std::list<struct tlv_data*>::iterator iter;
	struct tlv_data* tlv = NULL;
	const struct userinfo_t* t_user = NULL;
	int rval = SNMPX_noError;

	unsigned char* decode = NULL;
	unsigned int decodeLen = 0;

	if (buf == NULL || buf_len < 2)
	{
		m_szErrorMsg = "snmpx_group_unpack input data wrong, at least need 2 Bytes !";
		return SNMPX_failure;
	}
	else if (buf_len > MAX_MSG_LEN)
	{
		m_szErrorMsg = CErrorStatus::format_err_msg("snmpx_group_unpack input data wrong, MAX_MSG_LEN[%u] Bytes, it provide[%u] !",
			MAX_MSG_LEN, buf_len);
		return SNMPX_failure;
	}

	//第一个字节必定是0x30
	if (buf[0] != ASN_SEQ)
	{
		m_szErrorMsg = format_err_msg("snmpx_group_unpack input data wrong, first Bytes must be[0x%02X], it provide[0x%02X].",
			ASN_SEQ, buf[0]);
		return SNMPX_failure;
	}

	if (tlv_unpack(buf, buf_len, mlist) != 0)
	{
		m_szErrorMsg = "snmpx_group_unpack failed: " + m_szErrorMsg;
		free_tlv_glist_data(mlist);
		return SNMPX_failure;
	}
	else
	{
		if (mlist.empty())
		{
			m_szErrorMsg = "snmpx_group_unpack size wrong, TLV data is empty !";
			return SNMPX_failure;
		}
	}

	iter = mlist.begin();
	tlv = *iter; //0x30

	//把30段对应的消息再解一次
	if (tlv_unpack(tlv->value, tlv->value_len, mlist) != 0)
	{
		rval = SNMPX_failure;
		m_szErrorMsg = "snmpx_group_unpack data failed: " + m_szErrorMsg;
		goto g_end;
	}
	else
	{
		if (mlist.size() < 2)
		{
			rval = SNMPX_failure;
			m_szErrorMsg = "snmpx_group_unpack size wrong, snmp version TLV is NULL !";
			goto g_end;
		}
	}

	++iter;
	tlv = *iter;  //取version

	if (tlv->tag == ASN_INTEGER)
		r->msgVersion = (unsigned char)m_BerCode.asn_integer_decode(tlv->value, tlv->value_len);
	else
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("snmpx_group_unpack tag wrong, snmp-version tag must be INTEGER, it provide[0x%02X]",
			tlv->tag);
		goto g_end;
	}

	if (r->msgVersion == 0x01 || r->msgVersion == 0x00) //v2c v1
	{
		if (mlist.size() < 4)
		{
			rval = SNMPX_failure;
			m_szErrorMsg = "snmpx_group_unpack data wrong, community or PDU TLV is NULL !";
			goto g_end;
		}

		++iter;
		tlv = *iter;  //取用户名

		if (tlv->value_len == 0)
		{
			rval = SNMPX_failure;
			m_szErrorMsg = "snmpx_group_unpack wrong, community is NULL !";
			goto g_end;
		}
		else
		{
			if (tlv->tag == ASN_OCTET_STR)
			{
				r->community_len = tlv->value_len;

				if (0 != r->community_len)
				{
					r->community = (unsigned char*)malloc(r->community_len * sizeof(unsigned char) + 1);
					r->community[r->community_len] = '\0';
					memcpy(r->community, tlv->value, r->community_len);

					//判断是否存在该用户名
					if (is_user_info_multi)
					{
						const std::map<std::string, userinfo_t*> *user_table = (const std::map<std::string, userinfo_t*>*)user_info;
						std::map<std::string, userinfo_t*>::const_iterator user_iter;
						const std::string user_id = (const char*)r->community + std::string("_") + (r->msgVersion == 0x01 ? "v1" : "v2c");

						user_iter = user_table->find(user_id);

						if (user_iter == user_table->cend())
						{
							r->errcode = 2;
							rval = SNMPX_failure;
							m_szErrorMsg = format_err_msg("snmpx_group_unpack wrong, community[%s] version[0x%02X] not exists.",
								r->community, r->msgVersion);
							goto g_end;
						}
						else
							t_user = user_iter->second;
					}
					else
						t_user = (const userinfo_t*)user_info;
				}
				else
				{
					r->errcode = 2;
					rval = SNMPX_failure;
					m_szErrorMsg = "snmpx_group_unpack wrong, community is NULL.";
					goto g_end;
				}
			}
			else
			{
				rval = SNMPX_failure;
				m_szErrorMsg = format_err_msg("snmpx_group_unpack wrong, community tag must be OCTET_STR, it provide[0x%02X]",
					tlv->tag);
				goto g_end;
			}
		}

		++iter;
		tlv = *iter;  //取操作段

		r->tag = tlv->tag;

		if (r->msgVersion == 0x00 && r->tag == SNMPX_MSG_TRAP)
			rval = unpack_trap_v1_cmd(tlv->value, tlv->value_len, r);
		else
			rval = unpack_cmd(tlv->value, tlv->value_len, r);

		if (rval < 0)
			m_szErrorMsg = "snmpx_group_unpack failed: " + m_szErrorMsg;
	}
	else if (r->msgVersion == 0x03) //v3
	{
		if (mlist.size() < 5)
		{
			rval = SNMPX_failure;
			m_szErrorMsg = format_err_msg("snmpx_group_unpack size wrong, v3 TLV pair at leat need 5, it provide[0x%02X].",
				mlist.size());
			goto g_end;
		}

	   //翻译msgGlobalData
		++iter;
		tlv = *iter;

		if (unpack_v3_msgGlobalData(tlv->value, tlv->value_len, r) < 0)
		{
			rval = SNMPX_failure;
			m_szErrorMsg = "snmpx_group_unpack failed: " + m_szErrorMsg;
			goto g_end;
		}

		//翻译认证数据段
		++iter;
		tlv = *iter;

		if (unpack_v3_authData(tlv->value, tlv->value_len, r) < 0)
		{
			rval = SNMPX_failure;
			m_szErrorMsg = "snmpx_group_unpack failed: " + m_szErrorMsg;
			goto g_end;
		}
		else
		{
			//判断是否存在该用户名，v3第一次获取agent引擎ID信息，会使用SNMPX_MSG_REPORT，没有用户名
			if (!is_get_v3_engineID)
			{
				if (NULL != r->msgUserName)
				{
					if (is_user_info_multi)
					{
						const std::map<std::string, userinfo_t*> *user_table = (const std::map<std::string, userinfo_t*>*)user_info;
						std::map<std::string, userinfo_t*>::const_iterator user_iter;
						const std::string user_id = (const char*)r->msgUserName + std::string("_V3");

						user_iter = user_table->find(user_id);

						if (user_iter == user_table->cend())
						{
							r->errcode = 2;
							rval = SNMPX_failure;
							m_szErrorMsg = format_err_msg("snmpx_group_unpack wrong, user[%s] version[0x%02X] not exists.",
								r->msgUserName, r->msgVersion);
							goto g_end;
						}
						else
						{
							t_user = user_iter->second;

							//在agent trap中或其它消息，如果不使用网管限定的引擎ID（自己的），则要在此处生成解密需要的信息
							//在trap服务端配置用户的时候，引擎ID置空，且不调用snmpx_user_init
							//有加密才处理
							if (t_user->msgAuthoritativeEngineID == NULL && (r->msgFlags & 0x03) != 0x00)
							{
								std::string error;

								//获取用户认证信息
								t_user = generate_agent_user_info(t_user, r, user_auth_info_cache, error);

								if (NULL == t_user)
								{
									rval = SNMPX_failure;
									m_szErrorMsg = "snmpx_group_unpack wrong, get v3 agent authoritative info failed: " + error;
									goto g_end;
								}
							}
						}
					}
					else
					{
						//获取引擎ID时，有的设备是以get-response响应，防止t_user==NULL，在调用check_rc_snmpx_data时程序崩溃
						t_user = (const userinfo_t*)user_info;
					}
				}
				else
				{
					r->errcode = 2;
					rval = SNMPX_failure;
					m_szErrorMsg = "snmpx_group_unpack wrong, userName is NULL.";
					goto g_end;
				}
			}
			else
				t_user = (const userinfo_t*)user_info;
		}

		++iter;
		tlv = *iter; //处理msgData段

		if ((r->msgFlags & 0x02) == 0x02) //判断数据段是否是加密类型
		{
			if (tlv->tag != ASN_OCTET_STR)
			{
				rval = SNMPX_failure;
				m_szErrorMsg = format_err_msg("snmpx_group_unpack tag wrong, v3 PDU tag must be OCTET_STR, it provide[0x%02X]",
					tlv->tag);
				goto g_end;
			}

			//判断加密密码
			if (t_user->privPasswdPrivKey == NULL || t_user->privPasswdPrivKey_len == 0)
			{
				rval = SNMPX_failure;
				m_szErrorMsg = format_err_msg("snmpx_group_unpack tag wrong, user[%s] privPasswd not set.",
					r->msgUserName);
				goto g_end;
			}

			//解密，解密成功时，明文长度和密文长度一致，DES需要去除填充
			decode = (unsigned char*)malloc(tlv->value_len * (sizeof(unsigned char)));

			if (decrypt_pdu(tlv->value, tlv->value_len, t_user, r, decode, &decodeLen) < 0)
			{
				rval = SNMPX_failure;
				m_szErrorMsg = "snmpx_group_unpack msgData failed: " + m_szErrorMsg;
				goto g_end;
			}

			//把30层解出来
			if (tlv_unpack(decode, decodeLen, mlist) != 0)
			{
				rval = SNMPX_failure;
				m_szErrorMsg = "snmpx_group_unpack failed: " + m_szErrorMsg;
				goto g_end;
			}

			if (mlist.size() < 6)
			{
				rval = SNMPX_failure;
				m_szErrorMsg = "snmpx_group_unpack size wrong, v3-encrypted TLV pair at leat need 6 !";
				goto g_end;
			}

			++iter;
			tlv = *iter;

			if (tlv->tag == ASN_SEQ)
			{
				rval = unpack_v3_msgData(tlv->value, tlv->value_len, r);

				if (rval < 0)
					m_szErrorMsg = "snmpx_group_unpack msgData failed: " + m_szErrorMsg;
			}
			else
			{
				rval = SNMPX_failure;
				m_szErrorMsg = format_err_msg("snmpx_group_unpack wrong, v3-msgData tag must be[0x%02X], it provide[0x%02X].",
					ASN_SEQ, tlv->tag);
			}
		}
		else //不加密
		{
			//解开操作段
			rval = unpack_v3_msgData(tlv->value, tlv->value_len, r);

			if (rval < 0)
				m_szErrorMsg = "snmpx_group_unpack msgData failed: " + m_szErrorMsg;
		}
	}
	else //不支持的版本
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("snmpx_unpack wrong, unsupport snmp version[0x%02X].", r->msgVersion);
		goto g_end;
	}

	//检查数据
	if (rval >= 0)
	{
		rval = check_rc_snmpx_data(r, t_user, buf, buf_len);

		if (rval < 0)
			m_szErrorMsg = "snmpx_group_unpack failed: " + m_szErrorMsg;
	}

g_end:
	//清理内存
	free_tlv_glist_data(mlist);

	if (decode != NULL)
	{
		free(decode);
		decode = NULL;
	}

	return rval;
}

/*
 * 解密数据段
 */
int CSnmpxUnpack::decrypt_pdu(const unsigned char* in, unsigned int in_len, const userinfo_t* u, const snmpx_t *r, 
	unsigned char *out, unsigned int *out_len)
{
	int rval = SNMPX_noError;
	unsigned char iv[32] = { 0 }; /* 计算iv偏移量的值 */
	CCryptographyProccess crypto;
	
	if (u->PrivMode == 0) //aes 解密方式
	{
		unsigned int msgAuthoritativeEngineBootsHl = convert_to_nl(r->msgAuthoritativeEngineBoots);
		unsigned int msgAuthoritativeEngineTimeHl = convert_to_nl(r->msgAuthoritativeEngineTime);
		memcpy(iv, &msgAuthoritativeEngineBootsHl, 4);
		memcpy(iv + 4, &msgAuthoritativeEngineTimeHl, 4);
		memcpy(iv + 8, r->msgPrivacyParameters, r->msgPrivacyParameters_len);

		/* 调用AES算法解密 */
		rval = crypto.snmpx_aes_decode(in, in_len, u->privPasswdPrivKey, iv, out);
		if (rval >= 0)
		{
			if (out[0] != ASN_SEQ)
			{
				rval = SNMPX_failure;
				m_szErrorMsg = format_err_msg("decode_msgData tag wrong: first Bytes must be[0x%02X], it decode[0x%02X], maybe it decrypted failed.",
					ASN_SEQ, out[0]);
			}
			else
				*out_len = rval;
		}
		else
			m_szErrorMsg = "decode_msgData call aes-decrypt failed: " + crypto.GetErrorMsg();
	}
	else if (u->PrivMode == 1) //des解密方式
	{
		unsigned int ivLen = 8;

		for (unsigned i = 0; i < ivLen; i++) 
		{
			iv[i] = r->msgPrivacyParameters[i] ^ u->privPasswdPrivKey[ivLen + i];
		}

		/* 调用DES算法解密 */
		rval = crypto.snmpx_des_decode(in, in_len, u->privPasswdPrivKey, iv, ivLen, out);
		if (rval > 0)
		{
			//*out_len = rval;
			const unsigned char *p = out;
			unsigned int total_len = 0;

			if (p[0] != ASN_SEQ)
			{
				rval = SNMPX_failure;
				m_szErrorMsg = format_err_msg("decode_msgData tag wrong: first Bytes must be[0x%02X], it decode[0x%02X], maybe it decrypted failed.",
					ASN_SEQ, out[0]);
			}
			else
			{
				p++;
				total_len++;

				/* 获取len的长度 判断是短格式还是长格式 */
				if ((*p & 0x80) != 0x80) //短格式
				{
					total_len += *p + 1;
					p++;
				}
				else //长格式
				{
					unsigned int num_len = (*p & 0x7F);

					if (num_len > 2)
					{
						rval = SNMPX_failure;
						m_szErrorMsg = format_err_msg("decode_msgData length wrong, value len need one or two Bytes, it provide[%u].",
							num_len);
					}
					else
					{
						total_len += num_len + 1;
						p++;

						for (unsigned int i = 0; i < num_len; i++)
						{
							total_len += *p * (unsigned int)pow(2, 8 * (num_len - i - 1));
							p++;
						}
					}
				}

				if (rval != SNMPX_failure)
				{
					if (total_len > (unsigned int)rval)
					{
						rval = SNMPX_failure;
						m_szErrorMsg = format_err_msg("decode_msgData length wrong, tlv total len[%u], it provide[%d].",
							total_len, rval);
					}
					else
					{
						//验证填充
						unsigned int pad = (unsigned int)rval - total_len;

						if (0 != pad)
						{
							for (unsigned int i = 0; i < pad; ++i)
							{
								//如果需要填充3字节，标准填充：03 03 03，锐捷交换机填充：01 02 03，华为交换机填充：00 00 00
								if (out[total_len + i] != pad 
									&& out[total_len + i] != (unsigned char)0x00
									&& out[total_len + i] != (i + 1))
								{
									rval = SNMPX_failure;
									m_szErrorMsg = format_err_msg("decode_msgData padding wrong, pading len[%u], data[0x%s].",
										pad, get_hex_string(out + total_len, pad).c_str());
									break;
								}
							}
						}

						if (rval != SNMPX_failure)
							*out_len = total_len;
					}
				}
			}
		}
		else
			m_szErrorMsg = "decode_msgData call des-decrypt failed: " + crypto.GetErrorMsg();
	}
	else
	{
		rval = SNMPX_failure;
		m_szErrorMsg = format_err_msg("decode_msgData unsupport decryption type[0x%02X].", u->PrivMode);
	}

	return rval;
}

/*
 * 检查OCTET_STR是否可打印
 */
bool CSnmpxUnpack::check_octet_str_printable(unsigned char* str, unsigned int len, bool check_chinese)
{
	//英文字符在GBK,GB2312,GK18030,ISO-8859-1,UTF-8中都占1个字节，高位是0
	//汉字在GBK,GB2312,GK18030中占用2个字节，ISO-8859-1中占用1个字节，UTF-8中占用3个字节，高位是1
	//UTF-16，英文和汉字都占4个字节，暂时不考虑
	//UTF-16BE,UTF-16LE，英文和汉字都占2个字节，暂时不考虑

	unsigned char c1, c2;
#ifndef _WIN32
	unsigned c3;
#endif

	for (unsigned int i = 0; i < len;)
	{
		c1 = str[i];

		if (0 == (c1 >> 7)) //英文字符
		{
			//32到126是可打印字符
			if ((c1 <= 0x1F || c1 >= 0x7F)
				&& '\r' != c1 && '\n' != c1 && '\t' != c1)
				return false;
		}
		else //汉字
		{
			//汉字检查不准确，不建议使用
			if (check_chinese)
			{
#ifdef _WIN32 //在windows上默认用GBK
				++i;

				if (i < len)
				{
					//下一个字节高位也是1
					c2 = str[i];

					if (1 != (c2 >> 7))
						return false;
				}
				else
					return false;
#else //在linux上默认用UTF-8
				i += 2;

				if (i < len)
				{
					//下二个字节高位也是1
					c2 = str[i - 1];
					c3 = str[i];

					if (1 != (c2 >> 7) || 1 != (c3 >> 7))
						return false;
				}
				else
					return false;
#endif
			}
			else
				return false;
		}

		++i;
	}

	return true;
}

/*
 * 转绑定值到用户对象
 */
void CSnmpxUnpack::snmpx_get_vb_value(const variable_bindings* variable_binding, SSnmpxValue* value)
{
	if (NULL != variable_binding && NULL != value)
	{
		//oid
		value->szOid = get_oid_string(variable_binding->oid_buf, variable_binding->oid_buf_len);
		value->OidBuf[0] = variable_binding->oid_buf_len / sizeof(oid); //第一个位置存在OID长度
		for (oid i = 0; i < value->OidBuf[0]; ++i)
		{
			value->OidBuf[i + 1] = variable_binding->oid_buf[i];
		}

		//value
		oid  oidValue[MAX_OID_LEN] = { 0 };
		int  oidLen = 0;
		value->iValLen = variable_binding->val_len;
		std::string szHex;

		switch (variable_binding->val_tag)  //tlv 类型
		{
		case ASN_INTEGER:  //整数
			value->cValType = SNMPX_ASN_INTEGER;
			value->Val.num.i = CBerCode::asn_integer_decode(variable_binding->val_buf, variable_binding->val_len);

			break;
		case ASN_COUNTER:
		case ASN_GAUGE:
		case ASN_TIMETICKS:
			value->cValType = SNMPX_ASN_UNSIGNED;
			value->Val.num.u = CBerCode::asn_unsigned_decode(variable_binding->val_buf, variable_binding->val_len);

			break;
		case ASN_NULL:
			value->cValType = SNMPX_ASN_NULL;
			break;
		case ASN_INTEGER64:
			value->cValType = SNMPX_ASN_INTEGER64;
			value->Val.num.ll = CBerCode::asn_integer64_decode(variable_binding->val_buf, variable_binding->val_len);

			break;
		case ASN_COUNTER64:
		case ASN_UNSIGNED64:
			value->cValType = SNMPX_ASN_UNSIGNED64;
			value->Val.num.ull = CBerCode::asn_unsigned64_decode(variable_binding->val_buf, variable_binding->val_len);

			break;
		case ASN_IPADDRESS: //ip类型
			value->cValType = SNMPX_ASN_IPADDRESS;
			value->Val.num.i = CBerCode::asn_ipaddress_decode(variable_binding->val_buf, variable_binding->val_len);

			break;
		case ASN_FLOAT:
			value->cValType = SNMPX_ASN_FLOAT;
			value->Val.num.f = CBerCode::asn_float_decode(variable_binding->val_buf, variable_binding->val_len);

			break;
		case ASN_DOUBLE:
			value->cValType = SNMPX_ASN_DOUBLE;
			value->Val.num.d = CBerCode::asn_double_decode(variable_binding->val_buf, variable_binding->val_len);

			break;
		case ASN_BIT_STR:  //位类型
			value->cValType = SNMPX_ASN_OCTET_STR;
			value->Val.hex_str = true;

			if (variable_binding->val_len >= 1)
			{
				value->Val.str = get_hex_string(variable_binding->val_buf, variable_binding->val_len);
			}

			break;
		case ASN_OCTET_STR: //字符串
			value->cValType = SNMPX_ASN_OCTET_STR;

			if (variable_binding->val_len >= 1)
			{
				if (check_octet_str_printable(variable_binding->val_buf, variable_binding->val_len))
					value->Val.str = std::string((const char*)variable_binding->val_buf, variable_binding->val_len);
				else
				{
					value->Val.hex_str = true;

					if (6 == variable_binding->val_len) //MAC地址
					{
						char MAC[32] = { '\0' };

						snprintf(MAC, sizeof(MAC), "%02X:%02X:%02X:%02X:%02X:%02X", variable_binding->val_buf[0], variable_binding->val_buf[1],
							variable_binding->val_buf[2], variable_binding->val_buf[3], variable_binding->val_buf[4], variable_binding->val_buf[5]);
						value->Val.str = MAC;
					}
					else
						value->Val.str = get_hex_string(variable_binding->val_buf, variable_binding->val_len);
				}
			}

			break;
		case ASN_OBJECT_ID: //OID 类型 ,转换成字符串类型
			value->cValType = SNMPX_ASN_OCTET_STR;

			if (variable_binding->val_len >= 1)
			{
				CBerCode ber;
				oidLen = ber.asn_oid_decode(variable_binding->val_buf, variable_binding->val_len, oidValue);

				if (oidLen > 0)
					value->Val.str = get_oid_string(oidValue, oidLen);
			}

			break;
		case ASN_NO_SUCHOBJECT: //oid不存在
		case ASN_NO_SUCHOBJECT1:
		case ASN_NO_SUCHOBJECT2:
			value->cValType = SNMPX_ASN_NO_SUCHOBJECT;
			break;
		default:
		{
			//无效类型无法显示转hex
			value->cValType = SNMPX_ASN_UNSUPPORT;

			if (variable_binding->val_len >= 1)
			{
				if (check_octet_str_printable(variable_binding->val_buf, variable_binding->val_len))
				{
					value->Val.str = CErrorStatus::format_err_msg("tag: 0x%02X data: %s", variable_binding->val_tag,
						(std::string((const char*)variable_binding->val_buf, variable_binding->val_len)).c_str());
				}
				else
				{
					value->Val.hex_str = true;
					value->Val.str = CErrorStatus::format_err_msg("tag: 0x%02X data: %s", variable_binding->val_tag,
						get_hex_string(variable_binding->val_buf, variable_binding->val_len).c_str());
				}
			}
		}
		break;
		}
	}

	return;
}

const userinfo_t* CSnmpxUnpack::generate_agent_user_info(const userinfo_t *t_user, const struct snmpx_t* r, const void *user_authinfo_cache, 
	std::string &error)
{
	if (user_authinfo_cache == NULL)
	{
		error = "generate_agent_user_info failed: user_authinfo_cache is NULL.";
		return NULL;
	}

	userinfo_t* user_info = NULL;
	const std::string agent_ip(r->ip);
	const std::pair<std::map<std::string, userinfo_t*>*, std::mutex*> *authinfo_cache = 
		(const std::pair<std::map<std::string, userinfo_t*>*, std::mutex*>*)user_authinfo_cache;
	std::map<std::string, userinfo_t*>::iterator agent_iter;

	authinfo_cache->second->lock();

	agent_iter = authinfo_cache->first->find(agent_ip);
	if (agent_iter != authinfo_cache->first->end())
	{
		//对比引擎ID，如果发生变化则要重新生成
		if (r->msgAuthoritativeEngineID_len != agent_iter->second->msgAuthoritativeEngineID_len)
		{
			authinfo_cache->first->erase(agent_iter);
			agent_iter = authinfo_cache->first->end();
		}
		else
		{
			if (0 != memcmp(r->msgAuthoritativeEngineID, agent_iter->second->msgAuthoritativeEngineID, r->msgAuthoritativeEngineID_len))
			{
				authinfo_cache->first->erase(agent_iter);
				agent_iter = authinfo_cache->first->end();
			}
		}
	}

	authinfo_cache->second->unlock();

	if (agent_iter == authinfo_cache->first->end())
	{
		//生成
		user_info = (struct userinfo_t*)malloc(sizeof(struct userinfo_t));

		if (NULL != user_info)
		{
			memset(user_info, 0x00, sizeof(struct userinfo_t));

			user_info->version = 3;
			memcpy(user_info->userName, t_user->userName, MAX_USER_INFO_LEN);
			memcpy(user_info->AuthPassword, t_user->AuthPassword, MAX_USER_INFO_LEN);
			memcpy(user_info->PrivPassword, t_user->PrivPassword, MAX_USER_INFO_LEN);
			user_info->safeMode = t_user->safeMode;
			user_info->AuthMode = t_user->AuthMode;
			user_info->PrivMode = t_user->PrivMode;
			user_info->msgAuthoritativeEngineID_len = r->msgAuthoritativeEngineID_len;
			user_info->msgAuthoritativeEngineID = (unsigned char*)malloc(user_info->msgAuthoritativeEngineID_len * (sizeof(unsigned char)));
			memcpy(user_info->msgAuthoritativeEngineID, r->msgAuthoritativeEngineID, user_info->msgAuthoritativeEngineID_len);

			CUserProccess user_proc;

			//生成认证信息
			if (user_proc.snmpx_user_init(user_info) == SNMPX_noError)
			{
				authinfo_cache->second->lock();
				authinfo_cache->first->insert(std::make_pair(agent_ip, user_info));
				authinfo_cache->second->unlock();
			}
			else
			{
				free(user_info);
				user_info = NULL;
				error = "generate_agent_user_info failed: " + user_proc.GetErrorMsg();
			}
		}
		else
			error = "generate_agent_user_info failed, malloc return NULL: " + get_err_msg(errno, true, true);
	}
	else
	{
		//复用
		user_info = agent_iter->second;
	}

	return user_info;
}