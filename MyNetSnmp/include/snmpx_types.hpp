﻿#ifndef __SNMPX_TYPES_HPP__
#define __SNMPX_TYPES_HPP__

#include <list>
#include <string>
#include <string.h>
#include <map>
#include <vector>

//TLV编码说明：
/*
TLV(Type,Length,Value)三元组。编码可以是基本型或结构型，如果它表示一个简单类型的、完整的显式值，那么编码就是基本型 (primitive)；
如果它表示的值具有嵌套结构，那么编码就是结构型 (constructed)
长度域指明值域的长度,不定长,一般为一到三个字节。其格式可分为短格式（后面的值域没有超过127长）和长格式,如下所示
	短格式的表示方法：
		0（1bit）	长度（7bit）
	长格式的表示方法：
		1（1bit）	K（7bit）	K个八位组长度（K Byte）
例:
	length=30=>1E（30没有超过127，长度域为0001 1110）
	length=169=>81 A9（169超过127，长度域为 1000 0001 1010 1001，169是后9位的值，前八位的第一个1表示这是长格式的表示方法，
		前八位的后七位表示后面有多少个字节表示针对的长度，这里，是000 0001，后面有一个字节表示真正的长度，1010 1001是169，
		后面的值有169个字节长。）
	length=1500=>82 05 DC（1000 0010 0000 0101 1101 1100，先看第一个字节，表示长格式，后面有2个字节表示长度，
		这两个字节是0000 0101 1101 1100 表示1500）
*/

//报文说明：
/*
V1报文：
	消息格式：版本号 | 团体名 | PDU
	get/getnext/set PDU格式：PDU类型 | 请求ID | 0 | 0 | item段
	response PDU格式：PDU类型 | 请求ID | error_state | error_index | item段
	trap(无响应) PDU格式：PDU类型 | enterprise | agent addr | generic trap | specific trap | time stamp | item段
	item段格式：TLV...TLV

V2报文，在V1的基础上增加了GetBulk和inform(有响应)操作：
	getbulk PDU格式：PDU类型 | 请求ID | non-repeaters | max-repetitions | item段
	trap(无响应，修改与V1的get/getnext/set PDU相同，将enterprise和timestamp放在item段) PDU格式：PDU类型 | 请求ID | 0 | 0 | item段

V3报文：
	                |          Header                          | SecurityParam |              Data
	消息格式：版本号 | 请求ID | MaxSize | Flags | SecurityModel | SecurityParam | ContextEngineID | ContextName | PDU
	参数说明：
		RequestID：请求报文的序列号。
		MaxSize：消息发送者所能够容纳的消息最大字节，同时也表明了发送者能够接收到的最大字节数。
		Flags：消息标识位，占一个字节,三个标志位：reportPDU|privFlag|authFlag，当reportPDU=1时，在能导致reportPDU产生时，消息接收方必须回送reportPDU
		SecurityModel：消息的安全模型值，取值为0～3。0表示任何模型，1表示采用SNMPv1安全模型，2表示采用SNMPv2c安全模型，3表示采用SNMPv3安全模型。
		ContextEngineID：唯一识别一个SNMP实体。对于接收消息，该字段确定消息该如何处理；对于发送消息，该字段在发送一个消息请求时由应用提供。
		ContextName：唯一识别在相关联的上下文引擎范围内部特定的上下文。
		SecurityParameters又包括以下主要字段：
			AuthoritativeEngineID：消息交换中权威SNMP的snmpEngineID，用于SNMP实体的识别、认证和加密。该取值在Trap、Response、Report中是源端的
			snmpEngineID，对Get、GetNext、GetBulk、Set中是目的端的snmpEngineID。
			AuthoritativeEngineBoots：消息交换中权威SNMP的snmpEngineBoots。表示从初次配置时开始，SNMP引擎已经初始化或重新初始化的次数。
			AuthoritativeEngineTime：消息交换中权威SNMP的snmpEngineTime，用于时间窗判断。
			UserName：用户名，消息代表其正在交换。NMS和Agent配置的用户名必须保持一致。
			AuthenticationParameters：认证参数，认证运算时所需的密钥。如果没有使用认证则为空。
			PrivacyParameters：加密参数，加密运算时所用到的参数，比如DES CBC算法中形成初值IV所用到的取值。如果没有使用加密则为空
*/

