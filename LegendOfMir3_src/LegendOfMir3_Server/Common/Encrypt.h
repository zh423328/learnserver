/************************************************************************/
/* rc5,crc,md5    验证和加密											*/
/************************************************************************/
#ifndef ENCRYPT_H_
#define ENCRYPT_H_

#include "Common.h"

//md5help
class MD5Helper
{
public:
	//取文件md5
	static std::string FileMd5(std::string filename);

	//取数据md5
	static std::string DataMd5(uint8 *bytes, int32 nLen);
};

//crc
class CrcHelper
{
public:
	//crc8
	static uint8	GetCrc8(uint8 *bytes,int32 nLen);	

	//crc16
	static uint16	GetCrc16(uint8*bytes, int32 nLen);

	//crc32
	static uint32	GetCrc32(uint8*bytes, int32 nLen);
};


//随机算法
class CRandom
{
public:
	CRandom();
	~CRandom();

	void	Random_Seed (uint32 seed);
	int32	Random_Int(int32 min, int32 max);
	int32   Random_Int  ();
private:
	uint32 m_dwSeed;
};


//加密
class  CEncrypt
{
public:
	// 最简单的,就异或(异或2次就会原值)
	static void	Encrypt_Simple( uint8* pCode, int32 nSize, uint32 dwKEY );// 最简单的异或
	static void	Encrypt_Simple( uint8* pCode, int32 nSize, uint16 wKEY  );
	static void	Encrypt_Simple( uint8* pCode, int32 nSize, uint8  byKEY );

	// KEY为随机数种子
	// 适用于DWORD, WORD, BYTE 三种KEY
	static void	Encrypt_Random( uint8* pCode, int32 nSize, uint32 dwKEY );

	// 随机加密+crc16 key验证,wKEY_CRC(0:加密,有值:解密+验证)，返回值:解密验证不通过会失败
	static bool	Encrypt_CRC16R( uint8* pCode, int32 nSize, uint16& wKEY_CRC);


	//设置密钥
	static void	SetRc6Key(uint8 *pKey,uint32 nLen);

	//加密	
	static bool Encrypt_RC6(uint8* pData,uint32 nLen);

	//解密
	static bool Decrypt_RC6(uint8* pData,uint32 nLen);
private:
	static uint8*	m_Key;				//密钥 0-256，常用16，24，32
	static uint32   m_nLen;				//长度
	static bool		m_bInitKey;			//是否初始化
};
#endif