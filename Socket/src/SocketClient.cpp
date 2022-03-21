#include "../include/SocketClient.hpp"
#include <cstring>
#include <ctime>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

CSocketClient::CSocketClient(const std::string &szIPAddress, unsigned short nPort, int iTimeout) :
	CBaseAttributes(szIPAddress, nPort), m_iTimeout(iTimeout), m_iSocketFD(0)
{
}

CSocketClient::~CSocketClient()
{
}

/*****************************************************
功能：	初始化环境
参数：	szError 错误信息
返回：	成功返回true
*****************************************************/
bool CSocketClient::InitEnv(std::string &szError)
{
	bool bResult = true;

	if (!m_bInitWSA)
	{
#ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		{
			bResult = false;
			szError = "WSAStartup init failed !";
		}	
#endif

		if (bResult)
		{
			m_bInitWSA = true;
		}
	}

	return bResult;
}

/*****************************************************
功能：	清理环境资源
参数：	无
返回：	无
*****************************************************/
void CSocketClient::FreeEnv()
{
	if (m_bState)
	{
		DisConnect();
	}

#ifdef _WIN32
	if (m_bInitWSA)
	{
		WSACleanup();
		m_bInitWSA = false;
	}
#endif
}

/*****************************************************
功能：	连接服务器
参数：	szError 错误信息
返回：	成功返回true
*****************************************************/
bool CSocketClient::Connect(std::string& szError)
{
	bool bResult = true;

	//创建句柄
	m_iSocketFD = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
	if (INVALID_SOCKET == m_iSocketFD)
	{
		szError = "Create socket handle failed:" + GetErrorMsg(MY_SOCKET_ERRNO);
		return false;
	}
#else
	if (0 >= m_iSocketFD)
	{
		szError = "Create socket handle failed:" + GetErrorMsg(MY_SOCKET_ERRNO);
		return false;
	}
#endif

	//地址
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));

	sin.sin_family = AF_INET;
	if (m_szServerIP.empty())
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		inet_pton(AF_INET, m_szServerIP.c_str(), &sin.sin_addr);
	sin.sin_port = htons(m_nServerPort);

	//设置成非阻塞
	SetSocketBolckState(m_iSocketFD, false);

	//连接会立即返回
	if (-1 == connect(m_iSocketFD, (sockaddr*)&sin, sizeof(sin)))
	{
		//连接超时，3秒
		int iRet = 0;
		fd_set rfd;
		struct timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;

		FD_ZERO(&rfd);
		FD_SET(m_iSocketFD, &rfd);

		//检查连接状态
#ifdef _WIN32
		iRet = select(0, NULL, &rfd, NULL, &tv);
#else
		iRet = select(m_iSocketFD + 1, NULL, &rfd, NULL, &tv);
#endif
		if (iRet <= 0)
		{
			bResult = false;
			iRet = MY_SOCKET_ERRNO;
			DisConnect();

			if (0 == iRet)
				szError = std::string("Connect server failed:Connection timed out !");
			else
				szError = std::string("Connect server failed(errno:" + std::to_string(iRet) + "):") + GetErrorMsg(iRet);
		}
		else
		{
			int iLength = sizeof(iRet);

#ifdef _WIN32
			getsockopt(m_iSocketFD, SOL_SOCKET, SO_ERROR, (char*)&iRet, &iLength);
#else
			getsockopt(m_iSocketFD, SOL_SOCKET, SO_ERROR, &iRet, (socklen_t*)&iLength);
#endif

			if (iRet != 0)
			{
				bResult = false;
				DisConnect();
				szError = std::string("Connect server failed(errno:" + std::to_string(iRet) + "):") + GetErrorMsg(iRet);
			}
		}

		FD_CLR(m_iSocketFD, &rfd);
		FD_ZERO(&rfd);
	}

	if (bResult)
	{
		//设置成阻塞
		SetSocketBolckState(m_iSocketFD, true);

		//设置超时
#ifdef _WIN32
		int iSendTimeout = 3 * 1000; //毫秒
		setsockopt(m_iSocketFD, SOL_SOCKET, SO_SNDTIMEO, (const char*)&iSendTimeout, sizeof(iSendTimeout));

		int iRecvTimeout = m_iTimeout * 1000; //毫秒
		setsockopt(m_iSocketFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&iRecvTimeout, sizeof(iRecvTimeout));

#else
		struct timeval sndTV; //linux使用
		sndTV.tv_sec = 3;
		sndTV.tv_usec = 0;

		setsockopt(m_iSocketFD, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sndTV, sizeof(sndTV));

		struct timeval revTV; //linux使用
		revTV.tv_sec = m_iTimeout;
		revTV.tv_usec = 0;

		setsockopt(m_iSocketFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&revTV, sizeof(revTV));
#endif

		//连接成功
		m_bState = true;
	}
	else
	{
		CloseSocket(m_iSocketFD);
		m_iSocketFD = 0;
	}

	return bResult;
}

