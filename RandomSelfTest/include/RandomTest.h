#ifndef _H_RANDOM_TEST_H_
#define _H_RANDOM_TEST_H_

#include <string>
#include <vector>
#include "utils.h"

using namespace std;

typedef struct
{
	uint32_t uiSampleSize;
	uint32_t uiSampleNum;
	uint32_t uiBlockSize;
	
} RandomTestArgs;

// 测试基类
class RandomTest
{
	public:
		RandomTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		virtual ~RandomTest() = 0;
	public:
		int DoRandomTest(string &strResult);
		virtual int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
	protected:
		uint8_t* m_pSample;
		RandomTestArgs m_stRandomTestArgs;
};

// 单比特频数检测
class FreauencyTest : public RandomTest
{
	public:
		FreauencyTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~FreauencyTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 块内频数检测
class FreauencyBlockTest : public RandomTest
{
	public:
		FreauencyBlockTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~FreauencyBlockTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 扑克检测
class PokerTest : public RandomTest
{
	public:
		PokerTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~PokerTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 重叠子序列检测
class SerialTest : public RandomTest
{
	public:
		SerialTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~SerialTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 游程总数检测
class RunsTest : public RandomTest
{
	public:
		RunsTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~RunsTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 游程分布检测
class RunsDisbTest : public RandomTest
{
	public:
		RunsDisbTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~RunsDisbTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 块内最大1游程检测
class LongestTest : public RandomTest
{
	public:
		LongestTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~LongestTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 二元推倒检测
class BinaryDectiveTest : public RandomTest
{
	public:
		BinaryDectiveTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~BinaryDectiveTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 自相关检测
class AutoRelationTest : public RandomTest
{
	public:
		AutoRelationTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~AutoRelationTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 矩阵秩检测
class BinMrTest : public RandomTest
{
	public:
		BinMrTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~BinMrTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 累加和检测
class Cumulative : public RandomTest
{
	public:
		Cumulative(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~Cumulative();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 近似熵检测
class AepTest : public RandomTest
{
	public:
		AepTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~AepTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 线性复杂度检测
class LinerTest : public RandomTest
{
	public:
		LinerTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~LinerTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 通用统计检测
class MaurerTest : public RandomTest
{
	public:
		MaurerTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~MaurerTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

// 离线傅里叶检测
class DftTest : public RandomTest
{
	public:
		DftTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs);
		~DftTest();
	public:
		int Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt);
};

class DevRandomTest
{
	public:
		DevRandomTest();
		~DevRandomTest();
	public:
		virtual int DoTest(string &strRest) = 0;
	
	protected:
		vector<RandomTest *> m_vecRandomImpl;
};

// 周期自检
class CycleTest : public DevRandomTest
{
	public:
		CycleTest(uint8_t *pSample, uint32_t uiSampleSize, uint32_t uiSampleNum);
		~CycleTest();
	public :
		int DoTest(string &strRest);
};

// 开机自检
class PowerOnTest : public DevRandomTest
{
	public:
		PowerOnTest(uint8_t *pSample, uint32_t uiSampleSize, uint32_t uiSampleNum);
		~PowerOnTest();
	public :
		int DoTest(string &strRest);
};

// 单次自检
class SingleTest : public DevRandomTest
{
	public:
		SingleTest(uint8_t *pSample, uint32_t uiSampleSize, uint32_t uiSampleNum);
		~SingleTest();
	public :
		int DoTest(string &strRest);
};

int RandomSelfTest(const char *pRandomFileName, int iCheckType);

#endif