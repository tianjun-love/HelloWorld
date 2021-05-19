#include "../include/snmpx_types.hpp"
#include <cstdio>
#include <time.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#else
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#endif

//主机字节序，默认是小端
static bool g_bByteOrder_LE = true;

struct userinfo_t* clone_userinfo_data(const struct userinfo_t *user_info)
{
	if (NULL == user_info)
		return NULL;

	struct userinfo_t *clone_info = (struct userinfo_t*)malloc(sizeof(struct userinfo_t));

	if (NULL == clone_info)
		return NULL;
	else
		memset(clone_info, 0x00, sizeof(struct userinfo_t));

	//深拷贝
	clone_info->msgID = user_info->msgID;
	clone_info->version = user_info->version;
	memcpy(clone_info->userName, user_info->userName, sizeof(user_info->userName));
	clone_info->safeMode = user_info->safeMode;
	clone_info->AuthMode = user_info->AuthMode;
	memcpy(clone_info->AuthPassword, user_info->AuthPassword, sizeof(user_info->AuthPassword));
	clone_info->PrivMode = user_info->PrivMode;
	memcpy(clone_info->PrivPassword, user_info->PrivPassword, sizeof(user_info->PrivPassword));
	clone_info->agentMaxMsg_len = user_info->agentMaxMsg_len;

	clone_info->msgAuthoritativeEngineID_len = user_info->msgAuthoritativeEngineID_len;
	if (clone_info->msgAuthoritativeEngineID_len > 0 && NULL != user_info->msgAuthoritativeEngineID)
	{
		clone_info->msgAuthoritativeEngineID = (unsigned char*)malloc(clone_info->msgAuthoritativeEngineID_len * (sizeof(unsigned char)));
		memcpy(clone_info->msgAuthoritativeEngineID, user_info->msgAuthoritativeEngineID, clone_info->msgAuthoritativeEngineID_len);
	}

	clone_info->authPasswordPrivKey_len = user_info->authPasswordPrivKey_len;
	if (clone_info->authPasswordPrivKey_len > 0 && NULL != user_info->authPasswordPrivKey)
	{
		clone_info->authPasswordPrivKey = (unsigned char*)malloc(clone_info->authPasswordPrivKey_len * (sizeof(unsigned char)));
		memcpy(clone_info->authPasswordPrivKey, user_info->authPasswordPrivKey, clone_info->authPasswordPrivKey_len);
	}

	clone_info->privPasswdPrivKey_len = user_info->privPasswdPrivKey_len;
	if (clone_info->privPasswdPrivKey_len > 0 && NULL != user_info->privPasswdPrivKey)
	{
		clone_info->privPasswdPrivKey = (unsigned char*)malloc(clone_info->privPasswdPrivKey_len * (sizeof(unsigned char)));
		memcpy(clone_info->privPasswdPrivKey, user_info->privPasswdPrivKey, clone_info->privPasswdPrivKey_len);
	}

	return clone_info;
}

void free_userinfo_data(struct userinfo_t *user_info)
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

void free_userinfo_map_data(std::map<std::string, struct userinfo_t*> &user_info_map)
{
	if (!user_info_map.empty())
	{
		std::map<std::string, struct userinfo_t*>::iterator iter;

		for (iter = user_info_map.begin(); iter != user_info_map.end(); ++iter)
		{
			free_userinfo_data(iter->second);
			iter->second = NULL;
		}

		user_info_map.clear();
	}
}

void free_tlv_data(struct tlv_data *m_tlv_data)
{
	if (m_tlv_data->value != NULL)
	{
		free(m_tlv_data->value);
		m_tlv_data->value = NULL;
	}

	if (m_tlv_data != NULL)
	{
		free(m_tlv_data);
		m_tlv_data = NULL;
	}
}

