//------------------------------------------------------------------------
//manager--
//------------------------------------------------------------------------
#ifndef DBMANAGER_H_
#define DBMANAGER_H_

#include "singleton.h"
#include "Common.h"
#include "mutex.h"
#include "threadtask.h"
#include "threadpool.h"
#include "DBServer.h"
#include "pool.h"

using namespace db;

enum
{
	CMD_NORMAL,
	CMD_BLOB,
};


//数据库任务
class DBTask : public thread::TPTask,public PoolObject
{
public:
	DBTask();
	DBTask(DBServer *pServer,char* szCmd,uint8 nType = CMD_NORMAL,uint8 *pData= NULL,uint32 nLen = 0);
	virtual ~DBTask();

	static ObjectPool<DBTask>& ObjPool();

	static void destroyObjPool();

	void onReclaimObject();


	//获取大小
	virtual size_t getPoolObjectBytes()
	{
		size_t bytes = sizeof(DBServer*) + 2*sizeof(uint8) + (m_szCmd?strlen(m_szCmd):0) + m_nLen;

		return bytes;
	}

	//为了内存池
	void * operator new(size_t n);
	void  operator delete ( void* ptr, size_t n );

	//任务进行时
	virtual bool process();

	//set
	void SetCmd(char* szCmd);
	void SetData(uint8 *pData,uint32 nLen);
	void SetType(uint8 nType);

	virtual thread::TPTask::TPTaskState presentMainThread();

private:
	DBServer *m_pServer;

	uint8 m_nType;

	uint8* m_pData;		//数据
	uint32 m_nLen;		//长度
	char*  m_szCmd;

	thread::TPTask::TPTaskState state;

	uint32 m_nTry;
};


typedef std::list<DBServer*> DBServerList;

class DBManager:public CSingleton<DBManager>
{
public:
	DBManager();
	virtual ~DBManager();

	bool Initialize(const char* Hostname, unsigned int port,const char* Username, const char* Password, const char* DatabaseName,int ConnectionCount);

	bool Run();

public:
	// pool func
	DBServer * GetFreeCon();
	void ReleaseCon(DBServer*con);
private:
	DBServerList m_freeDBList;
	DBServerList m_busyDBList;

	Mutex  dbConMutex;
	thread::ThreadPool  pool;

	uint32 m_nNum;
};
#endif

