/***********************************************************************
* FUNCTION:     数据库公共定义
* AUTHOR:       田俊
* DATE：        2015-04-13
* NOTES:        
* MODIFICATION:
**********************************************************************/
#ifndef __DB_PUBLIC_HPP__
#define __DB_PUBLIC_HPP__

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <mutex>
#include <list>
#include <vector>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cmath>

//数据类型
enum EDBDataType
{
	DB_DATA_TYPE_NONE      = 0,   //未知类型
	DB_DATA_TYPE_CHAR      = 1,   //char类型
	DB_DATA_TYPE_VCHAR     = 2,   //varchar类型
	DB_DATA_TYPE_TINYINT   = 3,   //1个字节的int
	DB_DATA_TYPE_SMALLINT  = 4,   //2个字节的int
	DB_DATA_TYPE_MEDIUMINT = 5,   //3个字节的int
	DB_DATA_TYPE_INT       = 6,   //4个字节的int
	DB_DATA_TYPE_BIGINT    = 7,   //8个字节的int
	DB_DATA_TYPE_FLOAT     = 8,   //4个字节的浮点型，oracle是binary_float
	DB_DATA_TYPE_DOUBLE    = 9,   //8个字节的浮点型，oracle是binary_double
	DB_DATA_TYPE_DECIMAL   = 10,  //number或decimal类型
	DB_DATA_TYPE_YEAR      = 11,  //year类型，如：2009
	DB_DATA_TYPE_TIME      = 12,  //time类型
	DB_DATA_TYPE_DATE      = 13,  //date类型
	DB_DATA_TYPE_DATETIME  = 14,  //datetime类型
	DB_DATA_TYPE_TIMESTAMP = 15,  //timestamp类型，mysql使用时：字段类型为timestamp(n)，n表示毫秒的位数
	DB_DATA_TYPE_CLOB      = 16,  //文本类型
	DB_DATA_TYPE_BLOB      = 17,  //二进制类型
	DB_DATA_TYPE_FLOAT2    = 18,  //8个字节的浮点型，oracle的float使用
	DB_DATA_TYPE_RAW       = 19,  //二进制类型，oracle使用
	DB_DATA_TYPE_BFILE     = 20,  //二进制文件，oracle使用
	DB_DATA_TYPE_RESULT    = 21   //结果类型，oracle使用
};

#define D_DB_NULL_INTGER_VALUE  (-9999)            //数据库整型null值标识
#define D_DB_NULL_DECIMAL_VALUE (-9999.0)          //数据库浮点型null值标识
#define D_DB_NULL_CHAR_VALUE    (-128)             //数据库char型null值标识
#define D_DB_NULL_UCHAR_VALUE   (255)              //数据库无符号char型null值标识
#define D_DB_NULL_USHORT_VALUE  (65535)            //数据库无符号short型null值标识
#define D_DB_NULL_UINT_VALUE    (4294967295)       //数据库无符号int型null值标识
#define D_DB_NULL_ULONG_VALUE   (4294967295)       //数据库无符号long型null值标识
#define D_DB_NULL_ULLONG_VALUE  (1000000000000000) //数据库无符号long long型null值标识

#define	D_DB_COLNUM_NAME_LENGTH (64)               //字段名最大长度
#define D_DB_NO_DATA            (100)              //fetch完成，没有剩余数据了

#define D_DB_LOB_RETURN_MAX_LEN (33554432)         //支持的返回LOB最大长度32M，mysql使用
#define D_DB_STRING_MAX_LEN     (4000)             //支持的字符串最大长度
#define D_DB_DATETIME_MAX_LEN   (32)               //支持的绑定日期时间字符串最大长度
#define D_DB_NUMBER_MAX_LEN     (38)               //支持的绑定decimal/number串最大长度

#endif // __DB_PUBLIC_HPP__