void free_tlv_glist_data(std::list<struct tlv_data*> &mlist)
{
	std::list<struct tlv_data*>::iterator iter;

	for (iter = mlist.begin(); iter != mlist.end(); ++iter)
	{
		free_tlv_data(*iter);
		*iter = NULL;
	}

	mlist.clear();
}

void free_variable_glist_data(std::list<struct variable_bindings*> *variableList)
{
	if (nullptr != variableList)
	{
		std::list<variable_bindings*>::iterator iter;

		for (iter = variableList->begin(); iter != variableList->end(); ++iter)
		{
			struct variable_bindings* &tvb = *iter;

			if (tvb != NULL) {
				if (tvb->oid_buf != NULL) {
					free(tvb->oid_buf);
					tvb->oid_buf = NULL;
				}
				if (tvb->val_buf != NULL) {
					free(tvb->val_buf);
					tvb->val_buf = NULL;
				}
				free(tvb);
				tvb = NULL;
			}
		}

		variableList->clear();
	}
}

void init_snmpx_t(struct snmpx_t *s)
{
	if (nullptr != s)
	{
		memset(s, 0, sizeof(struct snmpx_t));
		s->variable_bindings_list = new std::list<variable_bindings*>();
	}
}

void clear_snmpx_t(struct snmpx_t *s)
{
	if (nullptr != s)
	{
		free_snmpx_t(s);
		init_snmpx_t(s);
	}
}

void free_snmpx_t(struct snmpx_t* s)
{
	if( s->community != NULL ){
		free(s->community);
		s->community=NULL;
	}

	if( s->msgAuthoritativeEngineID != NULL ){
		free(s->msgAuthoritativeEngineID);
		s->msgAuthoritativeEngineID = NULL;
	}
	if( s->msgUserName != NULL ){
		free(s->msgUserName);
		s->msgUserName = NULL;
	}
	if( s->msgAuthenticationParameters != NULL ){
		free(s->msgAuthenticationParameters);
		s->msgAuthenticationParameters = NULL;
	}
	if( s->msgPrivacyParameters != NULL ){
		free(s->msgPrivacyParameters);
		s->msgPrivacyParameters = NULL;
	}

	if( s->contextEngineID != NULL){
		free(s->contextEngineID);
		s->contextEngineID = NULL;
	}
	if( s->contextName != NULL ){
		free(s->contextName);
		s->contextName = NULL;
	}
	if( s->variable_bindings_list != nullptr ){
		free_variable_glist_data(s->variable_bindings_list);

		delete s->variable_bindings_list;
		s->variable_bindings_list = nullptr;
	}
	if( s->enterprise_oid != NULL ){
		free(s->enterprise_oid);
		s->enterprise_oid = NULL;
	}
}

int find_str_lac(const unsigned char* src, const unsigned int src_len, const unsigned char* dst, const unsigned int dst_len)
{
	for (unsigned int i = 0; i <= src_len - dst_len; i++)
	{
		if (memcmp(src + i, dst, dst_len) == 0)
		{
			return i;
		}
	}

	return -1;
}

int parse_ipaddress_string(const std::string &IP)
{
	//值为空或长度为0
	if (IP.empty())
	{
		return 0;
	}

	unsigned int a[4] = { 0 };

#ifdef _WIN32
	sscanf_s(IP.c_str(), "%u.%u.%u.%u", &a[0], &a[1], &a[2], &a[3]);
#else
	sscanf(IP.c_str(), "%u.%u.%u.%u", &a[0], &a[1], &a[2], &a[3]);
#endif

	if (get_byteorder_is_LE())
		return (int)((a[3] << 24) + (a[2] << 16) + (a[1] << 8) + a[0]);
	else
		return (int)((a[0] << 24) + (a[1] << 16) + (a[2] << 8) + a[3]);
}

