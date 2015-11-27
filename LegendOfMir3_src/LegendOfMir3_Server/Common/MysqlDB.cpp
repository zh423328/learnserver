#include "MysqlDB.h"
//#define SET_NAMES_GBK
namespace db
{

MySqlDB::MySqlDB()
{
	m_pMySql = NULL;
	m_szServer = NULL;
	m_szDB = NULL;
	m_szUser = NULL;
	m_szPWD = NULL;
	m_pErrorDump = NULL;
	m_pCallBackHandle = NULL;

	m_nPort = 0;
}

MySqlDB::~MySqlDB()
{
	if(m_pMySql != NULL)
	{
		Close();
	}
}

bool MySqlDB::Connect(const char* szServer,const char* szDB,const char* szUser,const char* szPWD,unsigned int nPort)
{
	Close();
	m_pMySql = mysql_init(NULL);
	if(m_pMySql == NULL)
	{
		_ErrorDumpSimple("mysql_init() failed,insufficient memory");
		return false;
	}
	if(mysql_real_connect(m_pMySql,szServer,szUser,szPWD,szDB,nPort,NULL,0))
	{
		if(szServer != NULL)
			m_szServer = strdup(szServer);
		if(szDB != NULL)
			m_szDB = strdup(szDB);
		if(szUser != NULL)
			m_szUser = strdup(szUser);
		if(szPWD != NULL)
			m_szPWD = strdup(szPWD);
		m_nPort = nPort;

#ifdef SET_NAMES_GBK	
		mysql_query(m_pMySql, "set names 'gbk'");
#endif
		m_bConnected = true;
		return true;
	}
	_ErrorDump();
	return false;
}

bool IsThreadSafe()
{
	return mysql_thread_safe();
}

bool MySqlDB::_Reconnect()
{
	if(m_pMySql != NULL)
	{
		mysql_close(m_pMySql);
		m_pMySql = NULL;

		m_bConnected = false;
	}
	m_pMySql = mysql_init(NULL);
	if(m_pMySql == NULL)
	{
		_ErrorDumpSimple("mysql_init() failed,insufficient memory");
		return false;
	}
	if(mysql_real_connect(m_pMySql,m_szServer,m_szUser,m_szPWD,m_szDB,m_nPort,NULL,0))
	{
#ifdef SET_NAMES_GBK
		mysql_query(m_pMySql, "set names 'gbk'");
#endif
		m_bConnected = true;
		_ErrorDumpSimple("reconnect to MySQL successful.");
		return true;
	}
	_ErrorDump();
	_ErrorDumpSimple("reconnect to MySQL failed.");
	return false;
}


void MySqlDB::Close()
{
	if(m_pMySql != NULL)
	{
		mysql_close(m_pMySql);
		m_pMySql = NULL;
	}
	if(m_szServer != NULL)
	{
		free(m_szServer);
		m_szServer = NULL;
	}
	if(m_szDB != NULL)
	{
		free(m_szDB);
		m_szDB = NULL;
	}
	if(m_szUser != NULL)
	{
		free(m_szUser);
		m_szUser = NULL;
	}
	if(m_szPWD != NULL)
	{
		free(m_szPWD);
		m_szPWD = NULL;
	}
	m_nPort = 0;
	m_bConnected = false;
}

bool MySqlDB::Connected()
{
	if(m_pMySql == NULL) return false;
	if (mysql_ping(m_pMySql))
	{
		_ErrorDump();
		if(!_Reconnect())
		{
			return false;
		}
	}
	return true;
}

void MySqlDB::SetCallBackFunc(MYSQL_ERROR pFunc,void* pHandle)
{
	m_pErrorDump = pFunc;
	m_pCallBackHandle = pHandle;
}

void MySqlDB::_ErrorDump()
{
	if(m_pErrorDump == NULL) return;
	char szError[1024] = {0};
	sprintf(szError, "%d %s", mysql_errno(m_pMySql), mysql_error(m_pMySql));
	m_pErrorDump(szError,m_pCallBackHandle);
}

void MySqlDB::_ErrorDumpSimple(const char* szError,...)
{
	if(m_pErrorDump == NULL) return;
	char	szTemp[4096]={0};
	vsprintf(szTemp, szError, (char*)(&szError+1));
	m_pErrorDump(szTemp,m_pCallBackHandle);
}

bool MySqlDB::IsConnected( void )
{
	return m_bConnected;
}

bool MySqlDB::UseDB( const char * szDB )
{
	char szSql[1024] = {0};
	sprintf(szSql, "use %s", szDB);
	if(mysql_query(m_pMySql, szSql) != 0) return false;

	if(m_szDB) free(m_szDB);
	m_szDB = strdup(szDB);
	_ErrorDumpSimple("UseDB %s successful.", szDB);
	return true;
}

const char * MySqlDB::GetDBName( void )
{
	return m_szDB;
}


}//--db--