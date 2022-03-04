#include "../include/OS.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#include <direct.h>
#include <TlHelp32.h>
#include <shlwapi.h>
#include <IPHlpApi.h>
#include <Psapi.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#endif

COS::COS()
{
}

COS::~COS()
{
}

/*********************************************************************
功能：	获取系统错误信息
参数：	err_code 系统错误码
返回：	错误信息
修改：
*********************************************************************/
std::string COS::GetErrnoMsg(int err_code)
{
	std::string szRet;
	char buff[2048] = { '\0' };

#ifdef _WIN32
	if (0 == strerror_s(buff, sizeof(buff), err_code))
		szRet = buff;
	else
		szRet = "strerror_s work wrong, source error code:" + std::to_string(err_code);
#else
	char *p_error = strerror_r(err_code, buff, sizeof(buff));
	if (nullptr != p_error)
		szRet = p_error;
	else
		szRet = "strerror_r work wrong, source error code:" + std::to_string(err_code);
#endif

	return std::move(szRet);
}

/*********************************************************************
功能：	获取执行命令结果
参数：	szCommand        命令
*		szResult         执行结果串
*		szError          错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool COS::Popen(const std::string &szCommand, std::string &szResult, std::string &szError)
{
	if (szCommand.empty())
	{
		szError = "command string is empty !";
		return false;
	}

	FILE *fp = nullptr;

	//执行命令
#ifdef _WIN32
	fp = _popen(szCommand.c_str(), "r");
#else
	fp = popen(szCommand.c_str(), "r");
#endif

	if (fp != nullptr)
	{
		szResult.clear();
		const unsigned buf_len = 1024;
		char buf[buf_len] = { '\0' };

		//读取结果内容
		while (fgets(buf, buf_len, fp) != nullptr)
		{
			szResult += buf;
			memset(buf, '\0', buf_len);
		}

		int iRet = 0;

		//关闭管道
#ifdef _WIN32
		iRet = _pclose(fp);
#else
		iRet = pclose(fp);
#endif

		if (iRet < 0)
		{
			szError = std::string("shell executed return error: ") + GetErrnoMsg(errno);
			return false;
		}
		else
		{
#ifndef _WIN32
			char cBashRet = WIFEXITED(iRet); //bash退出状态，非零表示成功

			if (0 != cBashRet)
			{
				char cCmdRet = WEXITSTATUS(iRet); //cmd退出状态

				if (0 != cCmdRet)
				{
					szError = "shell executed return errcode: " + std::to_string(cCmdRet);
					if (!szResult.empty())
					{
						szError += " msg: " + szResult;
					}

					return false;
				}
			}
			else
			{
				szError = "bash end with exception !";
				return false;
			}
#else
			if (0 != iRet)
			{
				szError = "shell executed return errcode: " + std::to_string(iRet);
				if (!szResult.empty())
				{
					szError += " msg: " + szResult;
				}

				return false;
			}
#endif
		}
	}
	else
	{
		szError = std::string("popen executed failed: ") + GetErrnoMsg(errno);
		return false;
	}

	return true;
}

/*********************************************************************
功能：	分解输入参数
参数：	argc 参数个数，main方法输入
*		argv 参数数组，main方法输入
*		optstring 参数选项字符串，如：ab:c:e::，后面带":"表示必须有参数且可以不带空格；后面带"::"表示可选参数，不能带空格
*		opt_map 分解后的参数map，<参数选项，参数>，如：<'b', "hello">，<'c', "123">，<'e', "">
*		szError 失败后存放错误信息
返回：	成功返回true
修改：
*********************************************************************/
bool COS::GetOpts(int argc, char *argv[], const char *optstring, std::map<char, std::string> &opt_map, std::string &szError)
{
	//检查参数
	if (argc <= 1)
	{
		szError = "参数个数太少！";
		return false;
	}

	if (nullptr == argv)
	{
		szError = "参数列表不能为空！";
		return false;
	}

	if (nullptr == optstring || 0 == strlen(optstring))
	{
		szError = "参数选项串不能为空！";
		return false;
	}

	//分析参数选项
	std::map<char, short> optsTypeMap; //<参数选项，参数类型，0：普通，1：带参数，2：可选参数>

	for (const char *p = optstring, *t = nullptr; '\0' != *p; )
	{
		//检查合法性
		if ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))
		{
			if (nullptr != t) {
				optsTypeMap.insert(std::make_pair(*t, (short)(p - t - 1)));
			}

			t = p;
			++p;
		}
		else if (*p == ':')
		{
			//开头不能是":"，中间不能连续超过两个":"
			if (nullptr == t)
			{
				szError = "参数选项字符串格式错误：不能以':'开始！";
				return false;
			}
			else if ((p - t) >= 3)
			{
				szError = "参数选项字符串格式错误：':'不能连续超过两个！";
				return false;
			}

			++p;
		}
		else
		{
			szError = std::string("参数选项字符串包含非法字符：'") + *p + std::string("'！");
			return false;
		}

		//判断最后一个
		if ('\0' == *p) {
			optsTypeMap.insert(std::make_pair(*t, (short)(p - t - 1)));
		}
	}

	//处理参数
	std::map<char, short>::const_iterator optTypeIter;
	const char *arg = nullptr;
	size_t len = 0;

	for (int i = 1; i < argc; )
	{
		arg = argv[i];
		len = strlen(arg);

		if (1 == len)
		{
			szError = std::string("参数选项错误，不能是单个字符：") + *arg + std::string("！");
			return false;
		}
		else
		{
			if ('-' != arg[0])
			{
				szError = "参数选项：" + std::string(arg) + " 错误，必须以'-'开始！";
				return false;
			}
			else
			{
				optTypeIter = optsTypeMap.find(arg[1]);

				if (optTypeIter == optsTypeMap.cend())
				{
					szError = std::string("参数选项错误，不存在的选项：-") + arg[1] + "！";
					return false;
				}
				else
				{
					if (0 == optTypeIter->second)
					{
						if (2 == len)
						{
							opt_map.insert(std::make_pair(arg[1], ""));
							++i;
						}
						else
						{
							szError = "参数选项：" + std::string(arg) + " 错误，选项：-" + arg[1] + " 不支持参数！";
							return false;
						}
					}
					else if (1 == optTypeIter->second)
					{
						if (2 == len)
						{
							if (i == argc - 1)
							{
								szError = "参数选项：" + std::string(arg) + " 错误，参数不能为空！";
								return false;
							}
							else
							{
								//检查后面一个参数
								if ('-' == argv[i + 1][0])
								{
									szError = "参数选项：" + std::string(arg) + " 错误，参数不能为空！";
									return false;
								}
								else
								{
									opt_map.insert(std::make_pair(arg[1], std::string(argv[i + 1])));
									i += 2;
								}
							}
						}
						else
						{
							opt_map.insert(std::make_pair(arg[1], std::string(arg + 2)));
							++i;
						}
					}
					else
					{
						if (2 == len)
						{
							//检查后面一个参数
							if (i < argc - 1 && '-' != argv[i + 1][0])
							{
								szError = "参数选项：" + std::string(arg) + " 错误，可选参数选项与参数之间不能有空格！";
								return false;
							}
							else
								opt_map.insert(std::make_pair(arg[1], ""));
						}
						else
							opt_map.insert(std::make_pair(arg[1], std::string(arg + 2)));

						++i;
					}
				}
			}
		}
	}

	return true;
}

