#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <string>
using namespace std;

#include "../include/utils.h"

/***********************************************************
  Function: approximate_entropy
  Description: APPROXIMATE_ENTROPY_TEST??近似熵检 ??
                    算法的实??算法 实现细节说明
                    参照国标GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @buf : 测试样本数据的缓??
  Output: none
  Return: 参看枚举RSLT_CODE
  Others: none
************************************************************/
int approximate_entropy(unsigned int n, unsigned int m, unsigned char *buf,string &strResult, RandomCnt &stRandomCnt)
{
    int           i, j, k, r, blockSize, seqLength;
    int           powLen, index;
    double        sum, numOfBlocks, ApEn[2], apen, chi_squared, p_value;
    unsigned int* P;
    
    seqLength = n;
    r = 0;
    
    for(blockSize=(int)m; blockSize<=(int)m+1; blockSize++) 
    {
        if (blockSize == 0) 
        {
            ApEn[0] = 0.00;
            r++;
        }
        else 
        {
            numOfBlocks = (double)seqLength;
            powLen = (int)pow(2,blockSize+1)-1;
            P = (unsigned int*)calloc(powLen,sizeof(unsigned int));
            if (P == NULL )
            {
				strResult = "进行近似熵检测时,分配内存空间失败";
                return -1;
            }
            for(i=1; i<powLen-1; i++)
            {
                P[i] = 0;
            }
            for(i=0; i<numOfBlocks; i++) 
            { 
                k = 1;
                for( j=0; j<blockSize; j++) 
                {
                    if ((int)buf[(i+j)%seqLength] == 0)
                    {
                        k *= 2;
                    }
                    else if ((int)buf[(i+j)%seqLength] == 1)
                    {
                        k = 2*k+1;
                    }
                }
                P[k-1]++;
            }

            sum = 0.0;
            index = (int)pow(2, blockSize)-1;
            for(i=0; i<(int)pow(2, blockSize); i++) 
            {
                if (P[index] > 0)
                {
                    sum += P[index]*log(P[index]/numOfBlocks);
                }
                index++;
            }
            sum /= numOfBlocks;
            ApEn[r] = sum;
            r++;
            free(P);
        }
    }
    apen = ApEn[0] - ApEn[1];
    chi_squared = 2.0*seqLength*(log(2) - apen);
    p_value = igamc(pow(2,m-1),chi_squared/2.);

	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
    return 0;
}


