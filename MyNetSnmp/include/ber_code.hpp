/********************************************************************
功能:	整数类型编解码
作者:	田俊
时间:	2020-12-31
说明:	这里采用大端编码，参数为本机编码，整数的编码都是一样的，解码不同，长度单位为字节
*********************************************************************/
#ifndef __BER_CODE_HPP__
#define __BER_CODE_HPP__

#include "error_status.hpp"

/*长度域编码说明：
	长度域指明值域的长度,不定长,一般为一到三个字节。其格式可分为短格式（后面的值域没有超过127长）和长格式,如下所示
		短格式的表示方法：
		0（1bit）	长度（7bit）
	长格式的表示方法：
		1（1bit）	K（7bit）	K个八位组长度（K Byte）
	例:
	length=30=>1E（30没有超过127，长度域为0001 1110）
	length=169=>81 A9（169超过127，长度域为 1000 0001 1010 1001，169是后9位的值，前八位的第一个1表示这是长格式的表示方法，
		前八位的后七位表示后面有多少个字节表示针对的长度，这里，是000 0001，后面有一个字节表示真正的长度，1010 1001是169，后面的值有169个字节长）
	length=1500=>82 05 DC（1000 0010 0000 0101 1101 1100，先看第一个字节，表示长格式，后面有2个字节表示长度，这两个字节是0000 0101 1101 1100 表示1500）
*/

/*OID数字编码说明：
	标识域编码为0x06，长度域根据情况而定，值域的编码比较复杂，如下所示。
	1、首两个ID被合并为一个字节X * 40+Y。
		例如：1.3合并为1x40+3 = 43 = 0x2B
	2、后续的ID，如果在区间[1,127]内，直接编码表示，如果大于127，那么按照下面(3)所述方法编码
	3、如果ID大于127，那么使用多个字节来表示。
		a.这多个字节中除最后一个字节外，前面的字节最高位为1
		b.这多个字节的最后一个字节的最高位为0
		c.这里每个字节剩下的7个比特位用来表示实际的数值
		例如这里的201566这个数，用十六进制表示是0x03 13 5e
		那么用二进制表示是000 1100 010 0110 101 1110
		注意上面是以7个比特位为单位进行分划的，现在我们来填充最高位
		将前面的最高位填1，最后一个最高位填0即可得到
		1000 1100 1010 0110 0101 1110用十六进制表示为0x8c a6 5e
*/

/*IPADDRESS类型编码说明
	把IP分为4段,转为整型a1.a2.a3.a4
	int result=a4<<2^24+a3<<2^16+ a2<<a3^8 + a1
	再把result编码即可
*/

class CBerCode : public CErrorStatus
{
public:
	CBerCode();
	~CBerCode();

	//编码
	int asn_integer_code(unsigned char tag, unsigned int val_len, unsigned char* val_buf, unsigned char* tlv_buf);
	int asn_unsigned_code(unsigned char tag, unsigned int val_len, unsigned char* val_buf, unsigned char* tlv_buf);
	int asn_oid_code(unsigned char tag, unsigned int val_len, oid* val_buf, unsigned char* tlv_buf);
	int asn_oid_decode(unsigned char* buf, unsigned int len, oid* decode_oid); //与基本类型不同，解码也要判断错误
	int asn_ipaddress_code(unsigned int val_len, unsigned char* val_buf, unsigned char* tlv_buf);

	//解码
	static int asn_integer_decode(unsigned char* buf, unsigned int len);
	static unsigned int asn_unsigned_decode(unsigned char* buf, unsigned int len);
	static long long asn_integer64_decode(unsigned char* buf, unsigned int len);
	static float asn_float_decode(unsigned char* buf, unsigned int len);
	static double asn_double_decode(unsigned char* buf, unsigned int len);
	static unsigned long long asn_unsigned64_decode(unsigned char* buf, unsigned int len);
	static int asn_ipaddress_decode(unsigned char* buf, unsigned int len);
	static std::string asn_ipaddress_decode_string(unsigned char* buf, unsigned int len);

};

#endif
