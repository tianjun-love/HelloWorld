#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <string>
using namespace std;

#include "../include/utils.h"

double sub_searial_10000len(unsigned int n, unsigned char *buf);
double sub_searial_128len(unsigned int n, unsigned char *buf);
double sub_searial_8len(unsigned int n, unsigned char *buf);

/***********************************************************
  Function: longest_run
  Description: TEST_FOR_THE_LONGEST_RUN_OF_ONES_IN_A_BLOCK??
                    块内最??游程检??算法的实现，
                    算法 实现细节说明 参照国标
                    GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @m   : 子序列测长度
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int longest_run(unsigned int n, unsigned int m, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    double p_value = 0;

    switch(m)
    {
        case 10000:
            p_value = sub_searial_10000len(n, buf);
            break;
        case 128:
            p_value = sub_searial_128len(n, buf);
            break;
        case 8:
            p_value = sub_searial_8len(n, buf);
            break;
        default:
			strResult = "进行块内最大1游程检测时,传入的子序列长度不合法";
            return -1;
    }
	
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
    return 0;
}

/***********************************************************
  Function: sub_searial_10000len
  Description: TEST_FOR_THE_LONGEST_RUN_OF_ONES_IN_A_BLOCK??
                    块内最??游程检??算法子块长度
                    ??0000时的实现??算法 实现细节??
                    ??参照国标 GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @m   : 子序列测长度
             @buf : 测试样本数据的缓??
  Output: none
  Return: p_value: P-value??
  Others: none
************************************************************/
double sub_searial_10000len(unsigned int n, unsigned char *buf)
{
	double run, v_n_obs, p_value, sum, chi2;
	int    N, i, j;
	double pi[7] = {0.0882, 0.2092, 0.2483, 0.1933, 0.1208, 0.0675, 0.0727};
	double K = 6;
	unsigned int nu[7] = {0, 0, 0, 0, 0, 0, 0};
	int M = 10000;

	N = (int)floor(n/M);
	for(i = 0; i < N; i++)  
    {
		v_n_obs = 0.e0;
		run = 0.e0;
		for(j = i*M; j < (i+1)*M; j++) 
        {
			if ((int)buf[j] == 1) 
            {
				run++;
				v_n_obs = MAX(v_n_obs, run); 
			}
			else 
            {         
				run = 0.e0;
            }
		}
        
        if ( (int)v_n_obs <= 10 )
        {
			nu[0]++;
        }
		else
        {      
			switch((int)v_n_obs) 
            {
    			case 11:
    				nu[1]++;
    				break;
    			case 12:
    				nu[2]++;
    				break;
    			case 13:
    				nu[3]++;
    				break;
    			case 14:
    				nu[4]++;
    				break;
    			case 15:
    				nu[5]++;
    				break;
    			default:
    				nu[6]++;
    				break;
			}
        }
    }

    chi2 = 0.0;					
	sum = 0;
	for(i = 0; i < K+1; i++) 
    {
		chi2 += pow((double)nu[i] - (double)N*pi[i],2)/((double)N*pi[i]);
		sum += nu[i];
	}
	p_value = igamc(K/2.,chi2/2.);

    return p_value;
}

/***********************************************************
  Function: sub_searial_128len
  Description: TEST_FOR_THE_LONGEST_RUN_OF_ONES_IN_A_BLOCK??
                    块内最??游程检??算法子块长度
                    ??28时的实现??算法 实现细节??
                    ??参照国标 GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @m   : 子序列测长度
             @buf : 测试样本数据的缓??
  Output: none
  Return: p_value: P-value??
  Others: none
************************************************************/
double sub_searial_128len(unsigned int n, unsigned char *buf)
{
	double run, v_n_obs, p_value, sum, chi2;
	int    N, i, j;
	double pi[6] = {0.1174035788, 0.242955959, 0.249363483, 0.17517706,
					0.1027010710, 0.112398847};
	double k[6] = {4, 5, 6, 7, 8, 9};
	double K = 5;
	unsigned int nu[6] = {0, 0, 0, 0, 0, 0};
	int M = 128;

	N = (int)floor(n/M);
	for(i = 0; i < N; i++)  
    {
		v_n_obs = 0.e0;
		run = 0.e0;
		for(j = i*M; j < (i+1)*M; j++) 
        {
			if ((int)buf[j] == 1) 
            {
				run++;
				v_n_obs = MAX(v_n_obs, run); 
			}
			else 
            {         
				run = 0.e0;
            }
		}

        if ((int)v_n_obs <= 4)
        {
			nu[0]++;
        }
		else
        {      
			switch((int)v_n_obs) 
            {
			case 5:
				nu[1]++;
				break;
			case 6:
				nu[2]++;
				break;
			case 7:
				nu[3]++;
				break;
			case 8:
				nu[4]++;
				break;
			default:
				nu[5]++;
				break;
			}
        }
    }
    
    chi2 = 0.0;					
	sum = 0;
	for(i = 0; i < K+1; i++) 
    {
		chi2 += pow((double)nu[i] - (double)N*pi[i],2)/((double)N*pi[i]);
		sum += nu[i];
	}
	p_value = igamc(K/2.,chi2/2.);

    return p_value;
}

/***********************************************************
  Function: sub_searial_8len
  Description: TEST_FOR_THE_LONGEST_RUN_OF_ONES_IN_A_BLOCK??
                    块内最??游程检??算法子块长度
                    ??时的实现??算法 实现细节??
                    ??参照国标 GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @m   : 子序列测长度
             @buf : 测试样本数据的缓??
  Output: none
  Return: p_value: P-value??
  Others: none
************************************************************/
double sub_searial_8len(unsigned int n, unsigned char *buf)
{
	double run, v_n_obs, p_value, sum, chi2;
	int    N, i, j;
	unsigned int nu[4] = {0, 0, 0, 0};
	double pi[4] = {0.21484375, 0.3671875, 0.23046875, 0.1875};
	double k[4] = {1, 2, 3, 8};
	double K = 3;
	int M = 8;

    N = (int)floor(n/M);
    for(i = 0; i < N; i++)  
    {
        v_n_obs = 0.e0;
        run = 0.e0;
        for(j = i*M; j < (i+1)*M; j++) 
        {
            if ((int)buf[j] == 1) 
            {
                run++;
                v_n_obs = MAX(v_n_obs, run); 
            }
            else 
            {         
                run = 0.e0;
            }
        }
		switch((int)v_n_obs) 
        {
    		case 0:
    		case 1:
    			nu[0]++;
    			break;
    		case 2:
    			nu[1]++;
    			break;
    		case 3:
    			nu[2]++;
    			break;
    		default:
    			nu[3]++;
    			break;
		}
    }

    chi2 = 0.0;					
	sum = 0;
	for(i = 0; i < K+1; i++) 
    {
		chi2 += pow((double)nu[i] - (double)N*pi[i],2)/((double)N*pi[i]);
		sum += nu[i];
	}
	p_value = igamc(K/2.,chi2/2.);

    return p_value;
}

