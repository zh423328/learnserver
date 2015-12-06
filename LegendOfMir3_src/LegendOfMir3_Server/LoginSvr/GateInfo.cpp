#include "stdafx.h"
#include "../def/dbmgr.h"
#include <stdio.h>
#include "../Common/Common.h"
#include "../Common/DBManager.h"
#include "AccountManager.h"
#include "../packet/Category.h"
#include "../packet/logingate_protocol.h"
#include "../packet/loginsrv_protocol.h"
#include "../packet/Account_protocol.h"

extern CWHList< GAMESERVERINFO* >	g_xGameServerList;
extern char							g_szServerList[1024];
extern CRandom						g_pRandom;

int CompareDBString( char *str1, char *str2 )
{
	ChangeSpaceToNull( str1 );
	ChangeSpaceToNull( str2 );

	return strcmp( str1, str2 );
}

int GetCertification() 
{ 
	static long	g_nCertification = 30;

	InterlockedIncrement(&g_nCertification);

	if (g_nCertification >= 0x7FFFFFFF) 
		g_nCertification = 30; 
	
	return g_nCertification; 
}

/* **************************************************************************************

		Close
		
		PURPOSE : Send packet to login gate server.
		
		NOTE
		
		1. Packet construction : Packet % + client socket handle + / # + packet + ! + $

   ************************************************************************************** */

void CGateInfo::Close()
{
	PLISTNODE		pListNode;
	CUserInfo*		pUserInfo;

	if (xUserInfoList.GetCount())
	{
		pListNode = xUserInfoList.GetHead();

		while (pListNode)
		{
			pUserInfo = xUserInfoList.GetData(pListNode);

			if (pUserInfo)
			{
				delete pUserInfo;
				pUserInfo = NULL;

				pListNode = xUserInfoList.RemoveNode(pListNode);

				continue;
			}

			pListNode = xUserInfoList.GetNext(pListNode);
		}
	}

	closesocket(sock);
	sock = INVALID_SOCKET;
}

/* **************************************************************************************

		SendToGate 
		
		PURPOSE : Send packet to login gate server.
		
		NOTE
		
		1. Packet construction : Packet % + client socket handle + / # + packet + ! + $

   ************************************************************************************** */

void CGateInfo::SendToGate(SOCKET cSock, char *pszPacket)
{
	char	szData[256];

	sprintf(szData, "%%%d/#%s!$", (int)cSock, pszPacket);
	
	int nLen = memlen(pszPacket) - 1;

	szData[0] = '%';
	
	char *pszNext = ValToAnsiStr((int)cSock, &szData[1]);
	
	*pszNext++ = '/';
	*pszNext++ = '#';

	memmove(pszNext, pszPacket, nLen);

	pszNext += nLen;

	*pszNext++ = '!';
	*pszNext++ = '$';
	*pszNext++ = '\0';

	//buf.len = pszNext - szData;
	//buf.buf = szData;
	SendToGate(szData,pszNext-szData);
//	WSASend(sock, &buf, 1, &dwSendBytes, 0, NULL, NULL);
}

