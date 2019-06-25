#include <math.h>
#include <string.h>
#include <stdio.h>

#include <string>
using namespace std;

#include "../include/utils.h"

int frequency_within_block(unsigned int n, unsigned int m, unsigned char * buf, string &strResult, RandomCnt &stRandomCnt)
{
    unsigned int i, j, N;
    double blockSum, arg1, arg2, p_value;
    double sum, pi, v, chi_squared;
            
    N = (unsigned int)floor((double)n / (double)m);       
    sum = 0.0;

    for(i = 0; i < N; i++) 
    {         
        pi = 0.0;
        blockSum = 0.0;
        for(j = 0; j < m; j++)  
        {
            blockSum += buf[j + i*m];
        }
        pi = (double)blockSum/(double)m;
        v = pi - 0.5;
        sum += v * v;
    }
    chi_squared = 4.0 * m * sum;

    arg1 = (double) N/2.e0;
    arg2 = chi_squared/2.e0;
    
    p_value = igamc(arg1, arg2);
	
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
	return 0;
}

