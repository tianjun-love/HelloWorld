#include "../include/MyCurl.hpp"
#include <thread>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>

#ifdef _WIN32
#include <io.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "curl.h"
#include "libssh2_publickey.h"
#include "libssh2_sftp.h"

#define WRITEBUFFERSIZE (32767)

bool CMyCurl::m_bCurlEnvInit = false;
std::mutex* CMyCurl::m_pCurlEnvInitLock = new std::mutex();

CMyCurl::CMyCurl(const std::string &szServerIP, unsigned int uiServerPort, const std::string &szUserName,
	const std::string &szPassword, int iConnTimeout, bool bAuthByPwd, const std::string &szPubKeyFile,
	const std::string &szPriKeyFile) : m_szServerIP(szServerIP), m_uiServerPort(uiServerPort), m_szUserName(szUserName), 
	m_szPassword(szPassword), m_iConnTimeout(iConnTimeout), m_bAuthByPwd(bAuthByPwd), m_szPubKeyFile(szPubKeyFile), 
	m_szPriKeyFile(szPriKeyFile), m_pCurl(nullptr), m_iConnectFd(-1), m_bBlocked(true), m_pSession(nullptr),
	m_pChannel(nullptr)
{
}

CMyCurl::~CMyCurl()
{
	FtpLogout();
}

/*********************************************************
功能：	初始化curl环境，进程调用一次即可
参数：	szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::CurlEnvInit(std::string &szError)
{
	bool bResult = true;

	m_pCurlEnvInitLock->lock();

	if (!m_bCurlEnvInit)
	{
		//不是线程安全的
		CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);

		if (CURLE_OK == res)
			m_bCurlEnvInit = true;
		else
		{
			bResult = false;
			szError = std::string("curl_global_init error:") + curl_easy_strerror(res);
		}
	}

#ifdef _WIN32
	WSADATA wsadata;

	int err = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (err != 0) 
	{
		szError = "WSAStartup error:" + GetErrorMsg(err);
		bResult = false;
	}
#endif

	m_pCurlEnvInitLock->unlock();

	return bResult;
}

/*********************************************************
功能：	清理curl环境
参数：	无
返回：	无
修改：
*********************************************************/
void CMyCurl::CurlEnvFree()
{
	m_pCurlEnvInitLock->lock();

	if (m_bCurlEnvInit)
	{
		curl_global_cleanup();
		m_bCurlEnvInit = false;
	}

#ifdef _WIN32
	WSACleanup();
#endif

	m_pCurlEnvInitLock->unlock();
}

/*********************************************************
功能：	FTP登陆
参数：	szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpLogin(std::string &szError)
{
	std::string szTemp;
	SDebugData debugData = { this, true };

	if (nullptr != m_pCurl)
	{
		szError = "Is already logined !";
		return false;
	}

	m_pCurl = curl_easy_init();

	if (nullptr == m_pCurl)
	{
		szError = "curl_easy_init return nullptr !";
		return false;
	}

	return FtpInternalHandle("", szTemp, 0, &debugData, szError);
}

/*********************************************************
功能：	FTP断开连接
参数：	无
返回：	无
修改：
*********************************************************/
void CMyCurl::FtpLogout()
{
	if (nullptr != m_pCurl)
	{
		curl_easy_cleanup(m_pCurl);
		m_pCurl = nullptr;

		m_szCurrentDir.clear();
	}
}

/*********************************************************
功能：	显示当前目录，要开启日志输出，不然获取不到
参数：	szCurrentDir 目录名，输出
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpPWD(std::string &szCurrentDir, std::string &szError)
{
	SDebugData debugData = { this, true };

	return FtpInternalHandle("PWD", szCurrentDir, 1, &debugData, szError);
}

/*********************************************************
功能：	进入目录
参数：	szDirPath 目录名，可以是相对路径
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpCWD(const std::string &szDirPath, std::string &szError)
{
	if (szDirPath.empty())
	{
		szError = "Path name is empty !";
		return false;
	}

	std::string szTemp;
	SDebugData debugData = { this, true };

	return FtpInternalHandle("CWD " + szDirPath, szTemp, 2, &debugData, szError);
}

/*********************************************************
功能：	创建目录
参数：	szDirPath 目录名，可以是相对路径
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpMKD(const std::string &szDirPath, std::string &szError)
{
	if (szDirPath.empty())
	{
		szError = "Path name is empty !";
		return false;
	}

	std::string szTemp;
	SDebugData debugData = { this, true };

	return FtpInternalHandle("MKD " + szDirPath, szTemp, 3, &debugData, szError);
}

/*********************************************************
功能：	删除目录
参数：	szDirPath 目录名，可以是相对路径
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpRMD(const std::string &szDirPath, std::string &szError)
{
	if (szDirPath.empty())
	{
		szError = "Path name is empty !";
		return false;
	}

	std::string szTemp;
	SDebugData debugData = { this, true };

	return FtpInternalHandle("RMD " + szDirPath, szTemp, 4, &debugData, szError);
}

/*********************************************************
功能：	删除文件
参数：	szFilePath 文件名，可以是相对路径
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpDELE(const std::string &szFilePath, std::string &szError)
{
	if (szFilePath.empty())
	{
		szError = "File name is empty !";
		return false;
	}

	std::string szTemp;
	SDebugData debugData = { this, true };

	return FtpInternalHandle("DELE " + szFilePath, szTemp, 5, &debugData, szError);
}

/*********************************************************
功能：	获取目录下的所有文件信息
参数：	szDirPath 路径，为空则默认当前目录
*		FileList 文件信息
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpGetFileList(const std::string &szDirPath, std::list<SFileInfo> &FileList, std::string &szError)
{
	std::string szCommand("LIST"), szFileList;
	SDebugData debugData = { this, true };

	if (!szDirPath.empty())
		szCommand += " " + szDirPath;

	if (!FtpInternalHandle(szCommand, szFileList, 6, &debugData, szError))
		return false;

	SplitFiles(szFileList, FileList);

	return true;
}

/*********************************************************
功能：	FTP下载文件，如果是FTP则默认21端口，SFTP默认22端口
参数：	szRemoteFile 要下载的远程文件名称，如果是ftp则可以使用相对路径，sftp要全路径
*		szLocalFile 保存在本地的文件名
*		bUseSSH 是否使用ssh，当对象初始化为使用证书登陆时，此参数无效，即必须是ssh
*		szError 错误信息
*		down_size 下载的总大小，字节
*		down_speed 下载的速度，字节/秒
*		down_time 下载的总时间，秒
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpDwonloadFile(const std::string &szRemoteFile, const std::string &szLocalFile, bool bUseSSH, std::string &szError,
	double *down_size, double *down_speed, double *down_time)
{
	if (!m_bCurlEnvInit)
	{
		szError = "Curl global env is not init !";
		return false;
	}

	if (szRemoteFile.empty())
	{
		szError = "Remote file name can't be empty !";
		return false;
	}

	if (szLocalFile.empty())
	{
		szError = "Local file name can't be empty !";
		return false;
	}

	//不要在URL中指定用户名和密码，如果有特殊字符的话URL识别不了。
	std::string szUrl;
	CURL *curl = nullptr;
	CURLcode res = CURLE_OK;
	SFtpReadFile st = { szLocalFile, nullptr };
	SDebugData debugData = { this, false };
	bool bResult = true;
	
	//使用证书登陆时，必须是SSH
	if (!m_bAuthByPwd)
		bUseSSH = true;

	if (bUseSSH)
		szUrl = "sftp://" + m_szServerIP + ":" + std::to_string(m_uiServerPort);
	else
		szUrl = "ftp://" + m_szServerIP + ":" + std::to_string(m_uiServerPort);

	if ('/' == szRemoteFile[szRemoteFile.length() - 1])
		szUrl += szRemoteFile;
	else
		szUrl += "/" + szRemoteFile;

	//初始化curl句柄
	if (!bUseSSH && nullptr != m_pCurl)
	{
		curl_easy_reset(m_pCurl);
		curl = m_pCurl;
	}
	else
	{
		curl = curl_easy_init();
		if (nullptr == curl)
		{
			szError = "curl_easy_init return nullptr !";
			return false;
		}
	}

	//设置参数
	if (SetCurlAuthenticationInfo(curl, bUseSSH, szError) && SetCurlPublicInfo(curl, &debugData, szError))
	{
		curl_easy_setopt(curl, CURLOPT_URL, szUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ftp_write);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &st);

		//请求处理
		res = curl_easy_perform(curl);
		if (CURLE_OK == res)
		{
			/* check for bytes downloaded */
			if (nullptr != down_size)
				curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, down_size);

			/* check for average download speed */
			if (nullptr != down_speed)
				curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, down_speed);

			/* check for total download time */
			if (nullptr != down_time)
				curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, down_time);
		}
		else
		{
			bResult = false;
			szError = std::string("curl_easy_perform error:") + curl_easy_strerror(res);
		}
	}
	else
		bResult = false;

	if (nullptr != st.fstream)
	{
		fclose(st.fstream);
		st.fstream = nullptr;
	}

	if (bUseSSH || nullptr == m_pCurl)
	{
		curl_easy_cleanup(curl);
		curl = nullptr;
	}

	return bResult;
}

