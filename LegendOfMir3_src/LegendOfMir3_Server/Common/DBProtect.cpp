#include "DBProtect.h"

namespace db
{

CDBBLOB::CDBBLOB(char * pData, uint32 nLen)
{
	m_pData = new char[nLen];
	m_nLen = nLen;
	memcpy(m_pData,pData,nLen);
}

CDBBLOB::~CDBBLOB()
{
	SAFE_DELETE_ARRAY(m_pData);
	m_nLen = 0;
}



CDBProtect::CDBProtect( char * szFileName )
{
	m_szFileName = strdup(szFileName);
}
CDBProtect::~CDBProtect()
{
	SAFE_FREE(m_szFileName);
}

//添加operation
void CDBProtect::AddOperation( CDBOperation * pOperation )
{
	if (pOperation == NULL)
		return;

	m_listOperation.push_back(pOperation);
}

bool CDBProtect::LoadOperation()
{
	FILE * fp = fopen(m_szFileName,"rb");
	if (fp == NULL)
		return false;

	uint32 nLen = filelength(fileno(fp));

	uint32 nPos = 0;

	while(nPos < nLen)
	{
		//读取
		CDBOperation *pOperation = CreateOperation();
		nPos += pOperation->Load(fp);
		m_listOperation.push_back(pOperation);
	}

	fclose(fp);

	//备份
	_BackUpOperation();


	return true;
}

//保存二进制文件
bool CDBProtect::SaveOperation()
{
	if(m_listOperation.size() == 0) return true;

	FILE * pFile = fopen(m_szFileName, "wb");
	if(!pFile) return false;

	for(CDBOperationList::iterator iterator = m_listOperation.begin(); iterator != m_listOperation.end(); )
	{
		CDBOperation * pOperation = *iterator;
		pOperation->Save(pFile);

		SAFE_DELETE(pOperation);
		iterator = m_listOperation.erase(iterator++);
	}
	fclose(pFile);
	return true;
}

bool CDBProtect::Excute()
{
	CDBOperationList::iterator iterator = m_listOperation.begin();
	for(; iterator != m_listOperation.end(); )
	{
		CDBOperation * pOperation = *iterator;
		if (!pOperation->Excute(this))
			break;

		SAFE_DELETE(pOperation);
		iterator = m_listOperation.erase(iterator++);
	}

	if(iterator != m_listOperation.end()) 
	{	
		//保存
		SaveOperation();
		return false;
	}

	return true;
}

//重命名
bool CDBProtect::_BackUpOperation()
{
	char szNewFileName[2048] = {0};

	time_t tmt_time = time(0);
	tm * tm_time = localtime(&tmt_time);

	sprintf(szNewFileName, "%s_%d%d%d_%d", m_szFileName, tm_time->tm_year+1900, tm_time->tm_mon+1, tm_time->tm_mday, tmt_time);
	rename(m_szFileName, szNewFileName);

	return true;
}

}