/* **************************************************************************************

		MakeNewUser
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

void CGateInfo::MakeNewUser(char *pszPacket)
{
	char				szDecodeMsg[256];
	char				szEncodeMsg[32];
	char				*pszID, *pszName, *pszPassword;
	_TDEFAULTMESSAGE	DefMsg;

	fnDecodeMessageA(&DefMsg, pszPacket);

	if (DefMsg.wIdent == CM_ADDNEWUSER)
	{
		int nPos = fnDecode6BitBufA((pszPacket + DEFBLOCKSIZE), szDecodeMsg, sizeof(szDecodeMsg));
		szDecodeMsg[nPos] = '\0';

		pszID		= &szDecodeMsg[0];
		
		pszName		= (char *)memchr(szDecodeMsg, '/', memlen(szDecodeMsg) - 1);
		*pszName = '\0';
		pszName++;

		pszPassword	= (char *)memchr(pszName, '/', memlen(pszName) - 1);
		*pszPassword = '\0';
		pszPassword++;

		if ((memlen(pszID) - 1) || (memlen(pszName) - 1) || (memlen(pszPassword) - 1))
		{
			char szQuery[1024];
			sprintf( szQuery, 
				"INSERT TBL_ACCOUNT( FLD_LOGINID, FLD_PASSWORD, FLD_USERNAME, FLD_CERTIFICATION ) "
				"VALUES( '%s', '%s', '%s', 0 )",
				pszID, pszPassword, pszName );

			CRecordset *pRec = GetDBManager()->CreateRecordset();
			if ( pRec->Execute( szQuery ) && pRec->GetRowCount() )
				fnMakeDefMessageA( &DefMsg, SM_NEWID_SUCCESS, 0, 0, 0, 0 );
			else
				fnMakeDefMessageA( &DefMsg, SM_NEWID_FAIL, 0, 0, 0, 0 );
			GetDBManager()->DestroyRecordset( pRec );
			// -----------------------------------------------------------------------------------
		}
		else
			fnMakeDefMessageA(&DefMsg, SM_NEWID_FAIL, 0, 0, 0, 0);

		fnEncodeMessageA(&DefMsg, szEncodeMsg, sizeof(szEncodeMsg));

		szDecodeMsg[0] = '#';
		memmove(&szDecodeMsg[1], szEncodeMsg, DEFBLOCKSIZE);
		szDecodeMsg[DEFBLOCKSIZE + 1] = '!';
		szDecodeMsg[DEFBLOCKSIZE + 2] = '\0';

		send(sock, szDecodeMsg, DEFBLOCKSIZE + 2, 0); 
	}
}

/* **************************************************************************************

		ReceiveServerMsg
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

void CGateInfo::ReceiveServerMsg(char *pszPacket)
{
	char		*pszPos;
	int			nCertification;
	int			nLen = memlen(pszPacket);

	if (pszPos = (char *)memchr(pszPacket, '/', nLen))
	{
		*pszPos++ = '\0';
		nCertification = AnsiStrToVal(pszPos);

		char szQuery[256];
		sprintf( szQuery, 
			"UPDATE TBL_ACCOUNT SET FLD_CERTIFICATION=%d WHERE FLD_LOGINID='%s'", 
			nCertification, pszPacket );

		CRecordset *pRec = GetDBManager()->CreateRecordset();
		pRec->Execute( szQuery );
		GetDBManager()->DestroyRecordset( pRec );
	}
}

void CGateInfo::ReceiveOpenUser(uint32 sock,char *szIp)
{

	CUserInfo* pUserInfo = new CUserInfo;

	if (pUserInfo)
	{
		pUserInfo->sock					= sock;
		pUserInfo->nCertification		= 0;
		pUserInfo->nClientVersion		= 0;
		pUserInfo->fSelServerOk			= FALSE;

		strcpy(pUserInfo->szAddress,szIp);

		ZeroMemory(pUserInfo->szUserID, sizeof(pUserInfo->szUserID));

		xUserInfoList.AddNewNode(pUserInfo);

		InsertLogMsgParam(IDS_OPEN_USER, pUserInfo->szAddress);
	}
}

/* **************************************************************************************

		ReceiveCloseUser
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

void CGateInfo::ReceiveCloseUser(uint32 sock)
{
	//删除user

	xUserInfoList.RemoveNodeByKey(sock);
}

/* **************************************************************************************

		ReceiveSendUser
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

void CGateInfo::ReceiveSendUser(uint32 socket,char * szMsg,uint32 nLen)
{
	_LPTSENDBUFF lpSendUserData = new _TSENDBUFF;
	lpSendUserData->nLen	   = nLen;

	lpSendUserData->sock		= socket;

	memmove(lpSendUserData->szData, szMsg, nLen);

	g_SendToGateQ.PushQ((BYTE *)lpSendUserData);
}

/* **************************************************************************************

		ProcSelectServer
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

void CGateInfo::ProcSelectServer(SOCKET s, WORD wServerIndex)
{
	_TDEFAULTMESSAGE	DefMsg;
	char				szEncodePacket[128];
	char				szEncodeAllPacket[256];
	char				szEncodeMsg[24];
	char				*pServerIP;
	GAMESERVERINFO		*pServerInfo;

	PLISTNODE pListNode = xUserInfoList.GetHead();

	while (pListNode)
	{
		CUserInfo *pUserInfo = xUserInfoList.GetData(pListNode);

		if (pUserInfo->sock == s)
		{
			if (!pUserInfo->fSelServerOk)
			{
				fnMakeDefMessageA(&DefMsg, SM_SELECTSERVER_OK, 0, pUserInfo->nCertification, 0, 0);		
				int nPos = fnEncodeMessageA(&DefMsg, szEncodeMsg, sizeof(szEncodePacket));
				szEncodeMsg[nPos] = '\0';

				for ( PLISTNODE pNode = g_xGameServerList.GetHead(); pNode; pNode = g_xGameServerList.GetNext( pNode ) )
				{
					pServerInfo = g_xGameServerList.GetData( pNode );
					
					if ( pServerInfo->index == wServerIndex )
					{
						pServerIP = pServerInfo->ip;
						pServerInfo->connCnt++;
						break;
					}
				}

				if ( !pServerIP )
					break;

				pUserInfo->nServerID = wServerIndex;

				int nPos2 = fnEncode6BitBufA((unsigned char *)pServerIP, szEncodePacket, memlen(pServerIP), sizeof(szEncodePacket));
				szEncodePacket[nPos2] = '\0';

				memmove(szEncodeAllPacket, szEncodeMsg, nPos);
				memmove(&szEncodeAllPacket[nPos], szEncodePacket, nPos2);
				szEncodeAllPacket[nPos + nPos2] = '\0';

				SendToGate(s, szEncodeAllPacket);

				pUserInfo->fSelServerOk = TRUE;

				pListNode = xUserInfoList.RemoveNode(pListNode);
			}
		}
		else
			pListNode = xUserInfoList.GetNext(pListNode);
	}
}

/* **************************************************************************************

		ParseUserEntry
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

bool CGateInfo::ParseUserEntry( char *buf, AccountUser *userInfo )
{
	char seps[] = "|";
	char *token = strtok( buf, seps );
	int  step   = 0;
	
	__try
	{
		while ( token )
		{
			switch ( step++ )
			{
			case 0:
				strcpy(userInfo->m_id,token);
				break;
			case 1:
				strcpy(userInfo->m_pwd,token);
				break;
			}	
			
			token = strtok( NULL, seps );
		}
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
	
	return step >= 2;
}

/* **************************************************************************************

		ProcAddUser
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

//fnMakeDefMessageA 数据头
//
void CGateInfo::ProcAddUser(SOCKET s, char *szID,char * szPwd)
{
	if (szID == NULL || szPwd == NULL)
		return;

	char				szEntryInfo[2048];
	AccountUser			UserEntryInfo;
	_TDEFAULTMESSAGE	DefMsg;
	char				szEncodePacket[64];

	int					nRlt;

	if (strlen(szID)< 6 || strlen(szPwd) < 6 )
		nRlt = SM_NEWID_FAIL;
	else
	{	
		AccountUser *pAlreadyUser = AccoutManager::GetInstance().GetAccount(UserEntryInfo.m_id);
		if (pAlreadyUser)
		{
			//已经存在
			nRlt = SM_NEWID_EXISTS;
		}
		else
		{
			char szQuery[1024] = {0};
			sprintf( szQuery, "insert into Account(id,pwd) values('%s','%s')",UserEntryInfo.m_id,UserEntryInfo.m_pwd);

			//解析
			db::DBServer *pServer = DBManager::GetInstance().GetFreeCon();
			if (pServer == NULL)
			{
				nRlt = SM_NEWID_FAIL;
			}
			else
			{
				if (pServer->ExecuteSQL(szQuery))
				{
					nRlt = SM_NEWID_SUCCESS;

					AccountUser *pNewUser = new AccountUser();
					strcpy(pNewUser->m_id,UserEntryInfo.m_id);
					strcpy(pNewUser->m_pwd,UserEntryInfo.m_pwd);

					AccoutManager::GetInstance().InsertAccountUser(pNewUser);
				}
				else
				{
					nRlt = SM_NEWID_FAIL;//fnMakeDefMessageA(&DefMsg, SM_NEWID_FAIL, 0, 0, 0, 0);
				}
				DBManager::GetInstance().ReleaseCon(pServer);
				
			}
			

			nRlt = SM_NEWID_SUCCESS;

			TCHAR szID[50];
			MultiByteToWideChar( CP_ACP, 0, UserEntryInfo.m_id, -1, szID, sizeof( szID ) / sizeof( TCHAR ) );
			InsertLogMsgParam(IDS_COMPLETENEWUSER, szID);
		}
		
	}		
	
	//加密数据头
	BuildPacketEx(pPacket,ACCOUNT,RES_REGISTERRLT,buf,256);
	SET_DATA(pData,ACCOUNT,RES_REGISTERRLT,pPacket);
	pData->nRlt = nRlt;
	SendToGate(pPacket);
}

/* **************************************************************************************

		ProcLogin
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

void CGateInfo::ProcLogin(SOCKET s, char *pszData)
{
	char				szIDPassword[32];
	char				*pszID, *pszPassword;
	char				szEncodePacket[64];
	_TDEFAULTMESSAGE	DefMsg;
	int					nPos;
	char				szQuery[256];

	if (memlen(pszData) - 1 <= 0) return;
	
	PLISTNODE pListNode = xUserInfoList.GetHead();

	while (pListNode)
	{
		CUserInfo *pUserInfo = xUserInfoList.GetData(pListNode);

		if (pUserInfo->sock == s)
		{
			int nDecodeLen = fnDecode6BitBufA(pszData, szIDPassword, sizeof(szIDPassword));
			szIDPassword[nDecodeLen] = '\0';

			pszID		= &szIDPassword[0];

			if (pszPassword	= (char *)memchr(szIDPassword, '/', sizeof(szIDPassword)))
			{
				*pszPassword = '\0';
				pszPassword++;

				sprintf( szQuery, "SELECT * FROM TBL_ACCOUNT WHERE FLD_LOGINID='%s'", pszID );

				CRecordset *pRec = GetDBManager()->CreateRecordset();

				if ( !pRec->Execute( szQuery ) || !pRec->Fetch() )
					fnMakeDefMessageA( &DefMsg, SM_ID_NOTFOUND, 0, 0, 0, 0 );
				else if ( CompareDBString( pszPassword, pRec->Get( "FLD_PASSWORD" ) ) != 0 )
					fnMakeDefMessageA( &DefMsg, SM_PASSWD_FAIL, 0, 0, 0, 0 );
				else
				{
					int nCertCode = atoi( pRec->Get( "FLD_CERTIFICATION" ) );
		/*
					if ( nCertCode > 0 && nCertCode < 30 )
						fnMakeDefMessageA(&DefMsg, SM_CERTIFICATION_FAIL, (nCertCode + 1), 0, 0, 0);
					else if ( nCertCode >= 30 )
						fnMakeDefMessageA(&DefMsg, SM_CERTIFICATION_FAIL, 1, 0, 0, 0);
					else*/
					{
						char szEncodeServerList[512];
						char szEncodeAllPacket[1024];
						
						fnMakeDefMessageA(&DefMsg, SM_PASSOK_SELECTSERVER, 0, 1, 0, 0);		
						nPos = fnEncodeMessageA(&DefMsg, szEncodePacket, sizeof(szEncodePacket));
						szEncodePacket[nPos] = '\0';
						
						int nPos2 = fnEncode6BitBufA((unsigned char *)g_szServerList, szEncodeServerList, memlen(g_szServerList), sizeof(szEncodeServerList));
						szEncodeServerList[nPos2] = '\0';
						
						memmove(szEncodeAllPacket, szEncodePacket, nPos);
						memmove(&szEncodeAllPacket[nPos], szEncodeServerList, memlen(szEncodeServerList));
						
						SendToGate(s, szEncodeAllPacket);
										
						GetDBManager()->DestroyRecordset( pRec );

						pUserInfo->nCertification = GetCertification();

		//				pRec = GetDBManager()->CreateRecordset();
		//				sprintf( szQuery, 
		//					"UPDATE TBL_ACCOUNT SET FLD_CERTIFICATION=%d WHERE FLD_LOGINID='%s'",
		//					GetCertification(), pszID );
		//				pRec->Execute( szQuery );
						
		//				GetDBManager()->DestroyRecordset( pRec );

						return;
					}
				}

				GetDBManager()->DestroyRecordset( pRec );

				nPos = fnEncodeMessageA(&DefMsg, szEncodePacket, sizeof(szEncodePacket));
				szEncodePacket[nPos] = '\0';

				SendToGate(s, szEncodePacket);
			}
		}

		pListNode = xUserInfoList.GetNext(pListNode);
	}
}