/*********************************************************
功能：	FTP上传文件，如果是FTP则默认21端口，SFTP默认22端口
参数：	szLocalFile 要上传的本地的文件名
*		szRemoteFile 要上传的远程文件名称，如果是ftp则可以使用相对路径，sftp要全路径
*		bUseSSH 是否使用ssh，当对象初始化为使用证书登陆时，此参数无效，即必须是ssh
*		szError 错误信息
*		up_size 上传的总大小，字节
*		up_speed 上传的速度，字节/秒
*		up_time 上传的总时间，秒
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpUploadFile(const std::string &szLocalFile, const std::string &szRemoteFile, bool bUseSSH, std::string &szError,
	double *up_size, double *up_speed, double *up_time)
{
	if (!m_bCurlEnvInit)
	{
		szError = "Curl global env is not init !";
		return false;
	}

	if (szLocalFile.empty())
	{
		szError = "Local file name can't be empty !";
		return false;
	}
	else
	{
		if (!CheckFileExists(szLocalFile))
		{
			szError = "Local file:" + szLocalFile + " is not exists !";
			return false;
		}
	}

	if (szRemoteFile.empty())
	{
		szError = "Remote file name can't be empty !";
		return false;
	}

	//不要在URL中指定用户名和密码，如果有特殊字符的话URL识别不了。
	std::string szUrl;
	CURL *curl = nullptr;
	CURLcode res = CURLE_OK;
	FILE *fstream = nullptr;
	long fileSize = 0;
	SDebugData debugData = { this, false };
	bool bResult = true;

	//使用证书登陆时，必须是SSH
	if (!m_bAuthByPwd)
		bUseSSH = true;

	if (bUseSSH)
		szUrl = "sftp://" + m_szServerIP + ":" + std::to_string(m_uiServerPort);
	else
		szUrl = "ftp://" + m_szServerIP + ":" + std::to_string(m_uiServerPort);

	if ('/' == szRemoteFile[szRemoteFile.length() - 1])
		szUrl += szRemoteFile;
	else
		szUrl += "/" + szRemoteFile;

	//初始化curl句柄
	if (!bUseSSH && nullptr != m_pCurl)
	{
		curl_easy_reset(m_pCurl);
		curl = m_pCurl;
	}
	else
	{
		curl = curl_easy_init();
		if (nullptr == curl)
		{
			szError = "curl_easy_init return nullptr !";
			return false;
		}
	}

	//设置参数
	if (SetCurlAuthenticationInfo(curl, bUseSSH, szError) && SetCurlPublicInfo(curl, &debugData, szError))
	{
		fstream = fopen(szLocalFile.c_str(), "rb");
		
		if (nullptr != fstream)
		{
			fseek(fstream, 0, SEEK_END);
			fileSize = ftell(fstream);
			fseek(fstream, 0, SEEK_SET);

			curl_easy_setopt(curl, CURLOPT_URL, szUrl.c_str());
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, ftp_read);
			curl_easy_setopt(curl, CURLOPT_READDATA, fstream);
			curl_easy_setopt(curl, CURLOPT_INFILESIZE, fileSize);

			//请求处理
			res = curl_easy_perform(curl);
			if (CURLE_OK == res)
			{
				/* check for bytes uploaded */
				if (nullptr != up_size)
					curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, up_size);

				/* check for average upload speed */
				if (nullptr != up_speed)
					curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, up_speed);

				/* check for total upload time */
				if (nullptr != up_time)
					curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, up_time);
			}
			else
			{
				bResult = false;
				szError = std::string("curl_easy_perform error:") + curl_easy_strerror(res);
			}

			fclose(fstream);
			fstream = nullptr;
		}
		else
		{
			bResult = false;
			szError = "Open file:" + szLocalFile + " error:" + GetErrorMsg(errno);
		}
	}
	else
		bResult = false;

	if (bUseSSH || nullptr == m_pCurl)
	{
		curl_easy_cleanup(curl);
		curl = nullptr;
	}

	return bResult;
}

