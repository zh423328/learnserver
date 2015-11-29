#include "ServerConfig.h"
#include "rapidxml.hpp"
#include "rapidxml_iterators.hpp"
#include "rapidxml_print.hpp"
#include "rapidxml_utils.hpp"


CreateSingleton(ServerConfig);

ServerConfig::ServerConfig()
{
	memset(&m_dbSrvInfo,0,sizeof(ENGINE_COMPONENT_INFO));
	memset(&m_logingateInfo,0,sizeof(ENGINE_COMPONENT_INFO));
	memset(&m_loginsrvInfo,0,sizeof(ENGINE_COMPONENT_INFO));
	memset(&m_selgateInfo,0,sizeof(ENGINE_COMPONENT_INFO));
	memset(&m_gamegateInfo,0,sizeof(ENGINE_COMPONENT_INFO));
	memset(&m_gamesrvInfo,0,sizeof(ENGINE_COMPONENT_INFO));

	m_dbSrvInfo.id		= DBSRV_TYPE;
	m_logingateInfo.id  = LOGINGATE_TYPE;
	m_loginsrvInfo.id	= LOGINSRV_TYPE;
	m_selgateInfo.id	= SELGATE_TYPE;
	m_gamegateInfo.id	= GAMEGATE_TYPE;
	m_gamesrvInfo.id	= GAMESRV_TYPE;
}

ServerConfig::~ServerConfig()
{

}
/************************************************************************/
/* <config>
		<dbsrv>
		</dbsrv>
   </config>
*/
/************************************************************************/

bool ServerConfig::LoadServerConfig(char * szXml)
{
	//读取文件
	rapidxml::file<> fdoc(szXml);

	//解析文件
	rapidxml::xml_document<> doc;
	doc.parse<0>(fdoc.data());

	rapidxml::xml_node<> *root = doc.first_node();
	if (root == NULL)
		return false;

	//dbsrv
	rapidxml::xml_node<> *pNode = root->first_node();
	while (pNode)
	{
		ENGINE_COMPONENT_INFO *pInfo = NULL;
		if (strcmp(pNode->name(),"dbsrv") == 0)
		{
			pInfo = &m_dbSrvInfo;
		}
		else if (strcmp(pNode->name(),"logingate") == 0)
		{
			pInfo = &m_logingateInfo;
		}
		else if (strcmp(pNode->name(),"loginsrv") == 0)
		{
			pInfo = &m_loginsrvInfo;
		}
		else if (strcmp(pNode->name(),"selgate") == 0)
		{
			pInfo = &m_selgateInfo;
		}
		else if (strcmp(pNode->name(),"gamegate") == 0)
		{
			pInfo = &m_gamegateInfo;
		}
		else if (strcmp(pNode->name(),"gamesrv") == 0)
		{
			pInfo = &m_gamesrvInfo;
		}
		if (pInfo == NULL)
		{
			pNode = root->next_sibling();
			continue;
		}

		rapidxml::xml_node<> *pChildNode = pNode->first_node("extip");
		if (pChildNode)
		{
			pInfo->extip = pChildNode->value();
		}

		pChildNode = pNode->first_node("extport");
		if (pChildNode)
		{
			pInfo->extport = atoi(pChildNode->value());
		}

		pChildNode = pNode->first_node("intip");
		if (pChildNode)
		{
			pInfo->intip = pChildNode->value();
		}

		pChildNode = pNode->first_node("intport");
		if (pChildNode)
		{
			pInfo->intport =  atoi(pChildNode->value());
		}
		
		pChildNode = pNode->first_node("dbip");
		if (pChildNode)
		{
			pInfo->dbip = pChildNode->value();
		}

		pChildNode = pNode->first_node("dbport");
		if (pChildNode)
		{
			pInfo->dbport =  atoi(pChildNode->value());
		}

		pChildNode = pNode->first_node("dbuser");
		if (pChildNode)
		{
			pInfo->dbuser = pChildNode->value();
		}

		pChildNode = pNode->first_node("dbpwd");
		if (pChildNode)
		{
			pInfo->dbpwd = pChildNode->value();
		}

		pChildNode = pNode->first_node("dbname");
		if (pChildNode)
		{
			pInfo->dbname = pChildNode->value();
		}

		pChildNode = pNode->first_node("dbcon");
		if (pChildNode)
		{
			pInfo->dbconnectcnt = atoi(pChildNode->value());
		}
		pNode = pNode->next_sibling();
	}

	return true;
}

ENGINE_COMPONENT_INFO& ServerConfig::getDBSrvInfo()
{
	return m_dbSrvInfo;
}

ENGINE_COMPONENT_INFO& ServerConfig::getLoginGateInfo()
{
	return m_logingateInfo;
}

ENGINE_COMPONENT_INFO& ServerConfig::getLoginSrvInfo()
{
	return m_loginsrvInfo;
}

ENGINE_COMPONENT_INFO& ServerConfig::getSelGateInfo()
{
	return m_selgateInfo;
}

ENGINE_COMPONENT_INFO& ServerConfig::getGameGateInfo()
{
	return m_gamegateInfo;
}

ENGINE_COMPONENT_INFO& ServerConfig::getGameSrvInfo()
{
	return m_gamesrvInfo;
}
