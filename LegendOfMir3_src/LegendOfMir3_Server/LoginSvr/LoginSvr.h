#pragma once

#include "../Common/Packet.h"

#define PACKET_KEEPALIVE				"%++$"

#define LOGPARAM_STR			1
#define LOGPARAM_INT			2

class CUserInfo
{
public:
	SOCKET		sock;
	TCHAR		szSockHandle[4];

	TCHAR		szUserID[11];				// User ID
	TCHAR		szAddress[20];				// User's local address 
	
	BYTE		btPayMode;
	
	int			nClientVersion;
	int			nCertification;
	BOOL		fVersionAccept;
	BOOL		fSelServerOk;

	int			nServerID;
};

class AccountUser;
class CGateInfo
{
public:
	SOCKET					sock;								//该LoginGate在服务器的socket

	CWHList<CUserInfo*>		xUserInfoList;						//通过该LoginGate
	
	// For Overlapped I/O
	OVERLAPPED				Overlapped;
	WSABUF					DataBuf;
	CHAR					Buffer[DATA_BUFSIZE];
	int						bufLen;

	CWHQueue				g_SendToGateQ;

public:
	void	SendToGate(SOCKET cSock, char *pszPacket);

	void	ReceiveSendUser(char *pszPacket);
	void	ReceiveCloseUser(char *pszPacket);
	void	ReceiveOpenUser(char *pszPacket);
	void	ReceiveServerMsg(char *pszPacket);
	void	MakeNewUser(char *pszPacket);

	bool	ParseUserEntry( char *buf, AccountUser *userInfo );

	void	ProcAddUser(SOCKET s, char *pszData);
	void	ProcLogin(SOCKET s, char *pszData);
	void	ProcSelectServer(SOCKET s, WORD wServerIndex);

	void	Close();
	
	void	SendToGate(char * szData,int nLen);
	
	__inline void SendKeepAlivePacket() { SendToGate(PACKET_KEEPALIVE, strlen(PACKET_KEEPALIVE)); }

	CGateInfo()
	{
		bufLen	= 0;
	}

	int  Recv()
	{
		DWORD nRecvBytes = 0, nFlags = 0;

		DataBuf.len = DATA_BUFSIZE - bufLen;
		DataBuf.buf = Buffer + bufLen;

		memset( &Overlapped, 0, sizeof( Overlapped ) );

		return WSARecv( sock, &DataBuf, 1, &nRecvBytes, &nFlags, &Overlapped, 0 );
	}

	//是否有个完整的包
	bool HasCompletionPacket()
	{
		//return memchr( Buffer, '!', bufLen ) ? true : false;
		if (bufLen >= PHLen)	//包头长度
		{
			Packet *pHavePacket = (Packet*)Buffer;
			if (pHavePacket == NULL)
				return false;

			if(pHavePacket->ver != PHVer || pHavePacket->hlen != PHLen)
				return false;

			return true;
		}
		else
			return false;
	}

	// recv 展开包
	char * ExtractPacket( char *pPacket )
	{
		Packet *pHavePacket = (Packet*)Buffer;
		if (pPacket== NULL)
			return NULL;

		if(pHavePacket->ver != PHVer || pHavePacket->hlen != PHLen)
			return NULL;

		int packetLen = pHavePacket->tlen;	//包长度

		memcpy( pPacket, Buffer, packetLen );

		memmove( Buffer, Buffer + packetLen, DATA_BUFSIZE - packetLen );
		bufLen -= packetLen;

		return pPacket + packetLen;
	}
};


struct GAMESERVERINFO
{
	int  index;
	char name[50];
	char ip[50];
	int  connCnt;	// 立加 荐 包府
};

// ORZ: 倾侩 霸捞飘辑滚 格废 畴靛 单捞磐
struct GATESERVERINFO
{
	char ip[50];
};


typedef struct tag_TSENDBUFF
{
	SOCKET			sock;
	char			szData[TCP_PACKET_SIZE];
}_TSENDBUFF, *_LPTSENDBUFF;

void InsertLogMsg(UINT nID);
void InsertLogMsg(LPTSTR lpszMsg);
void InsertLogMsgParam(UINT nID, void *pParam, BYTE btFlags = LOGPARAM_STR);
