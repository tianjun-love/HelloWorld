#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

#ifndef _WIN32
#include <unistd.h>
#endif

#include "../include/utils.h"

int psi_squre(unsigned int n, unsigned int m, unsigned char *buf, double *psi);

/***********************************************************
  Function: serial
  Description: SERIAL_TEST，重叠子序列检 ????
                    法的实现,算法 实现细节????
                    参照国标GM/T 0005-2012 
  Input:  @n    : 测试样本的长??
             @m   : 子序列的长度
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int serial(unsigned int n, unsigned int m, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    double p_value1, p_value2, psim0, psim1, psim2, del1, del2;
    int rslt;

    rslt = psi_squre(n, m, buf, &psim0);
    if (rslt != 0)
    {
        return rslt;
    }
    
    rslt = psi_squre(n, m-1, buf, &psim1);
    if (rslt != 0)
    {
        return rslt;
    }
    
    rslt = psi_squre(n, m-2, buf, &psim2);
    if (rslt != 0)
    {
        return rslt;
    }
    
    del1 = psim0 - psim1;
    del2 = psim0 - 2.0*psim1 + psim2;
    p_value1 = igamc(pow(2,m-1)/2,del1/2.0);
    p_value2 = igamc(pow(2,m-2)/2,del2/2.0);
    
	stRandomCnt.u8ResCnt = 2;
	stRandomCnt.dfValue1 = p_value1;
	stRandomCnt.dfValue2 = p_value2;
	
    return 0;
}

/***********************************************************
  Function: psi_squre
  Description: 重叠子序列检 ??中一??psi运算??
                    算法的实现细节说 ??参照??
                    标GM/T 0005-2012 
  Input:  @n    : 测试样本的长??
             @m   : 子序列的长度
             @buf : 测试样本数据的缓??
  Output: @psi : 计算所得和??
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int psi_squre(unsigned int n, unsigned int m, unsigned char *buf, double *psi)
{
	unsigned int i, j, k, powLen;
	double sum, numOfBlocks;
	unsigned int *P;

	if ((m == 0) || (m == -1))
    {   
        *psi = 0.0;
		return 0;
    }
	numOfBlocks = n;
	powLen = (unsigned int)pow(2, m+1) - 1;

    P = (unsigned int*)calloc(powLen, sizeof(unsigned int));
	if (P == NULL) 
    {
		return -1;
	}
	for(i=1; i<powLen-1; i++)
    {   
		P[i] = 0;	
    }
	for(i=0; i<numOfBlocks; i++) 
    {       
		k = 1;
		for(j=0; j<m; j++) 
        {
			if (buf[(i+j)%n] == 0)
            {         
				k *= 2;
            }
			else if (buf[(i+j)%n] == 1)
            {         
				k = 2*k+1;
            }
        }
		P[k-1]++;
	}

	sum = 0.0;
	for(i=(unsigned int)pow(2,m)-1; i<(unsigned int)pow(2,m+1)-1; i++)
    {   
		sum += pow(P[i],2);
    }
	sum = (sum * pow(2,m)/(double)n) - (double)n;
	free(P);
    *psi = sum;
    
	return 0;
}

