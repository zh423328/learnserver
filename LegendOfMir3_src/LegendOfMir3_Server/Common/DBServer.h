#ifndef _DBSERVER_H_
#define _DBSERVER_H_
#include "MysqlDB.h"

namespace db
{

class DBServer;
class QueryRlt
{
	friend			DBServer;
	MySqlQuery*		m_pRlt;
	DBServer*		m_pDB;
	DWORD			m_dwUseTime;
	DWORD			m_dwRecordCnt;
	char*			m_szText;
public:
	QueryRlt();
	~QueryRlt();
	MySqlQuery*		GetRlt();
	DWORD			GetRecordCnt();
};

class DBServer 
{
	friend			QueryRlt;

	void			_ReleaseQuery(MySqlQuery* pQuery);
	char			m_szName[MAX_PATH];

private:
	bool			_ConnectDB(const char * ip, const char * userName, const char * password, const char * dBaseName, bool create);
	bool			_ExecuteSQL(const char * szSQL);
	bool			_SetBLOB(BYTE * pData, DWORD dwSize, const char * szCmd);

	bool			_CreateLogFile(void);
	void			_OutputLog(const char * szSQL);

public:
	MySqlDB*		m_pDB;
	int				m_nLastTag;
	DWORD			m_dwID;

	//CRITICAL_SECTION m_xLock;
	DBServer(const char * szName);
	virtual ~DBServer();
	bool			ConnectDB(const char * ip, const char * userName, const char *password, const char *dBaseName, bool create = TRUE);
	void			DisconnectDB();
    bool			ExecuteSQL(const char* szFormat,...);
    bool			Query(QueryRlt* pRlt,const char* szFormat,...);	
    bool			GetBLOB(BYTE * pRltData, int nBufferSize, const char * szCmd);
	bool			GetBLOB(BYTE * pRltData, int nBufferSize, int & nDataSize, const char * szCmd);
    bool			SetBLOB(BYTE * pData, DWORD dwSize, const char * szCmd);
	bool			CheckDBConnect(void);
	bool			IsConnected(void);
};

}
#endif
