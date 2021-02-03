#include "../include/ber_code.hpp"
#include <malloc.h>
#include <math.h>
#include <stdio.h>

CBerCode::CBerCode()
{
}

CBerCode::~CBerCode()
{
}

/*
 * ber interger 编码
 * -244 ber  ff 0c  , -12 ber f4
 *  244 ber  00 f4  ,  12 ber 0c
 *  按照大端的方式传送,低位存高节点的数值
 *	返回写入字节数
 */
int CBerCode::asn_integer_code(unsigned char tag, unsigned int val_len, unsigned char* val_buf, unsigned char* tlv_buf)
{
	//判断类型
	switch (tag)
	{
	case ASN_INTEGER:
	case ASN_INTEGER64:
	case ASN_FLOAT:
	case ASN_DOUBLE:
		break;
	default:
		m_szErrorMsg = format_err_msg("asn_integer_code tag wrong, must be INTEGER/INTEGER64/FLOAT/DOUBLE, unsupport[0x%02X].",
			tag);
		return SNMPX_failure;
	}

	//值为空或长度为0
	if (val_buf == NULL || val_len == 0)
	{
		*tlv_buf = 0x00;
		return 1;
	}

	int rval = 0;
	unsigned char *s_buf = NULL;
	unsigned int i = 0, m = 0;

	//转换为大端
	s_buf = (unsigned char*)malloc(val_len);
	if (s_buf == NULL)
	{
		m_szErrorMsg = "asn_integer_code malloc wrong, request memory buffer failed !";
		return SNMPX_failure;
	}
	else
	{
		for (i = 0; i < val_len; i++)
		{
			if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
				s_buf[val_len - i - 1] = val_buf[i];
			else
				s_buf[i] = val_buf[i];
		}
	}

	if ((s_buf[0] & 0x80) == 0x80) //负数
	{
		for (i = 0; i < val_len; i++)
		{
			if (s_buf[i] != 0xFF)
			{
				//去掉多余的0xFF
				break;
			}
		}

		//判断是否需要补FF
		if ((s_buf[i] & 0x80) != 0x80) //最高位不是1需补0xFF
		{
			*tlv_buf = 0xFF;

			for (m = i; m < val_len; m++)
			{
				tlv_buf++;
				*tlv_buf = *(s_buf + m);
			}

			rval = (int)(val_len - i + 1);
		}
		else
		{
			for (m = i; m < val_len; m++)
			{
				*tlv_buf = *(s_buf + m);
				tlv_buf++;
			}

			rval = (int)(val_len - i);
		}
	}
	else //正数
	{
		for (i = 0; i < val_len; i++)
		{
			if (s_buf[i] != 0x00)
			{
				//去掉多余的0x00
				break;
			}
		}

		//输入的数为0
		if (i == val_len) 
		{
			*tlv_buf = 0x00;
			rval = 1;
		}
		else
		{
			//判断是否需要补00
			if ((s_buf[i] & 0x80) == 0x80)
			{
				*tlv_buf = 0x00;

				for (m = i; m < val_len; m++)
				{
					tlv_buf++;
					*tlv_buf = *(s_buf + m);
				}

				rval = (int)(val_len - i + 1);
			}
			else //不需要补00
			{
				for (m = i; m < val_len; m++)
				{
					*tlv_buf = *(s_buf + m);
					tlv_buf++;
				}

				rval = (int)(val_len - i);
			}
		}
	}

	//清理
	free(s_buf);

	return rval;
}

