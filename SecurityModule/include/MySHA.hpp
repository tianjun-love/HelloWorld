/********************************************************************
名称:	SHA类
功能:	SHA
作者:	田俊
时间:	2019-06-03
修改:
*********************************************************************/
#ifndef __SHA_HPP__
#define __SHA_HPP__

#include <string>
using std::string;

class CSHA
{
private:
	#define SHA256_HASH_SIZE 32
		
	// Hash size in 32-bit words
	#define SHA256_HASH_WORDS 8

	# define SHA384_DIGEST_LENGTH    48
	# define SHA512_DIGEST_LENGTH    64
	
	typedef struct _SHA256Context 
	{
		unsigned long long totalLength;
		unsigned int hash[SHA256_HASH_WORDS];
		unsigned int bufferLength;
		union 
		{
			unsigned int words[16];
			unsigned char bytes[64];
		} buffer;
	
#ifdef RUNTIME_ENDIAN
	int littleEndian;
#endif /* RUNTIME_ENDIAN */
	
	}SSHA256Context;

	typedef struct _SHA512Context
	{
		unsigned long long h[8];
		unsigned long long Nl, Nh;
		union
		{
			unsigned long long d[16];
			unsigned char p[128];
		} u;

		unsigned int num, md_len;

	}SSHA512Context;

	typedef SSHA512Context SSHA384Context;

public:
	CSHA() {}
	CSHA(const CSHA& Other) = delete;
	~CSHA() {}
		
	static string StringToSHA256(const char* pSrc, int iLen, bool bLowercase = false);
	static string StringToSHA256(const string& szSrc, bool bLowercase = false);
	static string FileToSHA256(const string& szFileName, bool bLowercase = false);

	static string StringToSHA384(const char* pSrc, int iLen, bool bLowercase = false);
	static string StringToSHA384(const string& szSrc, bool bLowercase = false);
	static string FileToSHA384(const string& szFileName, bool bLowercase = false);

	static string StringToSHA512(const char* pSrc, int iLen, bool bLowercase = false);
	static string StringToSHA512(const string& szSrc, bool bLowercase = false);
	static string FileToSHA512(const string& szFileName, bool bLowercase = false);

private:
	static bool CheckFileExist(const std::string &szFileName);
	static string BytesToHexString(const unsigned char *in, size_t size, bool bLowercase);

	static void SHA256Init(SSHA256Context *sc);
	static void SHA256Update(SSHA256Context *sc, const void *vdata, unsigned int len);
	static void SHA256Final(SSHA256Context *sc, unsigned char hash[SHA256_HASH_SIZE]);		
	static void SHA256Guts(SSHA256Context *sc, const unsigned int *cbuf);

	static void SHA512Init(SSHA512Context *sc);
	static void SHA512Update(SSHA512Context *sc, const void *vdata, unsigned int len);
	static void SHA512Final(SSHA512Context *sc, unsigned char hash[SHA512_DIGEST_LENGTH]);
	static void SHA512BlockDataOrder(SSHA512Context *ctx, const void *in, size_t num);
	
	static void SHA384Init(SSHA384Context *sc);
	static void SHA384Update(SSHA384Context *sc, const void *vdata, unsigned int len);
	static void SHA384Final(SSHA384Context *sc, unsigned char hash[SHA384_DIGEST_LENGTH]);
};



#endif//__SHA256_HPP__

