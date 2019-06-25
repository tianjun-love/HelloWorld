#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string>

using namespace std;

#include "../include/utils.h"


/***********************************************************
  Function: runs_distribution
  Description: RUNS_DISTRIBUTION_TEST，游程分布检 ??
                    算法 的实现，算法 实现细节说明
                    参照 国标 GM/T 0005-2012 
  Input:  @n   : 测试样本的长??
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int runs_distribution(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    unsigned int idx1 = 0, idx2 = 0, k = 0;
    int *b = NULL;  // 1游程记录缓存
    int *g = NULL;  // 0 游程记录缓存
    double V = 0, p_value = 0, tmp = 0;
    double *e = NULL;

    e = (double *)calloc(n, sizeof(double));
    if (e == NULL)
    {
		strResult = "进行游程分布检测时,分配内存空间失败";
        return -1;
    }
    
    for (idx1 = 1; idx1 <= n; idx1++)
    {
        tmp = (double)(n - idx1 + 3) / (double)pow(2, idx1 + 2);  
        e[idx1] = tmp;
        if (tmp >= 5)
            k = idx1;
        else
            break;
    }
    
    b = (int *)calloc(k + 1, sizeof(unsigned int));
    if (b == NULL)
    {
		strResult = "进行游程分布检测时,再次分配内存空间失败";
		free(e);
        return -1;
    }

    g = (int *)calloc(k + 1, sizeof(unsigned int));
    if (g == NULL)
    {
		strResult = "进行游程分布检测时,最后一次分配内存空间失败";
		free(e);
		free(b);
        return -1;
    }

    for (idx1 = 1, idx2 = 1; idx1 < n; idx1++, idx2++)
    {
        if (buf[idx1] ^ buf[idx1 - 1])
        {
            if (idx2 > k)
            {
                idx2 = 0;
                continue;
            }
            if (buf[idx1])
                g[idx2]++;
            else
                b[idx2]++;
            idx2 = 0;
        }
    }

    // process the last run
    if (buf[n-1] ^ buf[n-2])
    {
        if (buf[n-1])
            b[1]++;
        else
            g[1]++;
    }
    else
    {
        if (idx2 <= k)
        {
            if (buf[n-1])
                b[idx2]++;
            else
                g[idx2]++;
        }
    }

    //calc the Pvalue
    for (idx1 = 1; idx1 <= k; idx1++)
    {
        V += (double)(((double)b[idx1] - e[idx1]) * ((double)b[idx1] - e[idx1])) / (double)e[idx1];       
        V += (double)(((double)g[idx1] - e[idx1]) * ((double)g[idx1] - e[idx1])) / (double)e[idx1];       
    }

    p_value = igamc((double)(k - 1), V / 2);

	free(e);
    free(b);
    free(g);
	
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
	
    return 0;
}


