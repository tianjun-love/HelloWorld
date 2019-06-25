#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <string>
using namespace std;

#include "../include/utils.h"

/***********************************************************
  Function: binary_derivative
  Description: BINARY_DERIVATIVE_TEST，二元推导检 ??
                    算法的实??算法 实现细节说明
                    参照国标GM/T 0005-2012 
  Input:  @n    :测试样本的长??
             @k    : 二元推导重复的次??
             @buf : 测试数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int binary_derivative(unsigned int n, unsigned int k, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    unsigned int i, j;
    int sum = 0;
    double V = 0;
	double sqrt2 = 1.41421356237309504880;
    double p_value = 0;
    
    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n - 1 - i; j++)
        {
             buf[j] = buf[j] ^ buf[j+1];
        }
    }

    for (i = 0; i < n - k; i++)
    {
        sum += (buf[i] << 1) - 1;
    }

    V = fabs(sum) / sqrt(n - k);
    p_value = erfc_ext(fabs(V) / sqrt2);

	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
    return 0;
}

