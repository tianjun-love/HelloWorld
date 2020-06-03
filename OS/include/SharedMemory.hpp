/********************************************************************
func   :	共享内存
created:	2015-02-04
author :	田俊
purpose:	
*********************************************************************/
#ifndef __SHARED_MEMORY_HPP__
#define __SHARED_MEMORY_HPP__

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#endif
	
class CSharedMemory  
{
public:
	typedef unsigned int TShMemId;
	typedef unsigned int TSize;
	typedef unsigned char TByte;

public:
	CSharedMemory();
	~CSharedMemory();

	bool open(TShMemId ShMemId, TSize Size); //打开共享内存
	void close();                            //关闭共享内存
	
	operator void*();
	operator TByte*();
	TByte &operator[](TSize Pos);

private:
	CSharedMemory(const CSharedMemory &Other){};
	CSharedMemory &operator=(const CSharedMemory &Other){return *this;};

private:
#ifdef WIN32
	HANDLE m_hMemory;//共享内存句柄
#else
	int m_hMemory;//共享内存句柄
#endif
	void *m_pMemory;//共享内存地址
};

#endif