/*
* ber unsigned 编码
*/
int CBerCode::asn_unsigned_code(unsigned char tag, unsigned int val_len, unsigned char* val_buf, unsigned char* tlv_buf)
{
	switch (tag)
	{
	case ASN_TIMETICKS:
	case ASN_COUNTER:
	case ASN_GAUGE: //ASN_UNSIGNED
	case ASN_COUNTER64:
	case ASN_UNSIGNED64:
		break;
	default:
		m_szErrorMsg = format_err_msg("asn_unsigned_code tag wrong, must be TIMETICKS/COUNTER/GAUGE/COUNTER64/UNSIGNED64, unsupport[0x%02X].",
			tag);
		return SNMPX_failure;
	}

	//值为空或长度为0
	if (val_buf == NULL || val_len == 0)
	{
		*tlv_buf = 0x00;
		return 1;
	}

	int rval = 0;
	unsigned char *s_buf = NULL;
	unsigned int i = 0, m = 0;

	//转换为大端
	s_buf = (unsigned char*)malloc(val_len);
	if (s_buf == NULL)
	{
		m_szErrorMsg = "asn_unsigned_code malloc wrong, request memory buffer failed !";
		return SNMPX_failure;
	}
	else
	{
		for (i = 0; i < val_len; i++)
		{
			if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
				s_buf[val_len - i - 1] = val_buf[i];
			else
				s_buf[i] = val_buf[i];
		}
	}

	for (i = 0; i < val_len; i++)
	{
		if (s_buf[i] != 0x00)
		{
			//找到不是0的字节
			break;
		}
	}

	//输入的数为0
	if (i == val_len)
	{
		*tlv_buf = 0x00;
		rval = 1;
	}
	else
	{
		//判断是否需要补00
		if ((s_buf[i] & 0x80) == 0x80)
		{
			*tlv_buf = 0x00;

			for (m = i; m < val_len; m++)
			{
				tlv_buf++;
				*tlv_buf = *(s_buf + m);
			}

			rval = (int)(val_len - i + 1);
		}
		else
		{
			for (m = i; m < val_len; m++)
			{
				*tlv_buf = *(s_buf + m);
				++tlv_buf;
			}

			rval = (int)(val_len - i);
		}
	}

	//清理
	free(s_buf);

	return rval;
}

/*
* ber oid 编码
*/
int CBerCode::asn_oid_code(unsigned char tag , unsigned int val_len , oid* val_buf , unsigned char* tlv_buf)
{
	if (tag != ASN_OBJECT_ID)
	{
		m_szErrorMsg = "asn_oid_code tag wrong, must be OBJECT_ID !";
		return SNMPX_failure;
	}

	const unsigned int oid_len = val_len / (unsigned int)sizeof(oid);
	unsigned int i = 0, m = 0, total7 = 0;
	oid oid_value = 0;
	unsigned char v;
	int cnt = 0;

	if (oid_len < MIN_OID_LEN)
	{
		m_szErrorMsg = "asn_oid_code length wrong, oid length minimum is: " + std::to_string(oid_len) + " !";
		return -1;
	}

	//开头固定是.1.0、.1.1、.1.2、.1.3
	if (val_buf[0] != 0x01 || val_buf[1] > 0x03)
	{
		m_szErrorMsg = "asn_oid_code value wrong, oid must begin with 1.3 !";
		return SNMPX_failure;
	}

	//oid的前两个字节合并
	*(tlv_buf) = (unsigned char)(40 * val_buf[0] + val_buf[1]);
	cnt++;

	for (i = 2; i < oid_len; i++)
	{
		oid_value = val_buf[i];

		if (oid_value > 127)
		{
			const unsigned int s_len = (unsigned int)sizeof(oid);
			unsigned char s_buf[s_len] = { 0 };

			//统一转成大端
			if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
			{
				for (unsigned int j = s_len; j > 0; j--)
				{
					s_buf[j - 1] = ((unsigned char*)&oid_value)[s_len - j];
				}
			}
			else
				memcpy(s_buf, (unsigned char*)&oid_value, sizeof(s_buf));

			//去掉多余的00，4个字节不用for，提高效率
			if (0 == s_buf[3])
			{
				if (0 == s_buf[2])
				{
					if (0 == s_buf[1])
						m = 1; //不可能全0
					else
						m = 2;
				}
				else
					m = 3;
			}
			else
				m = 4;

			total7 = m * 8 / 7; //算出有多少个七字节组

			if (m * 8 % 7 != 0)
			{
				total7 = total7 + 1;
			}

			for (m = 0; m < total7; m++)
			{
				if (m == (total7 - 1)) 
				{
					*(tlv_buf + cnt) = (oid_value & (unsigned char)0x7F); //直接赋值
				}
				else
				{
					v = (oid_value >> (7 * (total7 - m - 1))) & (unsigned char)0x7F;

					if (0 == m && 0x00 == v) //高位是0，去掉
						continue;
					else
						*(tlv_buf + cnt) = (v | (unsigned char)0x80); //最高位加1
				}

				cnt++;
			}
		}
		else 
		{
			*(tlv_buf + cnt) = (unsigned char)oid_value; //直接赋值
			cnt++;
		}
	}

	return cnt;
}

