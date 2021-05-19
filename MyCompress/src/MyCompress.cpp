#include "../include/MyCompress.hpp"
#include <functional>
#include <fstream>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <stdlib.h>
#include <fcntl.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#include <direct.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#endif

#if (!defined(_WIN32)) && (!defined(WIN32)) && (!defined(__APPLE__))
#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif
#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifndef _FILE_OFFSET_BIT
#define _FILE_OFFSET_BIT 64
#endif
#endif

#ifdef _WIN32
#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define FTELLO_FUNC(stream) ftello64(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)
#else
// In darwin and perhaps other BSD variants off_t is a 64 bit value, hence no need for specific 64 bit functions
#define FOPEN_FUNC(filename, mode) fopen(filename, mode)
#define FTELLO_FUNC(stream) ftello(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko(stream, offset, origin)
#endif

#define WRITEBUFFERSIZE (32768)

#ifdef _WIN32
unsigned long CMyCompress::m_lVersionMadeBy = 0x0B1E; /* 00:表示fat，0b:NTFS,1E:表示zip版本（并没有什么用） */
#else
unsigned long CMyCompress::m_lVersionMadeBy = 0x031E; /* 03:表示unix,1E:表示zip版本 */
#endif

const unsigned short CMyCompress::m_DiskBlockSize = 512;

CMyCompress::CMyCompress(const std::string &szPassword, const std::string &szUserName) : m_szPassword(szPassword),
m_szUserName(szUserName)
{
}

CMyCompress::~CMyCompress()
{
}

CMyCompress::SFileInfo::SFileInfo() : eType(e_None), ullSize(0), ullOccupySize(0), llCreateTime(0),
llAccessTime(0), llWriteTime(0), ulDosWriteTime(0)
{
}

CMyCompress::SFileInfo::~SFileInfo()
{
	SubDirList.clear();
}

/*********************************************************
功能：	将整型时间转换为字符串"YYYY-mm-DD HH:MM:SS"时间
参数：	timeVal 从"1970-01-01 00:00:00"到现在的秒数
返回：	"YYYY-mm-DD HH:MM:SS"时间串
*********************************************************/
std::string CMyCompress::SFileInfo::TimeToString(const time_t& timeVal) const
{
	char strDateTime[32] = { '\0' };
	struct tm tmTimeTemp;

#ifdef _WIN32
	localtime_s(&tmTimeTemp, &timeVal);
#else
	localtime_r(&timeVal, &tmTimeTemp);
#endif

	strftime(strDateTime, 32, "%Y-%m-%d %X", &tmTimeTemp);

	return std::move(std::string(strDateTime));
}

/*********************************************************
功能：	ZIP压缩
参数：	fileList 待压缩文件列表
*		szZipFileName 压缩后的ZIP文件名称
*		eLevel 压缩级别
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::CompressZIP(const std::list<std::string> &fileList, const std::string &szZipFileName, ECompressLevel eLevel, 
	std::string &szError)
{
	bool bResult = true;
	zipFile zipPackge = NULL;
	const char *pwd = NULL;
	unsigned long size_buf = WRITEBUFFERSIZE;
	char *buf = NULL;

	//检查参数
	if (fileList.empty())
	{
		szError = "File to compress list is empty !";
		return false;
	}

	if (szZipFileName.empty())
	{
		szError = "Dst compress file name is empty !";
		return false;
	}

	//如果压缩包已存在，则删除
	RmFile(szZipFileName, false, szError);

	buf = new char[size_buf] { 0 };
	if (NULL == buf)
	{
		szError = "New memory buf return nullptr !";
		return false;
	}

	if (!m_szPassword.empty())
	{
		//设置密码
		pwd = m_szPassword.c_str();
	}

	//打开压缩包
	zipPackge = zipOpen64(szZipFileName.c_str(), APPEND_STATUS_CREATE);

	if (NULL == zipPackge)
	{
		szError = "Open zip file:" + szZipFileName + " failed !";
		delete[] buf;
		return false;
	}

	//处理文件
	for (const auto& file : fileList)
	{
		SFileInfo fileInfo;
		std::string szNameTemp(file);

		//获取真实路径
		if (std::string::npos != szNameTemp.find("..\\") || std::string::npos != szNameTemp.find("../"))
		{
			if (!GetRealPath(szNameTemp, szNameTemp, szError))
			{
				bResult = false;
				szError = "Get file real path failed:" + szError;
				break;
			}
		}

		//获取文件信息
		if (!GetFileInfo(szNameTemp, fileInfo, szError))
		{
			bResult = false;
			szError = "Get file info failed:" + szError;
			break;
		}

		if (e_Dir == fileInfo.eType)
		{
			if ('\\' == szNameTemp[szNameTemp.length() - 1] || '/' == szNameTemp[szNameTemp.length() - 1])
				szNameTemp.erase(szNameTemp.length() - 1);
		}
		
		//压缩处理
		if (!do_compressZIP(zipPackge, szNameTemp, fileInfo.szName, fileInfo, (int)eLevel, pwd, buf, size_buf, szError))
		{
			bResult = false;
			szError = "Do compress failed:" + szError;
			break;
		}
	}

	//关闭压缩包
	if (ZIP_OK != zipClose(zipPackge, NULL))
	{
		szError = "zipClose file:" + szZipFileName + " failed !";
		bResult = false;
	}

	if (!bResult)
	{
		//未压缩成功，删除压缩包
		std::string szTempError;
		RmFile(szZipFileName, false, szTempError);
	}
	else
	{
#ifndef _WIN32
		if (!m_szUserName.empty())
			Chown(szZipFileName, m_szUserName, szError);
#endif
	}

	delete[] buf;
	buf = NULL;

	return bResult;
}

/*********************************************************
功能：	ZIP解压缩处理
参数：	szZipFileName zip包文件名称
*		szDstDir 解压缩到的目录，空表示当前目录
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::UncompressZIP(const std::string &szZipFileName, const std::string &szDstDir, std::string &szError)
{
	std::string szDstDirTemp(szDstDir);
	unsigned long size_buf = WRITEBUFFERSIZE;
	char *buf = NULL;

	//检查参数
	if (szZipFileName.empty())
	{
		szError = "ZIP file name is empty !";
		return false;
	}
	else
	{
		if (!CheckFileExists(szZipFileName))
		{
			szError = "ZIP file is not exists !";
			return false;
		}
		else
		{
			if (2 != CheckFileCompressType(szZipFileName))
			{
				szError = "The file:" + szZipFileName + " is not zip file !";
				return false;
			}
		}
	}

	if (szDstDirTemp.empty())
	{
		//空表示当前目录
		szDstDirTemp = ".";
	}
	else
	{
		if (!CheckFileExists(szDstDirTemp))
		{
			szError = "ZIP uncompress dst dir is not exists !";
			return false;
		}
		else
		{
			if (e_Dir != CheckFileType(szDstDirTemp))
			{
				szError = "ZIP uncompress dst dir:" + szDstDirTemp + " is not a dir !";
				return false;
			}
			else
			{
				if ('\\' == szDstDirTemp[szDstDirTemp.length() - 1] || '/' == szDstDirTemp[szDstDirTemp.length() - 1])
					szDstDirTemp.erase(szDstDirTemp.length() - 1);

				for (std::string::size_type i = 0; i < szDstDirTemp.length(); ++i)
				{
					if ('\\' == szDstDirTemp[i])
						szDstDirTemp[i] = '/';
				}
			}
		}
	}

	buf = new char[size_buf] { 0 };
	if (NULL == buf)
	{
		szError = "New memory buf return nullptr !";
		return false;
	}

	//解压处理
	bool bResult = true;
	zipFile zipPackge = NULL;

	zipPackge = unzOpen64(szZipFileName.c_str());

	if (NULL == zipPackge)
	{
		szError = "Open zip file:" + szZipFileName + " failed !";
		delete[] buf;
		return false;
	}

	unz_global_info64 gi;
	int iRet = unzGetGlobalInfo64(zipPackge, &gi);
	if (UNZ_OK == iRet)
	{
		const char *pwd = NULL;
		char filename_inzip[288], extra_filed[64];
		std::list<SLinkInfo> linkList;

		for (ZPOS64_T i = 0; i < gi.number_entry; i++)
		{
			unz_file_info64 file_info;

			//获取当前zip文件信息
			iRet = unzGetCurrentFileInfo64(zipPackge, &file_info, filename_inzip, sizeof(filename_inzip), extra_filed, 
				sizeof(extra_filed), NULL, 0);
			if (UNZ_OK != iRet)
			{
				bResult = false;
				szError = "Get zip packge current file failed, ret:" + std::to_string(iRet);
				break;
			}

			//加密zip包
			if (1 == (1 & file_info.flag))
			{
				if (m_szPassword.empty())
				{
					bResult = false;
					szError = "Zip packge is encrypted, but password empty !";
					break;
				}
				else
					pwd = m_szPassword.c_str();
			}

			//解压处理
			if (!do_uncompressZIP(zipPackge, file_info, szDstDirTemp, filename_inzip, pwd, buf, size_buf,
				linkList, szError))
			{
				bResult = false;
				break;
			}

			//防止越界，获取下一个文件
			if ((i + 1) < gi.number_entry)
			{
				iRet = unzGoToNextFile(zipPackge);
				if (UNZ_OK != iRet)
				{
					bResult = false;
					szError = "Get zip packge next file failed, ret:" + std::to_string(iRet);
					break;
				}
			}
		}

		//创建软连接
		if (bResult && !linkList.empty())
		{
#ifndef _WIN32
			std::string szTempError;

			for (const auto &link : linkList)
			{
				RmFile(link.szLink, false, szTempError);
				iRet = symlink(link.szPath.c_str(), link.szLink.c_str());

#ifdef _DEBUG
				printf("create link:%s\n", link.szLink.c_str());
#endif

				if (0 != iRet)
				{
					bResult = false;
					szError = "Create link file:" + link.szLink + " failed:" + GetErrorMsg(errno);
					break;
				}
				else
				{
					if (!m_szUserName.empty())
						Chown(link.szLink, m_szUserName, szError);
				}
			}
#endif
		}
	}
	else
	{
		bResult = false;
		szError = "Get zip file global info failed, ret:" + std::to_string(iRet);
	}

	//关闭压缩包
	if (ZIP_OK != unzClose(zipPackge))
	{
		bResult = false;
		szError = "unzClose file:" + szZipFileName + " failed !";
	}

	delete[] buf;
	buf = NULL;

	return bResult;
}

/*********************************************************
功能：	获取时间字符串
参数：	无
返回：	时间字符串
修改：
*********************************************************/
std::string CMyCompress::GetTimeStr()
{
	char strDateTime[32] = { '\0' };
	struct tm tmTimeTemp;
	time_t timeTemp = 0;
	int iMillSeconds = 0;

	//获取当前秒数
	timeTemp = time(NULL);

	//localtime函数不可重入，即不是线程安全的，所以用下面的版本
#ifdef _WIN32
	localtime_s(&tmTimeTemp, &timeTemp);

	SYSTEMTIME ctT;
	GetLocalTime(&ctT);
	iMillSeconds = ctT.wMilliseconds;
#else
	localtime_r(&timeTemp, &tmTimeTemp);

	timeval tv;
	gettimeofday(&tv, NULL);
	iMillSeconds = (tv.tv_usec / 1000);
#endif

	snprintf(strDateTime, 24, "%02d:%02d:%02d.%03d>> ", tmTimeTemp.tm_hour, tmTimeTemp.tm_min,
		tmTimeTemp.tm_sec, iMillSeconds);

	return std::string(strDateTime);
}

