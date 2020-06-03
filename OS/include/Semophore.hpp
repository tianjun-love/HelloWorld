#ifndef __SEMOPHORE_HPP__
#define __SEMOPHORE_HPP__

#include <mutex>
#include <condition_variable>

class CSemophore
{
public:
	CSemophore(int iCount = 1);
	CSemophore(const CSemophore &Other) = delete;
	~CSemophore();

	bool TryAcquire();
	void Acquire();
	void Release();

private:
	int                     m_iCount;        //信号数
	int                     m_iWakeUp;       //唤醒标志
	std::mutex              m_SemophoreLock; //信号锁
	std::condition_variable m_CV;            //条件变量，当不满足条件时，会睡眠直到条件满足

};

#endif