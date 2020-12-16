/******************************************************
功能：	SM3摘要算法
作者：	田俊
时间：	2019-03-21
修改：	
******************************************************/
#ifndef __MY_SM3_HPP__
#define __MY_SM3_HPP__

#include "HashBase.hpp"

class CSM3 : public CHashBase
{
public:
	//长度定义
	enum ELENGTH_DEFINES : uint16_t
	{
		E_BLOCK_SIZE  = 64,
		E_PAD_SIZE    = 64,
		E_RESULT_SIZE = 32
	};

	struct SSM3Context
	{
		uint32_t      total[2];              /*!< number of bytes processed  */
		uint32_t      state[8];              /*!< intermediate digest state  */
		unsigned char buffer[E_BLOCK_SIZE];  /*!< data block being processed */
		unsigned char digest[E_RESULT_SIZE]; /*!< result                     */
	};
	
	struct SHMACContext
	{
		SSM3Context   SM3Context;
		unsigned char iIpad;                 //HMAC Inner padding
		unsigned char iOpad;                 //HMAC Outer padding   
		unsigned char pKeyHash[E_BLOCK_SIZE];
	};

public:
	CSM3();
	~CSM3();

	static std::string StringToSM3(const char* pSrc, uint32_t iLen, bool bLowercase = false);
	static std::string StringToSM3(const std::string& szSrc, bool bLowercase = false);
	static bool StringToSM3(const char* pSrc, uint32_t iLen, uint8_t* pDst, uint32_t iDstBufLen);
	static bool StringToSM3(const std::string& szSrc, uint8_t* pDst, uint32_t iDstBufLen);
	static std::string FileToSM3(const std::string& szFileName, bool bLowercase = false);
	static bool FileToSM3(const std::string& szFileName, uint8_t* pDst, uint32_t iDstBufLen);

	static std::string StringToHMAC(const char *pSrc, uint32_t iLen, const char* pKey, uint32_t iKeyLen, bool bLowercase = false);
	static std::string StringToHMAC(const std::string& szSrc, const char* pKey, uint32_t iKeyLen, bool bLowercase = false);
	static bool StringToHMAC(const char *pSrc, uint32_t iLen, const char* pKey, uint32_t iKeyLen, char* pDst, uint32_t iDstBufLen);
	static bool StringToHMAC(const std::string& szSrc, const char* pKey, uint32_t iKeyLen, char* pDst, uint32_t iDstBufLen);
	static std::string FileToHMAC(const std::string& szFileName, const char* pKey, uint32_t iKeyLen, bool bLowercase = false);
	static bool FileToHMAC(const std::string& szFileName, const char* pKey, uint32_t iKeyLen, char* pDst, uint32_t iDstBufLen);

	//为适应openssl evp 接口
	static void QtSM3Init(SSM3Context *pCtx);
	static void QtSM3Update(SSM3Context *pCtx, const void *pVdata, uint32_t iDataLen);
	static void QtSM3Final(SSM3Context *pCtx, unsigned char hash[E_RESULT_SIZE]);

private:
	static void SM3Init(SSM3Context *pCtx);
	static void SM3Update(SSM3Context *pCtx, const void *pVdata, uint32_t iDataLen);
	static void SM3Final(SSM3Context *pCtx);
	static void SM3Guts(SSM3Context *pCtx, unsigned char data[E_BLOCK_SIZE]);

	static void HMACInit(SHMACContext *pCtx, const char *pKey, uint32_t iKeyLen);
	static void HMACUpdate(SHMACContext *pCtx, const void *pVdata, uint32_t iDataLen);
	static void HMACFinal(SHMACContext *pCtx);
};

#endif
