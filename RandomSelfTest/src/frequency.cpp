#include <math.h>
#include <string.h>
#include <stdio.h>

#include <string>
using namespace std;

#include "../include/utils.h"

int frequency(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
	unsigned int i;
	double f, s_obs, p_value, sum;
	double sqrt2 = 1.41421356237309504880;
		
	sum = 0.0;
	for(i = 0; i < n; i++)
    {   
		sum += 2 * (int)buf[i] - 1; 
    }
	s_obs = fabs(sum)/sqrt(n);
	f = s_obs / sqrt2;
	p_value = erfc_ext(f);
	
	
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
	return 0;
}











