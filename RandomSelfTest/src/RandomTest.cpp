#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "../include/utils.h"
#include "../include/RandomTest.h"

using namespace std;

void byteReverse(uint32_t *buffer, int byteCount)
{
	uint32_t value;
	int count;

	byteCount /= sizeof(uint32_t);
	for(count = 0; count < byteCount; count++) 
    {
		value = (buffer[count] << 16) | (buffer[count] >> 16);
		buffer[count] = ((value & 0xFF00FF00L) >> 8) | ((value & 0x00FF00FFL) << 8);
	}
}


RandomTest::RandomTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs)
{
	m_pSample = pSample;
	memset(&m_stRandomTestArgs, 0x00, sizeof(m_stRandomTestArgs));
	memcpy(&m_stRandomTestArgs, &stRandomTestArgs, sizeof(m_stRandomTestArgs));
}

RandomTest::~RandomTest()
{
}

int RandomTest::DoRandomTest(string &strResult)
{
	uint8_t *pConvert = (uint8_t *)calloc(m_stRandomTestArgs.uiSampleSize, sizeof(uint8_t));
	if (pConvert == NULL)
	{
		strResult = "随机数自检时申请内存失败";
		return -1;
	}
	
	if (m_pSample == NULL)
	{
		strResult = "随机数检测时,检测样本为空";
		free(pConvert);
		return -1;
	}
	
	uint32_t uiRead = (m_stRandomTestArgs.uiSampleSize + 31) / 32;
	
	uint32_t uiLineSizeNow = 0;
	uint32_t uiBitsRead    = 0;
	
	uint32_t u32DataTemp = 0;
	
	uint32_t uiLineSize = (m_stRandomTestArgs.uiSampleSize + 7) / 8;
	
    uint32_t uiDisplayMask = 1 << 31;
	
	uint32_t uiRst     = 0;
	uint32_t uiFailCnt = 0;
	
	vector<RandomCnt> vecRst;
	vecRst.clear();
	
	RandomCnt stRandomCnt;
	
	
	for (int i = 0; i < m_stRandomTestArgs.uiSampleNum; i++)
	{
		memset(&stRandomCnt, 0x00, sizeof(stRandomCnt));
		
		uiLineSizeNow = 0;
		uiBitsRead    = 0;
		
		for(int j = 0; j < uiRead; j++)
		{
			if((i >= m_stRandomTestArgs.uiSampleNum) || (uiLineSizeNow >= uiLineSize))
			{
				strResult = "随机数自检时,数组越界";
				free(pConvert);
				return -1;
			}
			
			u32DataTemp = (*(uint32_t *)(m_pSample + i * uiLineSize + uiLineSizeNow * sizeof(uint32_t)));
			byteReverse(&u32DataTemp, 4);
			
			for(int k = 1; k <=32; k++)
			{
				if (u32DataTemp & uiDisplayMask)
				{
					pConvert[uiBitsRead] = 1;
				}
				else
				{
					pConvert[uiBitsRead] = 0;
				}
				
				u32DataTemp <<= 1;
				uiBitsRead++;
				if (uiBitsRead == m_stRandomTestArgs.uiSampleSize)
				{
					break;
				}
			}
			
			++uiLineSizeNow;
		}
		
		uiRst = this->Run(pConvert, strResult, stRandomCnt);

		if (0 != uiRst)
		{
			free(pConvert);
			return -1;
		}
		
		vecRst.push_back(stRandomCnt);
	}

	if (pConvert)
	{
		free(pConvert);
		pConvert = NULL;
	}
	
	double passRate =  0.99-3.0*sqrt(0.01*(1.0-ALPHA)/(double)m_stRandomTestArgs.uiSampleNum);
	
	vector<RandomCnt> :: iterator vit;
	
	int count1 = 0;
	int count2 = 0;
	
	int flag = 0;
	
	for(vit = vecRst.begin(); vit != vecRst.end(); vit++)
	{
		if((*vit).u8ResCnt == 1)
		{
			if((*vit).dfValue1 < ALPHA)
			{
				count1++;
			}
		}
		else if((*vit).u8ResCnt==2)
		{
			flag = 2;
			if((*vit).dfValue2 < ALPHA)
			{
				count2++;	
			}
		}
	}
	
	double proportion = 1.0 - (double)(count1/m_stRandomTestArgs.uiSampleNum);
	if((proportion < passRate) && (fabs(proportion - passRate) > 1e-6))
	{
		strResult = "Proportion flag 1 check failed !";
		return -1;
	}
	
	
	if (flag == 2)
	{
		proportion = 1.0 - (double)(count2/m_stRandomTestArgs.uiSampleNum);
		if((proportion < passRate) && (fabs(proportion - passRate) > 1e-6))
		{
			strResult = "Proportion flag 2 check failed !";
			return -1;
		}
	}
	
	return 0;
}

int RandomTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	strResult = "TestRandomBaseClass";
	return 0;
}


FreauencyTest::FreauencyTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs) : \
			   RandomTest(pSample, stRandomTestArgs)
{
	
}

FreauencyTest::~FreauencyTest()
{
	
}

int FreauencyTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = frequency(m_stRandomTestArgs.uiSampleSize, pConvert, strResult, pRandomCnt);

	return iRet;
}

FreauencyBlockTest::FreauencyBlockTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs) : \
			   RandomTest(pSample, stRandomTestArgs)
{
	m_stRandomTestArgs.uiBlockSize = stRandomTestArgs.uiBlockSize;
}

FreauencyBlockTest::~FreauencyBlockTest()
{
	
}

int FreauencyBlockTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = frequency_within_block(m_stRandomTestArgs.uiSampleSize, m_stRandomTestArgs.uiBlockSize, pConvert, strResult, pRandomCnt);
	return iRet;
}

PokerTest::PokerTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs) : \
			   RandomTest(pSample, stRandomTestArgs)
{
	m_stRandomTestArgs.uiBlockSize = stRandomTestArgs.uiBlockSize;
}

PokerTest::~PokerTest()
{
	
}

int PokerTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = poker(m_stRandomTestArgs.uiSampleSize, m_stRandomTestArgs.uiBlockSize, pConvert, strResult, pRandomCnt);
	return iRet;
}

SerialTest::SerialTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
			RandomTest(pSample, stRandomTestArgs)
{
	m_stRandomTestArgs.uiBlockSize = stRandomTestArgs.uiBlockSize;
}

SerialTest::~SerialTest()
{
	
}

int SerialTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = serial(m_stRandomTestArgs.uiSampleSize, m_stRandomTestArgs.uiBlockSize, pConvert, strResult, pRandomCnt);
	return iRet;
}

RunsTest::RunsTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
			RandomTest(pSample, stRandomTestArgs)
{
	
}

RunsTest::~RunsTest()
{
	
}

int RunsTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = runs(m_stRandomTestArgs.uiSampleSize, pConvert, strResult, pRandomCnt);
	return iRet;
}


RunsDisbTest::RunsDisbTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
			RandomTest(pSample, stRandomTestArgs)
{
	
}

RunsDisbTest::~RunsDisbTest()
{
	
}

int RunsDisbTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = runs_distribution(m_stRandomTestArgs.uiSampleSize, pConvert, strResult, pRandomCnt);
	return iRet;
}




LongestTest::LongestTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
			RandomTest(pSample, stRandomTestArgs)
{
	m_stRandomTestArgs.uiBlockSize = stRandomTestArgs.uiBlockSize;
}

LongestTest::~LongestTest()
{
	
}

int LongestTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = longest_run(m_stRandomTestArgs.uiSampleSize, m_stRandomTestArgs.uiBlockSize, pConvert, strResult, pRandomCnt);
	return iRet;
}



