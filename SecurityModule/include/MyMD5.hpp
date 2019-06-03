/********************************************************************
名称:	md5类
功能:	md5
作者:	田俊
时间:	2015-10-22
修改:
*********************************************************************/
#ifndef __MY_MD5_HPP__
#define __MY_MD5_HPP__

#include <string>
#include <cstring>

class CMD5
{
private:
	typedef unsigned char uint1; //  8bit
	typedef unsigned int uint4;  // 32bit
	typedef unsigned int uInt32;
	static const unsigned int blocksize = 64;
	static const unsigned int resultsize = 16;

public:
	CMD5();
	~CMD5();

	static std::string StringToMD5(const char* pSrc, uInt32 iLen, bool bLowercase = false);
	static std::string StringToMD5(const std::string& szSrc, bool bLowercase = false);
	static bool StringToMD5(const char* pSrc, uInt32 iLen, char* pDst, int iDstBufLen);
	static bool StringToMD5(const std::string& szSrc, char* pDst, uInt32 iDstBufLen);
	static std::string FileToMD5(const std::string& szFileName, bool bLowercase = false);
	static bool FileToMD5(const std::string& szFileName, char* pDst, uInt32 iDstBufLen);
	static bool CheckFileExist(const std::string &szFileName);
	static long long BKDHash(const char* pSrc);
	static long long APHash(const char* pSrc);

private:
	typedef struct _SMD5_ctx
	{
		bool finalized;           // update finished
		uint1 buffer[blocksize];  // bytes that didn't fit in last 64 byte chunk
		uint4 count[2];           // 64bit counter for number of bits (lo, hi)
		uint4 state[4];           // digest so far
		uint1 digest[resultsize]; // the result
	}md5_ctx;

	static md5_ctx* init();
	static void update(md5_ctx *pCTX, const uint1 input[], uInt32 len);
	static void finalize(md5_ctx *pCTX);
	static std::string hexdigest(md5_ctx *pCTX, bool bLowercase);

	static void transform(md5_ctx *pCTX, const uint1 block[blocksize]);
	static void decode(uint4 output[], const uint1 input[], uInt32 len);
	static void encode(uint1 output[], const uint4 input[], uInt32 len);

	// low level logic operations
	static inline uint4 F(uint4 x, uint4 y, uint4 z);
	static inline uint4 G(uint4 x, uint4 y, uint4 z);
	static inline uint4 H(uint4 x, uint4 y, uint4 z);
	static inline uint4 I(uint4 x, uint4 y, uint4 z);
	static inline uint4 rotate_left(uint4 x, uint4 n);
	static inline void FF(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac);
	static inline void GG(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac);
	static inline void HH(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac);
	static inline void II(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac);
};

#endif