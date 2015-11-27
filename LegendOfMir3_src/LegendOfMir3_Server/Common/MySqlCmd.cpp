#include "MysqlDB.h"

//----------------------------------------------------------------------------------------------------
//MySqlCmd
namespace db
{

MySqlCmd::MySqlCmd(MySqlDB* pDB)
{
	m_pDB = pDB;
	m_pStmt = NULL;
	m_pParams = NULL;
	m_pMetaRlt = NULL;
	m_pFields = NULL;
	m_pRow = NULL;
	m_pblIsNull = NULL;
	m_pblError = NULL;
	m_pValues = NULL;
	m_dwParamCnt = 0;
}

MySqlCmd::~MySqlCmd()
{
	Reset();
}

bool MySqlCmd::Reset()
{
	if(m_pStmt != NULL)
	{
		//printf("enter mysql_stmt_close\n");
		if(mysql_stmt_close(m_pStmt))
		{
			m_pDB->_ErrorDump();
			//printf("exit mysql_stmt_close\n");
			return false;
		}
		//printf("leave mysql_stmt_close\n");
		m_pStmt = NULL;
	}
	if(m_pParams != NULL)
	{
		delete[] m_pParams;
		m_pParams = NULL;
	}
	if(m_pRow != NULL)
	{
		delete[] m_pRow;
		m_pRow = NULL;
	}
	if(m_pValues != NULL)
	{
		delete[] m_pValues;
		m_pValues = NULL;
	}
	if(m_pblIsNull != NULL)
	{
		delete[] m_pblIsNull;
		m_pblIsNull = NULL;
	}
	if(m_pblError != NULL)
	{
		delete[] m_pblError;
		m_pblError = NULL;
	}
	if(m_pMetaRlt != NULL)
	{
		//printf("enter mysql_free_result\n");
		mysql_free_result(m_pMetaRlt);
		//printf("leave mysql_free_result\n");
		m_pMetaRlt = NULL;
	}
	m_pFields = NULL;
	m_dwParamCnt = 0;
	return true;
}

void MySqlCmd::SetParam(int index,BYTE bType,void* pValue,DWORD dwBufferLen,DWORD* pDataLen)
{
	if(m_pDB == NULL || m_pDB->m_pMySql == NULL || m_pStmt == NULL)
	{
		return;
	}
	if(index >= m_dwParamCnt || m_pParams == NULL)
	{
		m_pDB->_ErrorDumpSimple("Error: invalid param index.");
		return;
	}
	bool blsigned = false;
	enum_field_types bSQLType;
	switch(bType)
	{
	case DATA_TYPE_BYTE:
		bSQLType = MYSQL_TYPE_TINY;
		break;
	case DATA_TYPE_SHORT:
		bSQLType = MYSQL_TYPE_SHORT;
		break;
	case DATA_TYPE_WORD:
		bSQLType = MYSQL_TYPE_SHORT;
		blsigned = true;
		break;
	case DATA_TYPE_INT:
		bSQLType = MYSQL_TYPE_LONG;
		break;
	case DATA_TYPE_DWORD:
		bSQLType = MYSQL_TYPE_LONG;
		blsigned = true;
		break;
	case DATA_TYPE_FLOAT:
		bSQLType = MYSQL_TYPE_FLOAT;
		break;
	case DATA_TYPE_DOUBLE:
		bSQLType = MYSQL_TYPE_DOUBLE;
		break;
	case DATA_TYPE_STRING:
		bSQLType = MYSQL_TYPE_VAR_STRING;
		break;
	case DATA_TYPE_BLOB:
		bSQLType = MYSQL_TYPE_BLOB;
		break;
	case DATA_TYPE_DATETIME:
		bSQLType = MYSQL_TYPE_DATETIME;
		break;
	}
	m_pParams[index].buffer_type = bSQLType;
	m_pParams[index].buffer = pValue;
	m_pParams[index].buffer_length  = dwBufferLen;
	m_pParams[index].length = pDataLen;
	if(pValue != NULL)
		m_pParams[index].is_null = (my_bool*)0;
	else
		m_pParams[index].is_null = (my_bool*)1;
	m_pParams[index].is_unsigned = blsigned;	
}

void MySqlCmd::SetRltField(int index,BYTE bType,DWORD dwBufferLen)
{
	if(m_pDB == NULL || m_pDB->m_pMySql == NULL || m_pStmt == NULL)
	{
		return;
	}
	if(index >= m_dwFieldCnt|| m_pRow == NULL)
	{
		m_pDB->_ErrorDumpSimple("Error: invalid param index.");
		return;
	}
	bool blsigned = false;
	enum_field_types bSQLType;
	switch(bType) 
	{
	case DATA_TYPE_BYTE:
		bSQLType = MYSQL_TYPE_TINY;
		dwBufferLen = sizeof(BYTE);
		break;
	case DATA_TYPE_SHORT:
		bSQLType = MYSQL_TYPE_SHORT;
		dwBufferLen = sizeof(short);
		break;
	case DATA_TYPE_WORD:
		bSQLType = MYSQL_TYPE_SHORT;
		dwBufferLen = sizeof(WORD);
		blsigned = true;
		break;
	case DATA_TYPE_INT:
		bSQLType = MYSQL_TYPE_LONG;
		dwBufferLen = sizeof(INT);
		break;
	case DATA_TYPE_DWORD:
		bSQLType = MYSQL_TYPE_LONG;
		dwBufferLen = sizeof(DWORD);
		blsigned = true;
		break;
	case DATA_TYPE_FLOAT:
		bSQLType = MYSQL_TYPE_FLOAT;
		dwBufferLen = sizeof(float);
		break;
	case DATA_TYPE_DOUBLE:
		bSQLType = MYSQL_TYPE_DOUBLE;
		dwBufferLen = sizeof(double);
		break;
	case DATA_TYPE_STRING:
		bSQLType = MYSQL_TYPE_VAR_STRING;
		dwBufferLen ++;
		break;
	case DATA_TYPE_BLOB:
		bSQLType = MYSQL_TYPE_BLOB;
		dwBufferLen ++;
		break;
	case DATA_TYPE_DATETIME:
		bSQLType = MYSQL_TYPE_DATETIME;
		dwBufferLen = sizeof(MYSQL_TIME);
		break;
	}
	m_pValues->SetLen(dwBufferLen);
	m_pRow[index].buffer_type = bSQLType;
	m_pRow[index].buffer = m_pValues[index].m_pData;
	m_pRow[index].buffer_length  = dwBufferLen;
	m_pRow[index].length = &m_pValues->m_dwLen;
	m_pRow[index].is_null = &m_pValues[index].m_bIsNull;
	m_pRow[index].is_unsigned = blsigned;	
	m_pRow[index].error = &m_pValues[index].m_bError;
}

bool MySqlCmd::SetCmd(const char* szSQL,...)
{
	if(m_pDB == NULL || m_pDB->m_pMySql == NULL) return false;
	if(!Reset()) return false;
	m_pStmt = mysql_stmt_init(m_pDB->m_pMySql);
	if(m_pStmt == NULL)
	{
		m_pDB->_ErrorDump();
		return false;
	}
	char	szCMD[4096];
	vsprintf(szCMD, szSQL, (char*)(&szSQL+1));
	if(mysql_stmt_prepare(m_pStmt,szCMD,strlen(szCMD)))
	{
		_ErrorDump();
		return false;
	}
	m_dwParamCnt = mysql_stmt_param_count(m_pStmt);
	if(m_dwParamCnt > 0)
	{
		m_pParams = new MYSQL_BIND[m_dwParamCnt];
		memset(m_pParams,0,sizeof(MYSQL_BIND)*m_dwParamCnt);
	}
	m_dwFieldCnt = mysql_stmt_field_count(m_pStmt);
	if(m_dwFieldCnt > 0)
	{
		m_pMetaRlt = mysql_stmt_result_metadata(m_pStmt);
		if(m_pMetaRlt == NULL)
		{
			_ErrorDump();
			return false;
		}
		m_pFields = mysql_fetch_fields(m_pMetaRlt);
		m_pRow = new MYSQL_BIND[m_dwFieldCnt];
		memset(m_pRow,0,sizeof(MYSQL_BIND)*m_dwFieldCnt);
		m_pblIsNull = new my_bool[m_dwFieldCnt];
		memset(m_pblIsNull,0,sizeof(my_bool)*m_dwFieldCnt);
		m_pblError = new my_bool[m_dwFieldCnt];
		memset(m_pblError,0,sizeof(my_bool)*m_dwFieldCnt);
		m_pValues = new ValueNode[m_dwFieldCnt];
	}
	return true;
}

bool MySqlCmd::BindParams()
{
	if(m_pDB == NULL || m_pDB->m_pMySql == NULL || m_pStmt == NULL) return false;
	if(mysql_stmt_bind_param(m_pStmt, m_pParams))
	{
		_ErrorDump();
		return false;
	}
	return true;
}

bool MySqlCmd::Execut()
{
	if(m_pDB == NULL || m_pDB->m_pMySql == NULL || m_pStmt == NULL) return false;
	if(mysql_stmt_execute(m_pStmt))
	{
		unsigned int nError = mysql_errno(m_pDB->m_pMySql);
		if(nError == CR_SERVER_GONE_ERROR || nError == CR_SERVER_LOST)
		{
			if(m_pDB->Connected())
			{
				if(mysql_stmt_execute(m_pStmt))
				{
					_ErrorDump();
				}
				else
				{
					return true;
				}
			}
		}
		_ErrorDump();
		return false;
	}
	return true;
}

bool MySqlCmd::FetchRecords()
{
	if(m_pDB == NULL || m_pDB->m_pMySql == NULL || m_pStmt == NULL) return false;

	if(mysql_stmt_store_result(m_pStmt))
	{
		_ErrorDump();
		return false;
	}
	return true;
}

DWORD MySqlCmd::GetRecordsCnt()
{
	if(m_pDB == NULL || m_pDB->m_pMySql == NULL || m_pStmt == NULL || m_pFields == NULL) return 0;
	return (DWORD)mysql_stmt_num_rows(m_pStmt);
}

bool MySqlCmd::FetchRow()
{
	if(m_pDB == NULL || m_pDB->m_pMySql == NULL || m_pStmt == NULL || m_pFields == NULL) return false;
	if(mysql_stmt_fetch(m_pStmt))
	{
		return false;
	}
	return true;
}

bool MySqlCmd::BindRltFields()
{
	if(m_pDB == NULL || m_pDB->m_pMySql == NULL || m_pStmt == NULL) return false;
	if(mysql_stmt_bind_result(m_pStmt,m_pRow))
	{
		_ErrorDump();
		return false;
	}
	return true;
}

int MySqlCmd::_GetFieldIndex(const char* szField)
{
	if(m_pDB == NULL || m_pRow == NULL || m_pFields == NULL) return -1;

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

char* MySqlCmd::GetStr(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return "";
	if(m_pValues[i].m_bIsNull) return "";
	return (char*)m_pValues[i].m_pData;
}

BYTE* MySqlCmd::GetBLOB(const char* szField,int* pnLen)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return NULL;
	if(m_pValues[i].m_bIsNull) return NULL;
	*pnLen = m_pValues[i].m_dwLen;
	return m_pValues[i].m_pData;
}

WORD MySqlCmd::GetWORD(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return 0;
	if(m_pValues[i].m_bIsNull) return 0;
	return (WORD)m_pValues[i].m_pData;
}

DWORD MySqlCmd::GetDWORD(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return 0;
	if(m_pValues[i].m_bIsNull) return 0;
	return (DWORD)m_pValues[i].m_pData;
}

double MySqlCmd::GetDouble(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return 0;
	if(m_pValues[i].m_bIsNull) return 0;
	double* pV = (double*)m_pValues[i].m_pData;
	return (*pV);
}

int MySqlCmd::GetInt(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return 0;
	if(m_pValues[i].m_bIsNull) return 0;
	return (int)m_pValues[i].m_pData;
}

short MySqlCmd::GetShort(const char* szField)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return 0;
	if(m_pValues[i].m_bIsNull) return 0;
	return (short)m_pValues[i].m_pData;
}

bool MySqlCmd::GetTime(const char* szField,SYSTEMTIME* pSysTime)
{
	int i = _GetFieldIndex(szField);
	if(i < 0) return false;
	if(m_pValues[i].m_bIsNull) return false;
	MYSQL_TIME* pTime = (MYSQL_TIME*)m_pValues[i].m_pData;
	pSysTime->wYear = pTime->year;
	pSysTime->wMonth = pTime->month;
	pSysTime->wDay = pTime->day;
	pSysTime->wHour = pTime->hour;
	pSysTime->wMinute = pTime->minute;
	pSysTime->wSecond = pTime->second;
	return true;
}

void MySqlCmd::_ErrorDump()
{
	if(m_pDB != NULL && m_pStmt != NULL)
	{
		const char* szError = mysql_stmt_error(m_pStmt);
		m_pDB->_ErrorDumpSimple(szError);
	}
}

}//--db--