BinaryDectiveTest::BinaryDectiveTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
			       RandomTest(pSample, stRandomTestArgs)
{
	m_stRandomTestArgs.uiBlockSize = stRandomTestArgs.uiBlockSize;
}

BinaryDectiveTest::~BinaryDectiveTest()
{
	
}

int BinaryDectiveTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = binary_derivative(m_stRandomTestArgs.uiSampleSize, m_stRandomTestArgs.uiBlockSize, pConvert, strResult, pRandomCnt);
	return iRet;
}



AutoRelationTest::AutoRelationTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
			       RandomTest(pSample, stRandomTestArgs)
{
	m_stRandomTestArgs.uiBlockSize = stRandomTestArgs.uiBlockSize;
}

AutoRelationTest::~AutoRelationTest()
{
	
}

int AutoRelationTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = auto_correlation(m_stRandomTestArgs.uiSampleSize, m_stRandomTestArgs.uiBlockSize, pConvert, strResult, pRandomCnt);
	return iRet;
}


BinMrTest::BinMrTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
	       RandomTest(pSample, stRandomTestArgs)
{
	
}

BinMrTest::~BinMrTest()
{
	
}

int BinMrTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = binary_matrix_rank(m_stRandomTestArgs.uiSampleSize, pConvert, strResult, pRandomCnt);
	return iRet;
}



Cumulative::Cumulative(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
	        RandomTest(pSample, stRandomTestArgs)
{
	
}

Cumulative::~Cumulative()
{
	
}

int Cumulative::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = cumulative(m_stRandomTestArgs.uiSampleSize, pConvert, strResult, pRandomCnt);
	return iRet;
}

AepTest::AepTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
	        RandomTest(pSample, stRandomTestArgs)
{
	m_stRandomTestArgs.uiBlockSize = stRandomTestArgs.uiBlockSize;
}

AepTest::~AepTest()
{
	
}

int AepTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = approximate_entropy(m_stRandomTestArgs.uiSampleSize,m_stRandomTestArgs.uiBlockSize, pConvert, strResult, pRandomCnt);
	return iRet;
}

LinerTest::LinerTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
	        RandomTest(pSample, stRandomTestArgs)
{
	m_stRandomTestArgs.uiBlockSize = stRandomTestArgs.uiBlockSize;
}

LinerTest::~LinerTest()
{
	
}

int LinerTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = linear_complexity(m_stRandomTestArgs.uiSampleSize, m_stRandomTestArgs.uiBlockSize, pConvert, strResult, pRandomCnt);
	return iRet;
}


MaurerTest::MaurerTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
	        RandomTest(pSample, stRandomTestArgs)
{
	
}

MaurerTest::~MaurerTest()
{
	
}

int MaurerTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = maurer_universal(m_stRandomTestArgs.uiSampleSize, 7, 1280, pConvert, strResult, pRandomCnt);
	return iRet;
}


DftTest::DftTest(uint8_t *pSample, const RandomTestArgs stRandomTestArgs):\
	        RandomTest(pSample, stRandomTestArgs)
{
	
}

DftTest::~DftTest()
{
	
}

int DftTest::Run(uint8_t *pConvert, string &strResult, RandomCnt &pRandomCnt)
{
	int iRet = dft(m_stRandomTestArgs.uiSampleSize, pConvert, strResult, pRandomCnt);
	return iRet;
}

DevRandomTest::DevRandomTest()
{
	m_vecRandomImpl.clear();
}

DevRandomTest::~DevRandomTest()
{
	vector<RandomTest *> :: iterator vit;
	for (vit = m_vecRandomImpl.begin(); vit != m_vecRandomImpl.end(); ++vit)
	{
		delete (*vit);
	}
}

