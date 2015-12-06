/************************************************************************/
/* Account													
*/
/************************************************************************/
#ifndef ACCOUNT_PROTOCOL_H_
#define ACCOUNT_PROROTOL_H_

#include "Category.h"

#pragma pack(1)
enum
{
	REQ_REGISTER,			//注册
	RES_REGISTERRLT,		//结果

	REQ_LOGIN,			//注册
	RES_LOGINRLT,		//结果

	ACCOUNT_MAX,
};

#define SM_CERTIFICATION_FAIL	1				//证书
#define SM_ID_NOTFOUND			2				//未找到
#define SM_PASSWD_FAIL			3				//验证失败
#define SM_NEWID_SUCCESS        4				//添加成功
#define SM_NEWID_FAIL           5				//新id失败
#define SM_NEWID_EXISTS			6				//新id已存在
#define SM_PASSOK_SELECTSERVER	7				//
#define SM_SELECTSERVER_OK		8				

struct ACCOUNT_REQ_REGISTER_Data
{
	char szID[50];
	char szPwd[16];
};

struct ACCOUNT_RES_REGISTERRLT_Data
{
	uint8 nRlt;
};

struct ACCOUNT_REQ_LOGIN_Data
{
	char szID[50];
	char szPwd[16];
};

struct ACCOUNT_RES_LOGINRLT_Data
{
	uint8 nRlt;
};

#pragma pack()
#endif