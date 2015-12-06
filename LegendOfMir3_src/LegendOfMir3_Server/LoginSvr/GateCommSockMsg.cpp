#include "stdafx.h"

extern HWND								g_hStatusBar;

extern SOCKET							g_gcSock;

UINT WINAPI ThreadFuncForMsg(LPVOID lpParameter);

extern HANDLE							g_hIOCP;
extern BOOL								g_fTerminated;

CWHList<CGateInfo*>						g_xGateList;
CWHList<GAMESERVERINFO*>				g_xGameServerList;
CWHList<GATESERVERINFO*>				g_xGateServerList;
char									g_szServerList[1024];

unsigned long							g_hThreadForMsg = 0;

void UpdateStatusBar(BOOL fGrow)
{
	static long	nNumOfCurrSession = 0;

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

BOOL InitServerThreadForMsg()
{
	UINT	dwThreadIDForMsg = 0;

	if (!g_hThreadForMsg)
	{
		g_hThreadForMsg	= _beginthreadex(NULL, 0, ThreadFuncForMsg,	NULL, 0, &dwThreadIDForMsg);

		if (g_hThreadForMsg)
			return TRUE;
	}

	return FALSE;
}

DWORD WINAPI AcceptThread(LPVOID lpParameter)
{
	int					nLen = sizeof(SOCKADDR_IN);

	SOCKET				Accept;
	SOCKADDR_IN			Address;

	while (TRUE)
	{
		Accept = accept(g_gcSock, (struct sockaddr FAR *)&Address, &nLen);

		if (g_fTerminated)
			return 0;

		CGateInfo* pGateInfo = new CGateInfo;

		if (pGateInfo)
		{
			pGateInfo->sock = Accept;

			CreateIoCompletionPort((HANDLE)pGateInfo->sock, g_hIOCP, (DWORD)pGateInfo, 0);

			if (g_xGateList.AddNewNode(pGateInfo))
			{
				int zero = 0;
				setsockopt(pGateInfo->sock, SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero) );
				zero = 0;
				setsockopt( pGateInfo->sock, SOL_SOCKET, SO_RCVBUF, (char*)&zero, sizeof(zero));

				int nodelay = 1;
				setsockopt( pGateInfo->sock, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay) );

				int retcode = pGateInfo->Recv();

				if ( (retcode == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING))
				{
					//会接受到异常的信息
					closesocket(Accept);	// 触发完成端口失败消息
					continue;
				}

				UpdateStatusBar(TRUE);

#ifdef _DEBUG
				TCHAR szGateIP[256];
				wsprintf(szGateIP, _T("%d.%d.%d.%d"), Address.sin_addr.s_net, Address.sin_addr.s_host, 
															Address.sin_addr.s_lh, Address.sin_addr.s_impno);

				InsertLogMsgParam(IDS_ACCEPT_GATESERVER, szGateIP);
#endif
			}
		}
	}

	return 0;
}

void CloseGate(CGateInfo* pGateInfo)
{
}

DWORD WINAPI ServerWorkerThread(LPVOID CompletionPortID)
{
	DWORD					dwBytesTransferred = 0;
	CGateInfo*				pGateInfo = NULL;
	LPOVERLAPPED			lpOverlapped = NULL;
	char					szTmp[TCP_PACKET_SIZE + PHLen] = {0};

	while (TRUE)
	{
		BOOL bRet =  GetQueuedCompletionStatus((HANDLE)CompletionPortID, &dwBytesTransferred, (LPDWORD)&pGateInfo, (LPOVERLAPPED *)&lpOverlapped,INFINITE);

		if (bRet == FALSE)
		{
			//如果 *lpOverlapped为空并且函数没有从完成端口取出完成包，返回值则为0。函数则不会在lpNumberOfBytes and lpCompletionKey所指向的参数中存储信息
			if (lpOverlapped == NULL)
				continue;

			//退出
			DWORD dwErr = GetLastError();

			if (pGateInfo != NULL)
			{
				// 显示一下提示信息,退出返回true
				int nRet = HandleError( pGateInfo->sock,dwErr ) ;
				
				//服务器错误了
				if( nRet == 0)			
				{
					break;
				}

				//客户端端口连接或者网络关闭
				if (nRet == 1)
				{
					//关闭客户端
					pGateInfo->Close();

					//从列表删除
					g_xGateList.RemoveNodeByData(pGateInfo);

					SAFE_DELETE(pGateInfo);
				}

			}

			continue;
		}
		

		//全部关闭
		if (g_fTerminated)
		{
			PLISTNODE		pListNode;

			if (g_xGateList.GetCount())
			{
				pListNode = g_xGateList.GetHead();

				while (pListNode)
				{
					pGateInfo = g_xGateList.GetData(pListNode);

					if (pGateInfo)
						pGateInfo->Close();

					delete pGateInfo;
					pGateInfo = NULL;

					pListNode = g_xGateList.RemoveNode(pListNode);
				}
			}

			return 0;
		}
		
		if ( dwBytesTransferred == 0 )
		{
			pGateInfo->Close();
			continue;
		}

		pGateInfo->bufLen += dwBytesTransferred;

		//接受消息
		while ( pGateInfo->HasCompletionPacket() )
		{
			uint32 nLen = pGateInfo->ExtractPacket(szTmp);

			Packet *pPacket = (Packet*)szTmp;
			pPacket->dlen = pPacket->dlen ^ pPacket->crc;
			pGateInfo->PacketProcess(pPacket);
			//switch (pPacket->Category)
			//{
			//	case '-':
			//		pGateInfo->SendKeepAlivePacket();
			//		break;
			//	case 'A':
			//		//接收连接，和玩家数据
			//		pGateInfo->ReceiveSendUser(szTmp);
			//		break;
			//	case 'O':
			//		//
			//		pGateInfo->ReceiveOpenUser(&szTmp[nPacketHeader+2]);
			//		break;
			//	case 'X':
			//		//关闭
			//		pGateInfo->ReceiveCloseUser(&szTmp[nPacketHeader+2]);
			//		break;
			//	case 'S':
			//		pGateInfo->ReceiveServerMsg(&szTmp[nPacketHeader+2]);
			//		break;
			//	case 'M':
			//		//新建角色
			//		pGateInfo->MakeNewUser(&szTmp[nPacketHeader+2]);
			//		break;
			//}
		}

		if ( pGateInfo->Recv() == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING )
		{
			InsertLogMsg(_TEXT("WSARecv() failed"));
			closesocket(pGateInfo->sock);	
			continue;
		}
	}

	return 0;
}
