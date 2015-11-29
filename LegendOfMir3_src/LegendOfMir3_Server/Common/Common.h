#ifndef COMMON_H_
#define COMMON_H_

#include "platform.h"
#include "format.h"

#define INLINE inline

/** 安全的释放一个指针内存 */
#define SAFE_DELETE(i)										\
	if (i)													\
		{													\
		delete i;										\
		i = NULL;										\
		}

/** 安全的释放一个指针数组内存 */
#define SAFE_DELETE_ARRAY(i)								\
	if (i)													\
		{													\
		delete[] i;										\
		i = NULL;										\
		}

/** 安全的释放一个指针内存 */
#define SAFE_FREE(i)										\
	if (i)													\
		{													\
		free(i);										\
		i = NULL;										\
		}



void printmsg(std::string str);

void printmsg(char* str);

#define ERROR_MSG(str) printmsg(str)
#define INFO_MSG(str) printmsg(str)
#define WARNING_MSG(str) printmsg(str)
#define DEBUG_MSG(str) printmsg(str)

/** 定义服务器各组件类别 */
enum COMPONENT_TYPE
{
	UNKNOWN_COMPONENT_TYPE	= 0,
	DBSRV_TYPE				= 1,
	LOGINGATE_TYPE			= 2,
	LOGINSRV_TYPE			= 3,
	SELGATE_TYPE			= 4,
	GAMEGATE_TYPE			= 5,
	GAMESRV_TYPE			= 6,
	COMPONENT_END_TYPE		= 7,
};

/** 当前服务器组件类别和ID */
extern COMPONENT_TYPE g_componentType;

/** 定义服务器各组件名称 */
const char COMPONENT_NAME[][255] = {
	"unknown",
	"dbsrv",
	"logingate",
	"loginsrv",
	"selgate",
	"gamegate",
	"gamesrv",
};

#endif