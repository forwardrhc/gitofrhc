#ifndef __TRANSFER_HPP__
#define __TRANSFER_HPP__
static inline char* XOR(char* buf, size_t len)
{
	for (size_t i = 0; i < len; ++i)
	{
		buf[i] ^= 1;//通过异或进行加密解密
	}
}
static inline void Decrypt(char* buf, size_t len)//解密
{
	XOR(buf, len);
}
static inline void Encry(char* buf, size_t len)//加密
{
	XOR(buf, len);
}
#endif //__TRANSFER_HPP__
