#ifndef _JCH_MYSQL_DB_CLASS_
#define _JCH_MYSQL_DB_CLASS_

#include	<windows.h>
#include	<mysql.h>
#include	<stdio.h>
#include	<errmsg.h>

namespace db
{

typedef void (*MYSQL_ERROR)(const char* szError,void* pVoid);
class MySqlQuery;
class MySqlCmd;

class MySqlDB
{
private:
	friend	MySqlQuery;
	friend	MySqlCmd;
	MYSQL*					m_pMySql;
	unsigned int			m_nError;
	char*					m_szServer;
	char*					m_szDB;
	char*					m_szUser;
	char*					m_szPWD;
	int						m_nPort;
	MYSQL_ERROR				m_pErrorDump;
	void*					m_pCallBackHandle;
	bool					m_bConnected;

	void					_ErrorDump();
	void					_ErrorDumpSimple(const char* szMsg,...);
	bool					_Reconnect();
public:
	MySqlDB();
	~MySqlDB();
	bool Connect(const char* szServer,const char* szDB,const char* szUser,const char* szPWD,unsigned int nPort = 3306);	
	void Close();
	bool Connected();
	void SetCallBackFunc(MYSQL_ERROR pFunc,void* pHandle);
	bool IsConnected(void);
	bool UseDB(const char * szDB);
	const char * GetDBName(void);
};

bool IsThreadSafe();

class MySqlQuery
{
private:
	MYSQL_RES*		m_pRecords;
	MYSQL_ROW		m_pRow;
	MYSQL_FIELD*	m_pFields;
	DWORD*			m_pFieldsLens;
	MySqlDB*		m_pDB;
	DWORD			m_dwFieldCnt;
	char*			m_szSQL;
	int				_GetFieldIndex(const char* szField);
public:
	MySqlQuery(MySqlDB* pDB);
	~MySqlQuery();
	bool	ExecuteSQL(const char* szSQL,...);
	bool	FetchRecords(bool blAll);
	DWORD	GetRecordsCnt();
	DWORD	GetFieldCnt();
	bool	FetchRow();
	char*	GetStr(const char* szField);
	BYTE*	GetBLOB(const char* szField,int* pnLen);
	WORD	GetWORD(const char* szField);
	int		GetInt(const char* szField);
	DWORD	GetDWORD(const char* szField);
	double  GetDouble(const char* szField);	
	char*	GetFieldName(int nIndex);
	char*	GetFieldValue(int nIndex,int* pnLen = NULL);
	bool	IsBLOBField(int nIndex);
};

class MySqlCmd
{
private:
	class ValueNode
	{
	public:
		BYTE*		m_pData;
		DWORD		m_dwBufferLen;
		DWORD		m_dwLen;
		my_bool		m_bIsNull;
		my_bool		m_bError;
		ValueNode()
		{
			m_pData = NULL;
			m_dwLen = 0;
			m_dwBufferLen = 0;
			m_bIsNull = 0;
			m_bError = 0;
		}
		~ValueNode()
		{
			if(m_pData != NULL)
				delete[] m_pData;
		}
		void SetLen(DWORD dwLen)
		{
			if(m_pData != NULL)
				delete[] m_pData;
			m_pData = new BYTE[dwLen];
			m_dwBufferLen = dwLen;
			m_dwLen = 0;
			m_bIsNull = 0;
			m_bError = 0;
		}
	};
	MYSQL_STMT*		m_pStmt;
	MYSQL_BIND*		m_pParams;
	MYSQL_BIND*		m_pRow;
	MYSQL_RES*		m_pMetaRlt;
	MYSQL_FIELD*	m_pFields;
	MySqlDB*		m_pDB;
	DWORD			m_dwParamCnt;
	DWORD			m_dwFieldCnt;
	my_bool*		m_pblIsNull;
	my_bool*		m_pblError;
	ValueNode*		m_pValues;
	int				_GetFieldIndex(const char* szField);
	void			_ErrorDump();
public:
	enum
	{
		DATA_TYPE_BYTE,
		DATA_TYPE_SHORT,
		DATA_TYPE_WORD,
		DATA_TYPE_INT,
		DATA_TYPE_DWORD,
		DATA_TYPE_FLOAT,
		DATA_TYPE_DOUBLE,
		DATA_TYPE_STRING,
		DATA_TYPE_BLOB,
		DATA_TYPE_DATETIME
	};
	MySqlCmd(MySqlDB* pDB);
	~MySqlCmd();
	bool		SetCmd(const char* szSQL,...);
	bool		Reset();
	void		SetParam(int index,BYTE bType,void* pValue,DWORD dwBufferLen = 0,DWORD* pDataLen = NULL);
	bool		BindParams();
	bool		Execut();
	void		SetRltField(int index,BYTE bType,DWORD dwBufferLen = 0);
	bool		BindRltFields();
	bool		FetchRecords();
	DWORD		GetRecordsCnt();
	bool		FetchRow();
	char*		GetStr(const char* szField);
	BYTE*		GetBLOB(const char* szField,int* pnLen);
	WORD		GetWORD(const char* szField);
	int			GetInt(const char* szField);
	short		GetShort(const char* szField);
	DWORD		GetDWORD(const char* szField);
	double		GetDouble(const char* szField);
	bool		GetTime(const char* szField,SYSTEMTIME* pSysTime);
};

}
#endif