std::string get_ipaddress_string(int ipaddress)
{
	std::string szIP;

	if (0 != ipaddress)
	{
		unsigned char* buf = (unsigned char*)&ipaddress;
		char bufIP[16] = { '\0' };

		snprintf(bufIP, sizeof(bufIP), "%hhu.%hhu.%hhu.%hhu", *buf, *(buf + 1), *(buf + 2), *(buf + 3));
		szIP = bufIP;
	}
	else
		szIP = "0.0.0.0";

	return std::move(szIP);
}

bool parse_oid_string(const std::string &oidStr, oid *oid_buf, std::string &error)
{
	//检查格式，测试后发现太耗CPU，在下面手动实现
	/*
	std::regex reg("(\\.\\d{1,9}){3,80}");
	if (!regex_match(oidStr, reg))
	{
	szError = "OID格式错误：" + oidStr;
	return false;
	}
	*/

	const size_t parmLength = oidStr.length();

	//最少是1.3.6
	if (parmLength < 5)
	{
		error = "oid[" + oidStr + "]长度错误，最小：" + std::to_string(MIN_OID_LEN) + "！";
		return false;
	}

	//oid_buf必须先申请，且是MAX_OID_LEN + 1大小
	if (NULL == oid_buf)
	{
		error = "oid_buf不能为空！";
		return false;
	}

	/*如.1.3.6.1.4.1.46859.6.1.0*/
	oid oid_len = 0;
	const char *pb = oidStr.c_str(), *pe = pb;

	//处理
	do
	{
		if ('.' == *pe || '\0' == *pe)
		{
			if (pe == pb) {
				//相当于开始的"."，不处理
			}
			else if (1 == (pe - pb) && '.' == *pb) //连续两个"."
			{
				error = "oid[" + oidStr + "]格式错误，不能有连续的多个'.'！";
				return false;
			}
			else
			{
				oid_len++;

				if (oid_len > MAX_OID_LEN)
				{
					error = "oid[" + oidStr + "]长度错误，最大：" + std::to_string(MAX_OID_LEN) + "！";
					return false;
				}
				else
				{
					oid_buf[oid_len] = (oid)strtoul(('.' == *pb ? (pb + 1) : pb), nullptr, 10);
					pb = pe;
				}
			}
		}
		else
		{
			//字符判断
			if (*pe < '0' || *pe > '9')
			{
				error = "oid[" + oidStr + "]格式错误，必须是'.'分隔的0-9数字串！";
				return false;
			}
		}

		++pe;
	} while ('\0' != *pb);

	//判断长度及开始值
	if (oid_len >= MIN_OID_LEN)
	{
		//开头必须是.1.0-.1.3
		if (1 == oid_buf[1] && oid_buf[2] <= 3)
			oid_buf[0] = oid_len;
		else
		{
			error = "oid[" + oidStr + "]格式错误，开头必须是.1.0、.1.1、.1.2或.1.3！";
			return false;
		}
	}
	else
	{
		error = "oid[" + oidStr + "]长度错误，最小：" + std::to_string(MIN_OID_LEN) + "！";
		return false;
	}

	return true;
}

bool get_byteorder_is_LE()
{
	return g_bByteOrder_LE;
}

std::string get_oid_string(oid* buf, int len)
{
	std::string szOid;

	if (NULL != buf && len >= (int)sizeof(oid))
	{
		char OidBuf[1024] = { 0 };
		int n = 0, oidCounts = len / (int)sizeof(oid);

		for (int i = 0; i < oidCounts; ++i)
		{
			n += snprintf(&OidBuf[n], 1024 - n, ".%u", buf[i]);
		}

		szOid = OidBuf;
	}

	return std::move(szOid);
}

std::string get_timeticks_string(unsigned int ticks)
{
	std::string szRet;

	if (ticks > 0)
	{
		char buf[128] = { '\0' };

		//timeticks，单位百分之一秒
		const unsigned int uiDays = ticks / 8640000,
			uiHours = (ticks % 8640000) / 360000,
			uiMinutes = (ticks % 360000) / 6000,
			uiSeconds = (ticks % 6000) / 100,
			uiTemp = ticks % 100;

		snprintf(buf, 128, "%u days, %u:%02u:%02u.%02u", uiDays, uiHours, uiMinutes, uiSeconds, uiTemp);
		szRet = buf;
	}

	return std::move(szRet);
}

