/******************************************************
功能：	JNI接口测试
作者：	田俊
时间：	2019-06-20
修改：
******************************************************/
#ifndef __JNI_TEST_HPP__
#define __JNI_TEST_HPP__

#include "jni.h"

/*JNI方法说明
格式：JNIEXPORT 返回类型 JNICALL Java_包名_类名_方法名(JNIEnv *env, jobject obj, arg)
参数：env和obj是固定参数，自定义参数arg则跟在后面，可以多个
*/

#ifdef __cplusplus
extern "C" {
#endif

	JNIEXPORT jstring JNICALL Java_com_JNITest_Hello(JNIEnv *env, jobject obj, jstring name, jint age);
	JNIEXPORT jbyteArray JNICALL Java_com_JNITest_ByteArray(JNIEnv *env, jobject obj, jbyteArray bytes);
	JNIEXPORT jboolean JNICALL Java_com_JNITest_CallBack(JNIEnv *env, jobject obj, jstring msg);

#ifdef __cplusplus
}
#endif

#endif