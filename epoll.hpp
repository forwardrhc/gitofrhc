#ifndef __EPOLL_HPP__
#define __EPOLL_HPP__
#include "common.hpp"
#include "encry.hpp"
class IgnoreSigPipe
{
public:
	IgnoreSigPipe()
	{
		::signal(SIGPIPE, SIG_IGN);
	}
};
static IgnoreSigPipe initPIPE_IGN;
class EpollServer
{
public:
	EpollServer(int port)
		: _port(port)
		, _listenfd(-1)
		, _eventfd(-1)
	{}
	virtual ~EpollServer()
	{
		if(_listenfd != -1)
			close(_listenfd);
	}
	void OpEvent(int fd, int events, int op)
	{
		struct epoll_event event;
		event.events = events;
		event.data.fd = fd;
		if (epoll_ctl(_eventfd, op, fd, &event) < 0)
		{
			ErrorLog("epoll_ctl(op:%d, fd:%d)", op, fd);
		}
	}
	void SetNonblocking(int fd)
	{
		int flags, s;
		flags = fcntl(fd, F_GETFL, 0);
		if (flags == -1)
			ErrorLog("SetNonblocking:F_GETFL");
		flags |= O_NONBLOCK;
		s = fcntl(fd, F_SETFL, flags);
		if (s == -1)
			ErrorLog("SetNonblocking:F_SETFL");
	}
	enum Socks5State//Socks5的状态
	{
		AUTH,//身份认证
		ESTABLISHED,//建立连接
		FORWARDING//转发
	};
	struct Channel
	{
		int _fd;//描述
		string _buf;//写缓冲
		Channel()
			: _fd(-1)
		{}
	};
	struct Connect//一个连接由两个通道构成
	{
		Socks5State _state;//连接的状态，Socks服务器需要，Transfer服务器不需要
		Channel _clientChannel;//客户端(浏览器)通道
		Channel _serverChannel;//服务端(谷歌)通道
		int _ref;//引用计数控制Connect的销毁，因为通道是一条关闭，另一条再关闭的
		Connect()
			: _state(AUTH)
			, _ref(0)
		{}
	};
	void Start();//启动
	void EventLoop();//事件循环
	void Forwarding(Channel* clientChannel, Channel* serverChannel, bool sendencry, bool recvdecrypt);
	void RemoveConnect(int fd);
	void SendInLoop(int fd, const char* buf, int len);
	//多态实现的虚函数
	virtual void ConnectEventHandle(int connectfd) = 0;
	virtual void ReadEventHandle(int connectfd) = 0;
	virtual void WriteEventHandle(int connectfd);
protected:
	int _port;//端口
	int _listenfd;//监听描述符
	int _eventfd;//事件描述符
	map<int, Connect*> _fdConnectMap;//fd映射连接的map容器：区分
};
#endif //__EPOLL_HPP__
