#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <string>
using namespace std;

#include "../include/utils.h"

 void __ogg_fdrffti(int n, double *wsave, int *ifac);
 void __ogg_fdrfftf(int n,double *r,double *wsave,int *ifac);

/***********************************************************
  Function: dft
  Description: DISCRETE_FOURIER_TRANSFORM_TEST，离散傅
                    里叶变换检 ??算法的实??算法
                    实现细节说明 参照国标GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int dft(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    double   p_value, upperBound, percentile, *m = NULL, *X = NULL, *wsave = NULL, *ifac = NULL;
    int      i, count;
    double   N_l, N_o, d;
     
	X = (double*)calloc(n, sizeof(double));
	wsave = (double *)calloc(2 * n + 15, sizeof(double));
	ifac = (double *)calloc(15, sizeof(double));
	m = (double*)calloc(n / 2 + 1, sizeof(double));

    if ((X == NULL) ||
         (wsave == NULL) ||
         (ifac == NULL) ||
         (m == NULL) ) 
    {
        if(X != NULL)
            free(X);
        if(wsave != NULL)
            free(wsave);
        if(ifac != NULL)
            free(ifac);
        if(m != NULL)
            free(m);
		
		strResult = "进行离线傅里叶检测时,分配内存空间失败";
        return -1;
    }
    else 
    {
        for(i=0; i<(int)n; i++)
        {
            X[i] = 2*(int)buf[i] - 1;
        }

        __ogg_fdrffti(n, wsave, (int*)ifac);          /* INITIALIZE WORK ARRAYS */
        __ogg_fdrfftf(n, X, wsave, (int*)ifac);       /* APPLY FORWARD FFT */

        m[0] = sqrt(X[0]*X[0]);     /* COMPUTE MAGNITUDE */

        for( i=0; i<(int)n/2; i++ )      /* DISPLAY FOURIER POINTS */
        {            
            m[i+1] = sqrt( pow(X[2*i+1],2) + pow(X[2*i+2],2) ); 
        }
        count = 0;                     /* CONFIDENCE INTERVAL */
        upperBound = sqrt(2.995732274*n);
        for(i=0; i<(int)n/2; i++)
        {
            if (m[i] < upperBound)
            {
                count++;
            }
        }
        percentile = (double)count/(n/2)*100;
        N_l = (double) count;       /* number of peaks less than h = sqrt(3*n) */
        N_o = (double) 0.95*n/2.;
        d = (N_l - N_o)/sqrt(n/4.*0.95*0.05);
        p_value = erfc_ext(fabs(d)/sqrt(2.));

		free(X);
        free(wsave);
        free(ifac);
        free(m);
		
		stRandomCnt.u8ResCnt = 1;
		stRandomCnt.dfValue1 = p_value;
    }
	
    return 0;
}

