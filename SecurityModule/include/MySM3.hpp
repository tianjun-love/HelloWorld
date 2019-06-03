/******************************************************
功能：	SM3摘要算法
作者：	田俊
时间：	2019-03-21
修改：	
******************************************************/
#ifndef __MY_SM3_HPP__
#define __MY_SM3_HPP__

#include <string>

class CSM3
{
public:

	typedef unsigned int uInt32;  //需要确保字长为32位
	static const unsigned int blocksize  = 64;
	static const unsigned int padsize    = 64;
	static const unsigned int resultsize = 32;

	typedef struct sm3_context
	{
		uInt32        total[2];           /*!< number of bytes processed  */
		uInt32        state[8];           /*!< intermediate digest state  */
		unsigned char buffer[blocksize];  /*!< data block being processed */
		unsigned char digest[resultsize]; /*!< result                     */
	}SSM3Context;
	
	typedef struct hmac_sm3_context
	{
		SSM3Context   SM3Context;
		unsigned char iIpad;               //HMAC Inner padding
		unsigned char iOpad;               //HMAC Outer padding   
		unsigned char pKeyHash[blocksize];

	}SHMACContext;

public:
	CSM3();
	CSM3(const CSM3& Other) = delete;
	~CSM3();

	static std::string StringToSM3(const char* pSrc, uInt32 iLen, bool bLowercase = false);
	static std::string StringToSM3(const std::string& szSrc, bool bLowercase = false);
	static bool StringToSM3(const char* pSrc, uInt32 iLen, char* pDst, uInt32 iDstBufLen);
	static bool StringToSM3(const std::string& szSrc, char* pDst, uInt32 iDstBufLen);
	static std::string FileToSM3(const std::string& szFileName, bool bLowercase = false);
	static bool FileToSM3(const std::string& szFileName, char* pDst, uInt32 iDstBufLen);

	static std::string StringToHMAC(const char *pSrc, uInt32 iLen, const char* pKey, uInt32 iKeyLen, bool bLowercase = false);
	static std::string StringToHMAC(const std::string& szSrc, const char* pKey, uInt32 iKeyLen, bool bLowercase = false);
	static bool StringToHMAC(const char *pSrc, uInt32 iLen, const char* pKey, uInt32 iKeyLen, char* pDst, uInt32 iDstBufLen);
	static bool StringToHMAC(const std::string& szSrc, const char* pKey, uInt32 iKeyLen, char* pDst, uInt32 iDstBufLen);
	static std::string FileToHMAC(const std::string& szFileName, const char* pKey, uInt32 iKeyLen, bool bLowercase = false);
	static bool FileToHMAC(const std::string& szFileName, const char* pKey, uInt32 iKeyLen, char* pDst, uInt32 iDstBufLen);

	//为适应openssl evp 接口
	static void QtSM3Init(SSM3Context *pCtx);
	static void QtSM3Update(SSM3Context *pCtx, const void *pVdata, uInt32 iDataLen);
	static void QtSM3Final(SSM3Context *pCtx, unsigned char hash[resultsize]);

private:
	static bool CheckFileExist(const std::string &szFileName);
	static std::string BytesToHexString(const unsigned char *pIn, size_t iSize, bool bLowercase);

	static void SM3Init(SSM3Context *pCtx);
	static void SM3Update(SSM3Context *pCtx, const void *pVdata, uInt32 iDataLen);
	static void SM3Final(SSM3Context *pCtx);
	static void SM3Guts(SSM3Context *pCtx, unsigned char data[blocksize]);

	static void HMACInit(SHMACContext *pCtx, const char *pKey, uInt32 iKeyLen);
	static void HMACUpdate(SHMACContext *pCtx, const void *pVdata, uInt32 iDataLen);
	static void HMACFinal(SHMACContext *pCtx);
};

#endif
