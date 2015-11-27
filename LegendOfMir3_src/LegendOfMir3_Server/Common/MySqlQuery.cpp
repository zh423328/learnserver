#include "MysqlDB.h"

//------------------------------------------------------------------------------------------------------
//MySqlQuery

namespace db
{

MySqlQuery::MySqlQuery(MySqlDB* pDB)
{
	m_pRecords = NULL;
	m_pRow = NULL;
	m_pFields = NULL;
	m_pDB = pDB;
	m_szSQL = NULL;
	m_dwFieldCnt = 0;
	m_pFieldsLens = NULL;
}

MySqlQuery::~MySqlQuery()
{
	if(m_szSQL != NULL)
		free(m_szSQL);
	if(m_pRecords != NULL)
	{
		mysql_free_result(m_pRecords);
	}

}

bool MySqlQuery::ExecuteSQL(const char* szSQL,...)
{
	if(m_pDB == NULL)
		return false;
	if(m_szSQL != NULL)
		free(m_szSQL);

	char szCMD[4096] = {0};
	vsprintf(szCMD, szSQL, (char*)(&szSQL+1));
	m_szSQL = strdup(szCMD);

	if(mysql_real_query(m_pDB->m_pMySql, szCMD, strlen(szCMD)))
	{			
		m_pDB->_ErrorDumpSimple("%s", szCMD);
		unsigned int nError = mysql_errno(m_pDB->m_pMySql);
		if(nError == CR_SERVER_GONE_ERROR || nError == CR_SERVER_LOST)
		{
			if(m_pDB->Connected())
			{
				if(mysql_real_query(m_pDB->m_pMySql, szCMD, strlen(szCMD)))
				{
					m_pDB->_ErrorDump();
				}
				else
				{
					return true;
				}
			}
		}
		m_pDB->_ErrorDump();
		return false;
	}
	return true;
}

bool MySqlQuery::FetchRecords(bool blAll)
{
	if(m_pDB == NULL)
		return false;

	if(m_pRecords != NULL)
	{
		mysql_free_result(m_pRecords);
		m_pRecords = NULL;
	}
	if((m_dwFieldCnt = mysql_field_count(m_pDB->m_pMySql)) <= 0)
		return false;
	if(blAll)
		m_pRecords = mysql_store_result(m_pDB->m_pMySql);
	else
		m_pRecords = mysql_use_result(m_pDB->m_pMySql);

	if(m_pRecords == NULL)
	{
		m_pDB->_ErrorDump();
		return false;
	}
	m_pFields = mysql_fetch_fields(m_pRecords);	
	return true;
}

DWORD MySqlQuery::GetRecordsCnt()
{
	if(m_pDB == NULL || m_pRecords == NULL) return -1;
	return (DWORD)mysql_num_rows(m_pRecords);
}

DWORD MySqlQuery::GetFieldCnt()
{
	return m_dwFieldCnt;

}
bool MySqlQuery::FetchRow()
{
	if(m_pDB == NULL) return false;
	m_pRow = mysql_fetch_row(m_pRecords);
	if(m_pRow == NULL) 
	{
		unsigned int nError = mysql_errno(m_pDB->m_pMySql);
		if(nError == CR_SERVER_LOST)
		{
			m_pDB->_ErrorDumpSimple("%s",m_szSQL);
			m_pDB->_ErrorDump();
		}
		return false;
	}
	m_pFieldsLens = mysql_fetch_lengths(m_pRecords);
	return true;

}

int MySqlQuery::_GetFieldIndex(const char* szField)
{
	if(m_pDB == NULL || m_pRecords == NULL || m_pRow == NULL) return -1;
	int i = 0;
	for(i = 0; i < m_dwFieldCnt; i++)
	{
		if(!stricmp(szField,m_pFields[i].name))
			break;
	}

	if(i >= m_dwFieldCnt) 
		return -1;
	return i;
}

char* MySqlQuery::GetStr(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return "";
	if(m_pRow[i] == NULL) return "";
	return m_pRow[i];
}

BYTE* MySqlQuery::GetBLOB(const char* szField,int* pnLen)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return NULL;
	if(m_pRow[i] == NULL) return NULL;
	*pnLen = m_pFieldsLens[i];
	return (BYTE*)m_pRow[i];
}

WORD MySqlQuery::GetWORD(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return 0;
	if(m_pRow[i] == NULL) return 0;
	return (WORD)atoi(m_pRow[i]);
}

DWORD MySqlQuery::GetDWORD(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return 0;
	if(m_pRow[i] == NULL) return 0;
	return (DWORD)atoi(m_pRow[i]);
}

int MySqlQuery::GetInt(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return 0;
	if(m_pRow[i] == NULL) return 0;
	return atoi(m_pRow[i]);
}

double MySqlQuery::GetDouble(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return 0;
	if(m_pRow[i] == NULL) return 0;
	return atof(m_pRow[i]);
}

char* MySqlQuery::GetFieldName(int nIndex)
{
	if(nIndex < 0 || nIndex >= m_dwFieldCnt) return NULL;
	return m_pFields[nIndex].name;
}

char* MySqlQuery::GetFieldValue(int nIndex,int* pnLen /* = NULL */)
{
	if(nIndex < 0 || nIndex >= m_dwFieldCnt) return NULL;
	if(pnLen != NULL)
		*pnLen = m_pFieldsLens[nIndex];
	return m_pRow[nIndex];
}

bool MySqlQuery::IsBLOBField(int nIndex)
{
	if(nIndex < 0 || nIndex >= m_dwFieldCnt) return NULL;
	return (m_pFields[nIndex].flags & BLOB_FLAG);
}


}//--db--