std::string get_hex_string(unsigned char *data, unsigned int data_len, bool uppercase, bool add_space)
{
	std::string szReturn;

	if (nullptr != data && data_len > 0)
	{
		int n = 0;
		char *pStrTemp = new char[data_len * (add_space ? 3 : 2) + 1]{ '\0' };

		if (nullptr != pStrTemp)
		{
			for (unsigned int i = 0; i < data_len; ++i)
			{
				if (add_space)
					n += snprintf(pStrTemp + n, 4, (uppercase ? "%02X " : "%02x "), data[i]);
				else
					n += snprintf(pStrTemp + n, 3, (uppercase ? "%02X" : "%02x"), data[i]);
			}

			pStrTemp[(add_space ? n - 1 : n)] = '\0';
			szReturn = pStrTemp;
			delete[] pStrTemp;
		}
	}

	return std::move(szReturn);
}

std::string get_hex_print_string(const unsigned char *data, size_t data_len, unsigned char blank_space_len)
{
	if (NULL == data || 0 == data_len)
		return "";

	//防止前面空白太长
	if (blank_space_len > 128)
		blank_space_len = 128;

	unsigned char ch = 0, temp[16] = { 0 };
	size_t iPos = 0, iSurplusLen = 0;
	unsigned int uiAddress = 0;
	char *pPreSpace = new char[blank_space_len + 1]{ 0 };
	std::string szPrint, szDump;
	char printbuf[128] = { 0 };
	const std::string szHead = "Address   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  Dump",
		szFullFormat = "%s%08X %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  %s\n";

	memset(pPreSpace, ' ', blank_space_len);

	//头
	snprintf(printbuf, sizeof(printbuf), "%s%s\n", pPreSpace, szHead.c_str());
	szPrint = printbuf;
	memset(printbuf, 0, sizeof(printbuf));

	//数据
	do
	{
		iSurplusLen = data_len - iPos;

		if (iSurplusLen >= 16)
		{
			memcpy(temp, data + iPos, 16);

			for (unsigned int i = 0; i < 16; ++i)
			{
				ch = temp[i];

				if (ch >= 32 && ch <= 126)
					szDump += (char)ch;
				else
					szDump += ".";
			}

			snprintf(printbuf, sizeof(printbuf), szFullFormat.c_str(), pPreSpace, uiAddress, temp[0], temp[1], temp[2], temp[3],
				temp[4], temp[5], temp[6], temp[7], temp[8], temp[9], temp[10], temp[11], temp[12], temp[13], temp[14], temp[15],
				szDump.c_str());
			szPrint.append(printbuf);
			memset(printbuf, 0, sizeof(printbuf));

			iPos += 16;
		}
		else
		{
			if (iSurplusLen > 0)
			{
				memcpy(temp, data + iPos, iSurplusLen);

				for (unsigned int i = 0; i < iSurplusLen; ++i)
				{
					ch = temp[i];

					if (ch >= 32 && ch <= 126)
						szDump += (char)ch;
					else
						szDump += ".";
				}

				//计算格式
				unsigned int uiWide = 48;

				snprintf(printbuf, sizeof(printbuf), "%s%08X", pPreSpace, uiAddress);
				szPrint.append(printbuf);
				memset(printbuf, 0, sizeof(printbuf));

				for (unsigned int i = 0; i < iSurplusLen; ++i)
				{
					uiWide -= 3;

					snprintf(printbuf, sizeof(printbuf), " %02x", temp[i]);
					szPrint.append(printbuf);
					memset(printbuf, 0, sizeof(printbuf));
				}

				snprintf(printbuf, sizeof(printbuf), "  %*s", uiWide, " ");
				szPrint.append(printbuf + szDump + "\n");
			}

			iPos += iSurplusLen;
		}

		++uiAddress;
		memset(temp, 0, sizeof(temp));
		szDump.clear();
	} while (iPos < data_len);

	delete[] pPreSpace;

	return std::move(szPrint);
}