/*********************************************************
功能：	SCP下载文件，默认22端口
参数：	szRemoteFile 要下载的远程文件名称，如果是ftp则可以使用相对路径，sftp要全路径
*		szLocalFile 保存在本地的文件名
*		szError 错误信息
*		down_size 下载的总大小，字节
*		down_speed 下载的速度，字节/秒
*		down_time 下载的总时间，秒
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::ScpDwonloadFile(const std::string &szRemoteFile, const std::string &szLocalFile, std::string &szError,
	double *down_size, double *down_speed, double *down_time)
{
	if (!m_bCurlEnvInit)
	{
		szError = "Curl global env is not init !";
		return false;
	}

	if (szRemoteFile.empty())
	{
		szError = "Remote file name can't be empty !";
		return false;
	}

	if (szLocalFile.empty())
	{
		szError = "Local file name can't be empty !";
		return false;
	}

	//不要在URL中指定用户名和密码，如果有特殊字符的话URL识别不了。
	std::string szUrl = "scp://" + m_szServerIP + ":" + std::to_string(m_uiServerPort);
	CURL *curl = nullptr;
	CURLcode res = CURLE_OK;
	SFtpReadFile st = { szLocalFile, nullptr };
	SDebugData debugData = { this, false };
	bool bResult = true;

	if ('/' == szRemoteFile[szRemoteFile.length() - 1])
		szUrl += szRemoteFile;
	else
		szUrl += "/" + szRemoteFile;

	//初始化curl句柄
	curl = curl_easy_init();
	if (nullptr == curl)
	{
		szError = "curl_easy_init return nullptr !";
		return false;
	}

	//设置参数
	if (SetCurlAuthenticationInfo(curl, true, szError) && SetCurlPublicInfo(curl, &debugData, szError))
	{
		curl_easy_setopt(curl, CURLOPT_URL, szUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ftp_write);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &st);

		//请求处理
		res = curl_easy_perform(curl);
		if (CURLE_OK == res)
		{
			/* check for bytes downloaded */
			if (nullptr != down_size)
				curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, down_size);

			/* check for average download speed */
			if (nullptr != down_speed)
				curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, down_speed);

			/* check for total download time */
			if (nullptr != down_time)
				curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, down_time);
		}
		else
		{
			bResult = false;
			szError = std::string("curl_easy_perform error:") + curl_easy_strerror(res);
		}
	}
	else
		bResult = false;

	if (nullptr != st.fstream)
	{
		fclose(st.fstream);
		st.fstream = nullptr;
	}

	curl_easy_cleanup(curl);
	curl = nullptr;

	return bResult;
}

/*********************************************************
功能：	SCP上传文件，默认22端口
参数：	szLocalFile 要上传的本地的文件名
*		szRemoteFile 要上传的远程文件名称，如果是ftp则可以使用相对路径，sftp要全路径
*		szError 错误信息
*		up_size 上传的总大小，字节
*		up_speed 上传的速度，字节/秒
*		up_time 上传的总时间，秒
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::ScpUploadFile(const std::string &szLocalFile, const std::string &szRemoteFile, std::string &szError,
	double *up_size, double *up_speed, double *up_time)
{
	if (!m_bCurlEnvInit)
	{
		szError = "Curl global env is not init !";
		return false;
	}

	if (szLocalFile.empty())
	{
		szError = "Local file name can't be empty !";
		return false;
	}
	else
	{
		if (!CheckFileExists(szLocalFile))
		{
			szError = "Local file:" + szLocalFile + " is not exists !";
			return false;
		}
	}

	if (szRemoteFile.empty())
	{
		szError = "Remote file name can't be empty !";
		return false;
	}

	//不要在URL中指定用户名和密码，如果有特殊字符的话URL识别不了。
	std::string szUrl = "scp://" + m_szServerIP + ":" + std::to_string(m_uiServerPort);
	CURL *curl = nullptr;
	CURLcode res = CURLE_OK;
	FILE *fstream = nullptr;
	long fileSize = 0;
	SDebugData debugData = { this, false };
	bool bResult = true;

	if ('/' == szRemoteFile[szRemoteFile.length() - 1])
		szUrl += szRemoteFile;
	else
		szUrl += "/" + szRemoteFile;

	//初始化curl句柄
	curl = curl_easy_init();
	if (nullptr == curl)
	{
		szError = "curl_easy_init return nullptr !";
		return false;
	}

	//设置参数
	if (SetCurlAuthenticationInfo(curl, true, szError) && SetCurlPublicInfo(curl, &debugData, szError))
	{
		fstream = fopen(szLocalFile.c_str(), "rb");

		if (nullptr != fstream)
		{
			fseek(fstream, 0, SEEK_END);
			fileSize = ftell(fstream);
			fseek(fstream, 0, SEEK_SET);

			curl_easy_setopt(curl, CURLOPT_URL, szUrl.c_str());
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, ftp_read);
			curl_easy_setopt(curl, CURLOPT_READDATA, fstream);
			curl_easy_setopt(curl, CURLOPT_INFILESIZE, fileSize);

			//请求处理
			res = curl_easy_perform(curl);
			if (CURLE_OK == res)
			{
				/* check for bytes uploaded */
				if (nullptr != up_size)
					curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, up_size);

				/* check for average upload speed */
				if (nullptr != up_speed)
					curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, up_speed);

				/* check for total upload time */
				if (nullptr != up_time)
					curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, up_time);
			}
			else
			{
				bResult = false;
				szError = std::string("curl_easy_perform error:") + curl_easy_strerror(res);
			}

			fclose(fstream);
			fstream = nullptr;
		}
		else
		{
			bResult = false;
			szError = "Open file:" + szLocalFile + " error:" + GetErrorMsg(errno);
		}
	}
	else
		bResult = false;

	curl_easy_cleanup(curl);
	curl = nullptr;

	return bResult;
}