/*********************************************************
功能：	获取目录文件
参数：	szDir 目录名，如./,../xx,tx/
*		fileList 文件列表
*		szError 错误信息
返回：	成功返回true
*********************************************************/
bool CMyCompress::GetDirFiles(const std::string& szDir, std::list<SDirFiles> &fileList, std::string &szError)
{
	if (szDir.empty())
	{
		szError = "Dir path can not be empty !";
		return false;
	}

	std::string szDirTemp(szDir), szBaseDir;

	//处理目录
#ifdef _WIN32
	if ('*' != szDirTemp[szDirTemp.length() - 1])
	{
		if (szDirTemp.length() <= 1 || '\\' != szDirTemp[szDirTemp.length() - 2])
			szDirTemp.append("\\*");
		else
			szDirTemp.append("*");
	}
	else
	{
		if ('\\' != szDirTemp[szDirTemp.length() - 2])
		{
			szError = "Dir path is wrong !";
			return false;
		}
	}

	szBaseDir = szDirTemp.substr(0, szDirTemp.rfind("\\"));
#else
	if ("/" != szDirTemp)
	{
		if ('/' == szDirTemp[szDirTemp.length() - 1])
		{
			szDirTemp.erase(szDirTemp.length() - 1);
		}

		szBaseDir = szDirTemp;
	}
#endif

#ifdef _WIN32
	intptr_t hFile = 0; //文件夹句柄
	struct _finddata_t fileInfo; //文件句柄

	if ((hFile = _findfirst(szDirTemp.c_str(), &fileInfo)) != -1)
	{
		do
		{
			//过滤掉当前目录和上层目录
			if (0 != strcmp(".", fileInfo.name) && 0 != strcmp("..", fileInfo.name))
			{
				SDirFiles file;

				file.szFileName = fileInfo.name;

				if (fileInfo.attrib & _A_SUBDIR)
				{
					file.bIsDir = true;

					std::string szChildDir = szBaseDir + "\\" + file.szFileName + "\\*";
					if (!GetDirFiles(szChildDir, file.DirFiles, szError))
					{
						break;
					}
				}

				fileList.push_back(file);
			}
		} while (_findnext(hFile, &fileInfo) == 0);

		_findclose(hFile);
	}
	else
	{
		szError = "Open dir failed:" + GetErrorMsg(GetLastError());
		return false;
	}
#else
	DIR *pDir = nullptr; //目录句柄
	struct dirent *pDirent = nullptr; //文件句柄

	if ((pDir = opendir(szDirTemp.c_str())) != nullptr)
	{
		while ((pDirent = readdir(pDir)) != nullptr)
		{
			//过滤掉当前目录和上层目录
			if (0 != strcmp(".", pDirent->d_name) && 0 != strcmp("..", pDirent->d_name))
			{
				SDirFiles file;

				file.szFileName = pDirent->d_name;

				if (pDirent->d_type == DT_DIR)
				{
					file.bIsDir = true;
					
					//读取子文件夹
					std::string szChildDir = szBaseDir + "/" + pDirent->d_name;
					if (!GetDirFiles(szChildDir, file.DirFiles, szError))
					{
						break;
					}
				}

				fileList.push_back(file);
			}
		}

		closedir(pDir);
	}
	else
	{
		szError = "Open dir failed:" + GetErrorMsg(errno);
		return false;
	}
#endif

	return true;
}

/*********************************************************
功能：	读取目录里面的文件信息
参数：	szDir 要读取的目录，win下如：E:\\Work\\*，linux下如：/Work
*		infoList 文件信息容器
*		szError 错误信息
*		bCheckIsDir 是否检查是否目录
返回：	成功返回true
*********************************************************/
bool CMyCompress::GetDirFiles(const std::string& szDir, std::list<SFileInfo>& infoList, std::string& szError, bool bCheckIsDir)
{
	if (szDir.empty())
	{
		szError = "Dir path can not be empty !";
		return false;
	}

	std::string szDirTemp(szDir);
	SFileInfo file;

	//处理目录
#ifdef _WIN32
	if ('*' != szDirTemp[szDirTemp.length() - 1])
	{
		if (szDirTemp.length() <= 1 || ('\\' != szDirTemp[szDirTemp.length() - 2] && '/' != szDirTemp[szDirTemp.length() - 2]))
			szDirTemp.append("\\*");
		else
			szDirTemp.append("*");
	}
	else
	{
		if ('\\' != szDirTemp[szDirTemp.length() - 2] && '/' != szDirTemp[szDirTemp.length() - 2])
		{
			szError = "Dir path is wrong !";
			return false;
		}
	}

	if (std::string::npos != szDirTemp.rfind("\\"))
		file.szBaseDir = szDirTemp.substr(0, szDirTemp.rfind("\\"));
	else
		file.szBaseDir = szDirTemp.substr(0, szDirTemp.rfind("/"));
#else
	if ("/" != szDirTemp)
	{
		if ('/' == szDirTemp[szDirTemp.length() - 1])
		{
			szDirTemp.erase(szDirTemp.length() - 1);
		}

		file.szBaseDir = szDirTemp;
	}
#endif

	//检查是否存在
	if (!CheckFileExists(file.szBaseDir))
	{
		szError = "Path:" + szDir + " is not exists !";
		return false;
	}

	//检查是否是目录
	if (bCheckIsDir && e_Dir != CheckFileType(file.szBaseDir))
	{
		szError = "Path:" + szDir + " is not a dir !";
		return false;
	}

#ifdef _WIN32
	intptr_t hFile = 0; //文件夹句柄
	struct _finddata_t fileInfo; //文件句柄

	if ((hFile = _findfirst(szDirTemp.c_str(), &fileInfo)) != -1)
	{
		do
		{
			//过滤掉当前目录和上层目录
			if (0 != strcmp(".", fileInfo.name) && 0 != strcmp("..", fileInfo.name))
			{
				file.szName = fileInfo.name;
				file.iAttrib = fileInfo.attrib;
				file.llCreateTime = fileInfo.time_create;
				file.llAccessTime = fileInfo.time_access;
				file.llWriteTime = fileInfo.time_write;
				file.ullSize = fileInfo.size; //windows获取文件夹的大小是0
				file.ullOccupySize = (fileInfo.size / 4096 + (0 == (fileInfo.size % 4096) ? 0 : 1)) * 4096;

				WIN32_FIND_DATAA ff32;
				HANDLE hFind = FindFirstFile((file.szBaseDir + "\\" + file.szName).c_str(), &ff32);
				if (INVALID_HANDLE_VALUE != hFind)
				{
					FILETIME ftLocal;
					FileTimeToLocalFileTime(&(ff32.ftLastWriteTime), &ftLocal);
					FileTimeToDosDateTime(&ftLocal, ((LPWORD)&(file.ulDosWriteTime)) + 1, ((LPWORD)&(file.ulDosWriteTime)) + 0);
					FindClose(hFind);
				}

				if (fileInfo.attrib & _A_SUBDIR)
				{
					file.eType = e_Dir; //文件夹

										//读取子文件夹
					std::string szChildDir = file.szBaseDir + "\\" + file.szName + "\\*";
					if (!GetDirFiles(szChildDir, file.SubDirList, szError, false))
					{
						break;
					}
				}
				else
				{
					//windows下，快捷方式会当成文件，暂时只用后缀名.lnk判断
					std::string::size_type iPos = file.szName.rfind(".lnk");
					if (iPos != std::string::npos && file.szName.size() >= 5 && (file.szName.size() - 4) == iPos) //快捷方式
						file.eType = e_Link;
					else
						file.eType = e_File; //文件
				}

				infoList.push_back(file);
				file.SubDirList.clear();
			}
		} while (_findnext(hFile, &fileInfo) == 0);

		_findclose(hFile);
	}
	else
	{
		szError = "Open dir failed:" + GetErrorMsg(GetLastError());
		return false;
	}
#else
	DIR *pDir = nullptr; //目录句柄
	struct dirent *pDirent = nullptr; //文件句柄
	struct stat info;

	if ((pDir = opendir(szDirTemp.c_str())) != nullptr)
	{
		while ((pDirent = readdir(pDir)) != nullptr)
		{
			//过滤掉当前目录和上层目录
			if (0 != strcmp(".", pDirent->d_name) && 0 != strcmp("..", pDirent->d_name))
			{
				std::string szChildFile = file.szBaseDir + "/" + pDirent->d_name;

				//获取属性
				lstat(szChildFile.c_str(), &info);

				file.szName = pDirent->d_name;
				file.iAttrib = info.st_mode;
				file.llCreateTime = info.st_ctime;
				file.llAccessTime = info.st_atime;
				file.llWriteTime = info.st_mtime;
				file.ullSize = info.st_size; //linux获取文件夹大小是4096
				file.ullOccupySize = info.st_blocks * m_DiskBlockSize;

				if (pDirent->d_type == DT_DIR)
				{
					file.eType = e_Dir; //文件夹

										//读取子文件夹
					if (!GetDirFiles(szChildFile, file.SubDirList, szError, false))
					{
						break;
					}
				}
				else
				{
					if (pDirent->d_type == DT_LNK) //软连接
						file.eType = e_Link;
					else
						file.eType = e_File; //文件
				}

				infoList.push_back(file);
				file.SubDirList.clear();
			}
		}

		closedir(pDir);
	}
	else
	{
		szError = "Open dir failed:" + GetErrorMsg(errno);
		return false;
	}
#endif

	return true;
}

