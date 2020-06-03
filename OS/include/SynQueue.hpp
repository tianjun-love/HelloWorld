/********************************************************************
created:	2014/11/07
file base:	SynQueue
file ext:	hpp
author:		tianjun
purpose:	sync queue
*********************************************************************/
#ifndef __SYN_QUEUE_HPP__
#define __SYN_QUEUE_HPP__

#include "Semophore.hpp"
#include <mutex>
#include <vector>

using std::mutex;
using std::vector;

#define SYN_QUEUE_DEFAULT_SIZE (unsigned int)10000

template <class T>
class SynQueue
{
public:
	SynQueue(unsigned int Size = SYN_QUEUE_DEFAULT_SIZE) :
		m_QueueContainer(Size), m_ReadSemaphore(Size), m_WriteSemaphore(Size)
	{
		//初始无可读数据。
		for (unsigned int i = 0; i < Size; i++)
		{
			m_ReadSemaphore.Acquire();
		}

		m_WirteCur = 0;
		m_ReadCur = 0;
		m_UsedSize = 0;

		m_QueueSize = Size;
	};

	SynQueue(unsigned int Size, const T &InitVal) :
		m_QueueContainer(Size, InitVal), m_ReadSemaphore(Size), m_WriteSemaphore(Size)
	{
		//初始无可读数据。
		for (unsigned int i = 0; i < Size; i++)
		{
			m_ReadSemaphore.Acquire();
		}

		m_WirteCur = 0;
		m_ReadCur = 0;
		m_UsedSize = 0;

		m_QueueSize = Size;
	};

	~SynQueue()
	{
	};

	void Put(const T &Object)
	{
		m_WriteSemaphore.Acquire();

		m_DataPutLock.lock();

		m_QueueContainer[m_WirteCur] = Object;
		m_WirteCur = (m_WirteCur + 1) % m_QueueSize;

		m_UsedSizeLock.lock();
		m_UsedSize++;
		m_UsedSizeLock.unlock();

		m_DataPutLock.unlock();

		m_ReadSemaphore.Release();
	};

	bool TryPut(const T &Object)
	{
		bool bResult = false;

		if (m_WriteSemaphore.TryAcquire())
		{
			m_DataPutLock.lock();

			m_QueueContainer[m_WirteCur] = Object;
			m_WirteCur = (m_WirteCur + 1) % m_QueueSize;

			m_UsedSizeLock.lock();
			m_UsedSize++;
			m_UsedSizeLock.unlock();

			m_DataPutLock.unlock();

			m_ReadSemaphore.Release();

			bResult = true;
		}

		return bResult;
	};

	void Get(T &Object)
	{
		m_ReadSemaphore.Acquire();

		m_DataGettLock.lock();

		Object = m_QueueContainer[m_ReadCur];
		m_ReadCur = (m_ReadCur + 1) % m_QueueSize;

		m_UsedSizeLock.lock();
		m_UsedSize--;
		m_UsedSizeLock.unlock();

		m_DataGettLock.unlock();

		m_WriteSemaphore.Release();
	};

	bool TryGet(T &Object)
	{
		bool bResult = false;

		if (m_ReadSemaphore.TryAcquire())
		{
			m_DataGettLock.lock();

			Object = m_QueueContainer[m_ReadCur];
			m_ReadCur = (m_ReadCur + 1) % m_QueueSize;

			m_UsedSizeLock.lock();
			m_UsedSize--;
			m_UsedSizeLock.unlock();

			m_DataGettLock.unlock();

			m_WriteSemaphore.Release();

			bResult = true;
		}

		return bResult;
	};

	unsigned int GetUseSize() const
	{
		return m_UsedSize;
	};

	unsigned int GetQueueSize() const
	{
		return m_QueueSize;
	};

private:
	vector<T>    m_QueueContainer; //数据队列
	unsigned int m_QueueSize;      //队列大小

	CSemophore   m_ReadSemaphore;  //读信号量
	mutex        m_DataGettLock;   //读锁
	unsigned int m_ReadCur;        //读下标

	CSemophore   m_WriteSemaphore; //写信号量
	mutex        m_DataPutLock;    //写锁
	unsigned int m_WirteCur;       //写下标
	
	unsigned int m_UsedSize;       //已使用大小
	mutex        m_UsedSizeLock;   //修改已使用大小锁
};

#endif //!__SYN_QUEUE_HPP__
