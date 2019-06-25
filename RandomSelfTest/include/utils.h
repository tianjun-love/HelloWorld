#ifndef __H__UTILS__H__
#define __H__UTILS__H__

#include <stdint.h>
#include <string>

using namespace std;

typedef struct
{
	uint8_t u8ResCnt;
	double  dfValue1;
	double  dfValue2;
	
} RandomCnt;


#define ALPHA (0.01)

#define MAX(x,y) ((x) < (y) ? (y) : (x))
#define isZero(x) ((x) == 0.e0? 1 : 0)
#define MIN(x,y)  ((x) > (y) ? (y) : (x))

double igamc(double a, double x);
int frequency_within_block(unsigned int n, unsigned int m, unsigned char * buf, string &strResult, RandomCnt &stRandomCnt);
int frequency(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
double erf_ext(double x);
double erfc_ext(double x);
int poker(unsigned int n, unsigned int m, unsigned char *buf,string &strResult, RandomCnt &stRandomCnt);
int serial(unsigned int n, unsigned int m, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
int runs(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
int runs_distribution(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
int longest_run(unsigned int n, unsigned int m, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
int binary_derivative(unsigned int n, unsigned int k, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
int auto_correlation(unsigned int n, unsigned int d, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
int binary_matrix_rank(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
unsigned char** create_matrix(int M, int Q);
void def_matrix(int M, int Q, unsigned char **m, int k, unsigned char *buf);
int computeRank(int M, int Q, unsigned char** matrix);
int cumulative(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
int approximate_entropy(unsigned int n, unsigned int m, unsigned char *buf,string &strResult, RandomCnt &stRandomCnt);
int linear_complexity(unsigned int n, unsigned int m, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
int maurer_universal(unsigned int n, unsigned int sample_size, 
                       unsigned int sample_num, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);
int dft(unsigned int n, unsigned char *buf, string &strResult, RandomCnt &stRandomCnt);


#endif