/*********************************************************
功能：	读取文件信息
参数：	szFile 要读取的文件，win下如：E:\\Work\\1.txt，linux下如：/Work/t.txt
*		info 文件信息
*		szError 错误信息
返回：	成功返回true
*********************************************************/
bool CMyCompress::GetFileInfo(const std::string& szFile, SFileInfo &info, std::string &szError)
{
	//处理文件名
	std::string szFileTemp(szFile);
	if (!szFileTemp.empty() 
		&& "/" != szFileTemp 
		&& ('/' == szFileTemp[szFileTemp.length() - 1] || '\\' == szFileTemp[szFileTemp.length() - 1]))
	{
		szFileTemp.erase(szFileTemp.length() - 1);
	}

	if (szFileTemp.empty())
	{
		szError = "File path can not be empty !";
		return false;
	}

	//检查是否存在
	if (!CheckFileExists(szFileTemp))
	{
		szError = "File:" + szFileTemp + " is not exists !";
		return false;
	}

	//处理路径
	std::string::size_type iPos = szFileTemp.rfind("\\");
	if (std::string::npos == iPos)
		iPos = szFileTemp.rfind("/");

	if (std::string::npos == iPos)
	{
		if ("." == szFileTemp || ".." == szFileTemp)
			GetRealPath(szFileTemp, info.szBaseDir, szError);
		else
			info.szBaseDir = ".";

		info.szName = szFileTemp;
	}
	else
	{
		if (0 != iPos)
		{
			info.szBaseDir = szFileTemp.substr(0, iPos);
			info.szName = szFileTemp.substr(iPos + 1);
		}
		else
			info.szName = "/";
	}

	bool bResult = false;

#ifdef _WIN32
	intptr_t hFind;
	struct _finddata_t ff32;

	hFind = _findfirst(szFileTemp.c_str(), &ff32);
	if (-1 != hFind)
	{
		if (_A_SUBDIR & ff32.attrib)
			info.eType = e_Dir;
		else
		{
			if (0 < strlen(ff32.name))
			{
				std::string szTemp = ff32.name;
				std::string::size_type iPos = szTemp.rfind(".lnk");

				if (std::string::npos != iPos && iPos == (szTemp.length() - 4))
					info.eType = e_Link;
				else
					info.eType = e_File;
			}
			else
				info.eType = e_File;
		}

		info.iAttrib = ff32.attrib;
		info.llCreateTime = ff32.time_create;
		info.llAccessTime = ff32.time_access;
		info.llWriteTime = ff32.time_write;
		info.ullSize = ff32.size;
		info.ullOccupySize = (ff32.size / 4096 + (0 == (ff32.size % 4096) ? 0 : 1)) * 4096;

		WIN32_FIND_DATAA ffd;
		HANDLE hF= FindFirstFile(szFileTemp.c_str(), &ffd);
		if (INVALID_HANDLE_VALUE != hF)
		{
			FILETIME ftLocal;
			FileTimeToLocalFileTime(&(ffd.ftLastWriteTime), &ftLocal);
			FileTimeToDosDateTime(&ftLocal, ((LPWORD)&(info.ulDosWriteTime)) + 1, ((LPWORD)&(info.ulDosWriteTime)) + 0);
			FindClose(hF);
		}

		bResult = true;

		_findclose(hFind);
	}
	else
	{
		szError = "Read file info failed:" + GetErrorMsg(GetLastError());
		return false;
	}
#else
	struct stat st;

	if (0 == lstat(szFileTemp.c_str(), &st))
	{
		if (S_ISDIR(st.st_mode))
			info.eType = e_Dir;
		else if (S_ISLNK(st.st_mode))
			info.eType = e_Link;
		else
			info.eType = e_File;

		info.iAttrib = st.st_mode;
		info.llCreateTime = st.st_ctime;
		info.llAccessTime = st.st_atime;
		info.llWriteTime = st.st_mtime;
		info.ullSize = st.st_size;
		info.ullOccupySize = st.st_blocks * m_DiskBlockSize;

		bResult = true;
	}
	else
	{
		szError = "Read file info failed:" + GetErrorMsg(errno);
		return false;
	}

#endif

	return bResult;
}

