/************************************************************************/
/* ’À∫≈π‹¿Ì                                                               */
/************************************************************************/
#ifndef _ACCOUNTMANAGER_H_
#define _ACCOUNTMANAGER_H_

#include "../Common/Common.h"
#include "../Common/singleton.h"

class AccountUser
{
public:
	AccountUser();
	~AccountUser();

	uint32 m_uin;
	char   m_id[50];
	char   m_pwd[50];

	bool   m_bOnLine;
};

typedef  std::map<std::string,AccountUser*> AccountList;

class AccoutManager:public CSingleton<AccoutManager>
{
public:
	AccoutManager();
	~AccoutManager();

	AccountUser * GetAccount(char * szUser);
	bool InsertAccountUser(AccountUser*pUser);
private:
	AccountList m_AccountMap;
};
#endif