std::string get_current_time_string(bool add_millseconds)
{
	char strDateTime[32] = { '\0' };
	struct tm tmTimeTemp;
	time_t timeTemp = time(NULL);
	long milliseconds = 0;

	//localtime函数不可重入，即不是线程安全的，所以用下面的版本
#ifdef _WIN32
	localtime_s(&tmTimeTemp, &timeTemp);

	if (add_millseconds)
	{
		SYSTEMTIME ctT;
		GetLocalTime(&ctT);
		milliseconds = ctT.wMilliseconds;
	}
#else
	localtime_r(&timeTemp, &tmTimeTemp);

	if (add_millseconds)
	{
		timeval tv;
		gettimeofday(&tv, NULL);
		milliseconds = (tv.tv_usec / 1000);
	}
#endif

	if (add_millseconds)
	{
		snprintf(strDateTime, sizeof(strDateTime), "%02d:%02d:%02d.%03ld", tmTimeTemp.tm_hour,
			tmTimeTemp.tm_min, tmTimeTemp.tm_sec, milliseconds);
	}
	else
		strftime(strDateTime, sizeof(strDateTime), "%X", &tmTimeTemp);

	return std::string(strDateTime);
}

std::string get_vb_item_print_string(const SSnmpxValue &vb, size_t max_oid_string_len)
{
	if (max_oid_string_len > 128)
		max_oid_string_len = 128;

	std::string szResult;
	const std::string oid_format = "%-" + std::to_string(max_oid_string_len) + "s";
	const unsigned int buff_len = 1024;
	char buff[buff_len] = { '\0' };

	switch (vb.cValType)
	{
	case SNMPX_ASN_INTEGER:
		snprintf(buff, buff_len, (oid_format + " => %10s : %d").c_str(), vb.szOid.c_str(), "ingeter", vb.Val.num.i);
		break;
	case SNMPX_ASN_NULL:
		snprintf(buff, buff_len, (oid_format + " => %10s : ").c_str(), vb.szOid.c_str(), "null");
		break;
	case SNMPX_ASN_UNSIGNED:
		snprintf(buff, buff_len, (oid_format + " => %10s : %u").c_str(), vb.szOid.c_str(), "unsigned", vb.Val.num.u);
		break;
	case SNMPX_ASN_INTEGER64:
		snprintf(buff, buff_len, (oid_format + " => %10s : %lld").c_str(), vb.szOid.c_str(), "ingeter64", vb.Val.num.ll);
		break;
	case SNMPX_ASN_UNSIGNED64:
		snprintf(buff, buff_len, (oid_format + " => %10s : %llu").c_str(), vb.szOid.c_str(), "unsigned64", vb.Val.num.ull);
		break;
	case SNMPX_ASN_FLOAT:
		snprintf(buff, buff_len, (oid_format + " => %10s : %0.3f").c_str(), vb.szOid.c_str(), "float", vb.Val.num.f);
		break;
	case SNMPX_ASN_DOUBLE:
		snprintf(buff, buff_len, (oid_format + " => %10s : %0.3lf").c_str(), vb.szOid.c_str(), "double", vb.Val.num.d);
		break;
	case SNMPX_ASN_OCTET_STR:
		if (vb.Val.hex_str)
			snprintf(buff, buff_len, (oid_format + " => %10s : %s").c_str(), vb.szOid.c_str(), "hex-string", vb.Val.str.c_str());
		else
			snprintf(buff, buff_len, (oid_format + " => %10s : %s").c_str(), vb.szOid.c_str(), "string", vb.Val.str.c_str());
		break;
	case SNMPX_ASN_IPADDRESS:
		snprintf(buff, buff_len, (oid_format + " => %10s : %s").c_str(), vb.szOid.c_str(), "ipaddress", 
			get_ipaddress_string(vb.Val.num.i).c_str());
		break;
	case SNMPX_ASN_UNSUPPORT:
		snprintf(buff, buff_len, (oid_format + " => %10s : %s").c_str(), vb.szOid.c_str(), "unsupport", vb.Val.str.c_str());
		break;
	case SNMPX_ASN_NO_SUCHOBJECT:
		snprintf(buff, buff_len, (oid_format + " => invalid : no such object.").c_str(), vb.szOid.c_str());
		break;
	default:
		snprintf(buff, buff_len, (oid_format + " => tag 0x%02X : unknow print.").c_str(), vb.szOid.c_str(), vb.cValType);
		break;
	}

	szResult = buff;

	return std::move(szResult);
}

