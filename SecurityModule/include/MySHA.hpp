/********************************************************************
名称:	SHA类
功能:	SHA
作者:	马圆
时间:	2017-07-02
修改:
*********************************************************************/
#ifndef __MY_SHA_HPP__
#define __MY_SHA_HPP__

#include "HashBase.hpp"

class CSHA : public CHashBase
{
public:
	CSHA();
	virtual ~CSHA();
	
	static std::string StringToSHA256(const std::string &szSrc, bool bLowercase = false);
	static std::string StringToSHA256(const char* pSrc, size_t iLen, bool bLowercase = false);
	static bool StringToSHA256(const char* pSrc, size_t iLen, uint8_t* pDst, uint32_t iDstBufLen);
	static bool StringToSHA256(const std::string &szSrc, uint8_t* pDst, uint32_t iDstBufLen);
	static std::string FileToSHA256(const std::string& szFileName, bool bLowercase = false);
	static bool FileToSHA256(const std::string& szFileName, uint8_t* pDst, uint32_t iDstBufLen);
	
	static std::string StringToSHA384(const char* pSrc, size_t iLen, bool bLowercase = false);
	static std::string StringToSHA384(const std::string& szSrc, bool bLowercase = false);
	static bool StringToSHA384(const char* pSrc, size_t iLen, uint8_t* pDst, uint32_t iDstBufLen);
	static bool StringToSHA384(const std::string &szSrc, uint8_t* pDst, uint32_t iDstBufLen);
	static std::string FileToSHA384(const std::string& szFileName, bool bLowercase = false);
	static bool FileToSHA384(const std::string& szFileName, uint8_t* pDst, uint32_t iDstBufLen);
	
	static std::string StringToSHA512(const char* pSrc, size_t iLen, bool bLowercase = false);
	static std::string StringToSHA512(const std::string& szSrc, bool bLowercase = false);
	static bool StringToSHA512(const char* pSrc, size_t iLen, uint8_t* pDst, uint32_t iDstBufLen);
	static bool StringToSHA512(const std::string &szSrc, uint8_t* pDst, uint32_t iDstBufLen);
	static std::string FileToSHA512(const std::string& szFileName, bool bLowercase = false);
	static bool FileToSHA512(const std::string& szFileName, uint8_t* pDst, uint32_t iDstBufLen);

private:
	enum ELENGTH_DEFINES : uint16_t
	{
		SHA256_HASH_WORDS = 8,
		SHA256_DIGEST_LENGTH = 32,
		SHA384_DIGEST_LENGTH = 48,
		SHA512_DIGEST_LENGTH = 64
	};

	struct SSHA256Context
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

	};

	struct SSHA512Context
	{
		unsigned long long h[8];
		unsigned long long Nl, Nh;
		union
		{
			unsigned long long d[16];
			unsigned char p[128];
		} u;

		unsigned int num, md_len;
	};

	typedef SSHA512Context SSHA384Context;

	static void SHA256Init(SSHA256Context *sc);
	static void SHA256Update(SSHA256Context *sc, const void *vdata, size_t len);
	static void SHA256Final(SSHA256Context *sc, unsigned char hash[SHA256_DIGEST_LENGTH]);
	static void SHA256Guts(SSHA256Context *sc, const unsigned int *cbuf);

	static void SHA512Init(SSHA512Context *sc);
	static void SHA512Update(SSHA512Context *sc, const void *vdata, size_t len);
	static void SHA512Final(SSHA512Context *sc, unsigned char hash[SHA512_DIGEST_LENGTH]);
	static void SHA512BlockDataOrder(SSHA512Context *ctx, const void *in, size_t num);
	
	static void SHA384Init(SSHA384Context *sc);
	static void SHA384Update(SSHA384Context *sc, const void *vdata, size_t len);
	static void SHA384Final(SSHA384Context *sc, unsigned char hash[SHA384_DIGEST_LENGTH]);
};



#endif//__SHA256_HPP__

