#include <assert.h>
#include "rc6.h"

#define w 32    /* word为32bits */  
#define r 20    //加密轮数  

#define P32 0xB7E15163  /* 定义两个常量，用于子密钥生成 */  
#define Q32 0x9E3779B9  

#define bytes   (w / 8)                     /* 定义字节*/  
#define c       ((b + bytes - 1) / bytes)   /* 密钥字数 */  
#define R24     (2 * r + 4)  
#define lgw     5                           /* log2(w)  */  

/* 定义逻辑移位操作运算 */  
#define ROTL(x,y) (((x)<<(y&(w-1))) | ((x)>>(w-(y&(w-1)))))  
#define ROTR(x,y) (((x)>>(y&(w-1))) | ((x)<<(w-(y&(w-1)))))  

unsigned int S[R24 - 1];                    /* 子密钥组 */  


//0-256bit,常用为16，24，32字节
void rc6_key_setup(unsigned char *K, int b)  
{
	int i, j, s, v;  
	unsigned int L[(32 + bytes - 1) / bytes]; /* Big enough for max b */  
	unsigned int A, B;  

	L[c - 1] = 0;  
	for (i = b - 1; i >= 0; i--)  
		L[i / bytes] = (L[i / bytes] << 8) + K[i];  

	S[0] = P32;  
	for (i = 1; i <= 2 * r + 3; i++)  
		S[i] = S[i - 1] + Q32;  

	A = B = i = j = 0;  
	v = R24;  
	if (c > v) v = c;  
	v *= 3;  

	for (s = 1; s <= v; s++)  
	{  
		A = S[i] = ROTL(S[i] + A + B, 3);  
		B = L[j] = ROTL(L[j] + A + B, A + B);  
		i = (i + 1) % R24;  
		j = (j + 1) % c;  
	}  
}  

void rc6_block_encrypt(unsigned int *pt1,unsigned int *pt2,unsigned int *pt3,unsigned int *pt4)  
{  
	unsigned int A, B, C, D, t, u, x;  
	int i;  

	A = *pt1;  
	B = *pt2;  
	C = *pt3;  
	D = *pt4;  
	B += S[0];  
	D += S[1];  
	for (i = 2; i <= 2 * r; i += 2)  
	{  
		t = ROTL(B * (2 * B + 1), lgw);  
		u = ROTL(D * (2 * D + 1), lgw);  
		A = ROTL(A ^ t, u) + S[i];  
		C = ROTL(C ^ u, t) + S[i + 1];  
		x = A;  
		A = B;  
		B = C;  
		C = D;  
		D = x;  
	}  
	A += S[2 * r + 2];  
	C += S[2 * r + 3];  
	*pt1 = A;  
	*pt2 = B;  
	*pt3  = C;  
	*pt4   = D;  
}  

void rc6_block_decrypt(unsigned int *pt1,unsigned int *pt2,unsigned int *pt3,unsigned int *pt4)   
{  
	unsigned int A, B, C, D, t, u, x;  
	int i;  

	A = *pt1;  
	B = *pt2;  
	C = *pt3;  
	D = *pt4;  
	C -= S[2 * r + 3];  
	A -= S[2 * r + 2];  
	for (i = 2 * r; i >= 2; i -= 2)  
	{  
		x = D;  
		D = C;  
		C = B;  
		B = A;  
		A = x;  
		u = ROTL(D * (2 * D + 1), lgw);  
		t = ROTL(B * (2 * B + 1), lgw);  
		C = ROTR(C - S[i + 1], t) ^ u;  
		A = ROTR(A - S[i], u) ^ t;  
	}  
	D -= S[1];  
	B -= S[0];  
	*pt1 = A;  
	*pt2 = B;  
	*pt3 = C;  
	*pt4 = D;    
}  

//加密
void rc6_encrypt( unsigned char* K,int nLen )
{
	{
		int nLoop = nLen>>4;

		for (int i = 0; i < nLoop;++i)
		{
			rc6_block_encrypt((unsigned int*)(K+ i*16),(unsigned int*)(K+4 + i*16)
				,(unsigned int*)(K+8 + i*16),(unsigned int*)(K+12 + i*16));
		}
	}
}

//解密
void rc6_decrypt( unsigned char* K,int nLen )
{
	{
		int nLoop = nLen>>4;
		for (int i = 0; i < nLoop;++i)
		{
			rc6_block_decrypt((unsigned int*)(K+ i*16),(unsigned int*)(K+4 + i*16)
				,(unsigned int*)(K+8 + i*16),(unsigned int*)(K+12 + i*16));
		}
	}
}
