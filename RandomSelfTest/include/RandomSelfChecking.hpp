/******************************************************
功能：	随机数自检导出方法
作者：	田俊
时间：	2019-06-12
修改：
******************************************************/
#ifndef __RANDOM_SELF_CHECKING_HPP__
#define __RANDOM_SELF_CHECKING_HPP__

#ifdef JAVA_EXPORT

#include "jni.h"

#ifdef __cplusplus
extern "C" {
#endif

//方法名必须为Java_mxx_dxx_fxx的格式，mxx:java里面的模块名，dxx:动态库名，fxx为方法名
//在windows中执行时，将dll文件放在java项目根目录；在linux中执行时，需要将so文件目录加入LD_LIBRARY_PATH中，不然找不到

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
JNIEXPORT jstring JNICALL Java_com_RandomSelfChecking_RandomSelfTest(JNIEnv *, jobject, const jstring, jint);

#ifdef __cplusplus
}
#endif

#else

int RandomSelfTest(const char *pRandomFileName, int iCheckType);

#endif

#endif