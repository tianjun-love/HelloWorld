#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <string>
using namespace std;

#include "../include/utils.h"

/***********************************************************
  Function: linear_complexity
  Description: LINEAR_COMPLEXITY_TEST??线性复杂度检
                    ??算法的实??算法 实现细节说明
                    参照国标GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int linear_complexity(unsigned int n, unsigned int m, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    int       i, ii, j, d, N;
    int       L, M, N_, parity, sign;
    double    p_value, T_, mean;
    int       K = 6;
    double    pi[7]={0.01047,0.03125,0.12500,0.50000,0.25000,0.06250,0.020833};
    double    nu[7], chi2;
    unsigned char  *T, *P, *B_, *C;
    
    N = (int)floor(n/m);
    B_ = (unsigned char *)calloc(m,sizeof(unsigned char));
    if (B_ == NULL)
    {    
		strResult = "进行线性复杂度检测时,第一次分配内存空间失败";
        return -1;
    }
    C  = (unsigned char *)calloc(m,sizeof(unsigned char));
    if (C == NULL)
    {    
		strResult = "进行线性复杂度检测时,第二次分配内存空间失败";
        free(B_);
        return -1;
    }    
    P  = (unsigned char *)calloc(m,sizeof(unsigned char));
    if (P == NULL)
    {       
		strResult = "进行线性复杂度检测时,第三次分配内存空间失败";
        free(B_);
        free(C);
        return -1;
    }

    T  = (unsigned char *)calloc(m,sizeof(unsigned char));
    if (T == NULL)
    {       
		strResult = "进行线性复杂度检测时,第四次分配内存空间失败";
        free(B_);
        free(C);
        free(P);
        return -1;
    }
	
    for (i=0; i<K+1; i++)
    {
        nu[i] = 0.00;
    }
    for (ii=0; ii<N; ii++) 
    {
        for (i=0; i<(int)m; i++) 
        {
            B_[i] = 0;
            C[i] = 0;
            T[i] = 0;
            P[i] = 0;
        }

        L = 0;
        M = -1;
        d = 0;
        C[0] = 1;
        B_[0] = 1;

        /* DETERMINE LINEAR COMPLEXITY */
        N_ = 0;
        while (N_ < (int)m) 
        {
            d = (int)buf[ii*m+N_];
            for (i=1; i<=L; i++)
            {
                d += (int)C[i]*(int)buf[ii*m+N_-i];
            }
            d = d%2;
            if (d == 1) 
            {
                for (i=0; i<(int)m; i++) 
                {
                    T[i] = C[i];
                    P[i] = 0;
                }

                for (j=0; j<(int)m; j++)
                {
                    if (B_[j] == 1)
                    {
                        P[j+N_-M] = 1;
                    }
                }

                for (i=0; i<(int)m; i++)
                {
                    C[i] = (C[i] + P[i])%2;
                }

                if (L <= N_/2) 
                {
                    L = N_ + 1 - L;
                    M = N_;
                    for (i=0; i<(int)m; i++)
                    {
                        B_[i] = T[i];
                    }
                }
            }
            N_++;
        }
        if ((parity = (m+1)%2) == 0) 
        {
            sign = -1;
        }
        else 
        {
            sign = 1;
        }
        mean = m/2. + (9.+sign)/36. - 1./pow(2,m) * (m/3. + 2./9.);
        if ((parity = m%2) == 0) 
        {
            sign = 1;
        }
        else 
        {
            sign = -1;
        }
        T_ = sign * (L - mean) + 2./9.;

        if (T_ <= -2.5)
        {
            nu[0]++;
        }
        else if (T_ > -2.5 && T_ <= -1.5)
        {
            nu[1]++;
        }
        else if (T_ > -1.5 && T_ <= -0.5)
        {
            nu[2]++;
        }
        else if (T_ > -0.5 && T_ <= 0.5)
        {
            nu[3]++;
        }
        else if (T_ > 0.5 && T_ <= 1.5)
        {
            nu[4]++;
        }
        else if (T_ > 1.5 && T_ <= 2.5)
        {
            nu[5]++;
        }
        else
        {
            nu[6]++;
        }
    }
    chi2 = 0.00;
    for (i=0; i<K+1; i++)
    {
        chi2 += pow(nu[i]-N*pi[i],2)/(N*pi[i]);
    }
    p_value = igamc(K/2.0,chi2/2.0);
    
	
    free(B_);
    free(P);
    free(C);
    free(T);
	
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
    return 0;
}
