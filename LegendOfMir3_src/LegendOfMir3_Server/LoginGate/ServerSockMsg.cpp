#include "stdafx.h"
#include "../Common/Packet.h"

void SendExToServer(char *pszPacket);

extern SOCKET		g_ssock;
extern SOCKET		g_csock;

extern HWND			g_hStatusBar;

extern HANDLE					g_hIOCP;

CWHList<CSessionInfo*>			g_xSessionList;

void UpdateStatusBar(BOOL fGrow)
{
	static LONG	nNumOfCurrSession = 0;

	TCHAR	szText[20];

	(fGrow ? InterlockedIncrement(&nNumOfCurrSession) : InterlockedDecrement(&nNumOfCurrSession));
	
	wsprintf(szText, _TEXT("%d Sessions"), nNumOfCurrSession);

	SendMessage(g_hStatusBar, SB_SETTEXT, MAKEWORD(3, 0), (LPARAM)szText);
}

//UINT WINAPI AcceptThread(LPVOID lpParameter)
//logingate接受线程
DWORD WINAPI AcceptThread(LPVOID lpParameter)
{
	int							nLen = sizeof(SOCKADDR_IN);
	char						szMsg[128] = {0};

	SOCKET						Accept;
	SOCKADDR_IN					Address;

	while (TRUE)
	{
		Accept = accept(g_ssock, (struct sockaddr FAR *)&Address, &nLen);

		if (g_fTerminated)
			return 0;

		CSessionInfo* pNewUserInfo = CSessionInfo::ObjPool().createObject();//(CSessionInfo*)GlobalAlloc(GPTR, sizeof(CSessionInfo));

		if (pNewUserInfo)
		{
			pNewUserInfo->sock				= Accept;

			CreateIoCompletionPort((HANDLE)pNewUserInfo->sock, g_hIOCP, (DWORD)pNewUserInfo, 0);

			if (g_xSessionList.AddNewNode(pNewUserInfo))
			{
				int zero = 0;
				setsockopt(pNewUserInfo->sock, SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero) );
				zero = 0;
				setsockopt( pNewUserInfo->sock, SOL_SOCKET, SO_RCVBUF, (char*)&zero, sizeof(zero));

				int nodelay = 1;
				setsockopt( pNewUserInfo->sock, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay) );

				// ORZ:接受数据
				int retcode = pNewUserInfo->Recv();
				if ( (retcode == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING) )
				{
					continue;
				}

				UpdateStatusBar(TRUE);

				szMsg[0] = '%';
				szMsg[1] = 'O';

				char *pszPos = ValToAnsiStr((int)Accept, &szMsg[2]);
				
				*pszPos++ = '/';

				pszPos = ValToAnsiStr((int)Address.sin_addr.s_net, pszPos);
				*pszPos++ = '.';
				pszPos = ValToAnsiStr((int)Address.sin_addr.s_host, pszPos);
				*pszPos++ = '.';
				pszPos = ValToAnsiStr((int)Address.sin_addr.s_lh, pszPos);
				*pszPos++ = '.';
				pszPos = ValToAnsiStr((int)Address.sin_addr.s_impno, pszPos);

				*pszPos++	= '$';
				*pszPos		= '\0';

				//发送给loginsrv更新
				SendExToServer(szMsg);
			}
		}
	}

	return 0;
}

//发送给loginsrv关闭
void CloseSession(int s)
{
	char szMsg[32];

	// Send close msg to login server
	//wsprintf(szMsg, _TEXT("%%X%d$"), s);
	szMsg[0] = '%';
	szMsg[1] = 'X';

	char *pszPos = ValToAnsiStr(s, &szMsg[2]);

	*pszPos++	= '$';
	*pszPos		= '\0';
	
	SendExToServer(szMsg); 

	closesocket(s);

	UpdateStatusBar(FALSE);
}

DWORD WINAPI ServerWorkerThread(LPVOID CompletionPortID)
{
	DWORD					dwBytesTransferred = 0;

	CSessionInfo*			pSessionInfo = NULL;
	_LPTCOMPLETIONPORT		lpPerIoData = NULL;

	char					szPacket[DATA_BUFSIZE * 2];
	char					szMsg[32];
	char					*pszPos;

	while (TRUE)
	{
		if ( GetQueuedCompletionStatus( 
										(HANDLE)CompletionPortID, 
										&dwBytesTransferred, 
										(LPDWORD)&pSessionInfo, 										
										(LPOVERLAPPED *)&lpPerIoData, 
										INFINITE) == 0)
		{
			if (g_fTerminated)
				return 0;

			//失败关闭
			if (pSessionInfo)
			{
				szMsg[0] = '%';
				szMsg[1] = 'X';

				char *pszPos = ValToAnsiStr((int)pSessionInfo->sock, &szMsg[2]);

				*pszPos++	= '$';
				*pszPos		= '\0';

				SendExToServer(szMsg); 

				g_xSessionList.RemoveNodeByData(pSessionInfo);

				closesocket(pSessionInfo->sock);
				pSessionInfo->sock = INVALID_SOCKET;

				UpdateStatusBar(FALSE);

				//GlobalFree(pSessionInfo);
				CSessionInfo::ObjPool().reclaimObject(pSessionInfo);
			}

			continue;
		}

		if (g_fTerminated)
			return 0;

		if (dwBytesTransferred == 0)
		{
			szMsg[0] = '%';
			szMsg[1] = 'X';

			char *pszPos = ValToAnsiStr((int)pSessionInfo->sock, &szMsg[2]);

			*pszPos++	= '$';
			*pszPos		= '\0';

			SendExToServer(szMsg); 

			g_xSessionList.RemoveNodeByData(pSessionInfo);

			closesocket(pSessionInfo->sock);
			pSessionInfo->sock = INVALID_SOCKET;

			UpdateStatusBar(FALSE);

			//GlobalFree(pSessionInfo);
			CSessionInfo::ObjPool().reclaimObject(pSessionInfo);
			continue;
		}


		// ORZ:
		pSessionInfo->bufLen += dwBytesTransferred;

		while ( pSessionInfo->HasCompletionPacket() )
		{
			szPacket[0]	= '%';
			szPacket[1]	= 'A';
			pszPos		= ValToAnsiStr( (int) pSessionInfo->sock, &szPacket[2] );
			*pszPos++	= '/';
			pszPos		= pSessionInfo->ExtractPacket( pszPos );
			*pszPos++	= '$';
			*pszPos		= '\0';

			SendExToServer( szPacket );
		}

		// ORZ:
		if ( pSessionInfo->Recv() == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING )
		{
			InsertLogMsg(_TEXT("WSARecv() failed"));
			
			CloseSession(pSessionInfo->sock);
			continue;
		}
	}

	return 0;
}