/*********************************************************
功能：	FTP内部处理
参数：	szCommand 要执行的命令，目前sftp测试不成功，CWD，MKD，RMD，PWD，DELE不生效
*		szReplyData 响应消息
*		iType 类型，0：登陆，1：PWD，2：CWD，3：MKD，4：RMD，5：DELE，6：LIST
*		debugData debug回调数据
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::FtpInternalHandle(const std::string &szCommand, std::string &szReplyData, int iType, SDebugData *debugData, 
	std::string &szError)
{
	if (!m_bCurlEnvInit)
	{
		szError = "Curl global env is not init !";
		return false;
	}

	if (nullptr == m_pCurl)
	{
		szError = "Must call login first !";
		return false;
	}

	std::string szUrl = "ftp://" + m_szServerIP + ":" + std::to_string(m_uiServerPort) + "/";

	//登陆时不用处理
	if (0 != iType)
	{
		//重置属性
		curl_easy_reset(m_pCurl);
	}

	//设置属性
	if (SetCurlAuthenticationInfo(m_pCurl, false, szError) && SetCurlPublicInfo(m_pCurl, debugData, szError))
	{
		curl_easy_setopt(m_pCurl, CURLOPT_URL, szUrl.c_str());
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, ftp_req_reply);
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &szReplyData);

		//设置命令
		if (!szCommand.empty())
			curl_easy_setopt(m_pCurl, CURLOPT_CUSTOMREQUEST, szCommand.c_str());

		//执行
		CURLcode res = curl_easy_perform(m_pCurl);

		if (CURLE_OK != res)
		{
			if (!(CURLE_FTP_COULDNT_RETR_FILE == res && 0 != iType && 6 != iType))
			{
				szError = std::string("Get file list error:") + curl_easy_strerror(res);
				return false;
			}

			if (1 == iType)
				szReplyData = m_szCurrentDir;
		}
	}
	else
	{
		curl_easy_cleanup(m_pCurl);
		m_pCurl = nullptr;

		return false;
	}

	return true;
}

/*********************************************************
功能：	设置curl用户认证信息
参数：	curl curl句柄
*		bUseSSH 是否使用ssh，即sftp
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::SetCurlAuthenticationInfo(void *curl, bool bUseSSH, std::string &szError)
{
	if (nullptr == curl)
	{
		szError = "Curl handle is empty !";
		return false;
	}

	CURLcode res = curl_easy_setopt(curl, CURLOPT_USERNAME, m_szUserName.c_str());
	if (CURLE_OK != res)
	{
		szError = std::string("curl_easy_setopt CURLOPT_USERNAME error:") + curl_easy_strerror(res);
		return false;
	}

	if (m_bAuthByPwd) //密码
	{
		res = curl_easy_setopt(curl, CURLOPT_PASSWORD, m_szPassword.c_str());
		if (CURLE_OK != res)
		{
			szError = std::string("curl_easy_setopt CURLOPT_PASSWORD error:") + curl_easy_strerror(res);
			return false;
		}

		if (bUseSSH) //使用ssh
		{
			res = curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);
			if (CURLE_OK != res)
			{
				szError = std::string("curl_easy_setopt CURLOPT_SSH_AUTH_TYPES error:") + curl_easy_strerror(res);
				return false;
			}
		}
	}
	else //证书
	{
		if (!CheckFileExists(m_szPubKeyFile))
		{
			szError = "SSH public key file:" + m_szPubKeyFile + " is not exists !";
			return false;
		}

		res = curl_easy_setopt(curl, CURLOPT_SSH_PUBLIC_KEYFILE, m_szPubKeyFile.c_str());
		if (CURLE_OK != res)
		{
			szError = std::string("curl_easy_setopt CURLOPT_SSH_PUBLIC_KEYFILE error:") + curl_easy_strerror(res);
			return false;
		}

		if (!CheckFileExists(m_szPriKeyFile))
		{
			szError = "SSH private key file:" + m_szPriKeyFile + " is not exists !";
			return false;
		}

		res = curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, m_szPriKeyFile.c_str());
		if (CURLE_OK != res)
		{
			szError = std::string("curl_easy_setopt CURLOPT_SSH_PRIVATE_KEYFILE error:") + curl_easy_strerror(res);
			return false;
		}

		if (!m_szPassword.empty())
		{
			res = curl_easy_setopt(curl, CURLOPT_KEYPASSWD, m_szPassword.c_str());
			if (CURLE_OK != res)
			{
				szError = std::string("curl_easy_setopt CURLOPT_KEYPASSWD error:") + curl_easy_strerror(res);
				return false;
			}
		}

		if (bUseSSH) //使用ssh
		{
			res = curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PUBLICKEY);
			if (CURLE_OK != res)
			{
				szError = std::string("curl_easy_setopt CURLOPT_SSH_AUTH_TYPES error:") + curl_easy_strerror(res);
				return false;
			}
		}
	}

	return true;
}

/*********************************************************
功能：	设置curl公共信息
参数：	curl curl句柄
*		debugData debug数据
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::SetCurlPublicInfo(void *curl, SDebugData *debugData, std::string &szError)
{
	CURLcode res = curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	if (CURLE_OK != res)
	{
		szError = std::string("curl_easy_setopt CURLOPT_NOSIGNAL error:") + curl_easy_strerror(res);
		return false;
	}

	res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); /* Switch on full protocol/debug output */
	if (CURLE_OK != res)
	{
		szError = std::string("curl_easy_setopt CURLOPT_VERBOSE error:") + curl_easy_strerror(res);
		return false;
	}

	res = curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L); /* missing dirs to be created on the remote server */
	if (CURLE_OK != res)
	{
		szError = std::string("curl_easy_setopt CURLOPT_CONNECTTIMEOUT error:") + curl_easy_strerror(res);
		return false;
	}

	res = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, m_iConnTimeout);
	if (CURLE_OK != res)
	{
		szError = std::string("curl_easy_setopt CURLOPT_CONNECTTIMEOUT error:") + curl_easy_strerror(res);
		return false;
	}

	res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2 * 60 * 60); //读取数据超时，2小时
	if (CURLE_OK != res)
	{
		szError = std::string("curl_easy_setopt CURLOPT_TIMEOUT error:") + curl_easy_strerror(res);
		return false;
	}

	res = curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_debug_print); //debug输出方法
	if (CURLE_OK != res)
	{
		szError = std::string("curl_easy_setopt CURLOPT_DEBUGFUNCTION error:") + curl_easy_strerror(res);
		return false;
	}

	res = curl_easy_setopt(curl, CURLOPT_DEBUGDATA, debugData); //debug回调数据
	if (CURLE_OK != res)
	{
		szError = std::string("curl_easy_setopt CURLOPT_DEBUGDATA error:") + curl_easy_strerror(res);
		return false;
	}

	return true;
}

/*********************************************************
功能：	设置当前路径
参数：	buf curl返回数据
*		data_len 数据长度
返回：	无
修改：
*********************************************************/
void CMyCurl::SetCurrentDir(char *buf, size_t data_len)
{
	//buf like:257 "/home/test"
	m_szCurrentDir = buf;
	m_szCurrentDir.erase(0, m_szCurrentDir.find("\"") + 1);
	m_szCurrentDir.erase(m_szCurrentDir.find("\""));
}

/*********************************************************
功能：	curl的debug输出方法，当设置CURLOPT_VERBOSE为1时有效
参数：	handle curl句柄
*		type 消息类型，为curl_infotype
*		data 数据
*		size 数据长度
*		userptr 用户自己的数据
返回：	成功返回0
修改：
*********************************************************/
int CMyCurl::curl_debug_print(void *handle, int type, char *data, size_t size, void *userptr)
{
	CURL *curl = (CURL*)handle;
	curl_infotype eType = (curl_infotype)type;
	SDebugData *obj = (SDebugData*)userptr;
	char *buf = new char[size + 1]{ 0 };
	
	if (nullptr != buf)
	{
		memcpy(buf, data, size);

		switch (eType)
		{
		case CURLINFO_TEXT:
			printf("== Normal info: length:%llu data:%s", size, buf);
			break;
		case CURLINFO_HEADER_IN:
		{
			printf("<= Recv header: length:%llu data:%s", size, buf);

			//获取路径
			if (0 == strncmp(buf, "257 ", 4))
			{
				obj->object->SetCurrentDir(buf, size);
			}
		}
			break;
		case CURLINFO_HEADER_OUT:
			printf("=> Send header: length:%llu data:%s", size, buf);
			break;
		case CURLINFO_DATA_IN:
			if (obj->bPrintData)
				printf("<= Recv data: length:%llu data:\n%s\n", size, buf);
			else
				printf("<= Recv data: length:%llu\n", size);
			break;
		case CURLINFO_DATA_OUT:
			if (obj->bPrintData)
				printf("=> Send data: length:%llu data:\n%s\n", size, buf);
			else
				printf("=> Send data: length:%llu\n", size);
			break;
		case CURLINFO_SSL_DATA_IN:
			printf("<= Recv SSL data: length:%llu data:%s\n", size, buf);
			break;
		case CURLINFO_SSL_DATA_OUT:
			printf("=> Send SSL data: length:%llu data:%s\n", size, buf);
			break;
		default:
			printf("== Unkown info: type:%d length:%llu data:0x%s\n", type, size, 
				get_hex((unsigned char*)buf, size).c_str());
			break;
		}

		delete[] buf;
		buf = nullptr;
	}
	else
		printf("curl_debug_print error, new memory buf return nullptr:%s\n", GetErrorMsg(errno).c_str());
	
	return 0;
}