void CGateInfo::SendToGate( char * szData,int nLen )
{
	//发送给loginSrv更新
}


/************************************************************************/
/* 发送心电包                                                                     
*/
/************************************************************************/
void CGateInfo::SendKeepAlivePacket()
{
	BuildCmdPacketEx(pPacket,LOGIN_SRV,SRV2GATE_KEEPALIVE,buf,64);
	SendToGate(pPacket);
}

/************************************************************************/
/* 发送包
*/
/************************************************************************/
void CGateInfo::SendToGate(Packet *pPacket)
{
	if (pPacket == NULL)
		return;

	int nLen = pPacket->dlen;

	if (nLen >= TCP_PACKET_SIZE)
		return;

	uint16 crc = g_pRandom.Random_Int(0,65535);//CrcHelper::GetCrc16(pPacket->data,nLen);
	pPacket->crc = crc;

	//发送给loginSrv更新
	DWORD	dwSendBytes;
	WSABUF	buf;

	buf.len = pPacket->dlen + pPacket->hlen;
	buf.buf = (char*)pPacket;

	pPacket->dlen = crc ^ pPacket->dlen;

	if ( WSASend(sock, &buf, 1, &dwSendBytes, 0, NULL, NULL) == SOCKET_ERROR )
	{
		int nErr = WSAGetLastError();
	}
}


