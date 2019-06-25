#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <string>
using namespace std;

#include "../include/utils.h"

/***********************************************************
  Function: auto_correlation
  Description: AUTOCORRELATION_TEST，自相关检 ??
                    算法的实??算法 实现细节说明
                    参照国标GM/T 0005-2012 
  Input:  @n    : 测试样本的长??
             @d    : 自相关左偏移的位??
             @buf : 测试数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int auto_correlation(unsigned int n, unsigned int d, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    unsigned int i;
    unsigned int Ad = 0;
	double sqrt2 = 1.41421356237309504880;
    double p_value = 0;
    double V = 0;
    
    for (i = 0; i < n - d; i++)
    {
        Ad += buf[i] ^ buf[i+d];    
    }
    
    V = (double)(2*((double)Ad - (double)((n-d)/2))) / sqrt(n-d);
    
    p_value = erfc_ext(fabs(V) / sqrt2);
 
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
    return 0;
}