/*********************************************************
功能：	ftp上传时的读文件回调函数
参数：	buf 缓存buff
*		size 读取的每个对象大小，字节
*		nmemb 要读取的对象个数
*		stream 输入流
返回：	成功读取的对象个数
修改：
*********************************************************/
size_t CMyCurl::ftp_read(void *buf, size_t size, size_t nmemb, void *stream)
{
	return fread(buf, size, nmemb, (FILE*)stream) * size;
}

/*********************************************************
功能：	ftp请求命令应答数据接收回调函数
参数：	buf 缓存buff
*		size 读取的每个对象大小，字节
*		nmemb 要读取的对象个数
*		stream 输出流
返回：	成功读取的对象个数
修改：
*********************************************************/
size_t CMyCurl::ftp_req_reply(void *buf, size_t size, size_t nmemb, void *stream)
{
	if (nullptr == buf || nullptr == stream || 0 == size || 0 == nmemb)
		return 0;

	size_t len = size * nmemb;
	((std::string*)stream)->append((const char*)buf, len);

	return len;
}

/*********************************************************
功能：	ftp下载时的写文件回调函数
参数：	buf 缓存buff
*		size 写入的每个对象大小，字节
*		nmemb 要写入的对象个数
*		stream 输出流
返回：	成功写入的对象个数
修改：
*********************************************************/
size_t CMyCurl::ftp_write(void *buf, size_t size, size_t nmemb, void *stream)
{
	SFtpReadFile *out = (SFtpReadFile*)stream;

	if (nullptr != out && nullptr == out->fstream)
	{
		out->fstream = fopen(out->szFileName.c_str(), "wb");
		if (!out->fstream)
		{
			return -1; /* failure, can't open file to write */
		}
	}

	return fwrite(buf, size, nmemb, out->fstream);
}

/*********************************************************
功能：	检查文件是否存在
参数：	szFileName 文件名
返回：	成功返回true
*********************************************************/
bool CMyCurl::CheckFileExists(const std::string &szFileName)
{
	if (szFileName.empty())
		return false;

#ifdef _WIN32
	return (0 == _access(szFileName.c_str(), 0));
#else
	if (0 == access(szFileName.c_str(), F_OK))
		return true;
	else
	{
		if (ENOENT == errno) //处理软连接
			return true;
		else
			return false;
	}
#endif // _WIN32
}

/*********************************************************
功能：	根据错误码取得错误信息
参数：	errCode 错误码
返回：	错误信息字符串
*********************************************************/
std::string CMyCurl::GetErrorMsg(int errCode)
{
#ifdef _WIN32
	std::string szError;
	LPVOID lpMsgBuf = NULL;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);

	if (NULL != lpMsgBuf)
	{
		szError = (LPSTR)lpMsgBuf;
		LocalFree(lpMsgBuf);
	}
	else
		szError = "FormatMessage failed, error code:" + std::to_string(GetLastError());

	return std::move(szError);
#else
	return std::string(strerror((0 == errCode) ? errno : errCode));
#endif
}

/*********************************************************
功能：	char*转16进制串，不加0x
参数：	data char*串
*		data_len char*串长度
*		isLowercase 是否小写字母
返回：	转后的串
修改：
*********************************************************/
std::string CMyCurl::get_hex(const unsigned char* data, size_t data_len, bool isLowercase)
{
	std::string szReturn;

	if (nullptr == data || 0 >= data_len)
		return std::move(szReturn);

	size_t n = 0;
	char *pStrTemp = new char[data_len * 2 + 1]{ '\0' };

	if (nullptr != pStrTemp)
	{
		for (size_t i = 0; i < data_len; ++i)
		{
			n += snprintf(pStrTemp + n, 3, (isLowercase ? "%02x" : "%02X"), data[i]);
		}

		pStrTemp[n] = '\0';
		szReturn = pStrTemp;
		delete[] pStrTemp;
	}

	return std::move(szReturn);
}

/*********************************************************
功能：	分割字符串文件列表
参数：	szListResult 文件列表
*		FileList 分割后的文件信息
返回：	无
修改：
*********************************************************/
void CMyCurl::SplitFiles(const std::string &szListResult, std::list<SFileInfo> &FileList)
{
	/*list获取结果
	-rw-rw-r--    1 500      500      57497782 Jul 30 01:37 BackgroundServices.tar.gz
	drwxr-xr-x    8 500      500          4096 Aug 14 08:41 GKQuantum
	-rw-rw-r--    1 500      500      10247082 Jul 30 07:34 GKQuantum.tar.gz
	-rwxr-xr-x    1 500      500          2899 Jul 30 08:05 QtInstall.sh
	-rw-r--r--    1 500      500      30585169 Jul 30 07:18 QtNmsCorbaCollect.zip
	-rw-r--r--    1 500      500      19730180 Jul 30 01:43 QtNorthService.zip
	-rw-r--r--    1 500      500       5937516 Jul 30 01:38 QuantumMonitor.tar.gz
	-rw-r--r--    1 500      500      145170243 Jul 30 01:37 quantum-dev.war
	drwxrwxr-x    2 500      500          4096 Mar 22 2018 test
	*/

	if (szListResult.empty())
		return;

	std::list<std::string> AttrList;
	std::string szTemp;

	//分解
	for (std::string::size_type i = 0; i < szListResult.length(); ++i)
	{
		char c = szListResult[i];
		
		if (' ' != c && '\r' != c && '\n' != c && '\t' != c)
			szTemp += c;
		else
		{
			if (!szTemp.empty())
			{
				AttrList.push_back(szTemp);
				szTemp.clear();
			}
		}
	}

	//处理
	SFileInfo file;
	int iNum = 1;

	for (const auto &attr : AttrList)
	{
		switch (iNum % 9)
		{
		case 0:
			file.szFileName = attr;
			FileList.push_back(file);
			file.szTime.clear();
			break;
		case 1:
			if ('d' == attr[0])
				file.bIsDir = true;
			else
				file.bIsDir = false;
			break;
		case 5:
			file.ullSize = std::atoll(attr.c_str());
			break;
		case 6:
		case 7:
		case 8:
			if (file.szTime.empty())
				file.szTime = attr;
			else
				file.szTime += " " + attr;
			break;
		default:
			break;
		}

		++iNum;
	}
}

