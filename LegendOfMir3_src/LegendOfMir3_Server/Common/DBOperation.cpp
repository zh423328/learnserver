#include "DBProtect.h"

namespace db
{
CDBOperation::CDBOperation()
{
	m_szSQL = NULL;
}

CDBOperation::~CDBOperation()
{
	SAFE_FREE(m_szSQL);

	//释放
	for (CDBBLOBList::iterator iter = m_listBLOB.begin(); iter != m_listBLOB.end();++iter)
	{
		CDBBLOB *pBlob = *iter;
		SAFE_DELETE(pBlob);
	}
	m_listBLOB.clear();
}

//添加sql操作
void CDBOperation::SetSQL(char* szFormat,...)
{
	SAFE_FREE(m_szSQL);

	char szCmd[2048] = {0};
	vsprintf(szCmd,szFormat,(char*)(&szFormat + 1));

	m_szSQL = strdup(szCmd);
}

//添加二进制数据
void CDBOperation::AddBlob(char *pData,uint32 nLen)
{
	if (pData ==  NULL || nLen == 0)
		return;

	CDBBLOB *pBlob = new CDBBLOB(pData,nLen);
	m_listBLOB.push_back(pBlob);
}

//加载文件
uint32 CDBOperation::Load(FILE * fp)
{
	if (fp == NULL)
		return 0;
	else
	{
		//读取长度
		uint32 nLen = 0;
		uint32 nPos = 0;

		//读取sql
		fread(&nLen,sizeof(uint32),1,fp);
		
		nPos += sizeof(uint32);

		m_szSQL = new char[nLen];
		fread(m_szSQL,nLen,1,fp);

		//读取二进制数据
		fread(&nLen,sizeof(uint32),1,fp);
		nPos += sizeof(uint32);

		for (int i = 0 ; i < nLen; ++i)
		{
			uint32 nSize = 0;
			fread(&nSize,sizeof(uint32),1,fp);
			nPos += sizeof(uint32);

			char *pData = new char[nSize];
			fread(pData,nSize,1,fp);
			nPos += nSize;

			CDBBLOB *pBlob = new CDBBLOB(pData,nSize);
			m_listBLOB.push_back(pBlob);

			SAFE_DELETE_ARRAY(pData);
		}
		
		return nPos;
	}
}

//保存数据
void CDBOperation::Save(FILE * fp)
{
	if (fp ==NULL)
		return;
	else
	{
		//保存sql
		uint32 nLen = strlen(m_szSQL) + 1;
		fwrite(&nLen,sizeof(uint32),1,fp);

		fwrite(m_szSQL,nLen,1,fp);

		//保存二进制数据
		nLen = m_listBLOB.size();
		fwrite(&nLen,sizeof(uint32),1,fp);
		for (CDBBLOBList::iterator iter = m_listBLOB.begin(); iter != m_listBLOB.end(); ++iter)
		{
			CDBBLOB *pBlob = *iter;
			if (pBlob)
			{
				fwrite(&pBlob->m_nLen,sizeof(uint32),1,fp);
				fwrite(pBlob->m_pData,pBlob->m_nLen,1,fp);
			}
		}
	}
}
}