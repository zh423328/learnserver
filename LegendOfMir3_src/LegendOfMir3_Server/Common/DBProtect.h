/************************************************************************/
/* DBProtect保护程序                                                                     */
/************************************************************************/
#ifndef _DBPROTECT_H_
#define _DBPROTECT_H_

#include "Common.h"

namespace db
{

//db数据保存
//二进制数据模块
class CDBBLOB
{
public:
	CDBBLOB(char * pData, uint32 nLen);
	~CDBBLOB();

	char* m_pData;	//数据
	uint32 m_nLen;	//长度
};

//db操作
class CDBOperation;
//保护
class CDBProtect;	

typedef std::list<CDBBLOB*> CDBBLOBList;
typedef std::list<CDBOperation*> CDBOperationList;

//操作
class CDBOperation
{
public:
	CDBOperation();
	virtual ~CDBOperation();

	//设置二进制文本
	virtual void SetSQL(char* szSQL,...);

	//添加二进制数据
	virtual void AddBlob(char *pData,uint32 nLen);

	//保存和读取SQL记录
	virtual uint32 Load(FILE * fp);
	virtual void   Save(FILE * fp);
	
	//执行
	virtual bool Excute(CDBProtect * pProtect) = 0;
protected:
	char * m_szSQL;
	CDBBLOBList m_listBLOB;
};

//dbproect
class CDBProtect
{
public:
	CDBProtect(char * szFileName);
	~CDBProtect();

	//添加SQL 记录
	virtual CDBOperation * CreateOperation() = 0;
	void AddOperation(CDBOperation * pOperation);

	//保存和读取operation
	bool LoadOperation();
	bool SaveOperation();

	//执行
	virtual bool Excute();

private:
	//备份
	bool _BackUpOperation();

	CDBOperationList m_listOperation;
	char * m_szFileName;
};

}
#endif