/*********************************************************************
功能：	获取内存分页大小
参数：	无
返回：	内存分页大小，Byte
修改：
*********************************************************************/
int COS::GetMemPageSize()
{
#ifdef _WIN32
	SYSTEM_INFO si;
	memset(&si, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&si);

	return (int)si.dwPageSize;
#else
	return getpagesize();
#endif
}

/*********************************************************************
功能：	获取简要的进程状态信息
参数：	processMap 进程列表，<进程ID，进程信息>，注意释放
*		szError 错误信息
返回：	成功返回true;
修改：
*********************************************************************/
bool COS::GetProcessesState(std::map<int, SProcessStat*> &processMap, std::string &szError)
{
	bool bRet = true;

#ifdef _WIN32
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);

	//创建进程快照
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap || NULL == hProcessSnap)
	{
		bRet = false;
		szError = "CreateToolhelp32Snapshot failed: " + GetErrnoMsg(GetLastError());
	}
	else
	{
		//遍历首次进程
		BOOL irt = Process32First(hProcessSnap, &pe32);
		if (FALSE == irt)
		{
			bRet = false;
			szError = "Process32First failed: " + GetErrnoMsg(GetLastError());
		}
		else
		{
			SProcessStat *stat = NULL;
			HANDLE hProcess = NULL;
			DWORD dExeNameSize = MAX_PATH;
			char sExeName[MAX_PATH + 1] = { '\0' };
			PROCESS_MEMORY_COUNTERS pms;
			HANDLE hToken = NULL;
			char sUserName[128] = { '\0' };
			DWORD dUserNameLen = 128;
			char sDomain[128] = { '\0' };
			DWORD dDomainLen = 128;
			SID_NAME_USE SNU;
			DWORD dTokenSize = 0;
			PTOKEN_USER pTokenUser = NULL;

			do
			{
				stat = new SProcessStat();
				if (NULL == stat)
				{
					bRet = false;
					szError = "new SProcessStat object return nullptr !";
					break;
				}

				stat->pid = (int)pe32.th32ProcessID;
				memcpy(stat->name, pe32.szExeFile, strlen(pe32.szExeFile));
				stat->ppid = (int)pe32.th32ParentProcessID;

				hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
				if (INVALID_HANDLE_VALUE != hProcess && NULL != hProcess)
				{
					//进程名绝对路径
					if (TRUE == QueryFullProcessImageName(hProcess, 0, sExeName, &dExeNameSize))
					{
						memcpy(stat->cmdline, sExeName, dExeNameSize);
					}

					//占用内存
					if (TRUE == GetProcessMemoryInfo(hProcess, &pms, sizeof(pms)))
					{
						stat->vsize = (int)(pms.PagefileUsage / 1024);
						stat->rss = (int)(pms.WorkingSetSize / 1024);
					}

					//用户名
					if (TRUE == OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
					{
						//第一次获取需要的字节数
						if (TRUE != GetTokenInformation(hToken, TokenUser, pTokenUser, dTokenSize, &dTokenSize))
						{
							irt = GetLastError();
							if (ERROR_INSUFFICIENT_BUFFER == irt)
							{
								irt = ERROR_SUCCESS;
							}
						}

						if (irt == ERROR_SUCCESS)
						{
							pTokenUser = (PTOKEN_USER)malloc(dTokenSize);

							//第二次获取信息
							if (TRUE == GetTokenInformation(hToken, TokenUser, pTokenUser, dTokenSize, &dTokenSize))
							{
								if (TRUE == LookupAccountSid(NULL, pTokenUser->User.Sid, sUserName, &dUserNameLen,
									sDomain, &dDomainLen, &SNU))
								{
									memcpy(stat->user_name, sUserName, dUserNameLen);
								}
							}

							free(pTokenUser);
							pTokenUser = NULL;
						}

						CloseHandle(hToken);
					}

					CloseHandle(hProcess);
				}

				//插入
				processMap.insert(std::make_pair(stat->pid, stat));

				//遍历下一个进程
				dExeNameSize = MAX_PATH;
				dUserNameLen = 128;
				memset(sUserName, 0, sizeof(sUserName));
				dDomainLen = 128;
				memset(sDomain, 0, sizeof(sDomain));
				irt = Process32Next(hProcessSnap, &pe32);
			} while (TRUE == irt);
		}

		CloseHandle(hProcessSnap);
	}
#else
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	FILE *fd = NULL;
	size_t i = 0, read_len = 0, str_len = 0;
	uid_t user_id = 0;
	char file_name[256], stat_line[1024], status_line[2048], *str_temp = NULL, name_temp[256], cmdline_temp[1024];
	struct passwd *pwd = NULL;
	SProcessStat *stat = NULL;

	if ((dir = opendir("/proc")) != NULL)
	{
		//获取内存分页大小，bytes
		const int mem_page_size = GetMemPageSize();

		//读取目录
		while ((dirent = readdir(dir)) != NULL)
		{
			//过滤掉文件、非进程目录、当前目录及上层目录
			if (dirent->d_type != DT_DIR || !CheckIsDigital(dirent->d_name))
			{
				continue;
			}

			stat = new SProcessStat();
			if (NULL == stat)
			{
				bRet = false;
				szError = "new SProcessStat object return nullptr !";
				break;
			}

			memset(file_name, 0, sizeof(file_name));
			memset(stat, 0, sizeof(SProcessStat));
			memset(stat_line, 0, sizeof(stat_line));
			memset(status_line, 0, sizeof(status_line));
			memset(name_temp, 0, sizeof(name_temp));
			memset(cmdline_temp, 0, sizeof(cmdline_temp));

			//命令行
			snprintf(file_name, sizeof(file_name), "/proc/%s/cmdline", dirent->d_name);
			fd = fopen(file_name, "r");

			if (NULL != fd)
			{
				read_len = fread(stat->cmdline, 1, sizeof(stat->cmdline), fd);
				fclose(fd);

				if (read_len > 0)
				{
					for (i = 1; i < read_len; ++i)
					{
						if ('\0' == stat->cmdline[i])
							stat->cmdline[i] = ' ';
					}
				}
			}

			//状态信息
			snprintf(file_name, sizeof(file_name), "/proc/%s/stat", dirent->d_name);
			fd = fopen(file_name, "r");

			if (NULL != fd)
			{
				read_len = fread(stat_line, 1, sizeof(stat_line), fd);
				fclose(fd);

				if (read_len > 0)
				{
					//6791 (pickup) S 2161 2161 2161 0 -1 4202752 1006 0 0 0 0 2 0 0 20 0 1 0 6602750 83062784 858 18446744073709551615 1 1 0 0 0 0 0 16781312 8192 18446744073709551615 0 0 17 0 0 0 0 0 0
					sscanf(stat_line, "%d %s %c %d %d %d %d %d %d %d %d %d %d %llu %llu %llu %llu %d %d %d %u %llu %llu %d %llu", 
						&stat->pid, stat->name, &stat->state, &stat->ppid, &stat->pgid, &stat->sid, &stat->tty_nr, &stat->tty_pgrp, &stat->flags, &stat->min_flt, 
						&stat->cmin_flt, &stat->maj_flt, &stat->cmaj_flt, &stat->utime, &stat->stime, &stat->cutime, &stat->cstime, &stat->priority, &stat->nice, 
						&stat->threads_num, &stat->it_real_value, &stat->start_time, &stat->vsize, &stat->rss, &stat->rlim);

					//换算内存单位
					stat->vsize /= 1024;
					stat->rss *= mem_page_size / 1024;

					//处理进程名
					str_len = strlen(stat->name);
					stat->name[str_len - 1] = '\0';

					for (i = 1; i < str_len; ++i)
					{
						stat->name[i - 1] = stat->name[i];
					}

					//再看进程名是否有截断，如果有则补充完整
					//名称：dmserver        命令：/opt/dmdbms/bin/dmserver /opt/dmdbms/data/DAMENG/dm.ini -noconsole
					//名称：QuantumNMSServe 命令：/root/BackgroundServices_nms/bin/QuantumNMSServer_nms
					//名称：sshd            命令：sshd: root@pts/1
					//名称：udevd           命令：/sbin/udevd -d
					//名称：qmgr            命令：qmgr -l -t fifo -u
					//名称：bash            命令：-bash
					//名称：mysqld_safe     命令：/bin/sh /usr/bin/mysqld_safe --datadir=/var/lib/mysql
					//名称：show_process_me 命令：./show_process_memory
					if ('\0' != stat->cmdline[0])
					{
						//截取空格前的串
						str_temp = strchr(stat->cmdline, ' ');

						if (NULL == str_temp)
							memcpy(cmdline_temp, stat->cmdline, strlen(stat->cmdline));
						else
							memcpy(cmdline_temp, stat->cmdline, (str_temp - stat->cmdline));

						//截取最后一个/后面的串，没找到则不处理
						str_temp = strrchr(cmdline_temp, '/');

						if (NULL != str_temp)
						{
							memcpy(name_temp, str_temp + 1, strlen(str_temp + 1));

							//如果有截断，则stat->name和name_temp前面部分是一样的
							if (0 == memcmp(stat->name, name_temp, strlen(stat->name)));
							{
								//替换
								memcpy(stat->name, name_temp, strlen(name_temp));
							}
						}
					}
				}
			}

			//用户ID
			snprintf(file_name, sizeof(file_name), "/proc/%s/status", dirent->d_name);
			fd = fopen(file_name, "r");

			if (NULL != fd)
			{
				read_len = fread(status_line, 1, sizeof(status_line), fd);
				fclose(fd);

				if (read_len > 0)
				{
					str_temp = strstr(status_line, "Uid:") + 5;
					user_id = (uid_t)atoi(str_temp);

					if ((pwd = getpwuid(user_id)) != NULL)
						memcpy(stat->user_name, pwd->pw_name, strlen(pwd->pw_name));
					else
						snprintf(stat->user_name, sizeof(stat->user_name), "%u", user_id);
				}
			}

			//插入
			processMap.insert(std::make_pair(stat->pid, stat));
		}

		closedir(dir);
	}
	else
	{
		bRet = false;
		szError = "open dir: /proc failed: " + GetErrnoMsg(errno);
	}
#endif

	return bRet;
}