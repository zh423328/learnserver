/************************************************************************/
/* 服务器配置                                                                     */
/************************************************************************/
#ifndef SERVERCONFIG_H_
#define SERVERCONFIG_H_

#include  "Common.h"
#include "singleton.h"

// 引擎组件信息结构体
typedef struct EngineComponentInfo
{
	
	std::string extip;										// 最近的运行后对外的ip
	uint32		extport;									// 组件的运行后监听的端口
	std::string intip;										// 组件对内ip
	uint32		intport;									// 组件的运行后监听的端口
	COMPONENT_ID id;										// 组件id

	std::string		dbip;			//IP
	uint32			dbport;		//端口
	std::string		dbuser;		//用户名
	std::string		dbpwd;		//密码
	std::string		dbname;		//数据库名
	uint32			dbconnectcnt; //连接数

}ENGINE_COMPONENT_INFO;

class ServerConfig:public CSingleton<ServerConfig>
{
public:
	ServerConfig();
	~ServerConfig();
	
	//读取配置
	bool LoadServerConfig(char * szXml);

	ENGINE_COMPONENT_INFO& getDBSrvInfo();
	ENGINE_COMPONENT_INFO& getLoginGateInfo();
	ENGINE_COMPONENT_INFO& getLoginSrvInfo();
	ENGINE_COMPONENT_INFO& getSelGateInfo();
	ENGINE_COMPONENT_INFO& getGameGateInfo();
	ENGINE_COMPONENT_INFO& getGameSrvInfo();

private:
	ENGINE_COMPONENT_INFO m_dbSrvInfo;			//dbsrv
	ENGINE_COMPONENT_INFO m_logingateInfo;		//logingate
	ENGINE_COMPONENT_INFO m_loginsrvInfo;		//loginsrv
	ENGINE_COMPONENT_INFO m_selgateInfo;		//selgate
	ENGINE_COMPONENT_INFO m_gamegateInfo;		//gamegate
	ENGINE_COMPONENT_INFO m_gamesrvInfo;		//gamesrv
};

#define g_SeverConfig ServerConfig::GetInstance()
#endif