/*
* ber oid 解码
*/
int CBerCode::asn_oid_decode(unsigned char* buf, unsigned int len, oid* decode_oid)
{
	if (buf == NULL)
	{
		m_szErrorMsg = "asn_oid_decode value wrong, oid value is NULL !";
		return SNMPX_failure;
	}

	if (len < (MIN_OID_LEN - 1))
	{
		m_szErrorMsg = "asn_oid_decode length wrong, oid value length minimum is: " + std::to_string(MIN_OID_LEN) + " !";
		return SNMPX_failure;
	}

	//开头固定是.1.0、.1.1、.1.2、.1.3
	if (buf[0] < 0x28 || buf[0] > 0x2B)
	{
		m_szErrorMsg = "asn_oid_decode value wrong, oid value must in between 0x28 and 0x2B !";
		return SNMPX_failure;
	}

	int cnt = 0;
	*decode_oid = 0x01;
	cnt++;
	*(decode_oid + cnt) = buf[0] - 40;
	cnt++;

	unsigned int cb = 0, m = 0;
	unsigned char t[64] = { 0 };
	oid v = 0;

	for (unsigned int i = 1; i < len; i++)
	{
		if (((*(buf + i)) & 0x80) == 0x80) //判断最高字节是否为1
		{
			t[cb] = (*(buf + i)) & 0x7F;
			cb++;
		}
		else
		{
			//直接算值	
			t[cb] = *(buf + i);
			cb++;

			for (m = 0; m < cb; m++)
			{
				v = v + t[m] * ((oid)pow(2, 7 * (cb - m - 1)));   //重组oid字段
			}

			*(decode_oid + cnt) = v;

			cb = 0;
			v = 0;
			memset(t, 0x00, sizeof(t));
			cnt++;
		}
	}

	return cnt * sizeof(oid);
}

/*
* ip 编码，IPAddress
*/
int CBerCode::asn_ipaddress_code(unsigned int val_len, unsigned char* val_buf, unsigned char* tlv_buf)
{
	if (val_len < 4 && val_len != 0)
	{
		m_szErrorMsg = "asn_ipaddress_code length wrong, ipaddress must be 0 or 4 Bytes !";
		return SNMPX_failure;
	}

	//值为空或长度为0
	if (val_buf == NULL || val_len == 0)
	{
		*tlv_buf = 0x00;
		return 1;
	}

	for (int i = 0; i < 4; i++)
	{
		*(tlv_buf + i) = val_buf[i];
	}

	return 4;
}

/*
* ber integer 解码
*/
int CBerCode::asn_integer_decode(unsigned char* buf, unsigned int len)
{
	unsigned char t_buf[sizeof(int)];
	unsigned int i = 0;
	int rval = 0;

	if ((buf[0] & 0x80) == 0x80) //负数
		memset(t_buf, 0xFF, sizeof(t_buf));
	else
		memset(t_buf, 0x00, sizeof(t_buf));

	//低字节往高字节赋值，因为前面可能有补或去除的字节
	for (i = 0; i < len && i < (unsigned int)sizeof(t_buf); i++)
	{
		if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
			t_buf[i] = buf[len - i - 1];
		else
			t_buf[sizeof(t_buf) - i - 1] = buf[len - i - 1];
	}

	memcpy(&rval, t_buf, sizeof(rval));

	return rval;
}

/*
* ber unsigned int 解码
*/
unsigned int CBerCode::asn_unsigned_decode(unsigned char* buf, unsigned int len)
{
	unsigned char t_buf[sizeof(unsigned int)];
	unsigned int rval = 0, i = 0;

	memset(t_buf, 0x00, sizeof(t_buf));

	//低字节往高字节赋值，因为前面可能有补或去除的字节
	for (i = 0; i < len && i < (unsigned int)sizeof(t_buf); i++)
	{
		if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
			t_buf[i] = buf[len - i - 1];
		else
			t_buf[sizeof(t_buf) - i - 1] = buf[len - i - 1];
	}

	memcpy(&rval, t_buf, sizeof(rval));

	return rval;
}

