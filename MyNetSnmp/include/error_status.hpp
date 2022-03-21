#ifndef __ERROR_STATUS_HPP__
#define __ERROR_STATUS_HPP__

#include "snmpx_types.hpp"

#define SNMPX_pingFailure             (-3)  //��Ӧ��ʱ��pingʧ��
#define SNMPX_timeout                 (-2)  //�Զ����ڲ����󣬳�ʱ
#define SNMPX_failure                 (-1)  //�Զ����ڲ�����ͨ��

#define SNMPX_noError                 (0x00)
#define SNMPX_tooBig                  (0x01)
#define SNMPX_noSuchName              (0x02)
#define SNMPX_badValue                (0x03)
#define SNMPX_readOnly                (0x04)
#define SNMPX_genError                (0x05)
#define SNMPX_noAccess                (0x06)  
#define SNMPX_wrongType               (0x07)
#define SNMPX_wrongLength             (0x08)
#define SNMPX_wrongEncoding           (0x09)
#define SNMPX_wrongValue              (0x0A)
#define SNMPX_noCreation              (0x0B)
#define SNMPX_inconsistentValue       (0x0C)
#define SNMPX_resourceUnavailable     (0x0D)
#define SNMPX_commitFailed            (0x0E)
#define SNMPX_undoFailed              (0x0F)
#define SNMPX_authorizationError      (0x10)
#define SNMPX_notWritable             (0x11)
#define SNMPX_inconsistentName        (0x12)

#define SNMPX_usmUnsupportedSecLevels (0x51)  //��֧�ֵİ�ȫ����
#define SNMPX_usmNotInTimeWindows     (0x52)  //boots , times ������
#define SNMPX_usmUnknownUserNames     (0x53)  //�û��������    
#define SNMPX_usmUnknownEngineIDs     (0x54)  //��������	     
#define SNMPX_usmWrongDigests         (0x55)  //md5����֤����	     
#define SNMPX_usmDecryptionErrors     (0x56)  //����ʧ��

class CErrorStatus
{
public:
	CErrorStatus();
	virtual ~CErrorStatus();

	//��ȡ������Ϣ����Ҫ����������
	virtual const std::string& GetErrorMsg() const;

	//��ȡ�����ŵ���ϸ��Ϣ
	static std::string get_err_msg(int errstat, bool is_errno, bool chinese_desc = false);

	//��鲢����snmpv3��usm����״̬
	static void set_usm_error_status(oid* oid_buf, unsigned int oid_buf_len, snmpx_t *s);

	//��ʽ��������Ϣ
	static std::string format_err_msg(const char *format, ...);

private:
	static const char *m_error_string[2][19];    //snmp����˵��
	static const char *m_usm_error_string[2][6]; //usm����

protected:
	std::string m_szErrorMsg; //������Ϣ����Ҫ������ʹ��

};

#endif
