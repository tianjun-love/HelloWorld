#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <string>
using namespace std;

#include "../include/utils.h"

//extern double erf (double x);
double normal(double x);

/***********************************************************
  Function: cumulative
  Description: CUMULATIVE_TEST??累加和检 ????
                    法的实现,算法 实现细节说明
                    参照国标GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int cumulative(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    int    i, k, start, finish, tmp;
    double p_value, cusum, z, sum, sum1, sum2;
        
    sum = 0.0;
    cusum = 1;
    tmp = (int)n;
    for(i=0; i<tmp; i++) 
    {
        sum += 2*(int)buf[i] - 1;
        cusum = MAX(cusum, fabs(sum));
    }
    z = cusum;

    sum1 = 0.0;
    start = (-tmp/(int)z+1)/4;
    finish = (tmp/(int)z-1)/4;
    for(k=start; k<=finish; k++)
    {
        sum1 += (normal((4*k+1)*z/sqrt(tmp))-normal((4*k-1)*z/sqrt(tmp)));
    }

    sum2 = 0.0;
    start = (-tmp/(int)z-3)/4;
    finish = (tmp/(int)z-1)/4;
    for(k=start; k<=finish; k++)
    {
        sum2 += (normal((4*k+3)*z/sqrt(tmp))-normal((4*k+1)*z/sqrt(tmp)));
    }
    p_value = 1.0 - sum1 + sum2;
	
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
    return 0;
}

double normal(double x)
{
	double arg, result, sqrt2=1.414213562373095048801688724209698078569672;

	if (x > 0) 
    {
		arg = x/sqrt2;
		result = 0.5 * (1 + erf_ext(arg));
	}
	else 
    {
		arg = -x/sqrt2;
		result = 0.5 * (1 - erf_ext(arg));
	}

	return(result);
}
