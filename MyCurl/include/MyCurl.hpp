/******************************************************
功能：	自定义curl类
作者：	田俊
时间：	2020-01-02
修改：
******************************************************/
#ifndef __MY_CURL_HPP__
#define __MY_CURL_HPP__

#include <string>
#include <mutex>
#include <list>

class CMyCurl
{
public:
	CMyCurl(const std::string &szServerIP, unsigned int uiServerPort, const std::string &szUserName,
		const std::string &szPassword, int iConnTimeout = 3, bool bAuthByPwd = true, const std::string &szPubKeyFile = "", 
		const std::string &szPriKeyFile = "");
	~CMyCurl();

	//list文件信息
	struct SFileInfo
	{
		std::string szFileName;
		bool        bIsDir;
		size_t      ullSize; //字节
		std::string szTime; //如：Aug 14 08:41或Mar 22 2016
	};

	static bool CurlEnvInit(std::string &szError); //初始化curl环境，调用一次即可
	static void CurlEnvFree(); //清理curl环境

	bool FtpLogin(std::string &szError);
	void FtpLogout();
	bool FtpPWD(std::string &szCurrentDir, std::string &szError);
	bool FtpCWD(const std::string &szDirPath, std::string &szError);
	bool FtpMKD(const std::string &szDirPath, std::string &szError);
	bool FtpRMD(const std::string &szDirPath, std::string &szError);
	bool FtpDELE(const std::string &szFilePath, std::string &szError);
	bool FtpGetFileList(const std::string &szDirPath, std::list<SFileInfo> &FileList, std::string &szError);
	bool FtpDwonloadFile(const std::string &szRemoteFile, const std::string &szLocalFile, bool bUseSSH, std::string &szError,
		double *down_size = nullptr, double *down_speed = nullptr, double *down_time = nullptr);
	bool FtpUploadFile(const std::string &szLocalFile, const std::string &szRemoteFile, bool bUseSSH, std::string &szError,
		double *up_size = nullptr, double *up_speed = nullptr, double *up_time = nullptr);

	bool ScpDwonloadFile(const std::string &szRemoteFile, const std::string &szLocalFile, std::string &szError,
		double *down_size = nullptr, double *down_speed = nullptr, double *down_time = nullptr);
	bool ScpUploadFile(const std::string &szLocalFile, const std::string &szRemoteFile, std::string &szError,
		double *up_size = nullptr, double *up_speed = nullptr, double *up_time = nullptr);

	bool SshLogin(bool bBlocked, std::string &szError);
	void SshLogout();
	bool SshExec(const std::string &szCommand, std::string &szResult, std::string &szError);
	bool SshCreateChannel(std::string &szResult, std::string &szError);
	void SshCloseChannel();
	bool SshChannelShell(const std::string &szCommand, std::string &szResult, std::string &szError, int iTimeoutMill = 3000);

private:
	//ftp文件信息
	struct SFtpReadFile
	{
		std::string szFileName;
		FILE *fstream;
	};

	//curl debug数据
	struct SDebugData
	{
		CMyCurl *object;
		bool bPrintData;
	};

	bool FtpInternalHandle(const std::string &szCommand, std::string &szReplyData, int iType, SDebugData *debugData, 
		std::string &szError);
	bool SetCurlAuthenticationInfo(void *curl, bool bUseSSH, std::string &szError);
	bool SetCurlPublicInfo(void *curl, SDebugData *debugData, std::string &szError);
	void SetCurrentDir(char *buf, size_t data_len);
	static int curl_debug_print(void *handle, int type, char *data, size_t size, void *userptr);
	static size_t ftp_read(void *buf, size_t size, size_t nmemb, void *stream);
	static size_t ftp_req_reply(void *buf, size_t size, size_t nmemb, void *stream);
	static size_t ftp_write(void *buf, size_t size, size_t nmemb, void *stream);
	static bool CheckFileExists(const std::string &szFileName);
	static std::string GetErrorMsg(int errCode);
	static std::string get_hex(const unsigned char* data, size_t data_len, bool isLowercase = true);
	static void SplitFiles(const std::string &szListResult, std::list<SFileInfo> &FileList);
	static std::string GetSsh2SessionLastError(void *session);
	static int wait_socket(int socket_fd, void *session, long timeout_sec = 3, long timeout_usec = 0);
	static void SshCloseChannel(int fd, bool bBlocked, void *session, void *channel);
	static bool SshReadData(void *session, void *channel, std::string &szResult, std::string &szError,
		const std::string &szEnd = "]$ ", int timeout_msec = 3000);
	static bool SshWriteData(void *session, void *channel, const std::string &szData, std::string &szError,
		int timeout_msec = 3000);

private:
	const std::string  m_szServerIP;    //服务器IP
	const unsigned int m_uiServerPort;  //服务器端口
	const std::string  m_szUserName;    //用户名
	const std::string  m_szPassword;    //用户密码或私钥文件密码
	const int          m_iConnTimeout;  //连接超时，秒
	const bool         m_bAuthByPwd;    //是否使用密码认证，否则使用证书
	const std::string  m_szPubKeyFile;  //公钥文件名
	const std::string  m_szPriKeyFile;  //私钥文件名

	void               *m_pCurl;        //curl连接句柄
	std::string        m_szCurrentDir;  //当前目录，FTP有效

	int                m_iConnectFd;    //ssh使用
	bool               m_bBlocked;      //ssh使用
	void               *m_pSession;     //ssh使用
	void               *m_pChannel;     //ssh使用

	static bool        m_bCurlEnvInit;      //是否己初始化curl环境
	static std::mutex  *m_pCurlEnvInitLock; //初始化curl环境锁
};

#endif