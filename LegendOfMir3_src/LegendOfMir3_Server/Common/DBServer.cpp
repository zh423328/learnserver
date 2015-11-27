#include "DBServer.h"

namespace db
{
//
//--------------------------------------------------------------------------------------------------------------
QueryRlt::QueryRlt()
{
	m_pRlt			= NULL;
	m_pDB			= NULL;
	m_dwUseTime		= GetTickCount();
	m_szText		= NULL;
	m_dwRecordCnt	= 0;
}

QueryRlt::~QueryRlt()
{
	if(m_pRlt != NULL && m_pDB != NULL)
	{
		m_pDB->_ReleaseQuery(m_pRlt);
		m_dwUseTime = GetTickCount() - m_dwUseTime;
		if(m_dwUseTime > 1000)
		{
			printf("UseTime<%d>: %s\n", m_dwUseTime, m_szText);
		}
	}
	if(m_szText != NULL)
		free(m_szText);
}

MySqlQuery* QueryRlt::GetRlt()
{
	return m_pRlt;
}

DWORD QueryRlt::GetRecordCnt()
{
	return m_dwRecordCnt;
}
//--------------------------------------------------------------------------------------------------------------

void DumpErrorFunc(const char* szError,void* pVoid)
{
	DBServer*	pDBServer = (DBServer*)pVoid;
	printf("%s\n",(char*)szError);
}


DBServer::DBServer(const char * szName)
{
	m_pDB		= new MySqlDB;
	m_dwID		= 0;
	//InitializeCriticalSection(&m_xLock);
	if(m_pDB != NULL) m_pDB->SetCallBackFunc(DumpErrorFunc, this);
	m_nLastTag	= -1;
	strncpy(m_szName, szName, MAX_PATH);
}

DBServer::~DBServer()
{
	//DeleteCriticalSection(&m_xLock);
	delete m_pDB;
}

bool DBServer::IsConnected()
{
	return m_pDB->IsConnected();
}


bool DBServer::_ConnectDB( const char * ip, const char * userName, const char * password, const char * dBaseName, bool create )
{
	if(m_pDB == NULL) return false;
	if (!create) return m_pDB->Connect(ip,dBaseName,userName,password);
	if (!m_pDB->Connect(ip,dBaseName,userName,password))
	{
		if (!m_pDB->Connect(ip,"test",userName,password))
			return false;
		MySqlQuery query(m_pDB);
		if (!query.ExecuteSQL("CREATE DATABASE %s", dBaseName))
			return false;
		m_pDB->Close();
		return m_pDB->Connect(ip,dBaseName,userName,password);
	}
	return true;
}

bool DBServer::ConnectDB(const char * ip, const char * userName, const char * password, const char * dBaseName, bool create /* = TRUE */)
{
	bool bRlt = false;

	if(bRlt = _ConnectDB(ip, userName, password, dBaseName, create))
	{
		//	成功连接后,自动进行数据恢复功能
		return true;
	}
	return bRlt;
}

void DBServer::DisconnectDB()
{
	if(m_pDB != NULL)
		m_pDB->Close();
}

bool DBServer::_ExecuteSQL(const char * szSQL)
{
	bool rlt = false;

	MySqlQuery query(m_pDB);
	DWORD dwTick = GetTickCount();
	//EnterCriticalSection(&m_xLock);
	rlt = query.ExecuteSQL(szSQL);
	//LeaveCriticalSection(&m_xLock);
	dwTick = GetTickCount() - dwTick;
	if (dwTick >1500)
	{
		printf("\"%s\" - Update time = %d\n", szSQL, dwTick);
	}

	_OutputLog(szSQL);
	return rlt;
}

bool DBServer::ExecuteSQL(const char* szFormat, ...)
{
	bool rlt = false;

	char szSQL[4096] = {0};
	vsprintf(szSQL, szFormat, (char*)(&szFormat+1));

	if(m_pDB->IsConnected())
	{
		rlt = _ExecuteSQL(szSQL);
		if(rlt || (!rlt && m_pDB->IsConnected())) return rlt;
	}

	return rlt;	
}

bool DBServer::Query(QueryRlt* pRlt,const char* szFormat,...)
{
	char szSQL[4096]={0};
	vsprintf(szSQL, szFormat, (char*)(&szFormat+1));
	_OutputLog(szSQL);

	if(m_pDB == NULL) return NULL;

	int nCnt = 0,nBufferCnt = 0,nDataLen =0,i = 0;
	char* szBuffer = NULL;
	DWORD deltaTime = 0;

	//EnterCriticalSection(&m_xLock);
	MySqlQuery* pQuery = new MySqlQuery(m_pDB);
	if(pRlt->m_szText)
		free(pRlt->m_szText);
	pRlt->m_szText = strdup(szSQL);
	if(!pQuery->ExecuteSQL(szSQL))
	{
		delete pQuery;
		//LeaveCriticalSection(&m_xLock);
		return false;
	}
	if(!pQuery->FetchRecords(true))
	{
		delete pQuery;
		//LeaveCriticalSection(&m_xLock);
		return false;
	}
	pRlt->m_dwRecordCnt = pQuery->GetRecordsCnt();
	pRlt->m_pDB	= this;
	if(pRlt->m_pRlt)
		delete pRlt->m_pRlt;
	pRlt->m_pRlt = pQuery;
	return true;
}

void DBServer::_ReleaseQuery(MySqlQuery* pQuery)
{
	if(pQuery != NULL)
	{
		delete pQuery;
		pQuery = NULL;
		//LeaveCriticalSection(&m_xLock);
	}
}

bool DBServer::GetBLOB(BYTE* pRltData, int nBufferSize,const char* szCmd)
{
    const char * sqlCmd = szCmd;
	bool rlt = false;
	DWORD dwTick = GetTickCount();

	_OutputLog(szCmd);

	//EnterCriticalSection(&m_xLock);
	if(m_pDB == NULL) goto ExitGetBLOB;
	{
		MySqlQuery query(m_pDB);
		if(!query.ExecuteSQL(sqlCmd)) goto ExitGetBLOB;
		if(!query.FetchRecords(true))
			goto ExitGetBLOB;
		if(!query.FetchRow())
			goto ExitGetBLOB;

			int nCnt = query.GetRecordsCnt();
			for (int i=0; i< nCnt; i++)
			{
				int nFieldCnt = query.GetFieldCnt();
				for (int j=0; j< nFieldCnt; j++)
				{
					if (query.IsBLOBField(j))
					{
						char* szName = query.GetFieldName(j);
						int fieldNamelen = strlen(szName)+1;
						DWORD	datasize = 0;
						BYTE*	pData = (BYTE*)query.GetFieldValue(j,(int*)&datasize);
						DWORD dwSize = min(datasize, nBufferSize);
						memcpy(pRltData,pData, dwSize);
					}
				}
				query.FetchRow();
			}
			dwTick = GetTickCount() - dwTick;
			if (dwTick > 500)
			{
				printf("%s - GetBLOB time = %d", sqlCmd, dwTick);
			}
			rlt = true;
	}

ExitGetBLOB:
	//LeaveCriticalSection(&m_xLock);
	if(!rlt)
		printf("GetBLOB failed:%s\n",sqlCmd);
    return rlt;
}

bool DBServer::GetBLOB(BYTE* pRltData, int nBufferSize, int& nDataSize, const char* szCmd)
{
	const char * sqlCmd = szCmd;
	bool rlt = false;
	DWORD dwTick = GetTickCount();

	_OutputLog(szCmd);

	//EnterCriticalSection(&m_xLock);
	if(m_pDB == NULL) goto ExitGetBLOB;
	{
		MySqlQuery query(m_pDB);
		if(!query.ExecuteSQL(sqlCmd)) goto ExitGetBLOB;
		if(!query.FetchRecords(true))
			goto ExitGetBLOB;
		if(!query.FetchRow())
			goto ExitGetBLOB;

		int nCnt = query.GetRecordsCnt();
		for (int i=0; i< nCnt; i++)
		{
			int nFieldCnt = query.GetFieldCnt();
			for (int j=0; j< nFieldCnt; j++)
			{
				if (query.IsBLOBField(j))
				{
					char* szName = query.GetFieldName(j);
					int fieldNamelen = strlen(szName)+1;
					DWORD	datasize = 0;
					BYTE*	pData = (BYTE*)query.GetFieldValue(j,(int*)&datasize);
					nDataSize = min(datasize, nBufferSize);
					memcpy(pRltData,pData, nDataSize);
				}
			}
			query.FetchRow();
		}
		dwTick = GetTickCount() - dwTick;
		if (dwTick > 500)
		{
			printf("%s - GetBLOB time = %d", sqlCmd, dwTick);
		}
		rlt = true;
	}

ExitGetBLOB:
	//LeaveCriticalSection(&m_xLock);
	if(!rlt)
		printf("GetBLOB failed:%s\n",sqlCmd);
	return rlt;
}

bool DBServer::_SetBLOB(BYTE * pData, DWORD dwSize, const char * szCmd)
{
	MySqlCmd sqlcmd(m_pDB);
	sqlcmd.SetCmd(szCmd);
	sqlcmd.SetParam(0, MySqlCmd::DATA_TYPE_BLOB, pData, dwSize, &dwSize);
	sqlcmd.BindParams();

	_OutputLog(szCmd);
	return sqlcmd.Execut();
}

bool DBServer::SetBLOB(BYTE * pData, DWORD dwSize, const char * szCmd)
{
	bool rlt = false;

	if(m_pDB->IsConnected())
	{
		rlt = _SetBLOB(pData, dwSize, szCmd);
		if(!rlt) printf("SetBloB failed:%s\n",szCmd);

		if(rlt || (!rlt && m_pDB->IsConnected())) return rlt;
	}
	return rlt;
}

bool DBServer::CheckDBConnect()
{
	if(m_pDB == NULL || !m_pDB->Connected()) return false;
	return true;
}


bool DBServer::_CreateLogFile( void )
{
	char szLogFileName[64] = {0};
	sprintf(szLogFileName, "DX_DBLOG_%s", m_szName);

	return true;
}


void DBServer::_OutputLog( const char * szSQL )
{
}

} // --db--