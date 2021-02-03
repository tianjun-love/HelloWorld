#ifndef __SNMPX_USER_PROCCESS_HPP__
#define __SNMPX_USER_PROCCESS_HPP__

#include "error_status.hpp"

class CUserProccess : public CErrorStatus
{
public:
	CUserProccess();
	~CUserProccess();

	int snmpx_user_init(struct userinfo_t *user_info);

	static void snmpx_user_free(struct userinfo_t *user_info);
	static void snmpx_user_map_free(std::map<std::string, userinfo_t*> &user_info_map);

private:
	int calc_hash_user_auth(struct userinfo_t *user_info);
	int calc_hash_user_auth_priv(struct userinfo_t *user_info);

};

#endif