/*
* ber long long 解码
*/
long long CBerCode::asn_integer64_decode(unsigned char* buf, unsigned int len)
{
	unsigned char t_buf[sizeof(long long)];
	unsigned int i = 0;
	long long rval = 0;

	if ((buf[0] & 0x80) == 0x80) //负数
		memset(t_buf, 0xFF, sizeof(t_buf));
	else
		memset(t_buf, 0x00, sizeof(t_buf));

	//低字节往高字节赋值，因为前面可能有补或去除的字节
	for (i = 0; i < len && i < (unsigned int)sizeof(t_buf); i++)
	{
		if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
			t_buf[i] = buf[len - i - 1];
		else
			t_buf[sizeof(t_buf) - i - 1] = buf[len - i - 1];
	}

	memcpy(&rval, t_buf, sizeof(rval));

	return rval;
}

/*
* ber unsigned long long 解码
*/
unsigned long long CBerCode::asn_unsigned64_decode(unsigned char* buf, unsigned int len)
{
	unsigned char t_buf[sizeof(unsigned long long)];
	unsigned long long rval = 0;
	unsigned int i = 0;

	memset(t_buf, 0x00, sizeof(t_buf));

	//低字节往高字节赋值，因为前面可能有补或去除的字节
	for (i = 0; i < len && i < (unsigned int)sizeof(t_buf); i++)
	{
		if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
			t_buf[i] = buf[len - i - 1];
		else
			t_buf[sizeof(t_buf) - i - 1] = buf[len - i - 1];
	}

	memcpy(&rval, t_buf, sizeof(rval));

	return rval;
}

/*
* ber float 解码
*/
float CBerCode::asn_float_decode(unsigned char* buf, unsigned int len)
{
	unsigned char t_buf[sizeof(float)];
	unsigned int i = 0;
	float rval = 0.0f;

	if ((buf[0] & 0x80) == 0x80) //负数
		memset(t_buf, 0xFF, sizeof(t_buf));
	else
		memset(t_buf, 0x00, sizeof(t_buf));

	//低字节往高字节赋值，因为前面可能有补或去除的字节
	for (i = 0; i < len && i < (unsigned int)sizeof(t_buf); i++)
	{
		if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
			t_buf[i] = buf[len - i - 1];
		else
			t_buf[sizeof(t_buf) - i - 1] = buf[len - i - 1];
	}

	memcpy(&rval, t_buf, sizeof(rval));

	return rval;
}

/*
* ber double 解码
*/
double CBerCode::asn_double_decode(unsigned char* buf, unsigned int len)
{
	unsigned char t_buf[sizeof(double)];
	unsigned int i = 0;
	double rval = 0.0;

	if ((buf[0] & 0x80) == 0x80) //负数
		memset(t_buf, 0xFF, sizeof(t_buf));
	else
		memset(t_buf, 0x00, sizeof(t_buf));

	//低字节往高字节赋值，因为前面可能有补或去除的字节
	for (i = 0; i < len && i < (unsigned int)sizeof(t_buf); i++)
	{
		if (HOST_ENDIAN_TYPE == SNMPX_LITTLE_ENDIAN)
			t_buf[i] = buf[len - i - 1];
		else
			t_buf[sizeof(t_buf) - i - 1] = buf[len - i - 1];
	}

	memcpy(&rval, t_buf, sizeof(rval));

	return rval;
}

/*
 * ip 解码
 */
int CBerCode::asn_ipaddress_decode(unsigned char* buf, unsigned int len)
{
	unsigned char t_buf[sizeof(unsigned int)] = { 0x00 };
	int rval = 0;
	unsigned int i = 0;

	for (i = 0; i < len && i < (unsigned int)sizeof(unsigned int); i++)
	{
		t_buf[i] = buf[i];
	}

	memcpy(&rval, t_buf, sizeof(rval));

	return rval;
}

/*
* ip 解码
*/
std::string CBerCode::asn_ipaddress_decode_string(unsigned char* buf, unsigned int len)
{
	std::string szIP;

	if (len >= 4)
	{
		char bufIP[16] = { '\0' };

		snprintf(bufIP, sizeof(bufIP), "%hhu.%hhu.%hhu.%hhu", *buf, *(buf + 1), *(buf + 2), *(buf + 3));
		szIP = bufIP;
	}
	else
		szIP = "0.0.0.0";

	return std::move(szIP);
}