bool CGateInfo::PacketProcess( Packet*pPacket )
{
	if (pPacket == NULL)
		return false;

	switch(pPacket->Category)
	{
	case LOGIN_GATE:
		return LoginGateProcess(pPacket);
	case LOGIN_SRV:
		return LoginSrvProcess(pPacket);
	default:
		if (pPacket->Category >= CATEGORY_MAX)
			return false;
	}

	return true;
}


/************************************************************************/
/* LOGINGATE
*/
/************************************************************************/
bool CGateInfo::LoginGateProcess(Packet*pPacket)
{
	if (pPacket == NULL)
		return false;

	switch(pPacket->Protocol)
	{
	case GATE2SRV_KEEPALIVE:
		{
			//发送心跳包
			SendKeepAlivePacket();
		}
		break;
	case GATE2SRV_ADDSOCKET:
		{
			SET_DATA(pData,LOGIN_GATE,GATE2SRV_ADDSOCKET,pPacket);
			ReceiveOpenUser(pData->sock,pData->szIp);
		}
		break;
	case GATE2SRV_CLOSESOCKET:
		{
			SET_DATA(pData,LOGIN_GATE,GATE2SRV_CLOSESOCKET,pPacket);
			ReceiveCloseUser(pData->sock);
		}
		break;
	case GATE2SRV_MSG:
		{
			SET_DATA(pData,LOGIN_GATE,GATE2SRV_MSG,pPacket);
			ReceiveSendUser(pData->sock,pData->szPacket,pPacket->dlen);
		}
		break;
	default:
		if (pPacket->Category >= LOGINGATE_MAX)
			return false;
	}

	return true;
}

/************************************************************************/
/* LOGINSRV
*/
/************************************************************************/
bool CGateInfo::LoginSrvProcess(Packet*pPacket)
{
	return true;
}

/************************************************************************/
/* ACCOUNT
*/
/************************************************************************/
bool CGateInfo::AccountProcess(SOCKET sock,Packet*pPacket)
{
	if (pPacket == NULL)
		return false;

	if (pPacket->Category != ACCOUNT)
		return false;

	switch(pPacket->Protocol)
	{
	case REQ_REGISTER:
		{
			SET_DATA(pData,ACCOUNT,REQ_REGISTER,pPacket);
			ProcAddUser(sock,pData->szID,pData->szPwd);
		}
		break;
	default:
		if (pPacket->Category >= ACCOUNT_MAX)
			return false;
	}

	return true;
}