/*********************************************************
功能：	压缩处理
参数：	zip zip包
*		szFileName 文件名称
*		szZipFileName zip文件名称
*		fileInfo 文件信息
*		iLevel 压缩级别，1-9
*		pwd 压缩密码
*		buf 缓存
*		size_buf 缓存大小
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::do_compressZIP(zipFile zip, const std::string &szFileName, const std::string &szZipFileName, const SFileInfo &fileInfo,
	int iLevel, const char* pwd, char* buf, unsigned long size_buf, std::string &szError)
{
	bool bResult = false;
	std::string szFileInZipName(szZipFileName);
	zip_fileinfo fileZip;
	int iRet = 0, iMethod = 0, iZip64 = 0;
	unsigned long crcFile = 0, flagBase = 0;

#ifdef _DEBUG
	printf("%s\n", szFileName.c_str());
#endif

	FillFileTime(fileInfo, fileZip);
	fileZip.internal_fa = 0;

	if (e_Dir == fileInfo.eType)
	{
#ifdef _WIN32
		fileZip.external_fa = FILE_ATTRIBUTE_DIRECTORY; //0x10
#else
		fileZip.external_fa = 0x10 | (fileInfo.iAttrib << 16);
#endif
		szFileInZipName.append("/");
	}
	else
	{
#ifdef _WIN32
		fileZip.external_fa = FILE_ATTRIBUTE_ARCHIVE;
#else
		fileZip.external_fa = (fileInfo.iAttrib << 16);
#endif
		if (pwd != NULL)
		{
			//获取文件crc32
			if (!GetFileCrc(szFileName, buf, size_buf, &crcFile, (e_Link == fileInfo.eType ? true : false), szError))
			{
				szError = "Get file CRC32 value failed:" + szError;
				return false;
			}
		}

		if (e_Link != fileInfo.eType)
		{
			ZPOS64_T llSize = 0;
			iZip64 = CheckFileIsLarge(szFileName.c_str(), &llSize);

			//24是使用7-zip测试得出，当文件大于24字节时，便会压缩，软连接不压缩
			if (iLevel >= 1 || llSize >= 24)
				iMethod = Z_DEFLATED;
		}
	}

	//打开当前文件
	iRet = zipOpenNewFileInZip4_64(zip, szFileInZipName.c_str(), &fileZip, NULL, 0, NULL, 0, NULL,
		iMethod, iLevel, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, ((e_Dir == fileInfo.eType) ? NULL : pwd), 
		crcFile, m_lVersionMadeBy, flagBase, iZip64);

	if (ZIP_OK == iRet)
	{
		std::list<SFileInfo> fileList;

		if (e_Dir == fileInfo.eType)
			bResult = GetDirFiles(szFileName, fileList, szError, false);
		else if (e_File == fileInfo.eType)
			bResult = WriteNewFileInZip(zip, szFileName, buf, size_buf, szError);
		else
		{
#ifdef _WIN32
			bResult = WriteNewFileInZip(zip, szFileName, buf, size_buf, szError);
#else
			bResult = WriteNewFileLinkInZip(zip, szFileName, buf, size_buf, szError);
#endif
		}

		//关闭当前文件
		zipCloseFileInZip(zip);

		//查看子目录是否有文件
		if (bResult && e_Dir == fileInfo.eType)
		{
			std::string szNameTemp;

			for (const auto &f : fileList)
			{
#ifdef _WIN32
				szNameTemp = szFileName + "\\" + f.szName;
#else
				szNameTemp = szFileName + "/" + f.szName;
#endif
				bResult = do_compressZIP(zip, szNameTemp, (szZipFileName + "/" + f.szName), f, iLevel, pwd, buf, size_buf, szError);

				if (!bResult)
				{
					//失败就结束
					break;
				}
			}
		}
	}
	else
		szError = "Create new file in zip packge failed, ret:" + std::to_string(iRet);

	return bResult;
}

/*********************************************************
功能：	解压缩处理
参数：	zip zip包
*		file_info 当前文件信息
*		szDstDirName 解压到的目录名称
*		filename_inzip 压缩包中的文件名称
*		pwd 压缩密码
*		buf 缓存
*		size_buf 缓存大小
*		LinkList 软连接
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::do_uncompressZIP(zipFile zip, const unz_file_info64 &file_info, const std::string &szDstDirName, const char* filename_inzip,
	const char* pwd, char* buf, unsigned long size_buf, std::list<SLinkInfo> &LinkList, std::string &szError)
{
	bool bResult = true, bIsDir = false, bNormalWrite = true;
	std::string szOutFileName = szDstDirName + "/" + filename_inzip;
	char cTail = filename_inzip[file_info.size_filename - 1];
	unsigned long mode = file_info.external_fa >> 16;

#ifdef _DEBUG
	printf("%s\n", szOutFileName.c_str());
#endif

	if ('/' == cTail || '\\' == cTail) //文件夹
	{
		bIsDir = true;
		szOutFileName.erase(szOutFileName.length() - 1);

		//可能是windows上压缩的包，没有属性
#ifndef _WIN32
		if (0 == mode)
			mode = 0755;
#endif

		//创建目录
		if (!MkDir(szOutFileName, mode, false, szError))
		{
			bResult = false;
		}
	}
	else //文件
	{
		//可能是windows上压缩的包，没有属性
#ifndef _WIN32
		if (0 == mode)
			mode = 0644;
#endif

		//打开当前zip文件
		int iRet = unzOpenCurrentFilePassword(zip, pwd);
		if (iRet != UNZ_OK)
		{
			bResult = false;

			if (Z_DATA_ERROR == iRet)
				szError = "Open zip current file failed, password wrong !";
			else
				szError = "Open zip current file:" + std::string(filename_inzip) + " failed, ret:" + std::to_string(iRet);
		}
		else
		{
#ifndef _WIN32
			if (S_ISLNK(mode))
				bNormalWrite = false;
#endif

			if (bNormalWrite)
			{
				FILE *fout = FOPEN_FUNC(szOutFileName.c_str(), "wb+");
				if (NULL == fout)
				{
					bResult = false;
					szError = "Open file:" + szOutFileName + " for write failed:" + GetErrorMsg(errno);
				}
				else
				{
					do
					{
						//读取当前zip文件
						iRet = unzReadCurrentFile(zip, buf, size_buf);
						if (iRet < 0)
						{
							bResult = false;
							szError = "Read zip file:" + std::string(filename_inzip) + " for write failed, ret:" + std::to_string(iRet);
							break;
						}
						else if (iRet > 0)
						{
							//写入磁盘
							if (fwrite(buf, iRet, 1, fout) != 1)
							{
								bResult = false;
								szError = "Write data to file failed:" + GetErrorMsg(errno);
								iRet = UNZ_ERRNO;
								break;
							}
						}
					} while (iRet > 0);

					fclose(fout);
				}
			}
			else //写入linux软连接
			{
				//读取当前zip文件
				iRet = unzReadCurrentFile(zip, buf, size_buf);
				if (iRet < 0)
				{
					bResult = false;
					szError = "Read zip file:" + std::string(filename_inzip) + " link data failed, ret:" + std::to_string(iRet);
				}
				else if (iRet == 0)
				{
					bResult = false;
					szError = "Read zip file:" + std::string(filename_inzip) + " link data failed, length is 0 !";
				}
				else
				{
					SLinkInfo link;

					buf[iRet] = '\0';
					link.szPath = buf;
					link.szLink = szOutFileName;
					link.lMode = mode;

					LinkList.push_back(link);
				}
			}

			//关闭当前zip文件
			iRet = unzCloseCurrentFile(zip);

			if (UNZ_OK != iRet)
			{
				bResult = false;
				szError = "Close zip current file:" + std::string(filename_inzip) + " failed, ret:" + std::to_string(iRet);
			}
		}
	}

	//修改时间及权限
	if (bResult && bNormalWrite)
	{
		ChangeFileTime(szOutFileName, bIsDir, file_info.dosDate, &(file_info.tmu_date));
#ifndef _WIN32
		if (!bIsDir)
			bResult = Chmod(szOutFileName, mode, szError);

		if (!m_szUserName.empty())
			Chown(szOutFileName, m_szUserName, szError);
#endif
	}

	return bResult;
}

/*********************************************************
功能：	在zip包中写入新文件
参数：	zip zip包
*		szFileName 文件名称
*		buf 缓存
*		size_buf 缓存大小
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::WriteNewFileInZip(zipFile zip, const std::string &szFileName, char* buf, unsigned long size_buf, std::string &szError)
{
	if (NULL == zip)
	{
		szError = "ZIP file handle is null !";
		return false;
	}

	if (szFileName.empty())
	{
		szError = "File name is empty !";
		return false;
	}

	bool bResult = true;
	FILE * fin = FOPEN_FUNC(szFileName.c_str(), "rb");

	if (NULL != fin)
	{
		uLong size_read = 0;
		int iRet = ZIP_OK;

		do
		{
			size_read = (uLong)fread(buf, 1, size_buf, fin);
			if (size_read < size_buf)
			{
				if (feof(fin) == 0)
				{
					bResult = false;
					iRet = ZIP_ERRNO;
					szError = "Check read file ending failed !";
				}
			}

			if (size_read > 0)
			{
				iRet = zipWriteInFileInZip(zip, buf, size_read);
				if (iRet < 0)
				{
					bResult = false;
					szError = "Write data to zip file failed !";
				}
			}
		} while ((iRet == ZIP_OK) && (size_read > 0));

		fclose(fin);
	}
	else
	{
		bResult = false;
		szError = "Open file:" + szFileName + " for read failed:" + GetErrorMsg(errno);
	}

	return bResult;
}

/*********************************************************
功能：	在zip包中写入新连接文件
参数：	zip zip包
*		szFileName 文件名称
*		buf 缓存
*		size_buf 缓存大小
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::WriteNewFileLinkInZip(zipFile zip, const std::string &szFileName, char* buf, unsigned long size_buf, std::string &szError)
{
	if (NULL == zip)
	{
		szError = "ZIP file handle is null !";
		return false;
	}

	if (szFileName.empty())
	{
		szError = "File name is empty !";
		return false;
	}

#ifdef _WIN32
	szError = "Not support on WIN32 !";
	return false;
#else
	bool bResult = true;
	int iRet = readlink(szFileName.c_str(), buf, size_buf);

	if (0 < iRet)
	{
		buf[iRet] = '\0';
		iRet = zipWriteInFileInZip(zip, buf, iRet);
		if (iRet < 0)
		{
			bResult = false;
			szError = "Write data to zip file failed !";
		}
	}
	else
	{
		bResult = false;
		szError = "Read link file:" + szFileName + " failed:" + GetErrorMsg(errno);
	}

	return bResult;
#endif
}

/*********************************************************
功能：	检查文件类型
参数：	szFileName 文件名称
返回：	详见枚举
修改：	
*********************************************************/
CMyCompress::EFileType CMyCompress::CheckFileType(const std::string &szFileName)
{
	EFileType eType = e_None;
	std::string szTemp(szFileName);

	if (!szTemp.empty() && "/" != szTemp)
	{
		//去掉结尾的/
		if ('/' == szTemp[szTemp.length() - 1] || '\\' == szTemp[szTemp.length() - 1])
		{
			szTemp.erase(szTemp.length() - 1);
		}

#ifdef _WIN32
		HANDLE hFind;
		WIN32_FIND_DATAA ff32;

		hFind = FindFirstFile(szTemp.c_str(), &ff32);
		if (INVALID_HANDLE_VALUE != hFind)
		{
			if (FILE_ATTRIBUTE_DIRECTORY & ff32.dwFileAttributes)
				eType = e_Dir;
			else
			{
				if (0 < strlen(ff32.cAlternateFileName))
				{
					std::string szTemp = ff32.cAlternateFileName;
					std::string::size_type iPos = szTemp.rfind(".LNK");

					if (std::string::npos != iPos && iPos == (szTemp.length() - 4))
						eType = e_Link;
					else
						eType = e_File;
				}
				else
					eType = e_File;
			}

			FindClose(hFind);
		}
#else
		struct stat st;

		if (0 == lstat(szTemp.c_str(), &st))
		{
			if (S_ISDIR(st.st_mode))
				eType = e_Dir;
			else if (S_ISLNK(st.st_mode))
				eType = e_Link;
			else
				eType = e_File;
		}
#endif
	}

	return eType;
}

/*********************************************************
功能：	判断文件是否存在
参数：	szFileName 文件名称
*		bLinkDereferenced 当是软连接时，是否引用源文件，linux有效
返回：	存在返回true
修改：
*********************************************************/
bool CMyCompress::CheckFileExists(const std::string &szFileName, bool bLinkDereferenced)
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
		if (!bLinkDereferenced) //当是软连接时，如果源文件删除了，access判断会返回-1，下面检查软连接文件自己是否存在
		{
			struct stat st;

			if (0 == lstat(szTemp.c_str(), &st))
			{
				if (S_ISLNK(st.st_mode)) //软连接
					return true;
			}
		}

		return false;
	}
#endif // _WIN32
}

/*********************************************************
功能：	复制文件
参数：	srcFile 源文件对象
*		szDstDir 目的文件夹，后面不带/
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::CPFile(const SFileInfo &srcFile, const std::string &szDstDir, std::string &szError)
{
	bool bResult = true;
	const std::string szInFileName = srcFile.szBaseDir + "/" + srcFile.szName;
	const std::string szDstFileName = szDstDir + "/" + srcFile.szName;

	//如果文件己存在，则删除
	if (!RmFile(szDstFileName, true, szError))
		return false;
	
	if (e_Dir == srcFile.eType) //目录
	{
		int iRet = 0;

#ifdef _WIN32
		iRet = _mkdir(szDstFileName.c_str());
#else
		iRet = mkdir(szDstFileName.c_str(), 0755);
#endif

		if (0 != iRet)
		{
			bResult = false;
			szError = "Create dir:" + szDstFileName + " failed:" + GetErrorMsg(errno);
		}
	}
	else if (e_File == srcFile.eType || e_Link == srcFile.eType) //文件
	{
		bool bIsLinuxLink = false;

#ifndef _WIN32
		if (e_Link == eSrcFileType)
			bIsLinuxLink = true; //linux软连接要特殊处理
#endif

		if (bIsLinuxLink)
		{
			const unsigned int size_buf = 2048;
			char buf[size_buf + 1] = { '\0' };

#ifndef _WIN32
			int iReadLink = readlink(szInFileName.c_str(), buf, size_buf);
			if (0 < iReadLink)
			{
				//如果目的目录名变化，这里可能会有问题
				iReadLink = symlink(buf, szDstFileName.c_str());

				if (0 != iReadLink)
				{
					bResult = false;
					szError = "Create link file:" + szDstFileName + " failed:" + GetErrorMsg(errno);
				}
			}
			else
			{
				bResult = false;
				szError = "Read link:" + szInFileName + " failed:" + GetErrorMsg(errno);
			}
#endif
		}
		else
		{
			std::ifstream inFile(szInFileName, std::ios::in | std::ios::binary);

			if (inFile.is_open() && inFile.good())
			{
				std::ofstream outFile(szDstFileName, std::ios::out | std::ios::trunc | std::ios::binary);

				if (outFile.is_open() && outFile.good())
				{
					std::streamsize readCount = 0;
					const unsigned int iBufSize = 10240;
					char buf[iBufSize + 1] = { '\0' };

					do
					{
						inFile.read(buf, iBufSize);
						readCount = inFile.gcount();

						if (readCount > 0)
						{
							outFile.write(buf, readCount);
							memset(buf, 0, iBufSize + 1);
						}
					} while (readCount > 0);

					outFile.close();
				}
				else
				{
					bResult = false;
					szError = "Open out file:" + szDstFileName + " failed !";
				}

				inFile.close();
			}
			else
			{
				bResult = false;
				szError = "Open in file:" + szInFileName + " failed !";
			}
		}
	}
	else
	{
		bResult = false;
		szError = "Unknow file type:" + std::to_string((int)srcFile.eType);
	}

	return bResult;
}

/*********************************************************
功能：	判断是否是大文件
参数：	szFileName 文件名称
*		llSize 文件大小，字节
返回：	0：不是，1：是
修改：
*********************************************************/
int CMyCompress::CheckFileIsLarge(const std::string &szFileName, ZPOS64_T *llSize)
{
	int largeFile = 0;
	ZPOS64_T pos = 0;
	std::string szTemp(szFileName);

	if (!szTemp.empty() && "/" != szTemp)
	{
		//去掉结尾的/
		if ('/' == szTemp[szTemp.length() - 1])
		{
			szTemp.erase(szTemp.length() - 1);
		}

		FILE* pFile = FOPEN_FUNC(szTemp.c_str(), "rb");

		if (pFile != NULL)
		{
			FSEEKO_FUNC(pFile, 0, SEEK_END);
			pos = FTELLO_FUNC(pFile);

			if (pos >= 0xffffffff)
				largeFile = 1;

			fclose(pFile);
		}
	}

	if (NULL != llSize)
		*llSize = pos;

	return largeFile;
}

