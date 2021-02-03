/********************************************************************
����:	�������ͱ����
����:	�￡
ʱ��:	2020-12-31
˵��:	������ô�˱��룬����Ϊ�������룬�����ı��붼��һ���ģ����벻ͬ�����ȵ�λΪ�ֽ�
*********************************************************************/
#ifndef __BER_CODE_HPP__
#define __BER_CODE_HPP__

#include "error_status.hpp"

/*���������˵����
	������ָ��ֵ��ĳ���,������,һ��Ϊһ�������ֽڡ����ʽ�ɷ�Ϊ�̸�ʽ�������ֵ��û�г���127�����ͳ���ʽ,������ʾ
		�̸�ʽ�ı�ʾ������
		0��1bit��	���ȣ�7bit��
	����ʽ�ı�ʾ������
		1��1bit��	K��7bit��	K����λ�鳤�ȣ�K Byte��
	��:
	length=30=>1E��30û�г���127��������Ϊ0001 1110��
	length=169=>81 A9��169����127��������Ϊ 1000 0001 1010 1001��169�Ǻ�9λ��ֵ��ǰ��λ�ĵ�һ��1��ʾ���ǳ���ʽ�ı�ʾ������
		ǰ��λ�ĺ���λ��ʾ�����ж��ٸ��ֽڱ�ʾ��Եĳ��ȣ������000 0001��������һ���ֽڱ�ʾ�����ĳ��ȣ�1010 1001��169�������ֵ��169���ֽڳ���
	length=1500=>82 05 DC��1000 0010 0000 0101 1101 1100���ȿ���һ���ֽڣ���ʾ����ʽ��������2���ֽڱ�ʾ���ȣ��������ֽ���0000 0101 1101 1100 ��ʾ1500��
*/

/*OID���ֱ���˵����
	��ʶ�����Ϊ0x06��������������������ֵ��ı���Ƚϸ��ӣ�������ʾ��
	1��������ID���ϲ�Ϊһ���ֽ�X * 40+Y��
		���磺1.3�ϲ�Ϊ1x40+3 = 43 = 0x2B
	2��������ID�����������[1,127]�ڣ�ֱ�ӱ����ʾ���������127����ô��������(3)������������
	3�����ID����127����ôʹ�ö���ֽ�����ʾ��
		a.�����ֽ��г����һ���ֽ��⣬ǰ����ֽ����λΪ1
		b.�����ֽڵ����һ���ֽڵ����λΪ0
		c.����ÿ���ֽ�ʣ�µ�7������λ������ʾʵ�ʵ���ֵ
		���������201566���������ʮ�����Ʊ�ʾ��0x03 13 5e
		��ô�ö����Ʊ�ʾ��000 1100 010 0110 101 1110
		ע����������7������λΪ��λ���зֻ��ģ�����������������λ
		��ǰ������λ��1�����һ�����λ��0���ɵõ�
		1000 1100 1010 0110 0101 1110��ʮ�����Ʊ�ʾΪ0x8c a6 5e
*/

/*IPADDRESS���ͱ���˵��
	��IP��Ϊ4��,תΪ����a1.a2.a3.a4
	int result=a4<<2^24+a3<<2^16+ a2<<a3^8 + a1
	�ٰ�result���뼴��
*/

class CBerCode : public CErrorStatus
{
public:
	CBerCode();
	~CBerCode();

	//����
	int asn_integer_code(unsigned char tag, unsigned int val_len, unsigned char* val_buf, unsigned char* tlv_buf);
	int asn_unsigned_code(unsigned char tag, unsigned int val_len, unsigned char* val_buf, unsigned char* tlv_buf);
	int asn_oid_code(unsigned char tag, unsigned int val_len, oid* val_buf, unsigned char* tlv_buf);
	int asn_oid_decode(unsigned char* buf, unsigned int len, oid* decode_oid); //��������Ͳ�ͬ������ҲҪ�жϴ���
	int asn_ipaddress_code(unsigned int val_len, unsigned char* val_buf, unsigned char* tlv_buf);

	//����
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
