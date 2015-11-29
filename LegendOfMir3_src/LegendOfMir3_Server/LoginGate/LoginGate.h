
#ifndef _LOGINGATE_DEFINE
#define _LOGINGATE_DEFINE

#include "../Common/pool.h"
#include "../Common/Packet.h"

//玩家连入信息
class CSessionInfo : public PoolObject
{
public:
	SOCKET						sock;

	OVERLAPPED					Overlapped;
	WSABUF						DataBuf;
	CHAR						Buffer[DATA_BUFSIZE];
	int							bufLen;

public:
	// ORZ:
	CSessionInfo()
	{
		bufLen	= 0;
	}

	//创建一个
	static ObjectPool<CSessionInfo>& ObjPool();

	static void destroyObjPool();

	virtual void onReclaimObject()
	{
		//重置
		sock = INVALID_SOCKET;
		bufLen = 0;
		memset(Buffer,0,DATA_BUFSIZE);
		memset(&Overlapped,0,sizeof(OVERLAPPED));
	}

	virtual size_t getPoolObjectBytes()
	{
		size_t bytes = DATA_BUFSIZE + sizeof(bufLen) + sizeof(SOCKET) + sizeof(OVERLAPPED);

		return bytes;
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

		int packetLen = pHavePacket->tlen;	//包长度

		memcpy( pPacket, Buffer, packetLen );

		memmove( Buffer, Buffer + packetLen, DATA_BUFSIZE - packetLen );
		bufLen -= packetLen;

		return pPacket + packetLen;
	}
};


void InsertLogMsg(UINT nID);
void InsertLogMsg(LPTSTR lpszMsg);
void InsertLogPacket(char *pszPacket);

#endif //_LOGINGATE_DEFINE
