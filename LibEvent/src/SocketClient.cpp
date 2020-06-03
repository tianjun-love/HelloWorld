#include "../include/SocketClient.hpp"

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

CSocketClient::CSocketClient(const std::string& szServerIP, int iServerPort, int iTimeOut) : 
CSocketBase(szServerIP, iServerPort, iTimeOut), m_lConnectId(0)
{
}

CSocketClient::~CSocketClient()
{
	Free();
}

bool CSocketClient::Init(std::string &szError)
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
			srand((unsigned int)time(NULL));
		}
	}

	return bResult;
}

void CSocketClient::Free()
{
	if (m_bStatus)
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

bool CSocketClient::Connect(std::string& szError)
{
	bool bResult = true;

	m_lConnectId = socket(AF_INET, SOCK_STREAM, 0);
	if (m_lConnectId <= 0)
	{
		bResult = false;
		szError = "Create socket fd failed !";
	}
	else
	{
		sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		if (m_szServerIP.empty())
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
		else
			inet_pton(AF_INET, m_szServerIP.c_str(), &sin.sin_addr);
		sin.sin_port = htons(m_iServerPort);

		//设置成非阻塞
		unsigned long ulBlock = 1; //设置是否阻塞，默认阻塞，0：阻塞，1：非阻塞
#ifdef _WIN32
		ioctlsocket(m_lConnectId, FIONBIO, &ulBlock);
#else
		ioctl(m_lConnectId, FIONBIO, &ulBlock);
#endif

		//连接会立即返回
		if (-1 == connect(m_lConnectId, (sockaddr*)&sin, sizeof(sin)))
		{
			//连接超时，3秒
			fd_set rfd;
			struct timeval tv;
			tv.tv_sec = 3;
			tv.tv_usec = 0;

			FD_ZERO(&rfd);
			FD_SET(m_lConnectId, &rfd);

			//检查连接状态
			int iRet = select(m_lConnectId + 1, 0, &rfd, 0, &tv);
			if (iRet <= 0)
			{
				bResult = false;
				DisConnect();

				if (0 == iRet)
					szError = std::string("Connect server failed:Connection timed out !");
				else
					szError = std::string("Connect server failed:") + GetErrorMsg(errno);
			}
			else
			{
				int iError = -1, iLength = sizeof(iError);

#ifdef _WIN32
				getsockopt(m_lConnectId, SOL_SOCKET, SO_ERROR, (char*)&iError, &iLength);
#else
				getsockopt(m_lConnectId, SOL_SOCKET, SO_ERROR, &iError, (socklen_t*)&iLength);
#endif

				if (iError != 0)
				{
					bResult = false;
					DisConnect();
					szError = std::string("Connect server failed:") + GetErrorMsg(iError);
				}
			}

			FD_CLR(m_lConnectId, &rfd);
			FD_ZERO(&rfd);
		}

		if (bResult)
		{
			//设置成阻塞
			ulBlock = 0; //设置是否阻塞，默认阻塞，0：阻塞，1：非阻塞
	#ifdef _WIN32
			ioctlsocket(m_lConnectId, FIONBIO, &ulBlock);
	#else
			ioctl(m_lConnectId, FIONBIO, &ulBlock);
	#endif

			//设置超时
#ifdef _WIN32
			int iSendTimeout = 3 * 1000; //毫秒
			setsockopt(m_lConnectId, SOL_SOCKET, SO_SNDTIMEO, (const char*)&iSendTimeout, sizeof(iSendTimeout));

			int iRecvTimeout = m_iTimeOut * 1000; //毫秒
			setsockopt(m_lConnectId, SOL_SOCKET, SO_RCVTIMEO, (const char*)&iRecvTimeout, sizeof(iRecvTimeout));

#else
			struct timeval sndTV; //linux使用
			sndTV.tv_sec = 3;
			sndTV.tv_usec = 0;

			setsockopt(m_lConnectId, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sndTV, sizeof(sndTV));

			struct timeval revTV; //linux使用
			revTV.tv_sec = m_iTimeOut;
			revTV.tv_usec = 0;

			setsockopt(m_lConnectId, SOL_SOCKET, SO_RCVTIMEO, (const char*)&revTV, sizeof(revTV));
#endif

			m_bStatus = true;
		}
	}

	return bResult;
}

bool CSocketClient::Reconnect(std::string& szError)
{
	DisConnect();

	return Connect(szError);
}

void CSocketClient::DisConnect()
{
	m_bStatus = false;

	if (m_lConnectId > 0)
	{
#ifdef _WIN32
		closesocket(m_lConnectId);
#else
		close(m_lConnectId);
#endif
		m_lConnectId = 0;
	}
}

/*********************************************************************
功能：	发送数据
参数：	data 要发送的数据，输入
*		data_len 发送的数据长度，输入
*		szError 错误信息，输出
返回：	-2：内部错误，-1：socket错误，>=0：发送的字节数
修改：
*********************************************************************/
int CSocketClient::SendMsg(const char* data, int data_len, std::string& szError)
{
	if (!m_bStatus)
	{
		szError = "Client isn't connected to server !";
		return -2;
	}

	if (nullptr == data || data_len <= 0)
	{
		szError = "Send msg data is empty !";
		return -2;
	}

	bool bExit = true;
	time_t tBegin = time(NULL);
	int iRet = 0;
	
	do
	{
		iRet = send(m_lConnectId, data, data_len, 0);
		if (iRet <= 0)
		{
			if (0 == iRet)
			{
				m_bStatus = false;
				bExit = true;
				iRet = -1;
				szError = "Server active disconnected !";
			}
			else
			{
				int iTemp = errno;

				switch (iTemp)
				{
				case 0:
					m_bStatus = false;
					bExit = true;
					szError = "Server disconnected !";
					break;
				case EINTR: //由于信号中断，没写成功任何数据，重启操作
				case EAGAIN: //缓冲区数据己写满
#ifdef _WIN32
				case EWOULDBLOCK: //windows的EAGAIN
#endif
				{
					//重试一次
					if ((time(NULL) - tBegin) > (time_t)m_iTimeOut)
					{
						bExit = true;
						szError = "Send data to server timeout !";
					}
					else
					{
						iRet = 0;
						bExit = false; //继续写
					}
				}
				break;
				default:
					bExit = true;
					szError = std::string("Send error:") + GetErrorMsg(iTemp);
					break;
				}
			}
		}
	} while (!bExit);

	return iRet;
}

/*********************************************************************
功能：	接收数据
参数：	data 要接收的数据缓存，输入
*		data_len 接收的数据缓存长度，输入
*		szError 错误信息，输出
返回：	-2：内部错误，-1：socket错误，>=0：接收的字节数
修改：
*********************************************************************/
int CSocketClient::RecvMsg(char* data, int buf_len, std::string& szError)
{
	if (!m_bStatus)
	{
		szError = "Client isn't connected to server !";
		return -2;
	}

	if (nullptr == data)
	{
		szError = "Recv data buffer can't be empty !";
		return -2;
	}

	bool bExit = true;
	time_t tBegin = time(NULL);
	int iRet = 0;

	do
	{
		iRet = recv(m_lConnectId, data, buf_len, 0);
		
		if (iRet <= 0)
		{
			if (0 == iRet)
			{
				m_bStatus = false;
				bExit = true;
				iRet = -1;
				szError = "Server active disconnected !";
			}
			else
			{
				//查看错误码
				int iTemp = errno;

				switch (iTemp)
				{
				case 0:
				{
					m_bStatus = false;
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
					if ((time(NULL) - tBegin) > (time_t)m_iTimeOut)
					{
						bExit = true;
						szError = "Recv server data timeout !";
					}
					else
					{
						iRet = 0;
						bExit = false; //继续读
					}
				}
					break;
				default:
					bExit = true;
					szError = std::string("Recv error:") + GetErrorMsg(iTemp);
					break;
				}
			}
		}
	} while (!bExit);

	return iRet;
}

/*********************************************************************
功能：	发送带'\0'为结束符的消息
参数：	szData 要发送的数据，输入
*		szError 错误信息，输出
返回：	成功返回true
修改：
*********************************************************************/
bool CSocketClient::SendMsgWithEOF_Zero(const std::string &szData, std::string& szError)
{
	if (!m_bStatus)
	{
		szError = "Client isn't connected to server !";
		return false;
	}

	if (szData.empty())
	{
		szError = "Send msg data is empty !";
		return false;
	}

	//多发送两个换行
	bool bExit = true;
	std::string szTempData = szData + "\n\n"; //给web判断结束使用
	const int iTotalLength = (int)szTempData.length();
	const char *pBuf = szTempData.c_str();
	int iRet = 0, iSendLength = 0;
	time_t tBegin = time(NULL);
	
	do
	{
		iRet = SendMsg(pBuf + iSendLength, iTotalLength - iSendLength, szError);
		if (iRet < 0)
			return false;
		else
		{
			iSendLength += iRet;
			if (iTotalLength != iSendLength)
			{
				if ((time(NULL) - tBegin) >= (time_t)m_iTimeOut)
				{
					//防止缓冲区满，一直等待
					szError = "Send timeout, already send " + std::to_string(iSendLength) + " Bytes !";
					return false;
				}
				else
					bExit = false;
			}
		}
	} while (!bExit);

	return true;
}

/*********************************************************************
功能：	接收带'\0'为结束符的消息
参数：	szData 接收的数据，输出
*		szError 错误信息，输出
返回：	成功返回true
修改：
*********************************************************************/
bool CSocketClient::RecvMsgWithEOF_Zero(std::string &szData, std::string& szError)
{
	if (!m_bStatus)
	{
		szError = "Client isn't connected to server !";
		return false;
	}

	szData.clear();
	bool bArrivedEnd = false;
	int iRet = 0, iRecvLength = 0;
	const int buf_len = 4097; //每次接收4K
	char buf[buf_len]{ '\0' };
	time_t tBegin = time(NULL);

	do
	{
		memset(buf, '\0', buf_len);
		iRet = RecvMsg(buf, buf_len - 1, szError);

		if (iRet >= 0)
		{
			iRecvLength += iRet;

			//接收到结束符，结束读取
			if (iRet > 0 && '\0' == buf[iRet - 1])
			{
				bArrivedEnd = true;

				//防止WEB加了换行后，解密不成功
				if (iRet >= 2 && '\n' == buf[iRet - 2])
					buf[iRet - 2] = '\0';

				if (iRet >= 3 && '\r' == buf[iRet - 3])
					buf[iRet - 3] = '\0';
			}
			else
			{
				if ((time(NULL) - tBegin) >= (time_t)m_iTimeOut)
				{
					//防止服务端不发送结束符，一直等待
					szError = "Recv timeout, already recv " + std::to_string(iRecvLength) + " Bytes !";
					return false;
				}
			}

			//追加数据，16进制的字符串，不含开头的0x
			if (iRet > 0)
				szData += buf;
		}
		else
			return false;

	} while (!bArrivedEnd);

	return true;
}