// uiSampleSize 必须为1000000 uiSampleNum 必须为20 pSample 必须为20 * 1000000bit
PowerOnTest::PowerOnTest(uint8_t *pSample, uint32_t uiSampleSize, uint32_t uiSampleNum)
{
	RandomTestArgs stRandomTestArgs;
	memset(&stRandomTestArgs, 0x00, sizeof(RandomTestArgs));
	stRandomTestArgs.uiSampleSize = uiSampleSize;
	stRandomTestArgs.uiSampleNum  = uiSampleNum;

	// 单比特频数检测
	RandomTest *pRandomTest = new FreauencyTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 块内频数检测
	stRandomTestArgs.uiBlockSize = 100;
	pRandomTest = new FreauencyBlockTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);

	// 扑克检测
	stRandomTestArgs.uiBlockSize = 4;
	pRandomTest = new PokerTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 重叠子序列检测
	stRandomTestArgs.uiBlockSize = 2;
	pRandomTest = new SerialTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 游程总数检测
	pRandomTest = new RunsTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 游程分布检测
	pRandomTest = new RunsDisbTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 块内最大1游程检测
	stRandomTestArgs.uiBlockSize = 10000;
	pRandomTest = new LongestTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 二元推倒检测
	stRandomTestArgs.uiBlockSize = 3;
	pRandomTest = new BinaryDectiveTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 自相关检测
	stRandomTestArgs.uiBlockSize = 2;
	pRandomTest = new AutoRelationTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 矩阵秩检测
	pRandomTest = new BinMrTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 累加和检测
	pRandomTest = new Cumulative(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 近似熵检测
	stRandomTestArgs.uiBlockSize = 5;
	pRandomTest = new AepTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 离线傅里叶检测
	pRandomTest = new DftTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 通用统计检测
	pRandomTest = new MaurerTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 线性复杂度检测
	stRandomTestArgs.uiBlockSize = 500;
	pRandomTest = new LinerTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
}

PowerOnTest::~PowerOnTest()
{
	
}

int PowerOnTest::DoTest(string &strRest)
{
	int iRet = 0;
	
	vector<RandomTest *> :: iterator vit;
	
	for (vit = m_vecRandomImpl.begin(); vit != m_vecRandomImpl.end(); ++vit)
	{
		iRet = (*vit)->DoRandomTest(strRest);
		if(iRet)
		{
			break;
		}
	}
	
	return iRet;
}


// uiSampleSize 1000 uiSampleNum 必须为20 pSample 必须为20 * 1000bit
CycleTest::CycleTest(uint8_t *pSample, uint32_t uiSampleSize, uint32_t uiSampleNum)
{
	RandomTestArgs stRandomTestArgs;
	memset(&stRandomTestArgs, 0x00, sizeof(RandomTestArgs));
	stRandomTestArgs.uiSampleSize = uiSampleSize;
	stRandomTestArgs.uiSampleNum  = uiSampleNum;
	
	// 单比特频数检测
	RandomTest *pRandomTest = new FreauencyTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 块内频数检测
	stRandomTestArgs.uiBlockSize = 100;
	pRandomTest = new FreauencyBlockTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);

	// 扑克检测
	stRandomTestArgs.uiBlockSize = 4;
	pRandomTest = new PokerTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 重叠子序列检测
	stRandomTestArgs.uiBlockSize = 2;
	pRandomTest = new SerialTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 游程总数检测
	pRandomTest = new RunsTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 游程分布检测
	pRandomTest = new RunsDisbTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 块内最大1游程检测
	stRandomTestArgs.uiBlockSize = 10000;
	pRandomTest = new LongestTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 二元推倒检测
	stRandomTestArgs.uiBlockSize = 3;
	pRandomTest = new BinaryDectiveTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 自相关检测
	stRandomTestArgs.uiBlockSize = 2;
	pRandomTest = new AutoRelationTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 矩阵秩检测
	pRandomTest = new BinMrTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 累加和检测
	pRandomTest = new Cumulative(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
	// 近似熵检测
	stRandomTestArgs.uiBlockSize = 5;
	pRandomTest = new AepTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);

}

CycleTest::~CycleTest()
{
	
}