std::string get_vb_items_print_string(const std::list<SSnmpxValue> &vb_list, size_t max_oid_string_len)
{
	if (max_oid_string_len > 128)
		max_oid_string_len = 128;

	std::string szResult;

	for (const auto &p : vb_list)
	{
		if (szResult.empty())
			szResult = get_vb_item_print_string(p, max_oid_string_len);
		else
			szResult.append("\n" + get_vb_item_print_string(p, max_oid_string_len));
	}

	return std::move(szResult);
}

unsigned short convert_to_ns(unsigned short s)
{
	if (get_byteorder_is_LE())
	{
		u_digital_16 d;
		unsigned char ch;

		d.un = s;
		ch = d.buff[0];
		d.buff[0] = d.buff[1];
		d.buff[1] = ch;

		return d.un;
	}

	return s;
}

unsigned int convert_to_nl(unsigned int l)
{
	if (get_byteorder_is_LE())
	{
		u_digital_32 d;
		unsigned char ch;

		d.ui = l;

		ch = d.buff[0];
		d.buff[0] = d.buff[3];
		d.buff[3] = ch;

		ch = d.buff[1];
		d.buff[1] = d.buff[2];
		d.buff[2] = ch;

		return d.ui;
	}
	
	return l;
}

unsigned long long convert_to_nll(unsigned long long ll)
{
	if (get_byteorder_is_LE())
	{
		u_digital_64 d;
		unsigned char ch;

		d.ull = ll;

		ch = d.buff[0];
		d.buff[0] = d.buff[7];
		d.buff[7] = ch;

		ch = d.buff[1];
		d.buff[1] = d.buff[6];
		d.buff[6] = ch;

		ch = d.buff[2];
		d.buff[2] = d.buff[5];
		d.buff[5] = ch;

		ch = d.buff[3];
		d.buff[3] = d.buff[4];
		d.buff[4] = ch;

		return d.ull;
	}

	return ll;
}

bool init_snmpx_global_env(std::string &szError)
{
	//初始化windows的socket环境
#ifdef WIN32
	WSADATA wsa_data;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);

	if (ret != 0) {
		szError = "windows WSAStartup failed, return: " + std::to_string(ret);
		return false;
	}
#endif

	//获取本机CPU字节序
	u_digital_16 var16;

	var16.un = 0x0102;

	if (0x02 == var16.buff[0])
	{
		//小端存储为：0x0201
		g_bByteOrder_LE = true;
	}
	else
	{
		//大端存储为：0x0102
		g_bByteOrder_LE = false;
	}

	return true;
}

void free_snmpx_global_env()
{
	//清理windows的socket环境
#ifdef WIN32
	WSACleanup();
#endif
}

void close_socket_fd(int fd)
{
	if (fd > 0)
	{
#ifdef _WIN32
		closesocket(fd);
#else
		close(fd);
#endif
	}
}