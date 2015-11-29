/************************************************************************/
/* 包数据                                                                     */
/************************************************************************/
#ifndef PACKET_H_
#define PACKET_H_
#include "Common.h"

//按1字节对齐
#pragma pack(1)

struct Packet
{
	uint32 ver  :  4;      // header version   4bit.
	uint32 hlen :  4;      // header length    4bit.
	uint32 tos  :  8;      // type of service  8bit.
	uint32 tlen : 16;      // total  length   16bit. 
	uint32 seq;            // sequence number 32bit.
};

#define PHVer       0x01    // packet header version.
#define PHLen       0x08    // packet header length.

enum TYPE_OF_SERVICE
{
	TOS_CLIENT = 1,

	TOS_LOGINGATE_2_LOGINSRV,
	TOS_LOGINSRV_2_LOGINGATE,

	TOS_GAME,
	TOS_GAME_ACCEPT,
	TOS_GAME_2_GAME,
	TOS_GAME_LOG,

	TOS_SERVERTOOL,
	TOS_TTL,
	TOS_RELAY,
	TOS_RUDP,
	TOS_HACKSHIELD,
	TOS_MAX,
};

#pragma pack()
#endif