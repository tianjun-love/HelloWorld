#include "../include/Semophore.hpp"

CSemophore::CSemophore(int iCount) : m_iCount(iCount), m_iWakeUp(0)
{
}

CSemophore::~CSemophore()
{
}

bool CSemophore::TryAcquire()
{
	if (m_iCount <= 0)
	{
		return false;
	}
	else
	{
		std::unique_lock<std::mutex> lock(m_SemophoreLock);

		if (--m_iCount < 0)
		{
			m_CV.wait(lock, [&]()->bool{ return m_iWakeUp > 0; });
			--m_iWakeUp;
		}

		return true;
	}
}

void CSemophore::Acquire()
{
	std::unique_lock<std::mutex> lock(m_SemophoreLock);

	if (--m_iCount < 0)
	{
		m_CV.wait(lock, [&]()->bool{ return m_iWakeUp > 0; });
		--m_iWakeUp;
	}
}

void CSemophore::Release()
{
	std::unique_lock<std::mutex> lock(m_SemophoreLock);

	if (++m_iCount <= 0)
	{
		++m_iWakeUp;
		m_CV.notify_one();
	}
}