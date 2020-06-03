#include "../include/SharedMemory.hpp"
#include <sstream>
	
CSharedMemory::CSharedMemory()
{
#ifdef _WIN32
	m_hMemory = NULL;
	m_pMemory = NULL;
#endif
}

CSharedMemory::~CSharedMemory()
{
	close();
}

bool CSharedMemory::open(TShMemId ShMemId, TSize Size)
{
	bool bResult = false;
	close();

#ifdef _WIN32
	std::stringstream Convert;
	Convert<<ShMemId;

	m_hMemory = ::CreateFileMapping(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        Size,                    // maximum object size (low-order DWORD)
		(LPCWSTR)(Convert.str().c_str()));

	if (NULL != m_hMemory)
	{
		//获取内存映射
		m_pMemory = ::MapViewOfFile(m_hMemory, // handle to map object
			FILE_MAP_ALL_ACCESS, // read/write permission
			0, 0, Size);
		
		if (NULL != m_pMemory)
		{
			bResult = true;
		}
	}
#else
	m_hMemory = ::shmget(ShMemId, Size, IPC_CREAT | S_IRUSR| S_IWUSR );
	if (-1 != m_hMemory)
	{
		m_pMemory = (void*)::shmat(m_hMemory, 0, 0);

		if (NULL != m_pMemory)
		{
			bResult = true;
		}
	}
#endif

	if (!bResult)
	{
		close();
	}

	return bResult;
}

void CSharedMemory::close()
{
#ifdef _WIN32
	if (NULL != m_hMemory)
	{
		if (NULL != m_pMemory)
		{
			::UnmapViewOfFile(m_pMemory);
			m_pMemory = NULL;
		}

		::CloseHandle(m_hMemory);
		m_hMemory = NULL;
	}
#else
	if (-1 != m_hMemory)
	{
		if (NULL != m_pMemory)
		{
			::shmdt(m_pMemory);
			m_pMemory = NULL;
		}
		
		::shmctl(m_hMemory, IPC_RMID, 0);/* 释放这个共享内存块 */
		m_hMemory = NULL;
	}
#endif
}

CSharedMemory::operator void*()
{
	return (void*)m_pMemory;
}

CSharedMemory::operator CSharedMemory::TByte*()
{
	return (TByte*)m_pMemory;
}

CSharedMemory::TByte &CSharedMemory::operator[](TSize Pos)
{
	return ((TByte*)m_pMemory)[Pos];
}