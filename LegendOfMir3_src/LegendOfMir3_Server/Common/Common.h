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

#endif