/*********************************************************
功能：	获取ssh2会话最近的错误
参数：	session 会话句柄
返回：	错误信息
修改：
*********************************************************/
std::string CMyCurl::GetSsh2SessionLastError(void *session)
{
	std::string szError;
	char *buf = nullptr;
	int len = 0;

	if (LIBSSH2_ERROR_NONE != libssh2_session_last_error((LIBSSH2_SESSION*)session, &buf, &len, 0))
		szError = buf; //不能释放buf
	else
		szError = "No error in session !";

	return std::move(szError);
}

/*********************************************************
功能：	等待socket响应，非阻塞的
参数：	socket_fd socket句柄
*		session ssh2会话
*		timeout_sec 超时，秒
*		timeout_usec 超时，微秒
返回：	错误码
修改：
*********************************************************/
int CMyCurl::wait_socket(int socket_fd, void *session, long timeout_sec, long timeout_usec)
{
	struct timeval timeout;
	fd_set fd;
	fd_set *writefd = NULL;
	fd_set *readfd = NULL;

	timeout.tv_sec = timeout_sec;
	timeout.tv_usec = timeout_usec;

	FD_ZERO(&fd);
	FD_SET(socket_fd, &fd);

	/* now make sure we wait in the correct direction */
	int dir = libssh2_session_block_directions((LIBSSH2_SESSION*)session);

	if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;

	if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;

	return select(socket_fd + 1, readfd, writefd, NULL, &timeout);
}

/*********************************************************
功能：	SSH连接登陆
参数：	szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::SshLogin(bool bBlocked, std::string &szError)
{
	if (!m_bCurlEnvInit)
	{
		szError = "Curl global env is not init !";
		return false;
	}

	bool bResult = true;
	int iRet = 0, iAuthMethod = 0;

	//初始化libssh2 库
	iRet = libssh2_init(0);
	if (iRet != 0)
	{
		szError = "libssh2_init failed, errcode:" + std::to_string(iRet);
		return false;
	}

	//创建socket
	m_iConnectFd = (int)socket(AF_INET, SOCK_STREAM, 0);
	if (m_iConnectFd <= 0)
	{
		bResult = false;
		szError = "Create socket fd failed !";
	}

	//绑定地址及连接
	if (bResult)
	{
		struct sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(m_uiServerPort);
		if (m_szServerIP.empty())
			sin.sin_addr.s_addr = inet_addr(ADDR_ANY);
		else
			inet_pton(AF_INET, m_szServerIP.c_str(), &sin.sin_addr);

		//连接
		if (connect(m_iConnectFd, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0)
		{
			bResult = false;
			szError = "Connect to server failed:" + GetErrorMsg(errno);
		}
	}

	//初始化会话
	if (bResult)
	{
		m_pSession = libssh2_session_init();
		if (nullptr == m_pSession)
		{
			bResult = false;
			szError = "libssh2_session_init failed, return nullptr !";
		}
		else
		{
			m_bBlocked = bBlocked;

			//1：表示阻塞，0：非阻塞，要使用wait_socket判断返回值
			libssh2_session_set_blocking((LIBSSH2_SESSION*)m_pSession, (m_bBlocked ? 1 : 0));
		}
	}

	//握手
	if (bResult)
	{
		if (m_bBlocked)
			iRet = libssh2_session_handshake((LIBSSH2_SESSION*)m_pSession, m_iConnectFd);
		else
		{
			while ((iRet = libssh2_session_handshake((LIBSSH2_SESSION*)m_pSession, m_iConnectFd))
				== LIBSSH2_ERROR_EAGAIN);
		}

		if (0 != iRet)
		{
			bResult = false;
			szError = "Failure establishing SSH session:" + GetSsh2SessionLastError(m_pSession);
		}
	}

	if (bResult)
	{
		const char* pFingerprint = nullptr;
		size_t len = 0;
		int type = 0;

		//验证对端指纹，此处可以对比指纹和配置是否一致，指纹：服务器公钥的hash
		if (m_bBlocked)
			pFingerprint = libssh2_session_hostkey((LIBSSH2_SESSION*)m_pSession, &len, &type);
		else
		{
			while ((pFingerprint = libssh2_session_hostkey((LIBSSH2_SESSION*)m_pSession, &len, &type)) == nullptr)
			{
				wait_socket(m_iConnectFd, m_pSession, 2);
			}
		}

		/*if (nullptr != pFingerprint)
		{
			LIBSSH2_KNOWNHOSTS *nh = libssh2_knownhost_init((LIBSSH2_SESSION*)m_pSession);

			//read all hosts from here
			libssh2_knownhost_readfile(nh, "known_hosts", LIBSSH2_KNOWNHOST_FILE_OPENSSH);

			//store all known hosts to here
			//libssh2_knownhost_writefile(nh, "dumpfile", LIBSSH2_KNOWNHOST_FILE_OPENSSH);

			libssh2_knownhost *host;
			int check = libssh2_knownhost_checkp(nh, m_szServerIP.c_str(), m_uiServerPort, pFingerprint, len,
				LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW, &host);

			if (check != LIBSSH2_KNOWNHOST_CHECK_MATCH)
			{
				if (check == LIBSSH2_KNOWNHOST_CHECK_MISMATCH)
				{

				}
				else
				{

				}
			}

			libssh2_knownhost_free(nh);
		}*/

		char* pUserAuthList = nullptr;

		//检查可用认证方法列表
		if (m_bBlocked)
			pUserAuthList = libssh2_userauth_list((LIBSSH2_SESSION*)m_pSession, m_szUserName.c_str(), (unsigned int)m_szUserName.length());
		else
		{
			while ((pUserAuthList = libssh2_userauth_list((LIBSSH2_SESSION*)m_pSession, m_szUserName.c_str(),
				(unsigned int)m_szUserName.length())) == nullptr)
			{
				wait_socket(m_iConnectFd, m_pSession, 2);
			}
		}

		if (nullptr == pUserAuthList)
		{
			bResult = false;
			szError = "libssh2_userauth_list is null !";
		}
		else
		{
			if (strstr(pUserAuthList, "password") != NULL)
			{
				iAuthMethod |= 1;
			}

			if (strstr(pUserAuthList, "keyboard-interactive") != NULL)
			{
				iAuthMethod |= 2;
			}

			if (strstr(pUserAuthList, "publickey") != NULL)
			{
				iAuthMethod |= 4;
			}
		}
	}

	//认证
	if (bResult)
	{
		if (m_bAuthByPwd)
		{
			if (1 != (iAuthMethod & 1))
			{
				bResult = false;
				szError = "User auth by password is not support !";
			}
			else
			{
				if (m_bBlocked)
					iRet = libssh2_userauth_password((LIBSSH2_SESSION*)m_pSession, m_szUserName.c_str(), m_szPassword.c_str());
				else
				{
					while ((iRet = libssh2_userauth_password((LIBSSH2_SESSION*)m_pSession, m_szUserName.c_str(), m_szPassword.c_str()))
						== LIBSSH2_ERROR_EAGAIN);
				}

				if (0 != iRet)
				{
					bResult = false;
					szError = "User auth by password failed:" + GetSsh2SessionLastError(m_pSession);
				}
			}
		}
		else
		{
			if (4 != (iAuthMethod & 4))
			{
				bResult = false;
				szError = "User auth by publickey is not support !";
			}
			else
			{
				if (m_bBlocked)
					iRet = libssh2_userauth_publickey_fromfile((LIBSSH2_SESSION*)m_pSession, m_szUserName.c_str(), m_szPubKeyFile.c_str(),
						m_szPriKeyFile.c_str(), m_szPassword.c_str());
				else
				{
					while ((iRet = libssh2_userauth_publickey_fromfile((LIBSSH2_SESSION*)m_pSession, m_szUserName.c_str(), m_szPubKeyFile.c_str(),
						m_szPriKeyFile.c_str(), m_szPassword.c_str())) == LIBSSH2_ERROR_EAGAIN);
				}

				if (0 != iRet)
				{
					bResult = false;
					szError = "User auth by publickey failed:" + GetSsh2SessionLastError(m_pSession);
				}
			}
		}
	}

	//失败处理
	if (!bResult)
	{
		SshLogout();
	}

	return bResult;
}

