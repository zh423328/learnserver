/************************************************************************/
/* 包数据                                                                     */
/************************************************************************/
#ifndef PACKET_H_
#define PACKET_H_
#include "Common.h"

#define PHVer       0x01    // packet header version.
#define PHLen       0x0B    // packet header length.
//2048-sizeof(packet)
#define TCP_PACKET_SIZE 2048-PHLen 

#define DATA_BUFSIZE 2048*10

//按1字节对齐
#pragma pack(1)

struct Packet
{
	uint8	ver  :  4;		// header version   4bit.
	uint8	hlen :  4;		// header length    4bit.
	uint16	crc ;			// crc of data		16bit.
	uint16	dlen;			// data length    16bit.
	uint8	Category;		// type of category 8bit
	uint8	Protocol;		// type of protocol	8bit
	uint32	seq;			// sequence number 32bit.
	uint8   data[1];		// head of data
};

#define	AllocPacketSize(packet, cmdGroup, cmd, dataSize)        \
	packet = (Packet *)malloc(sizeof(Packet) + (dataSize));     \
	if (packet != NULL)                                         \
{                                                           \
	packet->ver = PHVer;									\
	packet->hlen = PHLen;									\
	packet->dlen = (dataSize);								\
	packet->Category = cmdGroup;                           \
	packet->Protocol = cmd;                                 \
}


#define	AllocPacket(packet, cmdGroup, cmd, dataStruct)	            \
	AllocPacketSize(packet, cmdGroup, cmd, sizeof(dataStruct))

#define	BuildPacketSize(packet, cmdGroup, cmd, buffer, dataSize)	\
	packet = (Packet *)buffer;                                      \
	packet->ver = PHVer;											\
	packet->hlen = PHLen;											\
	packet->dlen = (dataSize);										\
	packet->Category = cmdGroup;                                    \
	packet->Protocol = cmd

#define	BuildPacket(packet, cmdGroup, cmd, buffer)		        \
	BuildPacketSize(packet, cmdGroup, cmd, buffer, sizeof(cmdGroup##_##cmd##_Data))

#define	BuildPacketEx(packet, cmdGroup, cmd, buffer, buffersize)		\
	Packet *	packet		= NULL;									\
	char buffer[buffersize] = {0};									\
	BuildPacket(packet, cmdGroup, cmd, buffer)

#define BuildCmdPacket(packet, cmdGroup, cmd, buffer)   \
	BuildPacketSize(packet, cmdGroup, cmd, buffer, 0)

#define BuildCmdPacketEx(packet, cmdGroup, cmd, buffer, buffersize)   \
	Packet *	packet		= NULL;									\
	char buffer[buffersize] = {0};									\
	BuildCmdPacket(packet, cmdGroup, cmd, buffer)

#define SetPacketSocketKey(pPacket, socketKey)                  \
	pPacket->seq = socketKey

#define SET_DATA(dataName, cmdGroup, cmd, packet)   \
	cmdGroup##_##cmd##_Data * dataName = (cmdGroup##_##cmd##_Data*)packet->data;


#define BuildPacket1(pack,cmdGroup,cmd,dataName)\
	Packet * pack=NULL;\
	char buffer_adfsadfasdf[1024]={0};\
	BuildPacketSize(pack, cmdGroup, cmd, buffer_adfsadfasdf, sizeof(cmdGroup##_##cmd##_Data));\
	SET_DATA(dataName, cmdGroup, cmd, pack)


#pragma pack()
#endif