/*********************************************************
功能：	判断文件压缩类型
参数：	szFileName 文件名称
*		tarLength gzip文件解压后，tar文件的长度
返回：	0：未知压缩类型，1：gzip文件，2：zip文件，3：rar文件
修改：
*********************************************************/
int CMyCompress::CheckFileCompressType(const std::string &szFileName, unsigned int *tarLength)
{
	int iCompressType = 0;

	if (szFileName.empty())
		return iCompressType;

	FILE *fl = FOPEN_FUNC(szFileName.c_str(), "rb");
	if (NULL == fl)
		return iCompressType;

	//先获取总长度，防止越界
	FSEEKO_FUNC(fl, 0, SEEK_END);
	long long len = FTELLO_FUNC(fl);
	unsigned char buf[16] = { 0 };

	if (len >= 7)
	{
		rewind(fl);

		//先读取前7表字节
		if (7 == fread(buf, 1, 7, fl))
		{
			if (0x1f == buf[0])
			{
				//gzip文件，RFC 1952标准
				//第1字节：0x1f，第2字节：0x8b，第10字节：系统类型，0:FAT,3:Unix,6:HPFS,11:NTFS,255:unknow
				//最后4个字节表示压缩前的原长度，最大2^32，小端字节序，即第一个字节是低位，第4个字节是高位

				if (0x8b == buf[1])
				{
					//判断长度
					FSEEKO_FUNC(fl, len - 4, SEEK_SET);
					if (4 == fread(buf, 1, 4, fl))
					{
						unsigned int tarLen = buf[0];

						for (int i = 1; i < 4; ++i)
						{
							tarLen += (buf[i] << (8 * i));
						}

						if (tarLen <= 0xFFFFFFFF)
						{
							iCompressType = 1;

							if (NULL != tarLength)
								*tarLength = tarLen;
						}
					}
				}
			}
			else if (0x50 == buf[0])
			{
				//zip文件，RFC 1950标准
				//前4个字节：0x50,0x4b,0x03,0x04

				if (0x4b == buf[1] && 0x03 == buf[2] && 0x04 == buf[3])
					iCompressType = 2;
			}
			else if (0x52 == buf[0])
			{
				//rar文件
				//前7个字节，0x52,0x61,0x72,0x21,0x1a,0x07,0x00

				if (0x61 == buf[1] && 0x72 == buf[2] && 0x21 == buf[3]
					&& 0x1a == buf[4] && 0x07 == buf[5] && 0x00 == buf[6]
					&& len > 7)
					iCompressType = 3;
			}
		}
	}

	fclose(fl);

	return iCompressType;
}

/*********************************************************
功能：	获取文件CRC32值，有加密才使用
参数：	szFileName 文件名称
*		buf 读取缓存
*		size_buf 读取缓存大小
*		result_crc 文件CRC32值，输出
*		bIsLink 是否软连接，linux使用
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::GetFileCrc(const std::string &szFileName, char* buf, unsigned long size_buf, unsigned long* result_crc, bool bIsLink,
	std::string &szError)
{
	unsigned long calculate_crc = 0;
	unsigned long size_read = 0;
	std::string szTemp(szFileName);
	bool bRet = true,  bNormalGet = true;

#ifndef _WIN32
	if (bIsLink)
		bNormalGet = false;
#endif

	if (!szTemp.empty() && "/" != szTemp)
	{
		//去掉结尾的/
		if ('/' == szTemp[szTemp.length() - 1])
		{
			szTemp.erase(szTemp.length() - 1);
		}
		
		if (bNormalGet)
		{
			FILE * fin = FOPEN_FUNC(szTemp.c_str(), "rb");

			if (fin != NULL)
			{
				do
				{
					size_read = (int)fread(buf, 1, size_buf, fin);
					if (size_read < size_buf)
					{
						if (feof(fin) == 0)
						{
							bRet = false;
							szError = "Read file:" + szTemp + " end, check eof failed:" + GetErrorMsg(errno);
						}
					}

					if (size_read > 0)
						calculate_crc = crc32(calculate_crc, (const Bytef*)buf, size_read);

				} while (bRet && (size_read > 0));

				if (fin)
					fclose(fin);

				*result_crc = calculate_crc;
			}
			else
			{
				bRet = false;
				szError = "Open file:" + szTemp + " failed:" + GetErrorMsg(errno);
			}
		}
		else
		{
#ifndef _WIN32
			size_read = readlink(szTemp.c_str(), buf, size_buf);
#endif

			if (0 < size_read)
			{
				calculate_crc = crc32(calculate_crc, (const Bytef*)buf, size_read);
				*result_crc = calculate_crc;
			}
			else
			{
				bRet = false;
				szError = "Read link file:" + szTemp + " failed:" + GetErrorMsg(errno);
			}
		}
	}
	else
	{
		bRet = false;
		szError = "File name is empty or wrong !";
	}

	return bRet;
}

/*********************************************************
功能：	设置压缩文件的时间
参数：	info 文件属性信息
*		file 压缩文件
返回：	无
修改：
*********************************************************/
void CMyCompress::FillFileTime(const SFileInfo &info, zip_fileinfo &file)
{
	time_t tm_t = 0;
	struct tm filedate;

	memset(&file, 0, sizeof(file));

#ifdef _WIN32
	file.dosDate = info.ulDosWriteTime;
	tm_t = info.llWriteTime;
	localtime_s(&filedate, &tm_t);
#else
	tm_t = info.llWriteTime;
	localtime_r(&tm_t, &filedate);	
#endif

	file.tmz_date.tm_sec = filedate.tm_sec;
	file.tmz_date.tm_min = filedate.tm_min;
	file.tmz_date.tm_hour = filedate.tm_hour;
	file.tmz_date.tm_mday = filedate.tm_mday;
	file.tmz_date.tm_mon = filedate.tm_mon;
	file.tmz_date.tm_year = filedate.tm_year;
}

