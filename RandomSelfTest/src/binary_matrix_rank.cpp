#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <string>
using namespace std;

#include "../include/utils.h"


/***********************************************************
  Function: binary_matrix_rank
  Description: BINARY_MATRIX_RANK_TEST??矩阵秩检 ??
                    算法的实??算法 实现细节说明
                    参照国标GM/T 0005-2012 
  Input:  @n    : 测试样本长度
             @buf : 测试样本数据的缓??
  Output: none
  Return:  参看枚举RSLT_CODE
  Others: none
************************************************************/
int binary_matrix_rank(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt)
{
    int N = (int)floor(n/(32*32));  /* NUMBER OF MATRICES     */
    int r;
    double p_value, product;
    int i, k;
    double chi_squared, arg1;
    double p_32, p_31, p_30;         /* PROBABILITIES */
    double R;                        /* RANK          */
    double F_32, F_31, F_30;         /* FREQUENCIES   */
    unsigned char** matrix = NULL;

    matrix = create_matrix(32, 32);
    if (matrix == NULL)
    {
		strResult = "进行矩阵秩检测时,创建matrix失败";
        return -1;
    }
        
    if (isZero(N)) 
    {
        p_value = 0.00;
    }
    else 
    {
        r = 32;                 /* COMPUTE PROBABILITIES */
        product = 1;
        for(i=0; i<=r-1; i++)
        {
            product *= ((1.e0-pow(2,i-32))*(1.e0-pow(2,i-32)))/(1.e0-pow(2,i-r));
        }
        p_32 = pow(2,r*(32+32-r)-32*32) * product;

        r = 31;
        product = 1;
        for(i=0; i<=r-1; i++) 
        {
            product *= ((1.e0-pow(2,i-32))*(1.e0-pow(2,i-32)))/(1.e0-pow(2,i-r));
        }
        p_31 = pow(2,r*(32+32-r)-32*32) * product;

        p_30 = 1 - (p_32+p_31);

        F_32 = 0;
        F_31 = 0;
        for(k=0; k<N; k++) 
        {         
            def_matrix(32, 32, matrix, k, buf);

            R = computeRank(32, 32, matrix);
			
            if (R == 32)
            {
                F_32++;  
            }
            if (R == 31)
            {
                F_31++;
            }
        }
        F_30 = (double)N - (F_32+F_31);

        chi_squared =(pow(F_32 - N*p_32,2)/(double)(N*p_32)  
                    + pow(F_31 - N*p_31,2)/(double)(N*p_31) 
                    + pow(F_30 - N*p_30,2)/(double)(N*p_30));

        arg1 = -chi_squared/2.e0;
        
        p_value = exp(arg1);
    }

    for(i=0; i<32; i++)  
    {
        free(matrix[i]);
    }
    free(matrix);

	
	stRandomCnt.u8ResCnt = 1;
	stRandomCnt.dfValue1 = p_value;
	
    return 0;
}


