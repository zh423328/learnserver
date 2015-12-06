#include "stdafx.h"
#include "../Common/Common.h"
#include "../packet/Category.h"
#include "../packet/logingate_protocol.h"

void SendExToServer(Packet*pPacket);
void SendExToServer(uint8 Category,uint8 protocol);

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

/////////////////////////////////////////////////////////////////////
// 判断客户端Socket是否已经断开，否则在一个无效的Socket上投递WSARecv操作会出现异常
// 使用的方法是尝试向这个socket发送数据，判断这个socket调用的返回值
// 因为如果客户端网络异常断开(例如客户端崩溃或者拔掉网线等)的时候，服务器端是无法收到客户端断开的通知的
bool _IsSocketAlive( SOCKET s )
{
	int nByteSent=send(s,"",0,0);
	if (-1 == nByteSent) return false;
	return true;
}


// 显示并处理完成端口上的错误 0:错误退出，1：客户端关闭 2：服务器问题
int HandleError(SOCKET sock,const DWORD& dwErr )
{
	// 如果是超时了，就再继续等吧  
	if(WAIT_TIMEOUT == dwErr)  
	{  	
		// 确认客户端是否还活着...
		if( !_IsSocketAlive( sock ) )			//send校验
		{
			InsertLogMsg(_TEXT("检测到客户端异常退出！"));
			return 1;
		}
		else
		{
			InsertLogMsg( _TEXT("网络操作超时！重试中..."));
			return 2;
		}
	}  

	// 可能是客户端异常退出了
	else if( ERROR_NETNAME_DELETED==dwErr )		
	{
		InsertLogMsg(_TEXT( "检测到客户端异常退出！"));			//客户端是关X直接退出
		return 1;
	}

	else
	{
		TCHAR szMsg[1024];
		wsprintf(szMsg,_TEXT("完成端口操作出现错误，线程退出。错误代码：%d"),dwErr);
		InsertLogMsg(szMsg);
		return 0;
	}
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
					closesocket(Accept);
					continue;
				}

				UpdateStatusBar(TRUE);

				BuildPacketEx(pPacket,LOGIN_GATE,GATE2SRV_ADDSOCKET,buffer,256);
				SET_DATA(pData,LOGIN_GATE,GATE2SRV_ADDSOCKET,pPacket);
				pData->sock = Accept;
				strcpy(pData->szIp,inet_ntoa(Address.sin_addr));
				//发送给loginsrv更新
				SendExToServer(pPacket);
			}
		}
	}

	return 0;
}

//发送给loginsrv关闭
void CloseSession(int s)
{
	BuildPacketEx(pPacket,LOGIN_GATE,GATE2SRV_CLOSESOCKET,buffer,256);
	SET_DATA(pData,LOGIN_GATE,GATE2SRV_CLOSESOCKET,pPacket);
	pData->sock = s;
	//发送给loginsrv更新
	SendExToServer(pPacket);

	closesocket(s);

	UpdateStatusBar(FALSE);
}

DWORD WINAPI ServerWorkerThread(LPVOID CompletionPortID)
{
	DWORD					dwBytesTransferred = 0;

	CSessionInfo*			pSessionInfo = NULL;
	_LPTCOMPLETIONPORT		lpPerIoData = NULL;

	char					szMsg[32] = {0};

	while (TRUE)
	{
		BOOL bRet =  GetQueuedCompletionStatus((HANDLE)CompletionPortID, &dwBytesTransferred, (LPDWORD)&pSessionInfo,(LPOVERLAPPED *)&lpPerIoData, INFINITE);
		
		if ( bRet == FALSE)
		{
			if (g_fTerminated)
				return 0;

			//如果 *lpOverlapped为空并且函数没有从完成端口取出完成包，返回值则为0。函数则不会在lpNumberOfBytes and lpCompletionKey所指向的参数中存储信息
			if (lpPerIoData == NULL)
				continue;

			//退出
			DWORD dwErr = GetLastError();

			if (pSessionInfo != NULL)
			{
				// 显示一下提示信息,退出返回true
				int nRet = HandleError( pSessionInfo->sock,dwErr ) ;
				if( nRet == 0)			
				{
					break;
				}

				//客户端端口连接或者网络关闭
				if (nRet == 1)
				{
					//关闭客户端
					CloseSession(pSessionInfo->sock);

					g_xSessionList.RemoveNodeByData(pSessionInfo);

					//GlobalFree(pSessionInfo);
					CSessionInfo::ObjPool().reclaimObject(pSessionInfo);
				}

			}

			continue;
		}

		if (g_fTerminated)
			return 0;

		//错误，关闭服务器
		if (dwBytesTransferred == 0)
		{
			BuildPacketEx(pPacket,LOGIN_GATE,GATE2SRV_CLOSESOCKET,buffer,64);
			SET_DATA(pData,LOGIN_GATE,GATE2SRV_CLOSESOCKET,pPacket);
			pData->sock = pSessionInfo->sock;
			//发送给loginsrv更新
			SendExToServer(pPacket);

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
			BuildPacketEx(pPacket,LOGIN_GATE,GATE2SRV_MSG,buf,3096);
			SET_DATA(pData,LOGIN_GATE,GATE2SRV_MSG,pPacket);
			pData->sock = pSessionInfo->sock;
			int nLen = pSessionInfo->ExtractPacket( pData->szPacket );
			pPacket->dlen += nLen;
			SendExToServer(pPacket);
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


