#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <string>
using namespace std;

#include "../include/utils.h"

/***********************************************************
  Function: runs
  Description: RUNS_TEST??游程总数检 ??算法??
                    实现,算法 实现细节说明 参照
                    国标GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int runs(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    unsigned int i;
    unsigned int *r;        
    double argument, pi, V_n_obs, tau;
    double p_value, product, sum;
	
    r = (unsigned int *)calloc(n, sizeof(unsigned int));
    if (r  == NULL ) 
    {
		strResult = "进行游程总数检测时,申请空间失败";
        return -1;
    }

    sum = 0.0;
    for(i = 0; i < n; i++)
    {
        sum += (unsigned int)buf[i];
    }
    pi = sum/n;
    tau = 2.0/sqrt(n);
    if (fabs(pi - 0.5) < tau) 
    {
        for(i = 0; i < n-1; i++) 
        {
            if ((unsigned int)buf[i] == (unsigned int)buf[i+1])
            {
                r[i] = 0;
            }
            else
            {
                r[i] = 1;
            }
        }
        V_n_obs = 0;
        for(i = 0; i < n-1; i++)
        {
            V_n_obs += r[i];
        }
        V_n_obs++;
        product = pi * (1.e0 - pi);
        argument = fabs(V_n_obs - 2.e0*n*product)/(2.e0*sqrt(2.e0*n)*product);
        p_value = erfc_ext(argument);
    }
    else 
    {
        p_value = 0.0;
    }
	free(r);
	
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
    return 0;
}