/*********************************************************
功能：	修改文件的时间
参数：	szFileName 文件名
*		bIsDir 是否是目录
*		dosdate dos时间，win32使用
*		tmu_date tmu时间
*		utc_mtime linux的文件utc mtime，不为空时使用
返回：	无
修改：
*********************************************************/
void CMyCompress::ChangeFileTime(const std::string &szFileName, bool bIsDir, unsigned long dosdate, const tm_unz *tmu_date,
	time_t *utc_mtime)
{
#ifdef _WIN32
	HANDLE hFile;
	FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;

	if (bIsDir)
		hFile = CreateFile(szFileName.c_str(), GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	else
		hFile = CreateFile(szFileName.c_str(), GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE != hFile)
	{
		::GetFileTime(hFile, &ftCreate, &ftLastAcc, &ftLastWrite);

		if (NULL == utc_mtime)
		{
			DosDateTimeToFileTime((WORD)(dosdate >> 16), (WORD)dosdate, &ftLocal);
		}
		else
		{
			LONGLONG ll = Int32x32To64(UTCTimeToLocal(*utc_mtime), 10000000) + 116444736000000000;
			ftLocal.dwLowDateTime = (DWORD)ll;
			ftLocal.dwHighDateTime = (DWORD)(ll >> 32);
		}

		LocalFileTimeToFileTime(&ftLocal, &ftm);
		::SetFileTime(hFile, &ftm, &ftLastAcc, &ftm);
		CloseHandle(hFile);
	}
#else
	struct utimbuf ut;

	if (NULL == utc_mtime)
	{
		struct tm newdate;
		newdate.tm_sec = tmu_date->tm_sec;
		newdate.tm_min = tmu_date->tm_min;
		newdate.tm_hour = tmu_date->tm_hour;
		newdate.tm_mday = tmu_date->tm_mday;
		newdate.tm_mon = tmu_date->tm_mon;
		if (tmu_date->tm_year > 1900)
			newdate.tm_year = tmu_date->tm_year - 1900;
		else
			newdate.tm_year = tmu_date->tm_year;
		newdate.tm_isdst = -1;

		ut.actime = ut.modtime = mktime(&newdate);
	}
	else
		ut.actime = ut.modtime = UTCTimeToLocal(*utc_mtime);

	utime(szFileName.c_str(), &ut);
#endif
}

/*********************************************************
功能：	根据错误码取得错误信息
参数：	errCode 错误码
返回：	错误信息字符串
*********************************************************/
std::string CMyCompress::GetErrorMsg(int errCode)
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
功能：	删除文件
参数：	szFileName 文件名
*		bChilds 如果目录不为空，删除子文件
*		szError 错误信息
返回：	成功返回true
*********************************************************/
bool CMyCompress::RmFile(const std::string &szFileName, bool bChilds, std::string &szError)
{
	if (!CheckFileExists(szFileName))
		return true;

	EFileType eType = CheckFileType(szFileName);

	//删除单个文件方法
	auto f_delOneFile = [](const std::string &szPathName, bool bDir, std::string &szErr)->bool
	{
		if (szPathName.empty())
		{
			szErr = "File path name is empty !";
			return false;
		}

#ifdef _WIN32
		if (bDir)
		{
			if (FALSE == RemoveDirectory(szPathName.c_str()))
			{
				szErr = "Remove dir:" + szPathName + " failed:" + GetErrorMsg(GetLastError());
				return false;
			}
			else
				return true;
		}
#endif

		if (0 != remove(szPathName.c_str()))
		{
			int errNo = 0;

#ifdef _WIN32
			errNo = GetLastError();
#else
			errNo = errno;
#endif
			szErr = "Remove file:" + szPathName + " failed:" + GetErrorMsg(errNo);
			return false;
		}

		return true;
	};

	if (bChilds && e_Dir == eType)
	{
		std::string szTemp(szFileName);
		std::list<CMyCompress::SDirFiles> FileList;

		for (auto &ch : szTemp)
		{
			if ('\\' == ch)
				ch = '/';
		}

		if ('/' == szTemp[szTemp.length() - 1])
			szTemp.erase(szTemp.length() - 1);

		//递归删除方法
		std::function<bool(const std::string&, const CMyCompress::SDirFiles&, std::string &szError)> f_delFiles;
		f_delFiles = [&f_delFiles, &f_delOneFile](const std::string &szPath, const CMyCompress::SDirFiles &file, std::string &szError) -> bool
		{
			std::string szFilePath = szPath + "/" + file.szFileName;

			//删除子文件
			if (file.bIsDir)
			{
				for (const auto &f : file.DirFiles)
				{
					if (!f_delFiles(szFilePath, f, szError))
						return false;
				}
			}

			//删除自己
			return f_delOneFile(szFilePath, file.bIsDir, szError);
		};

		//获取目录文件
		if (GetDirFiles(szTemp, FileList, szError))
		{
			//删除子文件
			for (const auto &f : FileList)
			{
				if (!f_delFiles(szTemp, f, szError))
					return false;
			}
		}
		else
		{
			szError = "Get dir files failed:" + szError;
			return false;
		}
	}

	//删除自己
	return f_delOneFile(szFileName, (e_Dir == eType ? true : false), szError);
}

/*********************************************************
功能：	创建文件夹
参数：	szDir 目录名
*		iMode 权限，linux使用
*		bParents 是否递归创建
*		szError 错误信息
返回：	成功返回true
*********************************************************/
bool CMyCompress::MkDir(const std::string &szDir, unsigned int iMode, bool bParents, std::string &szError)
{
	if (szDir.empty())
	{
		szError = "Dir name is empty !";
		return false;
	}
	else if ("/" == szDir || "\\" == szDir || "." == szDir || ".." == szDir)
	{
		szError = "Dir name:" + szDir + " is wrong !";
		return false;
	}

	//创建单个目录方法
	auto mk_dir = [](const std::string &szDirName, unsigned int mode, std::string &error) -> bool 
	{
		if (CMyCompress::CheckFileExists(szDirName))
			return true;

		int iRet = 0;

#ifdef _WIN32
		iRet = _mkdir(szDirName.c_str());
#else
		iRet = mkdir(szDirName.c_str(), (mode_t)mode);
#endif

		if (0 != iRet)
		{
			error = "Create dir:" + szDirName + " failed:" + CMyCompress::GetErrorMsg(errno);
			return false;
		}

		return true;
	};

	if (bParents)
	{
		std::string::size_type iPos = 0;

		iPos = szDir.find("/", 1);
		if (std::string::npos == iPos)
			iPos = szDir.find("\\", 1);

		if (std::string::npos == iPos)
			return mk_dir(szDir, iMode, szError);
		else
		{
			do
			{
				if (mk_dir(szDir.substr(0, iPos), iMode, szError))
				{
					iPos = szDir.find("/", iPos + 1);
					if (std::string::npos == iPos)
						iPos = szDir.find("\\", iPos + 1);
				}
				else
					return false;
			} while (iPos != std::string::npos);
		}
	}
	else
		return mk_dir(szDir, iMode, szError);

	return true;
}

/*********************************************************
功能：	修改文件属性
参数：	szFile 文件名
*		iMode 权限，linux使用
*		szError 错误信息
返回：	成功返回true
*********************************************************/
bool CMyCompress::Chmod(const std::string &szFile, unsigned int iMode, std::string &szError)
{
	if (szFile.empty())
	{
		szError = "Chmod error, file name is empty !";
		return false;
	}

	if (!CheckFileExists(szFile))
	{
		szError = "Chmod file:" + szFile + " is not exists !";
		return false;
	}

	int iRet = 0;

#ifdef _WIN32
	iRet = _chmod(szFile.c_str(), iMode);
#else
	iRet = chmod(szFile.c_str(), (mode_t)iMode);
#endif

	if (0 != iRet)
	{
		szError = "Chmod file:" + szFile + " failed:" + GetErrorMsg(errno);
		return false;
	}

	return true;
}

/*********************************************************
功能：	修改文件所有者
参数：	szFile 文件名
*		szUser 用户，linux使用
*		szError 错误信息
返回：	成功返回true
*********************************************************/
bool CMyCompress::Chown(const std::string &szFile, const std::string &szUser, std::string &szError)
{
#ifndef _WIN32
	if (szFile.empty())
	{
		szError = "Chown error, file name is empty !";
		return false;
	}

	if (szUser.empty())
	{
		szError = "Chown error, user name is empty !";
		return false;
	}

	if (!CheckFileExists(szFile))
	{
		szError = "Chown file:" + szFile + " is not exists !";
		return false;
	}

	int uid, gid;
	std::string gid_name, szLoginUser;

	if (GetUserInfo(szUser, uid, gid, gid_name, szLoginUser, szError))
	{
		//用户名不同才处理
		if (szLoginUser != szUser)
		{
			if (0 != lchown(szFile.c_str(), uid, gid))
			{
				szError = "Chown error:" + GetErrorMsg(errno);
				return false;
			}
		}
	}
	else
		return false;
#endif

	return true;
}

/*********************************************************
功能：	获取路径信息
参数：	szPath 路径，如./,../xx,tx/
*		szResolvedPath 全路径
*		szError 错误信息
返回：	成功返回true
*********************************************************/
bool CMyCompress::GetRealPath(const std::string &szPath, std::string &szResolvedPath, std::string &szError)
{
	if (szPath.empty())
	{
		szError = "Path is empty !";
		return false;
	}

	char *p = nullptr;
	char buf[1024] = { 0 };

#ifdef _WIN32
	p = _fullpath(buf, szPath.c_str(), 1023);
#else
	p = realpath(szPath.c_str(), buf);
#endif

	if (nullptr != p)
		szResolvedPath = p;
	else
	{
		szError = "Path:" + szPath + " is wrong !";
		return false;
	}

	return true;
}

/*********************************************************
功能：	GZ压缩
参数：	fileList 待压缩文件列表
*		szGZFileName 压缩后的tar.gz文件名称
*		eLevel 压缩级别
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::CompressGZIP(const std::list<std::string> &fileList, const std::string &szGZFileName, ECompressLevel eLevel,
	std::string &szError)
{
	bool bResult = true;
	const char *pwd = NULL;
	unsigned long size_buf = WRITEBUFFERSIZE;
	char *buf = NULL;

	//检查参数
	if (fileList.empty())
	{
		szError = "File to compress list is empty !";
		return false;
	}

	if (szGZFileName.empty())
	{
		szError = "Dst compress file name is empty !";
		return false;
	}

	//如果压缩包已存在，则删除
	RmFile(szGZFileName, false, szError);

	buf = new char[size_buf] { 0 };
	if (NULL == buf)
	{
		szError = "New memory buf return nullptr !";
		return false;
	}

	if (!m_szPassword.empty())
	{
		//设置密码
		pwd = m_szPassword.c_str();
	}

	//打开文件
	std::string szOpenMode = "wb";
	if (eLevel != e_Level_Default)
		szOpenMode += std::to_string((int)eLevel);

	gzFile gzfout = gzopen(szGZFileName.c_str(), szOpenMode.c_str());
	if (nullptr == gzfout)
	{
		delete[] buf;
		return false;
	}
	else
		gzbuffer(gzfout, WRITEBUFFERSIZE);
	
	//处理文件
	for (const auto& file : fileList)
	{
		SFileInfo fileInfo;
		std::string szNameTemp(file);

		//获取真实路径
		if (std::string::npos != szNameTemp.find("..\\") || std::string::npos != szNameTemp.find("../"))
		{
			if (!GetRealPath(szNameTemp, szNameTemp, szError))
			{
				bResult = false;
				szError = "Get file real path failed:" + szError;
				break;
			}
		}

		//获取文件信息
		if (!GetFileInfo(szNameTemp, fileInfo, szError))
		{
			bResult = false;
			szError = "Get file info failed:" + szError;
			break;
		}

		if (e_Dir == fileInfo.eType)
		{
			if ('\\' == szNameTemp[szNameTemp.length() - 1] || '/' == szNameTemp[szNameTemp.length() - 1])
				szNameTemp.erase(szNameTemp.length() - 1);
		}

		//处理文件
		if (!do_compressGZIP(gzfout, szNameTemp, fileInfo.szName, fileInfo, pwd, buf, size_buf, szError))
		{
			bResult = false;
			szError = "Do compress failed:" + szError;
			break;
		}
	}

	//最后补一个空0块
	if (bResult)
	{
		memset(buf, 0, size_buf);
		bResult = WriteGzipCompressData(gzfout, buf, T_BLOCKSIZE, szError);
	}

	//关闭文件
	gzclose(gzfout);

	delete[] buf;
	buf = NULL;

	if (!bResult)
		RmFile(szGZFileName, false, szError);
	else
	{
		//修改文件头，设置时间及系统类型，gzopen系列方法无法设置文件头
		FILE *fx = FOPEN_FUNC(szGZFileName.c_str(), "r+");
		if (nullptr != fx)
		{
			FSEEKO_FUNC(fx, 4, SEEK_SET);

			//设置时间，小端
			char ch;
			time_t ti = time(NULL);

			for (int i = 0; i < 4; ++i)
			{
				ch = (0xFF & ti >> (i * 8));
				fwrite(&ch, 1, 1, fx);
			}

			//设置系统类型
			FSEEKO_FUNC(fx, 1, SEEK_CUR);

#ifdef _WIN32
			ch = 0x0B; //NTFS
#else
			ch = 0x03; //unix
#endif
			fwrite(&ch, 1, 1, fx);

			fclose(fx);
		}

#ifndef _WIN32
		if (!m_szUserName.empty())
			Chown(szGZFileName, m_szUserName, szError);
#endif
	}

	return bResult;
}

/*********************************************************
功能：	gz解压缩处理
参数：	szGZFileName gz包文件名称
*		szDstDir 解压缩到的目录，空表示当前目录
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::UncompressGZIP(const std::string &szGZFileName, const std::string &szDstDir, std::string &szError)
{
	std::string szDstDirTemp(szDstDir);
	unsigned int iTarFileLength = 0;
	unsigned long size_buf = WRITEBUFFERSIZE;
	char *buf = NULL;

	//检查参数
	if (szGZFileName.empty())
	{
		szError = "GZ file name is empty !";
		return false;
	}
	else
	{
		if (!CheckFileExists(szGZFileName))
		{
			szError = "GZ file is not exists !";
			return false;
		}
		else
		{
			if (1 != CheckFileCompressType(szGZFileName, &iTarFileLength))
			{
				szError = "The file:" + szGZFileName + " is not a gzip file !";
				return false;
			}
		}
	}

	if (szDstDirTemp.empty())
	{
		//空表示当前目录
		szDstDirTemp = ".";
	}
	else
	{
		if (!CheckFileExists(szDstDirTemp))
		{
			szError = "GZ uncompress dst dir is not exists !";
			return false;
		}
		else
		{
			if (e_Dir != CheckFileType(szDstDirTemp))
			{
				szError = "GZ uncompress dst dir:" + szDstDirTemp + " is not a dir !";
				return false;
			}
			else
			{
				if ('\\' == szDstDirTemp[szDstDirTemp.length() - 1] || '/' == szDstDirTemp[szDstDirTemp.length() - 1])
					szDstDirTemp.erase(szDstDirTemp.length() - 1);

				for (std::string::size_type i = 0; i < szDstDirTemp.length(); ++i)
				{
					if ('\\' == szDstDirTemp[i])
						szDstDirTemp[i] = '/';
				}
			}
		}
	}

	buf = new char[size_buf] { 0 };
	if (NULL == buf)
	{
		szError = "New memory buf return nullptr !";
		return false;
	}

	bool bResult = true;
	int iRet = 0;
	gzFile gzfin = NULL;
	tar_record tarHead;
	const char *pwd = NULL;
	unsigned int iHandlePos = 0, read_data_size = 0;
	std::list<SLinkInfo> linkList;

	//打开gz文件
	gzfin = gzopen(szGZFileName.c_str(), "rb");
	if (NULL == gzfin)
	{
		szError = "Open gz file failed !";
		delete[] buf;
		return false;
	}
	else
		gzbuffer(gzfin, WRITEBUFFERSIZE);

	if (!m_szPassword.empty())
		pwd = m_szPassword.c_str();

	//读取文件内容，获得tar文件并处理
	do
	{
		//该方法会读取并解压，读取头
		iRet = gzread(gzfin, &(tarHead.data), T_BLOCKSIZE);

		if (iRet == T_BLOCKSIZE)
		{
			iHandlePos += T_BLOCKSIZE;

			if ('\0' == tarHead.head.name[0])
			{
				if (iHandlePos < iTarFileLength)
					continue;
				else if (iHandlePos >= iTarFileLength)
					break;
			}

			//处理一个文件
			if (!do_uncompressGZIP(gzfin, tarHead, szDstDirTemp, pwd, buf, size_buf, read_data_size, linkList, szError))
			{
				bResult = false;
				break;
			}

			iHandlePos += read_data_size;
		}
		else
		{
			if (gzeof(gzfin) == 0)
			{
				bResult = false;
				szError = std::string("Read gzip file check ending filed, ret:") + gzerror(gzfin, &iRet);
				break;
			}
			else
			{
				if (iRet < T_BLOCKSIZE)
				{
					bResult = false;
					szError = std::string("Read gzip file filed, ret:") + std::to_string(iRet);
					break;
				}
			}
		}
	} while (bResult && iRet > 0);

	//创建软连接
	if (bResult && !linkList.empty())
	{
#ifndef _WIN32
		std::string szTempError;

		for (const auto &link : linkList)
		{
			RmFile(link.szLink, false, szTempError);
			iRet = symlink(link.szPath.c_str(), link.szLink.c_str());

#ifdef _DEBUG
			printf("create link:%s\n", link.szLink.c_str());
#endif

			if (0 != iRet)
			{
				bResult = false;
				szError = "Create link file:" + link.szLink + " failed:" + GetErrorMsg(errno);
				break;
			}
			else
			{
				if (!m_szUserName.empty())
					Chown(link.szLink, m_szUserName, szError);
			}
		}
#endif
	}

	//关闭gz文件
	gzclose(gzfin);

	delete[] buf;
	buf = NULL;

	return bResult;
}

/*********************************************************
功能：	压缩处理
参数：	gz tar.gz包文件
*		szFileName 文件名称
*		szGzipFileName zip文件名称
*		fileInfo 文件信息
*		pwd 压缩密码
*		buf 缓存
*		size_buf 缓存大小
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::do_compressGZIP(gzFile gz, const std::string &szFileName, const std::string &szGzipFileName, const SFileInfo &fileInfo,
	const char* pwd, char* buf, unsigned long size_buf, std::string &szError)
{
	bool bResult = true, bWriteData = false;
	tar_record head;

#ifdef _DEBUG
	printf("%s\n", szFileName.c_str());
#endif

	memset(head.data, 0, T_BLOCKSIZE);
	memcpy(head.head.name, szGzipFileName.c_str(), szGzipFileName.length());

	head.head.mode[6] = T_ENDCHAR;
#ifdef _WIN32
	if (e_Dir == fileInfo.eType)
		DecimalToOctal(0755, head.head.mode, 6);
	else
		DecimalToOctal(0644, head.head.mode, 6);
#else
	DecimalToOctal(fileInfo.iAttrib, head.head.mode, 6);
#endif

	head.head.size[11] = T_ENDCHAR;

	if (e_Dir == fileInfo.eType)
	{
		head.head.name[szGzipFileName.length()] = '/'; //目录补上结尾'/'
		head.head.typeflag = '5';
		DecimalToOctal(0, head.head.size, 11);
	}
	else if (e_Link == fileInfo.eType)
	{
#ifdef _WIN32
		bWriteData = true;

		head.head.typeflag = '0';
		DecimalToOctal((unsigned int)fileInfo.ullSize, head.head.size, 11);
#else
		head.head.typeflag = '2';
		DecimalToOctal(0, head.head.size, 11);

		int iReadLink = readlink(szFileName.c_str(), buf, size_buf);
		if (0 < iReadLink)
			memcpy(head.head.linkname, buf, iReadLink);
		else
		{
			szError = "Read link:" + szFileName + " failed:" + GetErrorMsg(errno);
			return false;
		}
#endif
	}
	else
	{
		bWriteData = true;

		head.head.typeflag = '0';
		DecimalToOctal((unsigned int)fileInfo.ullSize, head.head.size, 11);
	}

	head.head.mtime[11] = T_ENDCHAR;
	DecimalToOctal((unsigned int)fileInfo.llWriteTime, head.head.mtime, 11);

	memcpy(head.head.magic, "ustar", 5);

	FillUserInfo(head); //获取用户信息，主要是linux使用
	TarHeaderCheckSum(head);

	//写入头
	if (!WriteGzipCompressData(gz, head.data, T_BLOCKSIZE, szError))
		return false;

	//写入数据
	if (bWriteData && fileInfo.ullSize > 0)
	{
		FILE *ffile = FOPEN_FUNC(szFileName.c_str(), "rb");
		size_t iReadTempLen = 0, iReadTotalLen = 0;

		if (NULL != ffile)
		{
			do
			{
				iReadTempLen = fread(buf, 1, size_buf, ffile);
				
				if (iReadTempLen > 0)
				{
					if (!WriteGzipCompressData(gz, buf, (unsigned int)iReadTempLen, szError))
					{
						bResult = false;
						break;
					}
					else
						iReadTotalLen += iReadTempLen;
				}
			} while (iReadTempLen > 0 && iReadTotalLen < fileInfo.ullSize);

			fclose(ffile);

			//填充
			if (0 != (iReadTotalLen % T_BLOCKSIZE))
			{
				unsigned int uiFillLen = T_BLOCKSIZE - (iReadTotalLen % T_BLOCKSIZE);
				memset(buf, 0, size_buf);

				if (!WriteGzipCompressData(gz, buf, uiFillLen, szError))
					bResult = false;
			}
		}
		else
		{
			bResult = false;
			szError = "Open file:" + szFileName + " for read failed:" + GetErrorMsg(errno);
		}
	}

	//处理文件夹
	if (bResult && e_Dir == fileInfo.eType)
	{
		std::list<SFileInfo> fileList;

		if (GetDirFiles(szFileName, fileList, szError, false))
		{
			std::string szNameTemp;

			for (const auto &f : fileList)
			{
#ifdef _WIN32
				szNameTemp = szFileName + "\\" + f.szName;
#else
				szNameTemp = szFileName + "/" + f.szName;
#endif
				bResult = do_compressGZIP(gz, szNameTemp, (szGzipFileName + "/" + f.szName), f, pwd, buf, 
					size_buf, szError);

				if (!bResult)
				{
					//失败就结束
					break;
				}
			}
		}
		else
			bResult = false;
	}

	return bResult;
}

/*********************************************************
功能：	写入GZIP压缩数据
参数：	gz tar.gz文件句柄
*		flush flush标志
*		data 原始数据
*		data_len 原始数据长度
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::WriteGzipCompressData(gzFile gz, char *data, unsigned int data_len, std::string &szError)
{
	//gwrite会自动处理压缩
	int ret = gzwrite(gz, data, data_len);
	
	if ((unsigned int)ret != data_len)
	{
		szError = std::string("Write data to compressed file failed:") + gzerror(gz, &ret);
		return false;
	}

	return true;
}

/*********************************************************
功能：	解压gzip处理
参数：	gz gzip文件句柄
*		file_head 文件头
*		szDstDirName 解压到的目录
*		pwd 密码
*		buf 缓存
*		size_buf 缓存长度
*		iReadDataLength 己读取数据长度
*		LinkList 软连接列表,linux使用
*		szError 错误信息
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::do_uncompressGZIP(gzFile gz, const tar_record &file_head, const std::string &szDstDirName, const char* pwd, char* buf,
	unsigned long size_buf, unsigned int &iReadDataLength, std::list<SLinkInfo> &LinkList, std::string &szError)
{
	bool bResult = true;
	std::string szTarFileName = szDstDirName + "/" + file_head.head.name;
	bool bIsDir = false, bNormalWrite = true;
	unsigned int file_mode = OctalToDecimal(file_head.head.mode);
	unsigned int data_size = OctalToDecimal(file_head.head.size);
	time_t file_mtime = std::strtoll(file_head.head.mtime, nullptr, 8);

#ifdef _DEBUG
	printf("%s\n", szTarFileName.c_str());
#endif

#ifndef _WIN32
	if ('2' == file_head.head.typeflag) //软连接
		bNormalWrite = false;
#endif

	//处理
	if (bNormalWrite)
	{
		if ('5' == file_head.head.typeflag) //目录
		{
			bIsDir = true;
			iReadDataLength = 0;

			//windows上压缩的没有属性
#ifndef _WIN32
			if (0 == file_mode)
				file_mode = 0755;
#endif

			if (!MkDir(szTarFileName, file_mode, false, szError))
			{
				bResult = false;
			}
		}
		else //其它文件
		{
			//打开文件准备写
			FILE *fo = FOPEN_FUNC(szTarFileName.c_str(), "wb+");
			if (NULL == fo)
			{
				bResult = false;
				szError = "Open file:" + szTarFileName + " for write failed:" + GetErrorMsg(errno);
			}
			else
			{
				//软连接长度为0，连接原地址为linkname，windows直接写入
				if ('2' == file_head.head.typeflag)
				{
					iReadDataLength = 0;
					fwrite(file_head.head.linkname, 1, strlen(file_head.head.linkname), fo);
				}
				else
				{
					//windows上压缩的没有属性
#ifndef _WIN32
					if (0 == file_mode)
						file_mode = 0644;
#endif

					if (0 != data_size)
					{
						if (0 != (data_size % T_BLOCKSIZE))
							iReadDataLength = (data_size / T_BLOCKSIZE + 1) * T_BLOCKSIZE;
						else
							iReadDataLength = data_size;
					}
					else
						iReadDataLength = 0;

					if (iReadDataLength > 0)
					{
						int iRet = 0;
						unsigned int iTemp = 0, iReadTemp = iReadDataLength, iWriteTemp = data_size;

						do
						{
							//该方法会自动解压成tar数据，读取数据
							iRet = gzread(gz, buf, (iReadTemp > size_buf ? size_buf : iReadTemp));

							if (iRet >= 0)
							{
								//写入数据到文件
								iTemp = (unsigned int)fwrite(buf, 1, ((unsigned int)iRet < iWriteTemp ? (unsigned int)iRet : iWriteTemp), fo);

								iReadTemp -= (unsigned int)iRet;
								iWriteTemp -= iTemp;
							}
							else
							{
								bResult = false;
								szError = std::string("Read file data from gzip failed:") + gzerror(gz, &iRet);
								break;
							}
						} while (iReadTemp > 0 && iWriteTemp > 0);
					}
				}

				fclose(fo);
			}
		}

		//修改时间及权限
		if (bResult)
		{
			ChangeFileTime(szTarFileName, bIsDir, 0, NULL, &file_mtime);
#ifndef _WIN32
			if (!bIsDir)
				bResult = Chmod(szTarFileName, file_mode, szError);

			if (!m_szUserName.empty())
				Chown(szTarFileName, m_szUserName, szError);
#endif
		}
	}
	else
	{
		SLinkInfo info;

		info.szPath = file_head.head.linkname;
		info.szLink = szTarFileName;
		info.lMode = file_mode;

		iReadDataLength = 0;
		LinkList.push_back(info);
	}

	return bResult;
}

/*********************************************************
功能：	设置用户信息
参数：	record 文件头
返回：	无
修改：
*********************************************************/
void CMyCompress::FillUserInfo(tar_record &head)
{
	head.head.uid[6] = T_ENDCHAR;
	head.head.gid[6] = T_ENDCHAR;

#ifndef _WIN32
	int uid, gid;
	std::string gid_name, login_user, szError;

	if (GetUserInfo("", uid, gid, gid_name, login_user, szError))
	{
		DecimalToOctal(uid, head.head.uid, 6);
		DecimalToOctal(gid, head.head.gid, 6);
		memcpy(head.head.uname, login_user.c_str(), login_user.length());
		memcpy(head.head.gname, gid_name.c_str(), gid_name.length());
	}
#endif
}

/*********************************************************
功能：	获取用户信息
参数：	szUserName 要获取的用户名，如果为空，则是当前用户
*		uid 用户ID
*		gid 用户组ID
*		gid_name 用户组名
*		szLoginUserName 当前登陆用户名
返回：	成功返回true
修改：
*********************************************************/
bool CMyCompress::GetUserInfo(const std::string &szUserName, int &uid, int &gid, std::string &gid_name,
	std::string &szLoginUserName, std::string &szError)
{
	std::string szUserNameTemp(szUserName);

#ifndef _WIN32
	struct passwd pwd;
	struct passwd *pas = NULL;
	char buf[512] = { 0 };

	//获取当前登陆用户名称
	int iRet = getpwuid_r(getuid(), &pwd, buf, 512, &pas);
	if (nullptr != pas)
		szLoginUserName = pas->pw_name;
	else
	{
		szError = "Get current login user name failed:" + GetErrorMsg(iRet);
		return false;
	}

	//如果为空取当前用户
	if (szUserNameTemp.empty())
	{
		szUserNameTemp = szLoginUserName;
		uid = pas->pw_uid;
		gid = pas->pw_gid;
	}
	else
	{
		//获取用户信息
		iRet = getpwnam_r(szUserNameTemp.c_str(), &pwd, buf, 512, &pas);
		if (nullptr != pas)
		{
			uid = pas->pw_uid;
			gid = pas->pw_gid;
		}
		else
		{
			szError = "Get user:" + szUserNameTemp + " info failed, user is not exists !";
			return false;
		}
	}

	struct group pgp;
	struct group *pgs = NULL;

	//获取用户组名
	iRet = getgrgid_r(gid, &pgp, buf, 512, &pgs);
	if (nullptr != pgs)
		gid_name = pgs->gr_name;
	else
	{
		szError = "Get user:" + szUserNameTemp + " group name failed, group:" + std::to_string(gid)
			+ " is not exists !";
		return false;
	}

	return true;
#else
	szError = "WIN32 is not support !";
	return false;
#endif
}

/*********************************************************
功能：	八进制串转十进制数字int
参数：	buf 八进制串
*		buf_len 串长度
返回：	十进制数字
修改：
*********************************************************/
unsigned int CMyCompress::OctalToDecimal(const char *buf)
{
	if (nullptr == buf)
		return 0;

	unsigned int iRet = 0;

	sscanf(buf, "%o-", &iRet);

	return iRet;
}

/*********************************************************
功能：	十进制数字int转八进制串
参数：	deci 十进制数
*		buf 八进制串
*		buf_len buf长度
返回：	无
修改：
*********************************************************/
void CMyCompress::DecimalToOctal(unsigned int deci, char *buf, unsigned int buf_len)
{
	std::string szFormat = "%0" + std::to_string(buf_len) + "o";
	char bufTemp[64] = { 0 };

	//snprintf会将格式串后一个字节置0，所以不能直接用buf
	snprintf(bufTemp, sizeof(bufTemp), szFormat.c_str(), deci);
	memcpy(buf, bufTemp, buf_len);
}

/*********************************************************
功能：	计算tar文件头的checksum
参数：	record 文件头
返回：	无
修改：
*********************************************************/
void CMyCompress::TarHeaderCheckSum(tar_record &record)
{
	unsigned int sum = 0;

	//计算校验和时，chksum字段使用空格填充
	memcpy(record.head.chksum, "        ", 8);

	//求和
	for (int i = 0; i < T_BLOCKSIZE; ++i)
	{
		sum += (unsigned char)record.data[i];
	}

	record.head.chksum[7] = ' ';

	//转8进制，snprintf会把data[6]置0，正好是想要的
	snprintf(record.head.chksum, 7, "%06o", sum);
}

/*********************************************************
功能：	UTC时间转本地时间
参数：	utcTime UTC时间
返回：	本地时间
修改：
*********************************************************/
time_t CMyCompress::UTCTimeToLocal(const time_t utcTime)
{
	time_t tUTC, tLocal, tTemp = time(NULL);
	struct tm st;

#ifdef _WIN32
	localtime_s(&st, &tTemp);
#else
	localtime_r(&tTemp, &st);
#endif

	//处理时区
	tUTC = tTemp % (24 * 60 * 60); //UTC当前秒数
	tLocal = st.tm_hour * 3600 + st.tm_min * 60 + st.tm_sec; //本地当前秒数

	return (utcTime + tLocal - tUTC);
}
