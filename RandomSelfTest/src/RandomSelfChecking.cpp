#include "../include/RandomSelfChecking.hpp"
#include "../include/RandomTest.h"
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef JAVA_EXPORT

/*随机数自检
*参数：
*	pRandomFileName 用随机数填充的文件，至少2.5M
*	iCheckType 检测类型，1:单次自检，2:上电自检，3:周期自检
*返回值：
*	0:成功
*	-1:随机数文件名为空或不存在
*	-2:检测文件太小
*	-3:打开文件失败
*	-4:参数错误
*	-5:检测失败
*/
JNIEXPORT jstring JNICALL Java_com_RandomSelfChecking_RandomSelfTest(JNIEnv *env, jobject obj, const jstring pRandomFileName, jint iCheckType)
{
	int iRet = 0;
	std::string szFileName;
	jstring result = env->NewStringUTF("hello");

	//检测文件是否存在
	if (NULL == pRandomFileName)
		return result;
	else
	{
		const char *pTemp = env->GetStringUTFChars(pRandomFileName, false);
		
		if (NULL == pTemp)
			return result;

		szFileName = pTemp;
		env->ReleaseStringUTFChars(pRandomFileName, pTemp);
		pTemp = nullptr;
	}

#ifdef _WIN32
	iRet = _access(szFileName.c_str(), _A_NORMAL);
#else
	iRet = access(szFileName.c_str(), F_OK);
#endif // WIN32

	if (0 != iRet)
		return result;

	//检测文件大小
	long long llFileSize = 0;

#ifdef _WIN32
	struct _stat info;
	if (0 == _stat(szFileName.c_str(), &info))
		llFileSize = info.st_size;
	else
		llFileSize = -1;
#else
	struct stat info;
	if (0 == stat(szFileName.c_str(), &info))
		llFileSize = info.st_size;
	else
		llFileSize = -1;
#endif

	if (0 != iRet || 2510000 > llFileSize)
		return result;

	//从文件随机读取一段字节
	uint32_t uiRamdomLen = (20 * 1000000) / 8;
	uint8_t *pRandom = (uint8_t *)malloc(uiRamdomLen);

	FILE *pfile = fopen(szFileName.c_str(), "rb+");
	fseek(pfile, 110, 0);

	if (pfile == NULL)
	{
		free(pRandom);
		return result;
	}
	else
	{
		fread(pRandom, 1, uiRamdomLen, pfile);
		fclose(pfile);
	}
	
	string strRes;

	//自检
	if (1 == iCheckType)
	{
		SingleTest clsSingleTest(pRandom, 1, 16);
		iRet = clsSingleTest.DoTest(strRes);
	}
	else if (2 == iCheckType)
	{
		PowerOnTest clsPowerOnTest(pRandom, 1000000, 20);
		iRet = clsPowerOnTest.DoTest(strRes);
	}
	else
	{
		CycleTest clCycleTest(pRandom, 20000, 20);
		iRet = clCycleTest.DoTest(strRes);
	}

	if (0 != iRet)
		iRet = -4;

	free(pRandom);

	return result;
}

#else

int RandomSelfTest(const char *pRandomFileName, int iCheckType)
{
	int iRet = 0;
	string strRes;

	//检测文件是否存在
	if (NULL == pRandomFileName)
		return -4;

#ifdef _WIN32
	iRet = _access(pRandomFileName, _A_NORMAL);
#else
	iRet = access(pRandomFileName, F_OK);
#endif // WIN32

	if (0 != iRet)
		return -1;

	//检测文件大小
	long long llFileSize = 0;

#ifdef _WIN32
	struct _stat info;
	if (0 == _stat(pRandomFileName, &info))
		llFileSize = info.st_size;
	else
		llFileSize = -1;
#else
	struct stat info;
	if (0 == stat(pRandomFileName, &info))
		llFileSize = info.st_size;
	else
		llFileSize = -1;
#endif

	if (0 != iRet || 2510000 > llFileSize)
		return -2;

	//从文件随机读取一段字节
	uint32_t uiRamdomLen = (20 * 1000000) / 8;
	uint8_t *pRandom = (uint8_t *)malloc(uiRamdomLen);

	FILE *pfile = fopen(pRandomFileName, "rb+");
	fseek(pfile, 110, 0);

	if (pfile == NULL)
	{
		free(pRandom);
		return -3;
	}
	else
	{
		fread(pRandom, 1, uiRamdomLen, pfile);
		fclose(pfile);
	}

	//自检
	if (1 == iCheckType)
	{
		SingleTest clsSingleTest(pRandom, 1, 16);
		iRet = clsSingleTest.DoTest(strRes);
	}
	else if (2 == iCheckType)
	{
		PowerOnTest clsPowerOnTest(pRandom, 1000000, 20);
		iRet = clsPowerOnTest.DoTest(strRes);
	}
	else
	{
		CycleTest clCycleTest(pRandom, 20000, 20);
		iRet = clCycleTest.DoTest(strRes);
	}

	if (0 != iRet)
		iRet = -4;

	free(pRandom);

	return iRet;
}

#endif