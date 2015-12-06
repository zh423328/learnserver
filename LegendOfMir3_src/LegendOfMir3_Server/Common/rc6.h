#ifndef RC6_H_
#define RC6_H_

#include "stdafx.h"

//0-256bit,常用为16，24，32字节
void rc6_key_setup(unsigned char *K, int b);

//对称加密，不改变长度的
//注意：nLen 一定是 4的倍数，不够用0填充
//加密
void rc6_encrypt(unsigned char* K,int nLen);

//解密
void rc6_decrypt(unsigned char* K,int nLen); 

#endif