/*********************************************************
功能：	SSH断开连接
参数：	无
返回：	无
修改：
*********************************************************/
void CMyCurl::SshLogout()
{
	if (nullptr != m_pChannel)
	{
		SshCloseChannel(m_iConnectFd, m_bBlocked, m_pSession, m_pChannel);
		m_pChannel = nullptr;
	}

	if (nullptr != m_pSession)
	{
		libssh2_session_disconnect((LIBSSH2_SESSION*)m_pSession, "Normal shutdown.");
		libssh2_session_free((LIBSSH2_SESSION*)m_pSession);
		m_pSession = nullptr;
		libssh2_exit();
	}

	if (0 < m_iConnectFd)
	{
#ifdef _WIN32
		closesocket(m_iConnectFd);
#else
		close(m_iConnectFd);
#endif
		m_iConnectFd = -1;
	}
}

/*********************************************************
功能：	SSH执行命令，不会记住路径，即：cd xx; ls 显示的还是登陆的目录文件信息
参数：	szCommand 命令
*		szResult 结果
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::SshExec(const std::string &szCommand, std::string &szResult, std::string &szError)
{
	if (szCommand.empty())
	{
		szError = "Command str is null !";
		return false;
	}

	if (nullptr == m_pSession)
	{
		szError = "Must login first !";
		return false;
	}

	bool bResult = true;
	int iRet = 0;
	LIBSSH2_CHANNEL *channel = nullptr;

	//打开通道
	if (m_bBlocked)
		channel = libssh2_channel_open_session((LIBSSH2_SESSION*)m_pSession);
	else
	{
		while ((channel = libssh2_channel_open_session((LIBSSH2_SESSION*)m_pSession)) == nullptr
			&& libssh2_session_last_error((LIBSSH2_SESSION*)m_pSession, NULL, NULL, 0) == LIBSSH2_ERROR_EAGAIN)
		{
			wait_socket(m_iConnectFd, m_pSession);
		}
	}

	if (nullptr == channel)
	{
		szError = "Open channel on session error:" + GetSsh2SessionLastError(m_pSession);
		return false;
	}

	//执行
	if (m_bBlocked)
		iRet = libssh2_channel_exec(channel, szCommand.c_str());
	else
	{
		while ((iRet = libssh2_channel_exec(channel, szCommand.c_str())) == LIBSSH2_ERROR_EAGAIN)
		{
			wait_socket(m_iConnectFd, m_pSession, 10);
		}
	}

	if (0 != iRet)
	{
		bResult = false;
		szError = "libssh2_channel_exec error:" + GetSsh2SessionLastError(m_pSession);
	}

	//读取返回
	if (bResult)
	{
		szResult.clear();

		if (m_bBlocked)
		{
			long long rc = 0;
			bool bRunning = true;
			char buffer[WRITEBUFFERSIZE + 1] = { '\0' };

			do
			{
				rc = libssh2_channel_read(channel, buffer, WRITEBUFFERSIZE);

				if (rc < 0)
				{
					bResult = false;
					szError = "libssh2_channel_read error:" + GetSsh2SessionLastError(m_pSession);
				}
				else
				{
					if (0 == rc)
						bRunning = false;
					else
					{
						szResult += buffer;
						memset(buffer, '\0', WRITEBUFFERSIZE + 1);
					}
				}
			} while (rc > 0);
		}
		else
		{
			bResult = SshReadData(m_pSession, channel, szResult, szError);
		}
	}

	SshCloseChannel(m_iConnectFd, m_bBlocked, m_pSession, channel);
	channel = nullptr;

	return bResult;
}

/*********************************************************
功能：	SSH创建通道
参数：	szResult 结果数据
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::SshCreateChannel(std::string &szResult, std::string &szError)
{
	if (nullptr == m_pSession)
	{
		szError = "Must login first !";
		return false;
	}

	if (m_bBlocked)
	{
		szError = "Must login by unblocked !";
		return false;
	}

	if (nullptr != m_pChannel)
	{
		SshCloseChannel(m_iConnectFd, m_bBlocked, m_pSession, m_pChannel);
		m_pChannel = nullptr;
	}

	bool bResult = true;
	int iRet = 0;

	//打开通道
	while ((m_pChannel = (void*)libssh2_channel_open_session((LIBSSH2_SESSION*)m_pSession)) == nullptr
		&& libssh2_session_last_error((LIBSSH2_SESSION*)m_pSession, NULL, NULL, 0) == LIBSSH2_ERROR_EAGAIN)
	{
		wait_socket(m_iConnectFd, m_pSession);
	}

	if (nullptr == m_pChannel)
	{
		szError = "Open channel on session error:" + GetSsh2SessionLastError(m_pSession);
		return false;
	}

	//请求创建pty，term:文档上是说根据/etc/termcap或/etc/terminfo来的，选择xterm或linux则会有颜色的字符在里面
	while ((iRet = libssh2_channel_request_pty((LIBSSH2_CHANNEL*)m_pChannel, "vt220")) == LIBSSH2_ERROR_EAGAIN)
	{
		wait_socket(m_iConnectFd, m_pSession, 5);
	}

	if (0 != iRet)
	{
		bResult = false;
		szError = "libssh2_channel_request_pty error:" + GetSsh2SessionLastError(m_pSession);
	}

	//在pty上开启shell
	if (bResult)
	{
		while ((iRet = libssh2_channel_shell((LIBSSH2_CHANNEL*)m_pChannel)) == LIBSSH2_ERROR_EAGAIN)
		{
			wait_socket(m_iConnectFd, m_pSession, 5);
		}

		if (0 != iRet)
		{
			bResult = false;
			szError = "libssh2_channel_shell error:" + GetSsh2SessionLastError(m_pSession);
		}
	}

	//读取返回
	return SshReadData(m_pSession, m_pChannel, szResult, szError);
}

/*********************************************************
功能：	SSH关闭通道
参数：	无
返回：	无
修改：
*********************************************************/
void CMyCurl::SshCloseChannel()
{
	SshCloseChannel(m_iConnectFd, m_bBlocked, m_pSession, m_pChannel);
	m_pChannel = nullptr;
}