int CycleTest::DoTest(string &strRest)
{
	int iRet = 0;
	
	vector<RandomTest *> :: iterator vit;
	
	for (vit = m_vecRandomImpl.begin(); vit != m_vecRandomImpl.end(); ++vit)
	{
		iRet = (*vit)->DoRandomTest(strRest);
		if(iRet)
		{
			break;
		}
	}
	return iRet;
	
}

// uiSampleSize 256 uiSampleNum 必须为1 pSample 必须为256bit
SingleTest::SingleTest(uint8_t *pSample, uint32_t uiSampleSize, uint32_t uiSampleNum)
{
	RandomTestArgs stRandomTestArgs;
	memset(&stRandomTestArgs, 0x00, sizeof(RandomTestArgs));
	stRandomTestArgs.uiSampleSize = uiSampleSize;
	stRandomTestArgs.uiSampleNum  = uiSampleNum;
	stRandomTestArgs.uiBlockSize = 2;
	
	RandomTest *pRandomTest = new PokerTest(pSample, stRandomTestArgs);
	m_vecRandomImpl.push_back(pRandomTest);
	
}

SingleTest::~SingleTest()
{
	
}

int SingleTest::DoTest(string &strRest)
{
	int iRet = 0;
	
	vector<RandomTest *> :: iterator vit;
	
	for (vit = m_vecRandomImpl.begin(); vit != m_vecRandomImpl.end(); ++vit)
	{
		iRet = (*vit)->DoRandomTest(strRest);
		if(iRet)
		{
			break;
		}
	}
	return iRet;
}

int RandomSelfTest(const char *pRandomFileName, int iCheckType)
{
	int iRet = 0;
	string strRes;

	//检测文件是否存在，文件内容可由随机数生成器生成不小于2.3M的随机数填充
	if (NULL == pRandomFileName)
		return -1;

#ifdef _WIN32
	iRet = _access(pRandomFileName, _A_NORMAL);
#else
	iRet = access(pRandomFileName, F_OK);
#endif // WIN32

	if (0 != iRet)
		return -1;

	//检测文件大小
	long long llFileSize = 0;

#ifdef _WIN32
	struct _stat info;
	if (0 == _stat(pRandomFileName, &info))
		llFileSize = info.st_size;
	else
		llFileSize = -1;
#else
	struct stat info;
	if (0 == stat(pRandomFileName, &info))
		llFileSize = info.st_size;
	else
		llFileSize = -1;
#endif

	if (0 != iRet || 2510000 > llFileSize)
		return -1;

	//从文件随机读取一段字节
	uint32_t uiRamdomLen = (20 * 1000000) / 8;
	uint8_t *pRandom = (uint8_t *)malloc(uiRamdomLen);

	FILE *pfile = NULL;

#ifdef WIN32
	if (0 != fopen_s(&pfile, pRandomFileName, "rb+"))
	{
		free(pRandom);
		return -1;
	}
#else
	pfile = fopen(pRandomFileName, "rb+");
#endif

	//从开头也行
	fseek(pfile, 110, SEEK_SET);

	if (pfile == NULL)
	{
		free(pRandom);
		return -1;
	}
	else
	{
		fread(pRandom, 1, uiRamdomLen, pfile);
		fclose(pfile);
	}

	if (1 == iCheckType) //单次自检
	{
		SingleTest clsSingleTest(pRandom, 1, 16);
		iRet = clsSingleTest.DoTest(strRes);
	}
	else if (2 == iCheckType) //上电自检
	{
		PowerOnTest clsPowerOnTest(pRandom, 1000000, 20);
		iRet = clsPowerOnTest.DoTest(strRes);
	}
	else //周期自检
	{
		CycleTest clCycleTest(pRandom, 20000, 20);
		iRet = clCycleTest.DoTest(strRes);
	}

	if (0 != iRet)
		iRet = -1;

	free(pRandom);

	return iRet;
}