//引擎ID说明：
/*
引擎ID标识为一个OCTET_STRING，长度为5-32字节长
	前4个字节标识厂商的私有企业号（由IANA分配），用HEX表示
	第5个字节表示剩下的字节如何表示
	0：保留
	1：后面4个字节是一个ipv4地址
	2：后面16个字节是一个ipv6地址
	3：后面6个字节是一个MAC地址
	4：文本，最长27个字节，由厂商自行定义
	5：16进制，最长27个字节，由厂商自行定义
	6-127：保留
	128-255：由厂商特定的格式
*/

//类型标识符(tag)说明：
/*1字节
 *7-6位：族比特位
 *		00：通用类，ASN_UNIVERSAL
		01：应用类，ASN_APPLICATION
		10：上下文类（如PDU），ASN_CONTEXT
		11：专用类，ASN_PRIVATE，保留为特定厂商定义的类型
 *  5位：格式
		0：简单类型
		1：结构类型，SEQ或PDU等
 *4-0位：标签码比特位，表示不同的数据类型，取值0-30
 */

#define ASN_BOOLEAN	        ((unsigned char)0x01)
#define	ASN_INTEGER	        ((unsigned char)0x02)
#define	ASN_BIT_STR	        ((unsigned char)0x03)
#define	ASN_OCTET_STR	    ((unsigned char)0x04)
#define	ASN_NULL	        ((unsigned char)0x05)
#define	ASN_OBJECT_ID	    ((unsigned char)0x06)
#define	ASN_ENUM	        ((unsigned char)0x0A)
#define	ASN_SET		        ((unsigned char)0x11)

#define	ASN_SEQ		        ((unsigned char)0x30) //用于列表，SEQ可以包含子SEQ，与C中的structure类似
#define	ASN_SETOF		    ((unsigned char)0x31) //用于表格，元素具有相同类型，与C中的array类似

#define ASN_IPADDRESS       ((unsigned char)0x40) //0或4字节        
#define ASN_COUNTER         ((unsigned char)0x41)	        
#define ASN_GAUGE           ((unsigned char)0x42)         
#define ASN_TIMETICKS       ((unsigned char)0x43) //以0.01秒为单位	        
#define ASN_OPAQUE          ((unsigned char)0x44)	        
#define ASN_NSAP            ((unsigned char)0x45) /* historic - don't use */     
#define ASN_COUNTER64       ((unsigned char)0x46)     
#define ASN_UINTEGER        ((unsigned char)0x47) /* historic - don't use */	        
#define ASN_FLOAT           ((unsigned char)0x48) 	        
#define ASN_DOUBLE          ((unsigned char)0x49)     
#define ASN_INTEGER64       ((unsigned char)0x4a)    
#define ASN_UNSIGNED64      ((unsigned char)0x4b)

#define ASN_NO_SUCHOBJECT   ((unsigned char)0x80) //oid不存在，抓包发现
#define ASN_NO_SUCHOBJECT1  ((unsigned char)0x81) //oid不存在，抓包发现
#define ASN_NO_SUCHOBJECT2  ((unsigned char)0x82) //oid不存在，抓包发现

#define	SNMPX_MSG_GET       ((unsigned char)0xA0) /* a0=160 */
#define	SNMPX_MSG_GETNEXT   ((unsigned char)0xA1) /* a1=161 */
#define	SNMPX_MSG_RESPONSE  ((unsigned char)0xA2) /* a2=162 */
#define	SNMPX_MSG_SET       ((unsigned char)0xA3) /* a3=163 */
#define SNMPX_MSG_TRAP      ((unsigned char)0xA4) /* a4=164  v1 trap，不建议使用*/ 
#define	SNMPX_MSG_GETBULK   ((unsigned char)0xA5) /* a5=165 v2增加*/
#define	SNMPX_MSG_INFORM    ((unsigned char)0xA6) /* a6=166 v2增加*/
#define	SNMPX_MSG_TRAP2     ((unsigned char)0xA7) /* a7=167 v2,v3*/
#define SNMPX_MSG_REPORT    ((unsigned char)0xA8) /* a8=168 v3增加，获取agent引擎ID或消息的PDU部分不能解密时，发起报告*/

#define MIN_OID_LEN	         (0x03)
#define MAX_OID_LEN	         (0x80)
#define MAX_MSG_LEN	         (65507) /* snmp抓包数据*/
#define MAX_USER_INFO_LEN    (128)   /* 用户信息数据最大长度，用户名、认证密码及加密密码*/
#define MAX_BULK_REPETITIONS (50)    /* table时最大回复条数*/
#define	SNMPX_LITTLE_ENDIAN  (0)     /* 小端*/
#define	SNMPX_BIG_ENDIAN     (1)     /* 大端*/
#define	HOST_ENDIAN_TYPE     SNMPX_LITTLE_ENDIAN  /* 主机大小端类型*/

