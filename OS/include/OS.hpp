/*************************************************
功能:	系统操作相应方法类
作者:	田俊
时间:	2022-08-31
修改:
*************************************************/
#ifndef __OS_HPP__
#define __OS_HPP__

#include <map>
#include <string>

class COS
{
public:
	COS();
	~COS();

	//进程信息，linux读取/proc/xx/stat
	struct SProcessStat
	{
		int                pid;                //ID
		char               name[256];          //名称，NAME_MAX + 1
		char               state;              //状态：R:running, S:sleeping, D:disk sleep, T:stopped, Z:zombie, X:dead
		int                ppid;               //父ID
		int                pgid;               //组号
		int                sid;                //会话组ID
		int                tty_nr;             //tty终端的设备号
		int                tty_pgrp;           //tty终端的进程组号
		int                flags;              //进程标志位
		int                min_flt;            //不需要从硬盘拷数据而发生的次缺页的次数
		int                cmin_flt;           //所有的waited-for进程曾经发生的次缺页的次数
		int                maj_flt;            //从硬盘拷数据而发生的主缺页的次数
		int                cmaj_flt;           //所有的waited-for进程曾经发生的主缺页的次数
		unsigned long long utime;              //在用户态的运行时间，单位为jiffies
		unsigned long long stime;              //在核心态的运行时间，单位为jiffies
		unsigned long long cutime;             //累计的所有的waited-for进程曾经在用户态的运行时间，单位为jiffies
		unsigned long long cstime;             //累计的所有的waited-for进程曾经在核心态的运行时间，单位为jiffies
		int                priority;           //动态优先级
		int                nice;               //静态优先级
		int                threads_num;        //线程的个数
		unsigned int       it_real_value;      //由于时间间隔导致的下一个SIGALRM发送进程时延，单位为jiffies
		unsigned long long start_time;         //启动时长，单位为jiffies
		unsigned long long vsize;              //占用虚拟地址空间大小，单位为Bytes，己换算为KB
		int                rss;                //占用物理地址空间大小，单位为page，己换算为KB
		unsigned long long rlim;               //该进程能使用的物理地址空间最大值，单位为byte

											   //后面还有一些属性，暂时用不上，先不加

											   //拓展
		char			   user_name[64];      //所属用户名
		char               cmdline[2048];      //命令行，linux读取/proc/xx/cmdline
	};

	static std::string GetErrnoMsg(int err_code); //获取errno的错误信息
	static bool GetOpts(int argc, char *argv[], const char *optstring, std::map<char, std::string> &opt_map, 
		std::string &szError); //分析命令参数
	static bool Popen(const std::string &szCommand, std::string &szResult, std::string &szError); //获取popen执行命令结果
	static int GetMemPageSize(); //获取内存分页大小,Byte
	static bool GetProcessesState(std::map<int, SProcessStat*> &processMap, std::string &szError); //获取简要的进程状态信息

private:

};

#endif