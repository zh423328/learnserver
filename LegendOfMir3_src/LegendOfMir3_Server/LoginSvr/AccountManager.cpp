#include "AccountManager.h"

CreateSingleton(AccoutManager);

AccountUser::AccountUser()
{
	m_bOnLine = false;
	memset(m_id,0,50);
	memset(m_pwd,0,50);
}

AccountUser::~AccountUser()
{

}

AccoutManager::AccoutManager()
{

}

AccoutManager::~AccoutManager()
{
	for (AccountList::iterator iter = m_AccountMap.begin();iter != m_AccountMap.end();++iter)
	{
		SAFE_DELETE(iter->second);
	}
	m_AccountMap.clear();
}

AccountUser * AccoutManager::GetAccount( char * szUser )
{
	if(szUser == NULL)
		return NULL;
	else
	{
		AccountUser*pUser = m_AccountMap[szUser];
		
		return pUser;
	}
}

bool AccoutManager::InsertAccountUser( AccountUser*pUser )
{
	if (pUser == NULL)
		return false;
	else
	{
		 std::pair<AccountList::iterator,bool > ret = m_AccountMap.insert(std::make_pair(pUser->m_id,pUser));
		 
		 return ret->second;
	}
}
