#include <stdio.h>
#include <stdlib.h>

#include "../include/utils.h"


int determine_rank(int m, int M, int Q, unsigned char **A);
void perform_elementary_row_operations(int flag, int i, int M, int Q, unsigned char** A);
int find_unit_element_and_swap(int flag, int i, int M, int Q, unsigned char** A);
int swap_rows(int i, int index, int Q, unsigned char **A);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   matrix function prototype definition
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int computeRank(int M, int Q, unsigned char** matrix)
{
    int i;
    int rank;
    int m = MIN(M,Q);
       
    for(i = 0; i < m-1; i++) 
    {
        if (matrix[i][i] == 1) 
        {
           perform_elementary_row_operations(0, i, M, Q, matrix);
        }
        else 
        {   
            if (find_unit_element_and_swap(0, i, M, Q, matrix) == 1) 
            {
                perform_elementary_row_operations(0, i, M, Q, matrix);
            }
        }
    }
 
    for(i = m-1; i > 0; i--) 
    {
        if (matrix[i][i] == 1)
        {
            perform_elementary_row_operations(1, i, M, Q, matrix);
        }
        else 
        {   /* matrix[i][i] = 0 */
            if (find_unit_element_and_swap(1, i, M, Q, matrix) == 1) 
            {
                perform_elementary_row_operations(1, i, M, Q, matrix);
            }
        }
    } 
    
    rank = determine_rank(m, M, Q, matrix);
    return rank;
}

void perform_elementary_row_operations(int flag, int i, int M, int Q, unsigned char** A)
{
    int j,k;
  
    switch(flag)
    { 
        case 0: 
        {
            for(j = i+1; j < M;  j++)
            {
                if (A[j][i] == 1) 
                {
                    for(k = i; k < Q; k++) 
                    {
                        A[j][k] = (A[j][k] + A[i][k]) % 2;
                    }
                }
            }
            break;
        }
        case 1: 
        {
            for(j = i-1; j >= 0;  j--)
            {
                if (A[j][i] == 1)
                {
                    for(k = 0; k < Q; k++)
                    {
                        A[j][k] = (A[j][k] + A[i][k]) % 2;
                    }
                }
            }
            break;
        }
        default:  
            break;
    }
    return;
}

int find_unit_element_and_swap(int flag, int i, int M, int Q, unsigned char** A)
{ 
    int index;
    int row_op = 0;
  
    switch(flag) 
    {
        case 0: 
        {
            index = i+1;
             while ((index < M) && (A[index][i] == 0)) 
                index++;
             if (index < M) 
                row_op = swap_rows(i, index, Q, A);
             break;
        }
        case 1:
        {
             index = i-1;
    	     while ((index >= 0) && (A[index][i] == 0)) 
    	       index--;
    	     if (index >= 0) 
                    row_op = swap_rows(i, index, Q, A);
             break;
        }
        default: 
            break;
    }
    return row_op;
}

int swap_rows(int i, int index, int Q, unsigned char **A)
{
  int p;
  unsigned char temp;

  for(p = 0; p < Q; p++) {
     temp = A[i][p];
     A[i][p] = A[index][p];
     A[index][p] = temp;
  }
  return 1;
}

int determine_rank(int m, int M, int Q, unsigned char **A)
{
    int i, j, rank, allZeroes;
  
    rank = m;
    for(i = 0; i < M; i++) 
    {
        allZeroes = 1; 
        for(j=0; j < Q; j++) 
        {
            if (A[i][j] == 1) 
            {
                allZeroes = 0;
                break;
            }
        }
        if (allZeroes == 1) 
        {
            rank--;
        }
    } 
    return rank;
}

unsigned char** create_matrix(int M, int Q)
{
    int i;
    unsigned char **matrix;
  
    matrix = (unsigned char**) calloc(M, sizeof(unsigned char *));
    if (matrix == NULL) 
    {
		return NULL;
    }
    else 
    {
        for (i = 0; i < M; i++) 
        {
            matrix[i] = (unsigned char *)calloc(Q, sizeof(unsigned char));
            if (matrix[i] == NULL) 
            {

                return NULL;
            }
        }
    }
    return matrix;
}

void display_matrix(int M, int Q, unsigned char** m)
{
  return;
}

void def_matrix(int M, int Q, unsigned char **m, int k, unsigned char *buf)
{
  int   i,j;
  for (i = 0; i < M; i++) 
     for (j = 0; j < Q; j++) { 
         m[i][j] = buf[k*(M*Q)+j+i*M];
     }
  return;
}

void delete_matrix(int M, unsigned char** matrix)
{
  int i;
  for (i = 0; i < M; i++)
    free(matrix[i]);
  free(matrix);
}

