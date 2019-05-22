/*******************************************
功能：公共方法或定义类
作者：田俊
时间：2019-05-07
修改：
*******************************************/
#ifndef __PUBLIC_HPP__
#define __PUBLIC_HPP__

#include <string>

class CPublic
{
public:
	CPublic();
	~CPublic();

	static std::string DateTimeString(short nType = 0); //获取当前时间字符串

private:

};

#endif