#include "DBManager.h"


CreateSingleton(DBManager);

static ObjectPool<DBTask> g_objPool("DBTask");

DBTask::DBTask(DBServer*pServer, char* szCmd, uint8 nType /*= CMD_NORMAL*/,uint8 *pData/*= NULL*/,uint32 nLen /*= 0*/ )
	:m_pServer(pServer),
	m_nTry(0),
	state (thread::TPTask::TPTASK_STATE_COMPLETED)
{
	SetCmd(szCmd);
	SetType(nType);
	SetData(pData,nLen);

	m_nTry = 0;
}

DBTask::DBTask()
	:m_pServer(NULL),
	m_nTry(0),
	state (thread::TPTask::TPTASK_STATE_COMPLETED),
	m_szCmd(NULL),
	m_nType(CMD_NORMAL),
	m_pData(NULL),
	m_nLen(0)
{

}

DBTask::~DBTask()
{

}


void DBTask::SetCmd( char* szCmd )
{
	if (szCmd  == NULL)
		return;

	SAFE_FREE(m_szCmd);
	m_szCmd = strdup(szCmd);
}

void DBTask::SetData( uint8 *pData,uint32 nLen )
{
	if (pData == NULL || nLen == 0)
		return;

	SAFE_FREE(m_pData);

	m_pData = (uint8*)malloc(nLen);

	memcpy(m_pData,pData,nLen);
}

void DBTask::SetType( uint8 nType )
{
	m_nType = nType;
}

bool DBTask::process()
{
	if (m_pServer)
	{
		switch(m_nType)
		{
		case CMD_NORMAL:
			{
				m_nTry++;
				bool bRlt = m_pServer->ExecuteSQL(m_szCmd);
				if (bRlt)
				{
					state = thread::TPTask::TPTASK_STATE_COMPLETED;
					DBManager::GetInstance().ReleaseCon(m_pServer);
				}
				else
				{
					if (m_nTry < 5)
					{
						state = thread::TPTask::TPTASK_STATE_CONTINUE_CHILDTHREAD;
					}
					else
					{
						state = thread::TPTask::TPTASK_STATE_COMPLETED;
						DBManager::GetInstance().ReleaseCon(m_pServer);
					}
				}
			}
			break;
		case CMD_BLOB:
			{
				if (m_pData&&m_nLen&&m_szCmd&&strlen(m_szCmd))
				{
					m_nTry++;
					bool bRlt = m_pServer->SetBLOB(m_pData,m_nLen,m_szCmd);
					if (bRlt)
					{
						state = thread::TPTask::TPTASK_STATE_COMPLETED;
						DBManager::GetInstance().ReleaseCon(m_pServer);
					}
					else
					{
						if (m_nTry < 5)
						{
							state = thread::TPTask::TPTASK_STATE_CONTINUE_CHILDTHREAD;
						}
						else
						{
							state = thread::TPTask::TPTASK_STATE_COMPLETED;
							DBManager::GetInstance().ReleaseCon(m_pServer);
						}
					}
				}
			}
			break;
		}
	}

	return true;
}

thread::TPTask::TPTaskState DBTask::presentMainThread()
{
	return state;
}

ObjectPool<DBTask>& DBTask::ObjPool()
{
	return g_objPool;
}

void DBTask::destroyObjPool()
{
	g_objPool.destroy();
}

void DBTask::onReclaimObject()
{
	SAFE_FREE(m_szCmd);
	m_nLen = 0;
	m_nTry = 0;
	m_nType = CMD_NORMAL;
	m_pServer = NULL;
	SAFE_FREE(m_pData);
	state = thread::TPTask::TPTASK_STATE_COMPLETED;
}

//------------------------------------------------------------------------
//内存池
void * DBTask::operator new( size_t n )
{
	if (n == sizeof(DBTask))
		return NULL;

	return DBTask::ObjPool().createObject();
}

void DBTask::operator delete( void* ptr, size_t n )
{
	if( ptr == 0 )
	{
		assert(0);
		return;
	}

	if( n != sizeof(DBTask) ) 
	{
		assert(0);
		return;
	}

	DBTask::ObjPool().reclaimObject(static_cast<DBTask*>(ptr));
}

//------------------------------------------------------------------------

DBManager::DBManager()
{
	m_nNum = 0;
}

DBManager::~DBManager()
{
	pool.finalise();
}

bool DBManager::Initialize( const char* Hostname, unsigned int port,const char* Username, const char* Password, const char* DatabaseName,int ConnectionCount)
{
	for (int i = 0; i < ConnectionCount; ++i)
	{
		DBServer *pServer = new DBServer(fmt::format("{}db{}",DatabaseName,i+1).c_str());
		if (pServer->ConnectDB(Hostname,Username,Password,DatabaseName))
		{
			m_freeDBList.push_back(pServer);
		}
	}

	m_nNum = ConnectionCount;

	pool.createThreadPool(1,m_nNum,2*m_nNum);
	return true;
}



DBServer * DBManager::GetFreeCon()
{
	AutoMutex lock(&dbConMutex);

	if (!m_freeDBList.empty())
	{
		DBServer * temp = m_freeDBList.front();

		m_busyDBList.push_back(temp);

		m_freeDBList.pop_front();

		return temp;
	}

	return NULL;
}

void DBManager::ReleaseCon( DBServer*con )
{
	if (con == NULL)
		return;
	dbConMutex.lockMutex();
	m_freeDBList.push_back(con);

	std::list<DBServer*>::iterator iter = std::find(m_busyDBList.begin(),m_busyDBList.end(),con);

	if (iter!=m_busyDBList.end())
	{
		m_busyDBList.erase(iter);
	}
	dbConMutex.unlockMutex();
}

bool DBManager::Run()
{
	//主线程执行
	pool.onMainThreadTick();

	return true;
}
