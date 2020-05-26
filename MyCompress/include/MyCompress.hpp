/***************************************************
功能：	zip文件的解压缩
作者：	田俊
时间：	2019-12-05
个性：	
***************************************************/
#ifndef __MY_COMPRESS_HPP__
#define __MY_COMPRESS_HPP__

#include <string>
#include <list>
#include "zip.h"
#include "unzip.h"

class CMyCompress
{
public:
	//文件类型
	enum EFileType : int
	{
		e_None = 0, //未知
		e_Dir  = 1, //文件夹
		e_File = 2, //文件
		e_Link = 3  //软连接/快捷方式
	};

	//压缩级别
	enum ECompressLevel : int
	{
		e_Level_Default    = Z_DEFAULT_COMPRESSION, //默认
		e_Level_NoCompress = Z_NO_COMPRESSION,      //不压缩
		e_Level1           = Z_BEST_SPEED,
		e_Level2,
		e_Level3,
		e_Level4,
		e_Level5,
		e_Level6,
		e_Level7,
		e_Level8,
		e_Level9           = Z_BEST_COMPRESSION
	};

	//文件信息对象
	struct SFileInfo
	{
		std::string          szName;        //文件名称
		std::string          szBaseDir;     //父目录，不带最后的/
		EFileType            eType;         //文件类型
		uint32_t             iAttrib;       //属性
		uint64_t             ullSize;       //文件大小，字节
		uint64_t             ullOccupySize; //占用大小，字节
		time_t               llCreateTime;  //创建时间
		time_t               llAccessTime;  //最后一次访问时间
		time_t               llWriteTime;   //最后一次写入时间
		unsigned long        ulDosWriteTime;//dos格式的最后一次写入时间
		std::list<SFileInfo> SubDirList;    //文件夹里面的文件，如果是文件夹才有值

		SFileInfo();
		~SFileInfo();
		std::string TimeToString(const time_t& timeVal) const; //将时间转为字符串“yyyy-mm-dd hh:MM:ss”
	};

	//软连接信息对象，linux解压使用
	struct SLinkInfo
	{
		std::string   szPath;
		std::string   szLink;
		unsigned long lMode = 0;
	};

	//目录文件信息
	struct SDirFiles
	{
		std::string          szFileName; //文件名称
		bool                 bIsDir;     //是否目录
		std::list<SDirFiles> DirFiles;   //如果是目录，则有值

		SDirFiles() : bIsDir(false) {};
	};

	CMyCompress(const std::string &szPassword = "", const std::string &szUserName = "");
	~CMyCompress();

	bool CompressZIP(const std::list<std::string> &fileList, const std::string &szZipFileName, ECompressLevel eLevel,
		std::string &szError);
	bool UncompressZIP(const std::string &szZipFileName, const std::string &szDstDir, std::string &szError);
	bool CompressGZIP(const std::list<std::string> &fileList, const std::string &szGZFileName, ECompressLevel eLevel,
		std::string &szError);
	bool UncompressGZIP(const std::string &szGZFileName, const std::string &szDstDir, std::string &szError);

	static std::string GetTimeStr();
	static bool GetDirFiles(const std::string& szDir, std::list<SDirFiles> &FileList, std::string &szError);
	static bool RmFile(const std::string &szFileName, bool bChilds, std::string &szError);
	static bool MkDir(const std::string &szDir, unsigned int iMode, bool bParents, std::string &szError);
	static bool CheckFileExists(const std::string &szFileName, bool bLinkDereferenced = false);

private:
	bool do_compressZIP(zipFile zip, const std::string &szFileName, const std::string &szZipFileName, const SFileInfo &fileInfo, 
		int iLevel, const char* pwd, char* buf, unsigned long size_buf, std::string &szError);
	bool do_uncompressZIP(zipFile zip, const unz_file_info64 &file_info, const std::string &szDstDirName, const char* filename_inzip,
		const char* pwd, char* buf, unsigned long size_buf, std::list<SLinkInfo> &LinkList, std::string &szError);
	static bool WriteNewFileInZip(zipFile zip, const std::string &szFileName, char* buf, unsigned long size_buf, std::string &szError);
	static bool WriteNewFileLinkInZip(zipFile zip, const std::string &szFileName, char* buf, unsigned long size_buf, std::string &szError);
	static EFileType CheckFileType(const std::string &szFileName);
	static int CheckFileIsLarge(const std::string &szFileName, ZPOS64_T *llSize = NULL);
	static int CheckFileCompressType(const std::string &szFileName, unsigned int *tarLength = NULL);
	static bool GetFileCrc(const std::string &szFileName, char* buf, unsigned long size_buf, unsigned long* result_crc, bool bIsLink,
		std::string &szError);
	static void FillFileTime(const SFileInfo &info, zip_fileinfo &file);
	static void ChangeFileTime(const std::string &szFileName, bool bIsDir, unsigned long dosdate, const tm_unz *tmu_date, 
		time_t *utc_mtime = NULL);
	static std::string GetErrorMsg(int errCode);
	static bool Chmod(const std::string &szFile, unsigned int iMode, std::string &szError);
	static bool Chown(const std::string &szFile, const std::string &szUser, std::string &szError);
	static bool GetFileInfo(const std::string& szFile, SFileInfo &info, std::string &szError);
	static bool GetDirInfo(const std::string& szDir, std::list<SFileInfo>& infoList, std::string& szError, bool bCheckIsDir = true);
	static bool GetRealPath(const std::string &szPath, std::string &szResolvedPath, std::string &szError);
	static bool CPFile(const SFileInfo &srcFile, const std::string &szDstDir, std::string &szError);

private:
#define T_BLOCKSIZE     512
#define T_NAMELEN       100
#define T_PREFIXLEN     155
#define T_ENDCHAR       ' ';

