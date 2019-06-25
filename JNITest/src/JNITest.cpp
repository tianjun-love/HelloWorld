#include "../include/JNITest.hpp"
#include <string>
#include <cstring>
#include <ctime>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif


static std::string JNICALL GetHexString(const unsigned char ch, bool lower = false)
{
	std::string szRet;
	char cFirst = ch >> 4;
	char cSecond = ch & 0x0F;

	if (cFirst < 10)
		szRet = (char)(cFirst + '0');
	else
		szRet = (char)(cFirst + (lower ? 'a' : 'A') - 10);

	if (cSecond < 10)
		szRet += (char)(cSecond + '0');
	else
		szRet += (char)(cSecond + (lower ? 'a' : 'A') - 10);

	return std::move(szRet);
}

/*********************************************************************
功能：	获取毫秒
参数：	无
返回：	当前毫秒
修改：
*********************************************************************/
static int JNICALL GetCurrentMillisecond()
{
	//获取当前时间的毫秒数
#ifdef _WIN32
	SYSTEMTIME ctT;
	GetLocalTime(&ctT);
	return ctT.wMilliseconds;
#else
	timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_usec / 1000);
#endif
}

/*********************************************************************
功能：	获取时间字符串
参数：	bNeedMilliseconds 是否需要毫秒
返回：	时间字符串
修改：
*********************************************************************/
static std::string JNICALL GetTimeString(bool bNeedMilliseconds = true)
{
	char strDateTime[32] = { '\0' };
	struct tm tmTimeTemp;
	time_t timeTemp = 0;

	//获取当前秒数
	timeTemp = time(NULL);

	//localtime函数不可重入，即不是线程安全的，所以用下面的版本
#ifdef _WIN32
	localtime_s(&tmTimeTemp, &timeTemp);
#else
	localtime_r(&timeTemp, &tmTimeTemp);
#endif

	if (bNeedMilliseconds)
	{
		snprintf(strDateTime, 24, "%02d:%02d:%02d.%03d>> ", tmTimeTemp.tm_hour, tmTimeTemp.tm_min,
			tmTimeTemp.tm_sec, GetCurrentMillisecond());
	}
	else
		strftime(strDateTime, 24, "%Y-%m-%d %X>> ", &tmTimeTemp);

	return std::move(std::string(strDateTime));
}

jstring JNICALL Java_com_JNITest_Hello(JNIEnv *env, jobject obj, jstring name, jint age)
{
	std::string szOut;
	std::string szName;

	if (NULL != name)
	{
		jboolean iscopy = JNI_FALSE;
		const char* pName = env->GetStringUTFChars(name, &iscopy);

		if (NULL != pName)
			szName = pName;

		env->ReleaseStringUTFChars(name, pName);
	}
	else
		return NULL;

	szOut = GetTimeString() + "Hello " + szName + ", your age is " + std::to_string(age) + " !";
	std::cout << szOut << std::endl;

	return env->NewStringUTF(szOut.c_str());
}

jbyteArray JNICALL Java_com_JNITest_ByteArray(JNIEnv *env, jobject obj, jbyteArray bytes)
{
	jsize byteNum = env->GetArrayLength(bytes);
	std::cout << GetTimeString() << "Recv java byte array length is " << byteNum << " !" << std::endl;

	if (byteNum > 0)
	{
		std::cout << GetTimeString() << "Recv java byte array is:" << std::endl;

		/*
		jboolean iscopy = JNI_FALSE;
		jbyte *pByte = env->GetByteArrayElements(bytes, &iscopy);

		for (jsize i = 0; i < byteNum; ++i)
		{
			std::cout << GetHexString(pByte[i]) << " ";
		}

		std::cout << std::endl;

		//mode说明：
		//0:对象数组将不会被限制，数据将会拷贝回原始数据，同时释放拷贝数据
		//JNI_COMMIT:对象数组什么都不作，数据将会拷贝回原始数据，不释放拷贝数据
		//JNI_ABORT:对象数组将不会被限制, 之前的数据操作有效，释放拷贝数据, 之前的任何数据操作会丢弃
		env->ReleaseByteArrayElements(bytes, pByte, 0);
		*/

		//更好的办法
		jbyte *pByte = new jbyte[byteNum + 1];

		memset(pByte, 0, byteNum + 1);
		env->GetByteArrayRegion(bytes, 0, byteNum, pByte);

		for (jsize i = 0; i < byteNum; ++i)
		{
			std::cout << GetHexString((unsigned char)pByte[i]) << " ";
		}

		std::cout << std::endl;
		delete[] pByte;
		pByte = NULL;
	}

	jbyteArray byteArr = env->NewByteArray(10);
	jbyte bt = 0;
	srand((unsigned int)time(NULL));
	
	for (jsize i = 0; i < 10; ++i)
	{
		bt = (jbyte)rand();
		env->SetByteArrayRegion(byteArr, i, 1, &bt);
	}
	
	return byteArr;
}

jboolean JNICALL Java_com_JNITest_CallBack(JNIEnv *env, jobject obj, jstring msg)
{
	jboolean bRet = JNI_TRUE;
	std::string szMsg;
	jclass objClass = env->GetObjectClass(obj);

	if (NULL != msg)
	{
		jboolean iscopy = JNI_FALSE;
		const char* pMsg = env->GetStringUTFChars(msg, &iscopy);

		if (NULL != pMsg)
			szMsg = pMsg;

		env->ReleaseStringUTFChars(msg, pMsg);
	}
	else
		szMsg = "Unknow";

	jfieldID objFieldIdInteger = env->GetFieldID(objClass, "mIntegerData", "I");
	jfieldID objFieldIdString = env->GetFieldID(objClass, "mStringData", "Ljava/lang/String;");
	jfieldID objFieldIdStaticShort = env->GetStaticFieldID(objClass, "mStaticShortData", "S");

	/*获取
	jint objFieldInteger = env->GetIntField(obj, objFieldIdInteger);
	jstring objFieldString = (jstring)env->GetObjectField(obj, objFieldIdString);
	jshort objFieldStaticShort = env->GetStaticShortField(objClass, objFieldIdStaticShort);
	*/

	//设置值
	jint objFieldInteger = 29;
	jstring objFieldString = env->NewStringUTF((szMsg + " callback !").c_str());
	jshort objFieldStaticShort = 100;

	env->SetIntField(obj, objFieldIdInteger, objFieldInteger);
	env->SetObjectField(obj, objFieldIdString, objFieldString);
	env->SetStaticShortField(objClass, objFieldIdStaticShort, objFieldStaticShort);

	//回调方法
	jmethodID objMethodId = env->GetMethodID(objClass, "onCallBack", "(Ljava/lang/String;)V");
	
	if (NULL == objMethodId)
		bRet = JNI_FALSE;
	else
		env->CallVoidMethod(obj, objMethodId, env->NewStringUTF("call success !"));

	return bRet;
}