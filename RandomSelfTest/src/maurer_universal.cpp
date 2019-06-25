#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <string>
using namespace std;

#include "../include/utils.h"

/***********************************************************
  Function: maurer_universal
  Description: MAURER_UNIVERSAL_TEST，Maurer通用统计检
                    ??算法的实??算法 实现细节说明
                    参照国标GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @sample_size : 字块的数??
             @sample_num   :  字块的长??
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int maurer_universal(unsigned int n, unsigned int sample_size, 
                     unsigned int sample_num, unsigned char *buf, string &strResult,
					 RandomCnt &stRandomCnt)
{
    int      i, j, p, K;
    double   arg, sqrt2, sigma, phi, sum, p_value, c;
    long*    T, decRep;
    double   expected_value[17] = {0, 0, 0, 0, 0, 0, 5.2177052, 6.1962507,
                                   7.1836656, 8.1764248, 9.1723243, 10.170032,
                                   11.168765, 12.168070, 13.167693, 14.167488,
                                   15.167379};
    double   variance[17] = {0, 0, 0, 0, 0, 0, 2.954, 3.125, 3.238, 3.311,
                             3.356, 3.384, 3.401, 3.410, 3.416, 3.419, 3.421};
    int Q, L;
    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     * THE FOLLOWING REDEFINES L, SHOULD THE CONDITION:     n >= 1010*2^L*L      *
     * NOT BE MET, FOR THE BLOCK LENGTH L.                                       *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

	 
    if ( n >= 387840 )     L = 6;
    if ( n >= 904960 )     L = 7;
    if ( n >= 2068480 )    L = 8;
    if ( n >= 4654080 )    L = 9;
    if ( n >= 10342400 )   L = 10;
    if ( n >= 22753280 )   L = 11;
    if ( n >= 49643520 )   L = 12;
    if ( n >= 107560960 )  L = 13;
    if ( n >= 231669760 )  L = 14;
    if ( n >= 496435200 )  L = 15;
    if ( n >= 1059061760 ) L = 16;

    Q = 10*(int)pow(2, L);
    K = (int) (floor(n/L) - (double)Q);             /* BLOCKS TO TEST */
	
    if ((L < 6) || (L > 16)) 
    {
		strResult = "进行通用检测时,检测到参数L错误";
        return -1;
    }
    else if ((double)Q < 10*pow(2,L))
    {
		strResult = "进行通用检测时,检测到参数Q错误";
        return -1;
    }
    else 
    {
        c = 0.7 - 0.8/(double)L + (4 + 32/(double)L)*pow(K,-3/(double)L)/15;
        sigma = c * sqrt(variance[L]/(double)K);
        sqrt2 = sqrt(2);
        sum = 0.0;
        p = (int)pow(2,L);
        T = (long*) calloc(p, sizeof(long));
        for(i=0; i<p; i++)
            T[i] = 0;
        for(i=1; i<=(int)Q; i++)          /* INITIALIZE TABLE */
        {     
            decRep = 0;
            for(j=0; j<(int)L; j++)
                decRep += buf[(i-1)*L+j] * (long)pow(2,L-1-j);
            T[decRep] = i;
        }
        for(i=(int)Q+1; i<=(int)Q+K; i++)     /* PROCESS BLOCKS */
        {  
            decRep = 0;
            for(j=0; j<(int)L; j++)
                decRep += buf[(i-1)*L+j] * (long)pow(2,L-1-j);
            sum += log(i - T[decRep])/log(2);
            T[decRep] = i;
        }
        phi = (double)(sum/(double)K);

        arg = fabs(phi-expected_value[L])/(sqrt2 * sigma);
        p_value = erfc_ext(arg);

        free(T);
		
		stRandomCnt.u8ResCnt = 1;
		stRandomCnt.dfValue1 = p_value;
		
	    return 0;
    }
}
