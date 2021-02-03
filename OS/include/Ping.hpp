/********************************************************************
名称:	ping类
功能:	ping远程主机
作者:	田俊
时间:	2014-11-26
修改:
*********************************************************************/
#ifndef __PING_HPP__
#define __PING_HPP__

#include <vector>
#include <string>

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

using std::vector;
using std::string;

//IP报头格式数据结构
struct SIP_HEADER
{
	unsigned int   h_len : 4;        //4位首部长度，以４字节为一个单位来记录IP报头的长度,小端
	unsigned int   version : 4;      //4位IP版本号,小端
	unsigned char  tos;              //8位服务类型TOS 
	unsigned short total_len = 0;    //16位IP包总长度（字节）
	unsigned short ident = 0;        //16位标识, 用于辅助IP包的拆装
	unsigned short frag_and_flags;   //3位标志位+13位偏移位, 也是用于IP包的拆装
	unsigned char  ttl = 0;          //8位IP包生存时间 TTL
	unsigned char  proto = 0;        //8位协议 (TCP, UDP 或其他)
	unsigned short checksum = 0;     //16位IP首部校验和,最初置零,等所有包头都填写正确后,计算并替换
	unsigned int   sourceIP;         //32位源IP地址
	unsigned int   destIP;           //32位目的IP地址
};

//ICMP报文格式，ICMP报头为８字节,数据报长度最大为64K字节
struct SICMP_HEADER
{
	//报头
	unsigned char  i_type = 0;    //8位类型
	unsigned char  i_code = 0;    //8位编码
	unsigned short i_cksum = 0;   //16位校验和, 从TYPE开始,直到最后一位用户数据,如果为字节数为奇数则补充一位
	unsigned short i_id = 0;      //识别号（一般用进程号作为识别号）, 用于匹配ECHO和ECHO REPLY包
	unsigned short i_seq = 0;     //报文序列号, 用于标记ECHO报文顺序

	//数据
	unsigned long  time_sec = 0;  //时间戳，秒
	unsigned long  time_usec = 0; //时间戳，微秒
};

//ICMP数据回应报文
struct PING_RESULT 
{
	unsigned short i_seq = 0;      //序号
	unsigned short i_len = 0;      //长度
	unsigned char  i_ttl = 0;      //IP包生存时间 TTL
	unsigned int   i_rtt = 0;      //发送到收到回复时间差，微秒
};

class CPing
{
public:
	CPing();
	~CPing();

	static bool ping(const string& szAddr, string& szError, bool isDomainName = false, unsigned short seq = 1, 
		int iTimeoutMsec = 1000, PING_RESULT* pResult = nullptr);

private:

#define ICMP_ECHO_       0         //请求回送
#define ICMP_ECHOREPLY_  8         //请求回应
#define ICMP_MAX_BUF     65536     //ICMP返回包最大64K

	static bool CheckIp(const string& szIp); //检查IP是否合法
	static unsigned short GetNetNumber(unsigned short val); //转网络字节序，小端
	static unsigned short CheckSum(unsigned short *buffer, int iSize); //计算校验和
	static int PackageIcmp(unsigned short pack_id, unsigned short pack_no, SICMP_HEADER* icmp); //创建ICMP包
	static bool UnPackageIcmp(int iDataLen, char* DataBuf, unsigned short pack_id, unsigned short pack_no, PING_RESULT* pResult, 
		string& szError); //解ICMP包
	static bool SendPacket(int& fd, sockaddr_in& addr, unsigned short pack_id, unsigned short pack_no, string& szError); //发送包
	static bool RecvPacket(int& fd, sockaddr_in& addr, unsigned short pack_id, unsigned short pack_no, PING_RESULT* pResult, 
		string& szError); //接收收包


};

#endif