/*****************************************************
功能：	重新连接服务器
参数：	szError 错误信息
返回：	成功返回true
*****************************************************/
bool CSocketClient::Reconnect(std::string& szError)
{
	DisConnect();

	return Connect(szError);
}

/*****************************************************
功能：	断开连接
参数：	无
返回：	无
*****************************************************/
void CSocketClient::DisConnect()
{
	m_bState = false;

	if (m_iSocketFD > 0)
	{
		CloseSocket(m_iSocketFD);
		m_iSocketFD = 0;
	}
}

/*****************************************************
功能：	发送数据
参数：	szMsg 要发送的数据
*		szError 错误信息
返回：	成功返回true
修改：
*****************************************************/
bool CSocketClient::SendMsg(const std::string &szMsg, std::string& szError)
{
	if (!m_bState)
	{
		szError = "Client isn't connected to server !";
		return false;
	}

	if (szMsg.empty())
	{
		szError = "Send msg data is empty !";
		return false;
	}

	bool bExit = true, bResult = false;
	time_t tBegin = time(NULL);
	int iRet = 0;

	do
	{
		iRet = send(m_iSocketFD, szMsg.c_str(), (int)szMsg.length(), 0);
		if (iRet <= 0)
		{
			if (0 == iRet)
			{
				m_bState = false;
				bExit = true;
				szError = "Server active disconnected !";
			}
			else
			{
				iRet = MY_SOCKET_ERRNO;

				switch (iRet)
				{
				case 0:
				case ECONNRESET:
#ifdef _WIN32
				case WSAECONNABORTED:
				case WSAECONNRESET:
#endif
				{
					m_bState = false;
					bExit = true;
					szError = "Server disconnected !";
				}
					break;
				case EINTR: //由于信号中断，没写成功任何数据，重启操作
				case EAGAIN: //缓冲区数据己写满
#ifdef _WIN32
				case EWOULDBLOCK: //windows的EAGAIN
#endif
				{
					//重试一次
					if ((time(NULL) - tBegin) > (time_t)m_iTimeout)
					{
						bExit = true;
						szError = "Send data to server timeout !";
					}
					else
						bExit = false; //继续写
				}
					break;
				default:
					bExit = true;
					szError = "Send error:" + GetErrorMsg(iRet);
					break;
				}
			}
		}
		else
			bResult = true;
	} while (!bExit);

	return bResult;
}

/*****************************************************
功能：	接收数据
参数：	szMsg 接收的数据缓存
*		szError 错误信息
返回：	成功返回true
修改：
*****************************************************/
bool CSocketClient::RecvMsg(std::string &szMsg, std::string& szError)
{
	if (!m_bState)
	{
		szError = "Client isn't connected to server !";
		return false;
	}

	bool bExit = true, bResult = false;
	time_t tBegin = time(NULL);
	const int buff_len = 10240;
	char buff[buff_len] = { '\0' };
	int iRet = 0;

	szMsg.clear();

	do
	{
		iRet = recv(m_iSocketFD, buff, buff_len, 0);

		if (iRet <= 0)
		{
			if (0 == iRet)
			{
				m_bState = false;
				bExit = true;
				szError = "Server active disconnected !";
			}
			else
			{
				//查看错误码
				iRet = MY_SOCKET_ERRNO;

				switch (iRet)
				{
				case 0:
				case ECONNRESET:
#ifdef _WIN32
				case WSAECONNABORTED:
				case WSAECONNRESET:
#endif
				{
					m_bState = false;
					bExit = true;
					szError = "Server disconnected or recv timeout !";
				}
					break;
				case EINTR: //由于信号中断返回，没有任何数据可用，重启调用
				case EAGAIN: //缓冲区没有数据可读
#ifdef _WIN32
				case EWOULDBLOCK: //windows的EAGAIN
#endif
				{
					//重试一次
					if ((time(NULL) - tBegin) > (time_t)m_iTimeout)
					{
						bExit = true;
						szError = "Recv server data timeout, return code:" + std::to_string(iRet);
					}
					else
						bExit = false; //继续读
				}
					break;
				default:
					bExit = true;
					szError = std::string("Recv error:") + GetErrorMsg(iRet);
					break;
				}
			}
		}
		else
		{
			szMsg += std::string(buff, iRet);

			//还未接收完成
			if (iRet == buff_len)
			{
				bExit = false;
				memset(buff, 0, buff_len);
			}
			else
				bResult = true;
		}
	} while (!bExit);

	return bResult;
}