#include "../include/Ping.hpp"

#ifdef _WIN32
#include <WS2tcpip.h>
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#endif

#include <sstream>
#include <thread>
#include <regex>
#include <random>
#include <string.h>

CPing::CPing()
{
}

CPing::~CPing()
{
}

/*****************************************************************
功能：	检查IP格式是否正确
参数：	szIp 待检查的IP串
返回：	格式正确返回true
*****************************************************************/
bool CPing::CheckIp(const string& szIp)
{
	bool bResult = false;

	if (!szIp.empty())
	{
		std::regex pattern("(\\d{1,3})(\\.)(\\d{1,3})(\\.)(\\d{1,3})(\\.)(\\d{1,3})");

		bResult = std::regex_match(szIp, pattern);
	}

	return bResult;
}

/*****************************************************************
功能：	转网络字节序，小端
参数：	val 待转值
返回：	转后值
*****************************************************************/
unsigned short CPing::GetNetNumber(unsigned short val)
{
	return (((val & 0xFF) << 8) | ((val >> 8) & 0xFF));
}

/*****************************************************************
功能：	获取ICMP包的校验和,包括ICMP报文数据部分在内的整个ICMP数据报的校验和
参数：	buffer ICMP包
*		iSize ICMP包的长度，bit
返回：	校验和
*****************************************************************/
unsigned short CPing::CheckSum(unsigned short *buffer, int iSize)
{
	unsigned long cksum = 0;

	while (iSize > 1)
	{
		cksum += *buffer++;
		iSize -= sizeof(unsigned short);
	}

	/*若ICMP报为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
	if (iSize)
	{
		cksum += *(unsigned short*)buffer;
	}

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	return (unsigned short)(~cksum);
}

/*****************************************************************
功能：	组ICMP包
参数：	pack_id 包ID
		pack_no ICMP包序号
*		icmp ICMP包
返回：	包长度
*****************************************************************/
int CPing::PackageIcmp(unsigned short pack_id, unsigned short pack_no, SICMP_HEADER* icmp)
{
	//获取当前时间
#ifdef _WIN32
	SYSTEMTIME ctT;
	GetLocalTime(&ctT);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
#endif

	icmp->i_type = ICMP_ECHOREPLY_;
	icmp->i_code = 0;
	icmp->i_seq = GetNetNumber(pack_no); //转小端
	icmp->i_id = GetNetNumber(pack_id); //转小端
#ifdef _WIN32
	icmp->time_sec = ctT.wSecond;
	icmp->time_usec = ctT.wMilliseconds * 1000;
#else
	icmp->time_sec = tv.tv_sec;
	icmp->time_usec = tv.tv_usec;
#endif
	icmp->i_cksum = CheckSum((unsigned short *)icmp, 64); /*校验算法*/

	//除开头部8字节，其它为ICMP数据
	return 64;
}

/*****************************************************************
功能：	解ICMP包
参数：	iDataLen 接收数据长度
*		DataBuf 接收数据容器
*		pack_id 包ID
*		pack_no 序号
*		pResult 结果容器
*		szError 错误信息
返回：	成功返回true
*****************************************************************/
bool CPing::UnPackageIcmp(int iDataLen, char* DataBuf, unsigned short pack_id, unsigned short pack_no, PING_RESULT* pResult,
	string& szError)
{
	bool bResult = false;

	if (iDataLen < 28) /*小于IP报头加ICMP报头长度则不合理*/
	{
		szError = "IP报头加ICMP报头数据不完整，小于28字节！";
		return false;
	}

	SIP_HEADER *ip = (SIP_HEADER *)DataBuf;
	int iphdrlen = ip->h_len << 2;  /*求ip报头长度,即ip报头的长度标志乘4*/
	iDataLen -= iphdrlen; /*ICMP报头及ICMP数据报的总长度*/

	if (iDataLen < 8) /*小于ICMP报头长度则不合理*/
	{
		szError = "ICMP报头数据不完整，小于8字节！";
		return false;
	}

	SICMP_HEADER *icmp = (SICMP_HEADER *)(DataBuf + iphdrlen);  /*越过ip报头,指向ICMP报头*/

	//获取当前时间
#ifdef _WIN32
	SYSTEMTIME ctT;
	GetLocalTime(&ctT);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
#endif

	if (icmp->i_type == ICMP_ECHO_ && GetNetNumber(icmp->i_id) == pack_id) //响应回显
	{
		bResult = true;

		if (nullptr != pResult)
		{
			pResult->i_len = iDataLen;
			pResult->i_ttl = ip->ttl;
#ifdef _WIN32
			pResult->i_rtt = (ctT.wSecond * 1000000 + ctT.wMilliseconds) - (icmp->time_sec * 1000000 + icmp->time_usec);
#else
			pResult->i_rtt = (tv.tv_sec * 1000000 + tv.tv_usec) - (icmp->time_sec * 1000000 + icmp->time_usec);
#endif
			pResult->i_seq = pack_no;
		}
	}
	else if (icmp->i_type == 3) //主机不可达
		szError = "主机或网络不可达！";
	else if (icmp->i_type == 4) //源端抑制
		szError = "源端被关闭！";
	else if (icmp->i_type == 11) //超时
		szError = "TTL超时！";
	else
	{
		szError = "其它错误：type:" + std::to_string(icmp->i_type) + " code:" + std::to_string(icmp->i_code);
	}

	return bResult;
}

/*****************************************************************
功能：	发送包
参数：	fd socket句柄
*		addr 地址
*		pack_id 包ID
*		pack_no 序号
*		szError 错误信息
返回：	成功返回true
*****************************************************************/
bool CPing::SendPacket(int& fd, sockaddr_in& addr, unsigned short pack_id, unsigned short pack_no, string& szError)
{
	bool bResult = false;
	char buf[256] = { 0 };

	//创建ICMP包
	memset(buf, 0, 256);
	int packetsize = PackageIcmp(pack_id, pack_no, (SICMP_HEADER*)buf); /*设置ICMP报头*/

	//发送包
	if (0 < sendto(fd, buf, packetsize, 0, (struct sockaddr *)&addr, sizeof(addr)))
		bResult = true;
	else
		szError = "发送ICMP数据包失败！";

	return bResult;
}

/*****************************************************************
功能：	接收包
参数：	fd socket句柄
*		addr 绑定的地址
*		pack_id 包ID
*		pack_no 序号
*		pResult 结果容器
*		szError 错误信息
返回：	成功返回true
*****************************************************************/
bool CPing::RecvPacket(int& fd, sockaddr_in& addr, unsigned short pack_id, unsigned short pack_no, PING_RESULT* pResult, 
	string& szError)
{
	bool bResult = false;
	int iDataLen = 0;
	socklen_t iLen = 0;
	char buf[ICMP_MAX_BUF] = { 0 };

	iLen = sizeof(addr);
	memset(buf, 0, ICMP_MAX_BUF);

	//接收返回包
	iDataLen = recvfrom(fd, buf, ICMP_MAX_BUF - 1, 0, (struct sockaddr*)&addr, &iLen);

	//解包
	if (iDataLen > 0)
		bResult = UnPackageIcmp(iDataLen, buf, pack_id, pack_no, pResult, szError);
	else
		szError = "接收返回超时！";
	
	return bResult;
}

/*****************************************************************
功能：	ping
参数：	szAddr 要ping的地址，ip或是域名
*		szError 错误信息
*		isDomainName 是否是域名
*		seq 序号
*		iTimeoutMsec 超时时间，毫秒
*		pResult 结果对象，保存详细信息
返回：	能ping通返回true
*****************************************************************/
bool CPing::ping(const string& szAddr, string& szError, bool isDomainName, unsigned short seq, 
	int iTimeoutMsec, PING_RESULT* pResult)
{
	bool bResult = false;
	sockaddr_in socket_addr;

	if (szAddr.empty())
	{
		szError = "ping地址为空！";
		return false;
	}

	//获取地址
	memset(&socket_addr, 0, sizeof(struct sockaddr_in));
	socket_addr.sin_family = AF_INET;

	if (isDomainName)
	{
		const hostent *hp = gethostbyname(szAddr.c_str());
		if (nullptr != hp)
			memcpy(&socket_addr.sin_addr, hp->h_addr_list[0], hp->h_length);
		else
		{
			szError = "获取域名对应IP地址失败！";
			return false;
		}
	}
	else
	{
		if (CheckIp(szAddr))
			socket_addr.sin_addr.s_addr = inet_addr(szAddr.c_str());
		else
		{
			szError = "IP格式错误！";
			return false;
		}
	}

#ifdef _WIN32
	WSADATA wsa_data;
	if (0 != WSAStartup(0x0202, &wsa_data))
	{
		szError = "初始化socket环境失败！";
		return false;
	}
#endif

	//以管理员身份/root运行才会返回成功
	int fd = (int)socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (fd > 0)
	{
		unsigned short pack_id = 0;
		std::stringstream Format;
		string szTemp;

		//获取线程ID
		Format << std::this_thread::get_id();
		Format >> szTemp;

		//取后5位转成数字，线程ID一定会大于5位数，万一小于等于5，用随机数
		if (szTemp.length() > 5)
		{
			unsigned int iTemp = 0;
			szTemp = szTemp.substr(szTemp.length() - 5);
			sscanf(szTemp.c_str(), "%u", &iTemp);
			pack_id = iTemp % (ICMP_MAX_BUF - 1);
		}
		else
		{
			std::random_device rd;
			pack_id = rd() % (ICMP_MAX_BUF - 1);
		}

		//设置属性
		/*扩大套接字接收缓冲区到64K这样做主要为了减小接收缓冲区溢出的的可能性,若无意中ping一个广播地址或多播地址,将会引来大量应答*/
		int size = ICMP_MAX_BUF;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));

#ifdef _WIN32
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&iTimeoutMsec, sizeof(iTimeoutMsec));
		setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&iTimeoutMsec, sizeof(iTimeoutMsec));
#else
		struct timeval ti;
		ti.tv_sec = iTimeoutMsec / 1000;
		ti.tv_usec = (iTimeoutMsec % 1000) * 1000;

		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&ti, sizeof(struct timeval));
		setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&ti, sizeof(struct timeval));
#endif

		//发送请求
		if (SendPacket(fd, socket_addr, pack_id, seq, szError))
		{
			//接收应答
			if (RecvPacket(fd, socket_addr, pack_id, seq, pResult, szError))
			{
				bResult = true;
			}
		}

#ifdef _WIN32
		closesocket(fd);
#else
		close(fd);
#endif
	}
	else
	{
		szError = "获取socket句柄失败：" + string(strerror(errno));
	}

#ifdef _WIN32
	WSACleanup();
#endif

	return bResult;
}