#ifndef __TRANSFER_HPP__
#define __TRANSFER_HPP__
#include "common.hpp"
#include "epoll.hpp"
class TransferServer : public EpollServer
{
public:
	TransferServer(int selfPort,const char* socks5IP,int socks5Port)
		: EpollServer(selfPort)
	{
		memset(&_socks5addr, 0, sizeof(struct sockaddr_in));
		_socks5addr.sin_family = AF_INET;
		_socks5addr.sin_port = htons(socks5Port);
		_socks5addr.sin_addr.s_addr = inet_addr(socks5IP);
	}
	//多态实现的虚函数
	virtual void ConnectEventHandle(int connectfd);
	virtual void ReadEventHandle(int connectfd);
protected:
	struct sockaddr_in _socks5addr;
};
#endif //__TRANSFER_HPP__