typedef unsigned long oid;

struct tlv_data
{
	unsigned char tag;
	unsigned int value_len;
	unsigned char* value;
};

struct userinfo_t
{
	int msgID;                                //消息ID
	unsigned char version;                    //版本号 //0:v1，1:v2c，2:v2u/v2，3:v3
	char userName[MAX_USER_INFO_LEN + 1];     //用户名，在v1和v2c时表示团体名
	unsigned char safeMode;                   //认证级别 0:noAuthNoPriv|1:authNoPriv|2:authPriv
	unsigned char AuthMode;                   //认证方式 0:MD5|1:SHA
	char AuthPassword[MAX_USER_INFO_LEN + 1]; //认证密码
	unsigned char PrivMode;                   //加密方式 0:AES|1:DES
	char PrivPassword[MAX_USER_INFO_LEN + 1]; //加密密码

	int agentMaxMsg_len; //agent支持的最大消息长度

	unsigned char* msgAuthoritativeEngineID;  //引擎ID
	unsigned int msgAuthoritativeEngineID_len;  //引擎ID长度

	unsigned char* authPasswordPrivKey;  //ku 转换
	unsigned int authPasswordPrivKey_len;

	unsigned char* privPasswdPrivKey;   //kul转换
	unsigned int privPasswdPrivKey_len;
};

struct variable_bindings
{
	oid*           oid_buf;
	unsigned int   oid_buf_len; //字节
	unsigned char  val_tag;
	unsigned int   val_len;
	unsigned char* val_buf;
};

/*v1 get v2 , v3 接收数据,发送数据结构体 */
struct snmpx_t
{
	char ip[32];
	unsigned short remote_port;

	unsigned char  msgVersion; //0:v1，1:v2c，2:v2u/v2，3:v3

	unsigned char* community; //v1,v2
	unsigned int   community_len; 

	//msgGlobalData v3
	int           msgID; //消息ID，每次请求加1
	int           msgMaxSize; //最大消息长度
	unsigned char msgFlags; //三个标志位：reportPDU|privFlag|authFlag，当reportPDU=1时，在能导致reportPDU产生时，消息接收方必须回送reportPDU
	int           msgSecurityModel; //0表示任何模型，1表示采用SNMPv1安全模型，2表示采用SNMPv2c安全模型，3表示采用SNMPv3安全模型

	//authData v3
	unsigned char* msgAuthoritativeEngineID;
	unsigned int   msgAuthoritativeEngineID_len;
	int            msgAuthoritativeEngineBoots;
	int            msgAuthoritativeEngineTime;
	unsigned char* msgUserName; //用户名
	unsigned int   msgUserName_len;
	unsigned char* msgAuthenticationParameters; //鉴别码(HMAC)，只比较前96位
	unsigned int   msgAuthenticationParameters_len; //12字节
	unsigned char* msgPrivacyParameters; //加/解密参数，随机数，用于生成初始向量IV
	unsigned int   msgPrivacyParameters_len; //8字节

	//msgData->plaintext v3
	unsigned char* contextEngineID;
	unsigned int   contextEngineID_len;
	unsigned char* contextName;
	unsigned int   contextName_len;

	//msgData->pdu v1,v2,v3
	unsigned char tag; /* 请求类型 */
	int request_id; /* 请求ID，每次请求加1*/
	int error_status; /* 用于表示在处理请求时出现的状况, snmptable时该字段用于 non_repeaters  */
	int error_index; /* 当出现异常情况时，提供变量绑定列表（Variable bindings）中导致异常的变量的信息，snmptable时该字段用于 max_repetitions*/

	//msgData->pdu->items
	std::list<struct variable_bindings*> *variable_bindings_list;

	//v1 trap 特定使用的结构体
    oid*         enterprise_oid;
	int          enterprise_oid_len;
	unsigned int agent_addr; //agent address
	int          generic_trap; //trap type(0-6)，coldStart、warmStart、linkDown、linkUp、authenticationFailure、egpNeighborLoss、enterpriseSpecific
	int          specific_trap; //特定代码
	unsigned int time_stamp; //时间戳

	unsigned int errcode; /*2:客户端用户名错误，3:客户端消息hash错误，在agent响应请求消息时使用 */
};

union u_digital_16
{
	short          n;
	unsigned short un;
	unsigned char  buff[2];
};

union u_digital_32
{
	int           i;
	unsigned int  ui;
	float         f;
	unsigned char buff[4];
};

union u_digital_64
{
	long long          ll;
	unsigned long long ull;
	double             d;
	unsigned char buff[8];
};