	//gz文件格式为：|0x1F|0x8B|CM(只有0x08)|FLG(1字节)|MTIME(4字节)|XFL(1字节)|OS(1字节)|额外头字段(可能0字节，根据FLG判断)
	//|压缩数据(N字节)|CRC32(原始未压缩数据的校验和，4字节)|ISIZE(原始未数据长度，4字节，小端字节序)
	//tar文件格式为：head+data + ... + head+data + 512全0块(也可能没有)
	//head长度固定为512，data为按512分的块，最后不足512，补0
	union tar_record
	{
		struct header
		{
			char name[T_NAMELEN]; //文件名，以'\0'结束
			char mode[8]; //八进制串，前面补0，最后二个字符为" \0"
			char uid[8]; //八进制串，前面补0，最后二个字符为" \0"
			char gid[8]; //八进制串，前面补0，最后二个字符为" \0"
			char size[12]; //八进制串，前面补0，最后一个字符为空格
			char mtime[12]; //八进制串，前面补0，最后一个字符为空格，是UTC时间
			char chksum[8]; //八进制串，前面补0，最后二个字符为"\0 "
			char typeflag; //'0':普通磁盘文件，'1':硬连接，'2':软连接，'3':字符设备，'4':块设备，'5':目录，'6':FIFO管道，'7':连续文件
			char linkname[T_NAMELEN]; //当是软/硬连接时，使用此名称，以'\0'结束
			char magic[6]; //如果uname和gname有效，则为："ustar\0"；//如果是GNU格式转储条目，则为："GNUtar\0"
			char version[2];
			char uname[32]; //用户名，以'\0'结束
			char gname[32]; //组名，以'\0'结束
			char devmajor[8]; //以'\0'结束
			char devminor[8]; //以' \0'结束
			char prefix[T_PREFIXLEN];
			char padding[12];
		} head;

		char data[T_BLOCKSIZE];
	};

	bool do_compressGZIP(gzFile gz, const std::string &szFileName, const std::string &szGzipFileName, const SFileInfo &fileInfo,
		const char* pwd, char* buf, unsigned long size_buf, std::string &szError);
	bool do_uncompressGZIP(gzFile gz, const tar_record &file_head, const std::string &szDstDirName, const char* pwd, char* buf,
		unsigned long size_buf, unsigned int &iReadDataLength, std::list<SLinkInfo> &LinkList, std::string &szError);
	static bool WriteGzipCompressData(gzFile gz, char *data, unsigned int data_len, std::string &szError);
	static void FillUserInfo(tar_record &head);
	static bool GetUserInfo(const std::string &szUserName, int &uid, int &gid, std::string &gid_name, 
		std::string &szLoginUserName, std::string &szError);
	static unsigned int OctalToDecimal(const char *buf);
	static void DecimalToOctal(unsigned int deci, char *buf, unsigned int buf_len);
	static void TarHeaderCheckSum(tar_record &record);
	static time_t UTCTimeToLocal(const time_t utcTime);

private:
	std::string                 m_szPassword;     //解压缩密码，gzip不支持
	std::string                 m_szUserName;     //用户名称，linux使用
	static unsigned long        m_lVersionMadeBy; //ZIP版本信息
	static const unsigned short m_DiskBlockSize;  //linux上的磁盘块大小

};

#endif // !__MY_COMPRESS_HPP__
