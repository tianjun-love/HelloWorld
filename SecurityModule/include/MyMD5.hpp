/********************************************************************
名称:	md5类
功能:	md5
作者:	田俊
时间:	2015-10-22
修改:	2020-11-20 田俊  增加基类，减少冗余代码
*********************************************************************/
#ifndef __MY_MD5_HPP__
#define __MY_MD5_HPP__

#include "HashBase.hpp"

class CMD5 : public CHashBase
{
public:
	CMD5();
	virtual ~CMD5();

	static std::string StringToMD5(const std::string &szSrc, bool bLowercase = false);
	static std::string StringToMD5(const char* pSrc, uint32_t iLen, bool bLowercase = false);
	static bool StringToMD5(const std::string &szSrc, uint8_t* pDst, uint32_t iDstBufLen);
	static bool StringToMD5(const char* pSrc, uint32_t iLen, uint8_t* pDst, uint32_t iDstBufLen);
	static std::string FileToMD5(const std::string& szFileName, bool bLowercase = false);
	static bool FileToMD5(const std::string& szFileName, uint8_t* pDst, uint32_t iDstBufLen);

	static long long BKDHash(const char* pSrc);
	static long long APHash(const char* pSrc);

private:
	//长度定义
	enum ELENGTH_DEFINES : uint16_t
	{
		E_BLOCK_SIZE  = 64, //处理块大小，字节
		E_RESULT_SIZE = 16  //结果长度，MD5固定16字节
	};

	//md5处理结构体
	struct md5_ctx
	{
		bool     finalized;             // update finished
		uint8_t  buffer[E_BLOCK_SIZE];  // bytes that didn't fit in last 64 byte chunk
		uint32_t count[2];              // 64bit counter for number of bits (lo, hi)
		uint32_t state[4];              // digest so far
		uint8_t  digest[E_RESULT_SIZE]; // the result
	};

	static md5_ctx* init();
	static void update(md5_ctx *pCTX, const uint8_t input[], uint32_t len);
	static void finalize(md5_ctx *pCTX);

	static void transform(md5_ctx *pCTX, const uint8_t block[E_BLOCK_SIZE]);
	static void decode(uint32_t output[], const uint8_t input[], uint32_t len);
	static void encode(uint8_t output[], const uint32_t input[], uint32_t len);

	// low level logic operations
	static inline uint32_t F(uint32_t x, uint32_t y, uint32_t z);
	static inline uint32_t G(uint32_t x, uint32_t y, uint32_t z);
	static inline uint32_t H(uint32_t x, uint32_t y, uint32_t z);
	static inline uint32_t I(uint32_t x, uint32_t y, uint32_t z);
	static inline uint32_t rotate_left(uint32_t x, uint32_t n);
	static inline void FF(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac);
	static inline void GG(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac);
	static inline void HH(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac);
	static inline void II(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac);
};

#endif