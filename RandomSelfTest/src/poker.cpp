#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <string>
using namespace std;

#include "../include/utils.h"

/***********************************************************
  Function: poker
  Description: POKER_TEST，扑克检 ??算法的实现，
                    算法 实现细节说明 参照国标
                    GM/T 0005-2012 
  Input:  @n   : 测试样本的长??
             @m  : 子序列的长度
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int poker(unsigned int n, unsigned int m, unsigned char *buf,string &strResult, RandomCnt &stRandomCnt)
{
    unsigned int i, offset, sb_blck_idx;
    unsigned int *sb_blck_cnt = NULL;
    unsigned long possible;
    unsigned long tmp;
    unsigned int N, real_n;
    double V, p_value;
   
    if (m > 16)
    {
		strResult = "进行扑克测试时,序列长度超过了16";
        return -1;
    }
    possible = 1 << m;
    sb_blck_cnt = (unsigned int*)calloc(possible, sizeof(unsigned int));
    if (sb_blck_cnt == NULL)
    {   
		strResult = "进行随机数扑克检测时,申请空间失败";
        return -1;
    }

    real_n = n - n % m;
    for (i=0, offset=0, sb_blck_idx=0; i < real_n; i++)
    {
        tmp = buf[i];
        sb_blck_idx += tmp << offset;   
        offset++;
        if (offset >= m)
        {
            offset = 0;
            sb_blck_cnt[sb_blck_idx]++;
            sb_blck_idx = 0;
        }
    }

    tmp = 0;
    for(i = 0; i < possible; i++)
    {
        tmp += sb_blck_cnt[i] * sb_blck_cnt[i];
    }
    
    N = n / m;
    V = (double)possible / (double)N * tmp - N;
    p_value = igamc((double)(possible - 1) / 2, V / 2);
	
	free(sb_blck_cnt);
	
	
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
    
    return 0;
}

