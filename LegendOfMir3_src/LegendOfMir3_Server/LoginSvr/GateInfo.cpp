#include "stdafx.h"
#include "../def/dbmgr.h"
#include <stdio.h>
#include "../Common/Packet.h"
#include "../Common/DBManager.h"
#include "AccountManager.h"

extern CWHList< GAMESERVERINFO* >	g_xGameServerList;
extern char							g_szServerList[1024];

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

void CGateInfo::ReceiveOpenUser(char *pszPacket)
{
	char	*pszPos;
	int		nSocket;
	int		nLen = memlen(pszPacket);

	if (pszPos = (char *)memchr(pszPacket, '/', nLen))
	{
		nSocket = AnsiStrToVal(pszPacket);

		pszPos++;

		CUserInfo* pUserInfo = new CUserInfo;

		if (pUserInfo)
		{
			MultiByteToWideChar(CP_ACP, 0, pszPacket, -1, pUserInfo->szSockHandle, sizeof(pUserInfo->szSockHandle)/sizeof(TCHAR));
			MultiByteToWideChar(CP_ACP, 0, pszPos, -1, pUserInfo->szAddress, sizeof(pUserInfo->szAddress)/sizeof(TCHAR));

			pUserInfo->sock					= nSocket;
			pUserInfo->nCertification		= 0;
			pUserInfo->nClientVersion		= 0;
			pUserInfo->fSelServerOk			= FALSE;

			ZeroMemory(pUserInfo->szUserID, sizeof(pUserInfo->szUserID));

			xUserInfoList.AddNewNode(pUserInfo);

			InsertLogMsgParam(IDS_OPEN_USER, pUserInfo->szAddress);
		}
	} 
}

/* **************************************************************************************

		ReceiveCloseUser
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

void CGateInfo::ReceiveCloseUser(char *pszPacket)
{
	int nSocket = AnsiStrToVal(pszPacket);
}

/* **************************************************************************************

		ReceiveSendUser
		
		PURPOSE : 
		
		NOTE

   ************************************************************************************** */

void CGateInfo::ReceiveSendUser(char *pszPacket)
{
	char	*pszPos;
	int		nSocket;
	int		nLen = memlen(pszPacket);

	if ((pszPos = (char *)memchr(pszPacket, '/', nLen)))
	{
		nSocket = AnsiStrToVal(pszPacket);

		pszPos++;

		_LPTSENDBUFF lpSendUserData = new _TSENDBUFF;

		lpSendUserData->sock		= (SOCKET)nSocket;

		memmove(lpSendUserData->szData, pszPos, memlen(pszPos));

		g_SendToGateQ.PushQ((BYTE *)lpSendUserData);
	}
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
void CGateInfo::ProcAddUser(SOCKET s, char *pszData)
{
	char				szEntryInfo[2048];
	AccountUser			UserEntryInfo;
	_TDEFAULTMESSAGE	DefMsg;
	char				szEncodePacket[64];

	int len = fnDecode6BitBufA(pszData, (char *)&szEntryInfo, sizeof(szEntryInfo));
	szEntryInfo[len] = '\0';

	if ( !ParseUserEntry( szEntryInfo, &UserEntryInfo ) )
		fnMakeDefMessageA(&DefMsg, SM_NEWID_FAIL, 0, 0, 0, 0);
	else
	{	
		AccountUser *pAlreadyUser = AccoutManager::GetInstance().GetAccount(UserEntryInfo.m_id);
		if (pAlreadyUser)
		{
			//已经存在
			fnMakeDefMessageA(&DefMsg, SM_NEWID_EXISTS, 0, 0, 0, 0);
		}
		else
		{
			char szQuery[1024] = {0};
			sprintf( szQuery, "insert into Account(id,pwd) values('%s','%s')",UserEntryInfo.m_id,UserEntryInfo.m_pwd);

			//解析
			db::DBServer *pServer = DBManager::GetInstance().GetFreeCon();
			if (pServer == NULL)
			{
				fnMakeDefMessageA(&DefMsg, SM_NEWID_FAIL, 0, 0, 0, 0);
			}
			else
			{
				if (pServer->ExecuteSQL(szQuery))
				{
					fnMakeDefMessageA(&DefMsg, SM_NEWID_SUCCESS, 0, 0, 0, 0);

					AccountUser *pNewUser = new AccountUser();
					strcpy(pNewUser->m_id,UserEntryInfo.m_id);
					strcpy(pNewUser->m_pwd,UserEntryInfo.m_pwd);

					AccoutManager::GetInstance().InsertAccountUser(pNewUser);
				}
				else
				{
					fnMakeDefMessageA(&DefMsg, SM_NEWID_FAIL, 0, 0, 0, 0);
				}
				DBManager::GetInstance().ReleaseCon(pServer);
				
			}
			

			fnMakeDefMessageA(&DefMsg, SM_NEWID_SUCCESS, 0, 0, 0, 0);

			TCHAR szID[50];
			MultiByteToWideChar( CP_ACP, 0, UserEntryInfo.m_id, -1, szID, sizeof( szID ) / sizeof( TCHAR ) );
			InsertLogMsgParam(IDS_COMPLETENEWUSER, szID);
		}
		
	}		
	
	//加密数据头
	fnEncodeMessageA(&DefMsg, szEncodePacket, sizeof(szEncodePacket));
	SendToGate(s, szEncodePacket);
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
	Packet *pPacket = new Packet();
	pPacket->ver  = PHVer;
	pPacket->hlen = PHLen;

	pPacket->tos = TOS_LOGINSRV_2_LOGINGATE;

	char szMsg[DATA_BUFSIZE] = {0};

	DWORD	dwSendBytes;
	WSABUF	buf;

	int datalen = nLen;
	pPacket->tlen = pPacket->hlen + datalen;

	memcpy(szMsg,pPacket,pPacket->hlen);
	memcpy(szMsg + pPacket->hlen,szData,datalen);

	buf.len = pPacket->tlen;
	buf.buf = szMsg;

	if ( WSASend(sock, &buf, 1, &dwSendBytes, 0, NULL, NULL) == SOCKET_ERROR )
	{
		int nErr = WSAGetLastError();
	}

	SAFE_DELETE(pPacket);
}