/*********************************************************
功能：	SSH执行命令，可以多次执行，即可以记住目录变化
参数：	szCommand 命令
*		szResult 结果
*		szError 错误信息
*		iTimeoutMill 超时，毫秒
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::SshChannelShell(const std::string &szCommand, std::string &szResult, std::string &szError, int iTimeoutMill)
{
	if (nullptr == m_pSession)
	{
		szError = "Must login first !";
		return false;
	}

	if (nullptr == m_pChannel)
	{
		szError = "Must create channel first !";
		return false;
	}

	if (szCommand.empty())
	{
		szError = "Shell command is null !";
		return false;
	}

	//至少3秒
	if (iTimeoutMill < 3000)
		iTimeoutMill = 3000;

	if (SshWriteData(m_pSession, m_pChannel, szCommand, szError, iTimeoutMill))
	{
		if (SshReadData(m_pSession, m_pChannel, szResult, szError, "]$ ", iTimeoutMill))
		{
			//删除前面一行，此行是输入的命令
			if (!szResult.empty())
			{
				szResult.erase(0, szResult.find("\n") + 1);
			}
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

/*********************************************************
功能：	SSH关闭通道
参数：	fd socket
*		bBlocked 是否阻塞的
*		session 会话
*		channel 通道
返回：	无
修改：
*********************************************************/
void CMyCurl::SshCloseChannel(int fd, bool bBlocked, void *session, void *channel)
{
	if (nullptr != channel)
	{
		if (bBlocked)
			libssh2_channel_close((LIBSSH2_CHANNEL*)channel);
		else
		{
			while (libssh2_channel_close((LIBSSH2_CHANNEL*)channel) == LIBSSH2_ERROR_EAGAIN)
			{
				wait_socket(fd, session, 1);
			}
		}

		libssh2_channel_free((LIBSSH2_CHANNEL*)channel);
	}
}

/*********************************************************
功能：	SSH从通道读取数据
参数：	session 会话
*		channel 通道
*		szResult 读取的数据
*		szError 错误信息
*		szEnd 结束字符
*		timeout_msec 超时，毫秒
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::SshReadData(void *session, void *channel, std::string &szResult, std::string &szError,
	const std::string &szEnd, int timeout_msec)
{
	bool bResult = true;
	LIBSSH2_POLLFD fds;
	long long rc = 0;
	char buffer[WRITEBUFFERSIZE + 1] = { '\0' };
	szResult.clear();

	fds.type = LIBSSH2_POLLFD_CHANNEL;
	fds.fd.channel = (LIBSSH2_CHANNEL*)channel;
	fds.events = LIBSSH2_POLLFD_POLLIN;

	while (timeout_msec > 0)
	{
		if (1 > (libssh2_poll(&fds, 1, 10)))
		{
			timeout_msec -= 50;
			std::this_thread::sleep_for(std::chrono::microseconds(50));
			continue;
		}

		//读
		if (fds.revents & LIBSSH2_POLLFD_POLLIN)
		{
			rc = libssh2_channel_read((LIBSSH2_CHANNEL*)channel, buffer, WRITEBUFFERSIZE);

			if (rc < 0)
			{
				if (rc != LIBSSH2_ERROR_EAGAIN)
				{
					bResult = false;
					szError = "libssh2_channel_read error:" + GetSsh2SessionLastError(session);
					break;
				}
			}
			else
			{
				if (0 < rc)
				{
					szResult += buffer;
					memset(buffer, '\0', WRITEBUFFERSIZE + 1);
				}

				if (!szResult.empty())
				{
					if (szEnd.empty())
					{
						if (rc < WRITEBUFFERSIZE)
							timeout_msec = 0;
					}
					else
					{
						std::string::size_type iPos = szResult.find(szEnd);

						if (iPos != std::string::npos && iPos >= (szResult.length() - szEnd.length() - 1))
							timeout_msec = 0;
					}
				}
			}
		}

		//判断结束，已经没有数据可读
		if (fds.revents & LIBSSH2_POLLFD_CHANNEL_CLOSED)
			timeout_msec = 0;
		else
			timeout_msec -= 50;

		if (timeout_msec > 0)
			std::this_thread::sleep_for(std::chrono::microseconds(50));
	}

	return bResult;
}

/*********************************************************
功能：	SSH向通道写入数据
参数：	session 会话
*		channel 通道
*		szData 写入的数据
*		szError 错误信息
*		timeout_msec 超时
返回：	成功返回true
修改：
*********************************************************/
bool CMyCurl::SshWriteData(void *session, void *channel, const std::string &szData, std::string &szError,
	int timeout_msec)
{
	bool bResult = true;
	LIBSSH2_POLLFD fds;
	long long rc = 0, writelength = 0;
	std::string szTemp(szData);

	if ('\n' != szTemp[szTemp.length() - 1])
		szTemp.append("\n");

	const long long data_len = szTemp.length();
	const char* data = szTemp.c_str();

	fds.type = LIBSSH2_POLLFD_CHANNEL;
	fds.fd.channel = (LIBSSH2_CHANNEL*)channel;
	fds.events = LIBSSH2_POLLFD_POLLOUT;

	while (timeout_msec > 0)
	{
		if (1 > (libssh2_poll(&fds, 1, 10)))
		{
			timeout_msec -= 50;
			std::this_thread::sleep_for(std::chrono::microseconds(50));
			continue;
		}

		//读
		if (fds.revents & LIBSSH2_POLLFD_POLLOUT)
		{
			rc = libssh2_channel_write((LIBSSH2_CHANNEL*)channel, data + writelength, data_len - writelength);

			if (rc < 0)
			{
				if (rc != LIBSSH2_ERROR_EAGAIN)
				{
					bResult = false;
					szError = "libssh2_channel_write error:" + GetSsh2SessionLastError(session);
					break;
				}
			}
			else
			{
				writelength += rc;

				//写入完成
				if (data_len == writelength)
					timeout_msec = 0;
			}
		}

		//判断结束
		if (fds.revents & LIBSSH2_POLLFD_CHANNEL_CLOSED)
		{
			timeout_msec = 0;

			if (data_len != writelength)
			{
				bResult = false;
				szError = "SSH2 channel is closed !";
			}
		}
	}

	if (timeout_msec <= 0 && writelength < data_len && szError.empty())
	{
		bResult = false;
		szError = "libssh2_channel_write timeout !";
	}

	return bResult;
}