//用户使用数据类型定义，因为有多个ASN类型对应一个C的数据类型，所以在这里简化
#define SNMPX_ASN_UNSUPPORT		     ((unsigned char)0x00) //还未支持的类型
#define	SNMPX_ASN_INTEGER		     ASN_INTEGER
#define	SNMPX_ASN_UNSIGNED		     ASN_GAUGE
#define SNMPX_ASN_NULL               ASN_NULL
#define	SNMPX_ASN_OCTET_STR		     ASN_OCTET_STR
#define SNMPX_ASN_IPADDRESS          ASN_IPADDRESS //注意，转换
#define	SNMPX_ASN_INTEGER64		     ASN_INTEGER64
#define SNMPX_ASN_UNSIGNED64         ASN_COUNTER64
#define SNMPX_ASN_FLOAT              ASN_FLOAT
#define SNMPX_ASN_DOUBLE             ASN_DOUBLE
#define SNMPX_ASN_NO_SUCHOBJECT      ASN_NO_SUCHOBJECT //该OID项不存在

//用户使用oid值信息，只能使用其中一个
struct SOidVal
{
	union
	{
		int                i;
		unsigned int       u;
		float              f;
		double             d;
		long long          ll;
		unsigned long long ull;
	} num; //数字值

	bool hex_str; //不可打印字符串转HEX串
	std::string str;  //字符串值

	SOidVal() : hex_str(false) { memset(&num, 0, sizeof(num)); }
	~SOidVal() { }
	void Clear()
	{
		memset(&num, 0, sizeof(num));
		hex_str = false;
		str.clear();
	}
}; 

//用户使用oid值对象信息
struct SSnmpxValue
{
	std::string   szOid;                   //字符串OID
	oid           OidBuf[MAX_OID_LEN + 1]; //数组OID，接收告警或get时有值，set时要自己设置，第一个位置放oid长度
	unsigned char cValType;                //数据类型	
	unsigned int  iValLen;                 //数据长度，字符串即字节数，整形为agent上报时的有效字节数，下发时可不做处理
	SOidVal       Val;                     //数据

	SSnmpxValue() : cValType(SNMPX_ASN_UNSUPPORT), iValLen(0)
	{
		memset(OidBuf, 0, MAX_OID_LEN + 1);
	}
	~SSnmpxValue() { }
	void Clear()
	{
		szOid.clear();
		memset(OidBuf, 0, MAX_OID_LEN + 1);
		cValType = SNMPX_ASN_UNSUPPORT;
		iValLen = 0;
		Val.Clear();
	}
};

//table结果类型
typedef std::map<oid, std::vector<std::pair<oid, SSnmpxValue>>> TTableResultType;
typedef std::map<oid, std::vector<std::pair<oid, SSnmpxValue>>>::iterator TTableResultType_iter;
typedef std::map<oid, std::vector<std::pair<oid, SSnmpxValue>>>::const_iterator TTableResultType_citer;

/*公共方法*/
void free_tlv_data(struct tlv_data *m_tlv_data);
void free_tlv_glist_data(std::list<struct tlv_data*> &mlist);
void free_variable_glist_data(std::list<struct variable_bindings*> *variableList);

void init_snmpx_t(struct snmpx_t *s);
void clear_snmpx_t(struct snmpx_t *s);
void free_snmpx_t(struct snmpx_t *s);

int find_str_lac(const unsigned char* src, const unsigned int src_len, const unsigned char* dst, const unsigned int dst_len);

int parse_ipaddress_string(const std::string &IP); //注意检测IP格式，里面不做检测
std::string get_ipaddress_string(int ipaddress);
bool parse_oid_string(const std::string &oidStr, oid *oid_buf, std::string &error);
std::string get_oid_string(oid* buf, int len);
std::string get_timeticks_string(unsigned int ticks);
std::string get_hex_string(unsigned char *data, unsigned int data_len, bool uppercase = true, bool add_space = true);
std::string get_hex_print_string(const unsigned char *data, size_t data_len, unsigned char blank_space_len = 4);
std::string get_current_time_string(bool add_millseconds = true);
std::string get_vb_item_print_string(const SSnmpxValue &vb, size_t max_oid_string_len = 0);
std::string get_vb_items_print_string(const std::list<SSnmpxValue> &vb_list, size_t max_oid_string_len = 29);

unsigned short convert_to_ns(unsigned short s);
unsigned int convert_to_nl(unsigned int l);
unsigned long long convert_to_nll(unsigned long long ll);

bool init_win32_socket_env(std::string &szError);
void free_win32_socket_env();
void close_socket_fd(int fd);

#endif